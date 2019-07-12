#include "tlah.h"
#include <assert.h>

#include <iostream>
#include <iomanip>
#include <random>
#include "redsvd.h"

#include <Eigen/Dense>
#include <limits>

using namespace Eigen;

//#define ARMA_NO_DEBUG

using namespace arma;
using arma::uvec;
using arma::qr_econ;

#define DIFF_THRESHOLD 0.01f

#define RANK_MINIMUM 0.5f
#define RANK_CUTOFF 0.01f
#define FLT_MAX std::numeric_limits<float>::max()

#define NORM_1
//#define NORM_2

//#define PRINT

//#define ERROR_CHG
#define ERROR_REM
//#define ERROR_ROW

#define PARTIAL_SVD_RANK_ESTIMATE
//#define QR_RANK_ESTIMATE

namespace tlah {
	/*
	%
	% Low - rank matrix fitting for sparse matrix separation :
	%                  L + S ~= D
	% where L = XY is low - rank and S is sparse.
	% This code applies Lagrange ADM to solving the model
	%         min || S || _1
	%         s.t.XY + S = D
	%             S_ij = 0, for i, j in Known
	%             X, Y, S
	% Output:
	%           X-- - m x k matrix
	%           Y-- - k x n matrix
	%           S-- - m x n(sparse) matrix
	%         Out-- - structure for others
	% Input :
	%           D			 -- - m x n data matrix
	%           maximum_rank -- - initial rank estimate
	%        opts-- - structure for option with fields :
	*/

	namespace {

		void shrink(Fmat &S, Float beta, const std::vector<bool> &known_tag) {
			beta = 1.0f / beta;
		
			//S = sign(S) % clamp(abs(S) - inv_beta, 0, S.max());
			//S = sign(S) % clamp(abs(S) - inv_beta, 0, FLT_MAX);

			Fmat::iterator s = S.begin();
			Fmat::iterator s_end = S.end();
			std::vector<bool>::const_iterator t = known_tag.begin();
			for (; s != s_end; s++, t++) {
				if (!(*t)) {

					Float &k = *s;
					if (abs(k) <= beta) {
						*s = 0;
					}
					else {
						*s = k > 0 ? k - beta : k + beta;
					}

				}
				else {

					*s = 0;
				}

			}

		}


		struct DiagItem {
			Float eigenvalue;
			uword id;

			DiagItem(Float e, uword i) : eigenvalue(e), id(i) { }
		};

		bool operator<(const DiagItem &a, const DiagItem &b) {
			return a.eigenvalue >= b.eigenvalue;
		}

		/*
		bool estimate_rank(Fmat &X, Fmat &R) {
			
			Col<Float> diag = R.diag();
			cout << diag << endl;

			std::vector<DiagItem> d_item;
			d_item.reserve(diag.n_elem);

			uword c = 0;
			for (Col<Float>::iterator i = diag.begin(); i != diag.end(); i++, c++) {
				d_item.push_back(DiagItem(std::abs(*i), c));
			}

			std::sort(d_item.begin(), d_item.end());

			
			Float *decline = new Float[c];
			Float sum_eigen = 0;

			for (int i = 0; i < c; i++) {
				if (d_item[i].eigenvalue > 0)
					cout << i << " " << d_item[i].eigenvalue << endl;
			}

			for (int i = 0; i < c - 1; i++) {
				Float &e0 = d_item[i].eigenvalue;
				Float &e1 = d_item[i + 1].eigenvalue;

				if (e1 == 0) {
					break;
				}

				sum_eigen += e0;
				decline[i] = e0 / e1;
			}
			decline[c - 1] = 1.0f;
			sum_eigen += d_item[c - 1].eigenvalue;



			for (int i = 0; i < c; i++) {
				cout << i << " " << decline[i] << endl;
			}


			if (d_item[c - 1].eigenvalue / sum_eigen > 0.001) { // remain are large, expand and continue
				X.eye(X.n_rows, X.n_cols * 2);
				return true;
			}

			// find cut off rank
			Float sum_decline = decline[0];
			Float s = d_item[0].eigenvalue;
			Float min_grad = std::numeric_limits<Float>::infinity();
			int cut_rank = -1;
			for (int i = 1; i < c; i++) {
				Float d = decline[i] / (sum_decline / i);
				if (d <= min_grad) {
					min_grad = d;
					cut_rank = i;
				}
				sum_decline += decline[i];
				s += d_item[i].eigenvalue;

				if (s / sum_eigen > 0.99) {
					break;
				}

			}

			uvec contract_col (cut_rank);
			for (int i = 0; i < cut_rank; i++) {
				contract_col.at(i) = d_item[i].id;
			}

			X = X.cols(contract_col);

			cout << cut_rank << endl;

			return false;
		}
		*/
		
		bool PDMS_kernel(int &iteration_time, Fmat &XY, Fmat &S, const Fmat &D, const uvec &t_known, const std::vector<bool> &known_tag, uint32_t max_iteration /*= 100*/, Float tolerance /*= 1e-5*/, Fmat &X, Fmat &Y, Float norm_two) {

			int m = D.n_rows, n = D.n_cols, k = X.n_cols;
		
			//Float d_norm = norm(D, "fro");
			//if (d_norm == 0) {
			//	return false;
			//}

			Float beta = 1.0f / norm_two;
			beta *= 500;
			Float max_beta = beta * 1000000;
			//beta *= std::max(m, n) / 2;
			
			//cout << beta / d_norm << " " << beta / 
			Float gamma = 1.6;
			//gamma *= 10;

			XY = X * Y;
			Fmat Res = D - XY;
			
		
#ifdef ERROR_ROW	
			Fmat pow_mat = pow(D, 2);
			Fvec row_sum = sqrt(sum(pow_mat, 1));
			pow_mat = pow(Res, 2);
			Fvec row_error_0 = sqrt(sum(pow_mat, 1)) / row_sum;
			if (max(row_error_0) < tolerance) return true;

			Fvec row_error;
#endif


#ifdef ERROR_REM
			Float d_norm = norm(D, "fro");
			Float e0 = 1;
#endif

			Float lambda = 1.0f / sqrt(std::min(m, n));
			Float norm_inf = max(max(D)) / lambda;
			Float dual_norm = std::max(norm_two, norm_inf);
			if (dual_norm <= 0) {
				return false;
			}

			Fmat Lambda = D / dual_norm;
			

#ifdef NORM_1	
			S = Res;
			shrink(S, beta, known_tag);
#else
			S = (beta / (beta + 1.0f)) * Res - (1.0f / (beta + 1.0f))  * Lambda;
			S(t_known).zeros();
#endif // DEBUG

			Lambda = beta * (XY + S - D);

			beta = std::min(max_beta, beta * gamma);
			Fmat B, R, temp;	
			
			uint32_t it;
			for (it = 0; it < max_iteration; it++) {
			
				B = D - S - (1.0f / beta) * Lambda;		
				qr_econ(X, R, B * Y.t());		
				Y = X.t() * B;	
				XY = X * Y;

				Res = D - XY;
				
#ifdef ERROR_ROW 
				pow_mat = pow(Res, 2);
				row_error = sqrt(sum(pow_mat, 1)) / row_sum;
				if (max(row_error) < tolerance || max(row_error_0 - row_error) < tolerance) break;

				row_error_0 = row_error;
#endif

#ifdef ERROR_REM
				Float e = norm(Res, "fro") / d_norm;
				//cout << "e is " << e << " e0 is " << e0 << " " << (e0 - e) / e0 << endl;
				if (e < tolerance || (it > 0 && (e0 - e) / e0 < tolerance)) {
					break;
				}
				e0 = e;
#endif

#ifdef ERROR_CHG

				Float e2 = norm(Res - Res0, "fro") / norm(Res0, "fro");
				if (e2 < tolerance) {
					break;
				}

				Res0 = Res;
#endif


#ifdef NORM_1		
				S = Res - (1.0f / beta) * Lambda;
				shrink(S, beta, known_tag);
				
#else
	
				S = (beta / (beta + 1.0f)) * Res - (1.0f / (beta + 1.0f))  * Lambda;
				S(t_known).zeros();
#endif // DEBUG


				Lambda += beta * (S - Res);
				beta = beta * gamma;


			}
#ifdef PRINT
			cout << "rank " << X.n_cols << endl;
			cout << "iteration " << it << endl;
#endif
			iteration_time = it;
			return true;
		}

		void getKnownTag(uvec &t_known, std::vector<bool> &known_tag, const uvec &known, int m, int n, bool transpose) {
#ifdef NORM_1
			if (!transpose)
			{
				for (uvec::const_iterator i = known.begin(); i != known.end(); i++) {
					known_tag[*i] = true;
				}
			}
			else {

				for (uvec::const_iterator i = known.begin(); i != known.end(); i++) {

					uword c = *i / m;
					uword r = *i - c * m;

					known_tag[r * n + c] = true;
				}
			}

#endif
#if 1

			if (!transpose)
			{
				t_known = known;
			}
			else {
				t_known.set_size(known.n_elem);
				uword count = 0;
				for (uvec::const_iterator i = known.begin(); i != known.end(); i++) {

					uword c = *i / m;
					uword r = *i - c * m;

					t_known.at(count++) = r * n + c;
				}
				t_known = arma::sort(t_known);
			}
		
#endif
			 
		}


#ifdef PARTIAL_SVD_RANK_ESTIMATE
		bool estmate_rank(int &cut_rank, Col<Float> &S, uvec &known, Float sum_eigent) {

			int c = S.n_elem;

			Col<Float> decline(c, fill::ones);
			int num_small_decline = 0;

			for (int i = 0; i < c - 1; i++) {
				if (S(i + 1) <= 0) {
					break;
				}
				decline[i] = S.at(i) / S.at(i + 1);

				if (decline[i] < 2.0f) {
					num_small_decline++;
				}
			}

			std::vector<int> possible_cut;
			possible_cut.reserve(S.n_elem);

			// followed by flat
			Float t_sum = 0;
			Float m_thresh = RANK_MINIMUM * sum_eigent;
			for (int i = 0; i < c - 1; i++) {
				t_sum += S[i];
				if (t_sum > m_thresh && decline[i + 1] < 2.0f && decline[i] > decline[i + 1]) {
					possible_cut.push_back(i + 1);
				}
			}

			// remove rank constrain
			if (possible_cut.empty()) {

				for (int i = 0; i < c - 1; i++) {
					t_sum += S[i];
					if (decline[i + 1] < 2.0f && decline[i] > decline[i + 1]) {
						possible_cut.push_back(i + 1);
					}
				}

			}

			if (possible_cut.empty()) {
				cut_rank = c;
				return false;
			}


			// kick out small 
			{
				Float eigent_cutoff = sum_eigent * RANK_CUTOFF;

				std::vector<int> temp;
				for (int i = 0; i < possible_cut.size(); i++) {
					if (S(possible_cut[i]) > eigent_cutoff) {
						temp.push_back(possible_cut[i]);
					}
				}

				if (!temp.empty()) {
					possible_cut = temp;
				}
			}


			// kick out short terrace
			{

				int threshold = std::round(num_small_decline * 0.25);
				std::vector<int> temp;
				for (int i = 0; i < possible_cut.size() - 1; i++) {

					int start = possible_cut[i];
					int end = std::min((int)c - 1, start + threshold);

					bool kick = false;
					for (int j = start + 1; j < end; j++) {
						if (decline[j] > 2.0f) {
							kick = true;
							break;
						}
					}


					if (!kick) {
						temp.push_back(start);
					}

				}

				if (!temp.empty()) {
					possible_cut = temp;
				}

			}


			// find the most clear drop
			cut_rank = -1;
			Float max_decline = -FLT_MAX;
			for (int i = 0; i < possible_cut.size(); i++) {
				Float d = possible_cut[i] == c ? 1.0f : decline[possible_cut[i] - 1] / decline[possible_cut[i]];
				if (d > max_decline) {
					max_decline = d;
					cut_rank = possible_cut[i];
				}
			}
			
			
#ifdef PRINT

			cout << S << endl;
			cout << decline << endl;
			cout << "rank " << cut_rank << " " << sum(S.rows(0, cut_rank - 1)) / sum(S) << endl;
#endif
			
			
			return (cut_rank > 0);

		}



		bool initXY(Fmat &X, Fmat &Y, const Fmat &D, Uvec &known, uint32_t maximum_rank) {
			//maximum_rank *= 10;

			Col<Float> S;
			uint32_t c = maximum_rank;
		
			Float sum_eigent;
			Float eigent_cutoff;
	
			while (1) {
		
				if (!redsvd(X, S, Y, D, c)) {
					return false;
				}
	
				sum_eigent = sum(S);
				eigent_cutoff = sum_eigent * RANK_CUTOFF;
				
				if (S(c - 1) < eigent_cutoff) {
					break;
				}
				else {
					break;
					c = c * 2;
				}
				
			}

			Float last_max_difference = 0;
			Float last_grad = 0;
			Float sd_last = 0, sv_last = 0, v_sum_last = 0, sum_last = 0;
			Float another_last_valid = 0, another_last_difference = 0;
			std::vector<Float> valid_known;
			for (int i = 0; i < maximum_rank; i++) {

				Fmat S_current = X.cols(0, i) * arma::diagmat(S.rows(0, i)) * Y.cols(0, i).t();
				Float s_sum = sum(sum(S_current));

				Float another_valid = sum(sum((min(S_current, D))));
				Float max_difference = max(max(abs(S_current - D)));

				Float sum_diff = sum(sum(abs(S_current - D)));

				Float known_sum = sum(sum(D(known)));

				Fmat diff = abs(S_current - D);
				

			
				Float valid_current = sum(sum(S_current(known)));
				Float s_difference = sum(sum(diff(known)));
				Float another_difference = sum(sum(diff)) - s_difference;
				Float s_valid = sum(sum(min(S_current(known), D(known))));
				valid_known.push_back(s_difference);
				another_valid -= s_valid;

				Float a = abs(sd_last - s_difference);// / valid_current;
				Float b = abs(another_last_difference - another_difference);// / (s_sum - valid_current);
				
				Float grad = abs(another_last_difference - another_difference) / abs(sd_last - s_difference);

				//cout << i << " " << s_difference << " " << s_valid << " " << s_difference / s_valid << " " << s_difference / valid_current << " " << s_difference / s_sum << endl;
				//cout << (s_sum - sum_last) / s_sum << " " << (sd_last - s_difference) / s_valid << " " << s_valid - sv_last << " " << valid_current - v_sum_last << " " << (sd_last - s_difference) / (s_valid - sv_last) << endl;
				cout << s_difference << " " << (max_difference - last_max_difference) / (sd_last - s_difference) << " " << s_difference / valid_current << " " << (sd_last - s_difference) / valid_current << " " << (s_valid - sv_last) / (sd_last - s_difference) << " " <<
					a / (a + b)  << endl;//a << " " << b << " " << a / b << endl;

				sd_last = s_difference;
				sv_last = s_valid;
				v_sum_last = valid_current;
				sum_last = s_sum;
				another_last_valid = another_valid;
				another_last_difference = another_difference;
				last_grad = grad;
				last_max_difference = max_difference;
			}
			system("pause");
			Fvec vk(valid_known);
			cout << "valid known" << endl;
			cout << vk << endl;

			int cut_rank;
			if (!estmate_rank(cut_rank, S, known, sum_eigent)) {
				return false;
			}
	
	
			X = X.cols(0, cut_rank - 1);
			Y = Y.cols(0, cut_rank - 1);
			for (int i = 0; i < cut_rank; i++) {
				Y.col(i) *= S.at(i);
			}

			Y = Y.t();
			
			return true;
		}
		
		bool initXY_new(Fmat &X, Fmat &Y, const Fmat &D, Uvec &known, uint32_t maximum_rank, Float &norm_two) {
		
			uint32_t m = D.n_rows;
			uint32_t n = D.n_cols;
			
			Col<Float> S;
			uint32_t c = maximum_rank;
			uint32_t cut_rank = 0;
			uint32_t max_rank = std::min(D.n_rows, D.n_cols);

			Fmat D_known(m, n, fill::zeros);
			D_known(known) = D(known);

			Float known_norm = norm(D(known));
			Float last_difference = 0;
			Fmat onion(m, n, fill::zeros);

			if (!redsvd(X, S, Y, D, c)) {
				return false;
			}

			while (1) {
				
				norm_two = S.at(0);
				bool jump_out = false;
				for (; cut_rank < c; cut_rank++) {
				
					Y.col(cut_rank) *= S.at(cut_rank);
					onion += X.col(cut_rank) * Y.col(cut_rank).t();
					Float difference = norm(onion(known) - D(known));
					Float e = std::abs(last_difference - difference) / known_norm;
					//cout << e << " " << difference << endl;
					if (e < DIFF_THRESHOLD)
					{
						jump_out = true;
						break;
					}

					last_difference = difference;
				}
				
				if (jump_out || cut_rank >= max_rank) {
					break;
				}
				else {
					
					//Fmat X_new, Y_new;
					//Col<Float> S_new;
					//Fmat D_new = D - onion;
					//if (!redsvd(X_new, S_new, Y_new, D_new, c)) {
					//	return false;
					//}

					//X.insert_cols(X.n_cols - 1, X_new);
					//Y.insert_cols(Y.n_cols - 1, Y_new);
					//S.insert_cols(S.n_cols - 1, S_new);
					//c *= 2;

					uint32_t cc = std::min(max_rank, c * 2);
					if (!redsvd(X, S, Y, D, cc)) {
						return false;
					}
					onion = X.cols(0, c - 1) * diagmat(S.rows(0, c - 1)) * Y.cols(0, c - 1).t();

					c = cc;
				}

			}

			
			cut_rank = std::max((int)0, (int)cut_rank-1); // remove the last "insignificant" rank		
			assert(cut_rank < c && cut_rank >= 0);
			X = X.cols(0, cut_rank);
			Y = Y.cols(0, cut_rank).t();
			
			 
			//if (!estmate_rank(cut_rank, S, known, sum_eigent)) {
			//	return false;
			//}
			//cout << S << endl;

			//X = X.cols(0, cut_rank);
			//Y = Y.cols(0, cut_rank);
			//for (int i = 0; i <= cut_rank; i++) {
			//	//X.col(i) *= S.at(i);
			//}
			//Y =  Y.t();

			//{
			//	cout << "norm " << norm(D, "fro") << endl;

			//	Fmat XX, YY;
			//	Fvec SS;
			//	arma::svd_econ(XX, SS, YY, D);

			//	Fmat LL = XX.col(0) * SS.col(0) * YY.col(0).t();
			//	cout << norm(D -LL, "fro") / norm(D, "fro") << endl;
			//	
			//	Fmat L = X * Y;
			//	cout << norm(D - L, "fro") / norm(D, "fro") << endl;

			//}

			//system("pause");
			return true;
		}
#endif

#ifdef QR_RANK_ESTIMATE

		bool estmate_rank(int &cut_rank, Col<Float> & S, Float sum_eigent) {


			int c = S.n_elem;

			Col<Float> decline(c, fill::ones);
			int num_small_decline = 0;

			for (int i = 0; i < c - 1; i++) {
				if (S(i + 1) <= 0) {
					break;
				}
				decline[i] = S.at(i) / S.at(i + 1);

				if (decline[i] < 2.0f) {
					num_small_decline++;
				}
			}


			std::vector<int> possible_cut;
			possible_cut.reserve(S.n_elem);

			// followed by flat
			Float t_sum = 0;
			Float m_thresh = RANK_MINIMUM * sum_eigent;
			for (int i = 0; i < c - 1; i++) {
				t_sum += S[i];
				if (t_sum > m_thresh && decline[i + 1] < 2.0f && decline[i] > decline[i + 1]) {
					possible_cut.push_back(i + 1);
				}
			}

			if (possible_cut.empty()) {
				cut_rank = c;
				return true;
			}


			// kick out small 
			{
				Float eigent_cutoff = sum_eigent * RANK_CUTOFF;

				std::vector<int> temp;
				for (int i = 0; i < possible_cut.size(); i++) {
					if (S(possible_cut[i]) > eigent_cutoff) {
						temp.push_back(possible_cut[i]);
					}
				}

				if (!temp.empty()) {
					possible_cut = temp;
				}
			}


			// kick out short terrace
			{

				int threshold = std::round(num_small_decline * 0.25);
				std::vector<int> temp;
				for (int i = 0; i < possible_cut.size() - 1; i++) {

					int start = possible_cut[i];
					int end = std::min((int)c - 1, start + threshold);

					bool kick = false;
					for (int j = start + 1; j < end; j++) {
						if (decline[j] > 2.0f) {
							kick = true;
							break;
						}
					}


					if (!kick) {
						temp.push_back(start);
					}

				}

				if (!temp.empty()) {
					possible_cut = temp;
				}

			}


			// find the most clear drop
			cut_rank = -1;
			Float max_decline = -FLT_MAX;
			for (int i = 0; i < possible_cut.size(); i++) {
				Float d = possible_cut[i] == c ? 1.0f : decline[possible_cut[i] - 1] / decline[possible_cut[i]];
				if (d > max_decline) {
					max_decline = d;
					cut_rank = possible_cut[i];
				}
			}

			cout << decline << endl;
			cout << "rank " << cut_rank << endl;
			system("pause");
			return (cut_rank > 0);

		}



		bool initXY(Fmat &X, Fmat &Y, const Fmat &D, uint32_t maximum_rank) {

			Col<Float> S;
			uint32_t c = maximum_rank;

			Float sum_eigent;
			Float eigent_cutoff;

			Fmat temp, R;

			while (1) {
	
				X.zeros(D.n_rows, c);

				for (int i = 0; i < c; i++) {
					Fmat::size_type r = ((Float)std::rand() / RAND_MAX) * D.n_rows;
					r = std::min(r, D.n_rows - 1);
					X.at(r, i) = 1.0f;
				}

				fmat xx = X;

				clock_t t0;

				t0 = clock();
				for (int i = 0; i < 2; i++) {

					Y = X.t() * D;
					temp = D * Y.t();

					qr_econ(X, R, temp);
					cout << sum(abs(R.diag())) << endl;
				}

				cout << endl;
				cout << abs(R.diag()) << endl;

				Fmat U, V;
				fvec S;
				redsvd(U, S, V, D, c);
				cout << S << endl;

		
				cout << clock() - t0 << endl;
			

				sum_eigent = sum(S);
				eigent_cutoff = sum_eigent * RANK_CUTOFF;

				if (S(c - 1) < eigent_cutoff) {
					break;
				}
				else {
					break;
					c = c * 2;
				}

			}



			int cut_rank;
			if (!estmate_rank(cut_rank, S, sum_eigent)) {
				return false;
			}


			X = X.cols(0, cut_rank - 1);
			Y = Y.cols(0, cut_rank - 1);
			for (int i = 0; i < cut_rank; i++) {
				Y.col(i) *= S[i];
			}

			Y = Y.t();

			return true;
		}

#endif



	}

	bool TLAH_EXPORT PDMS(int &iteration_time, Fmat &L, Fmat &S, const Fmat &D, const Uvec &known, uint32_t maximum_rank, uint32_t max_iteration /*= 100*/, Float tolerance /*= 1e-5*/, Fmat *init_X /*= NULL*/, Fmat *init_Y /*= NULL*/)
	{
		maximum_rank = std::min((uint32_t)maximum_rank, std::min((uint32_t)D.n_rows, (uint32_t)D.n_cols));
		Float v_inf_norm = D.max();
		if (v_inf_norm <= 0) {
			return false;
		}
	
		//Float d_scale = 1.0f / v_inf_norm;
		Float d_scale = 1.0f;
		v_inf_norm = 1.0f;

		bool transpose = D.n_rows > D.n_cols ? true : false;
		transpose = false;
	
		Fmat t_D = d_scale * (transpose ? D.t() : D);
		std::vector<bool> known_tag(D.n_elem, false);
		Uvec t_known;
		getKnownTag(t_known, known_tag, known, D.n_rows, D.n_cols, transpose);
		

		Fmat X, Y; 
		Float norm_two = 0;

		if (init_X && init_Y) {
			cout << "bug" << endl;
			X = sqrt(d_scale) * (transpose ? init_Y->t() : *init_X);
			Y = sqrt(d_scale) * (transpose ? init_X->t() : *init_Y);
		}
		else {

			if (!initXY_new(X, Y, t_D, t_known, maximum_rank, norm_two)) {
				return false;
			}
		}

		//{
		//	L = X * Y;
		//	S = t_D - L;
		//	cout << "e 2 " << norm(S, "fro") / norm(t_D, "fro") << endl;
		//	L = v_inf_norm * (transpose ? L.t() : L);
		//	//L(0, 0) = yy(0, 0);
		//	S = D - L;


		//	

		//	return true;
		//}
		//{
		//	
		//	{

		//		Fmat U, V;
		//		Fvec SS;
		//		svd_econ(U, SS, V, t_D);

		//		L = U.col(0) * SS.col(0) * V.col(0).t();
		//	}
		//	Fmat Res = t_D - L;

		//	cout << "e first " << norm(Res, "fro") / norm(t_D, "fro") << endl;
		//	L = X * Y;

		//	{

		//		Fmat U, V;
		//		Fvec SS;
		//		redsvd(U, SS, V, t_D, 5);

		//		L = U.col(0) * SS.col(0) * V.col(0).t();
		//	}
		//
		//	L = v_inf_norm * (transpose ? L.t() : L);


		//	S = D - L;
		//	cout << "e 2 " << norm(S, "fro") / norm(D, "fro") << endl;

		//	return true;

		//}


		

		if (!PDMS_kernel(iteration_time, L, S, t_D, t_known, known_tag, max_iteration, tolerance, X, Y, norm_two)) {
			return false;
		}
	
		//MatrixXf A = MatrixXf::Random(3, 2);
		//cout << "Here is the matrix A:\n" << A << endl;
		//VectorXf b = VectorXf::Random(3,3);
		//cout << "Here is the right hand side b:\n" << b << endl;
		//cout << "The least-squares solution is:\n"
		//	<< A.jacobiSvd(ComputeThinU | ComputeThinV).solve(b) << endl;

		//Fmat yt = solve(X, D);
		//MatrixXf A = MatrixXf::Random(X.n_rows, X.n_cols);
		//memcpy(A.data(), X.memptr(), sizeof(Float)* X.n_rows * X.n_cols);
		//MatrixXf C = MatrixXf::Random(D.n_rows, D.n_cols);
		//memcpy(C.data(), D.memptr(), sizeof(Float)* D.n_rows * D.n_cols);
		//MatrixXf B = A.jacobiSvd(ComputeThinU | ComputeThinV).solve(C);
		//memcpy(Y.memptr(), B.data(), sizeof(Float)* Y.n_rows * Y.n_cols);
		//
		//Fmat yy = pinv(Y);
		
		//{

		//	Fmat Z = t_D;
		//	//Fmat yy = pinv(Y);
		//	Uvec rr = arma::randi<Uvec>(D.n_rows * D.n_cols * 0.01, distr_param(0, D.n_rows * D.n_cols));
		//	Fmat Z0 = Z(rr);
		//	Float n = norm(Z0,"fro");
		//	for (int i = 0; i < 10; i++) {
		//		X = Z*yy;
		//		Z = X*Y;
		//		
		//		//Z = Z * jj;
		//		Z(t_known) = t_D(t_known);
		//		if (norm(Z(rr) - Z0, "fro") / n < 0.01) break;
		//		Z0 = Z(rr);
		//	}
		//	//X = Z*yy;
		//}


		//L = X*Y;
	
		//S = v_inf_norm * (transpose ? S.t() : S);
		//L = D - S;

		L = v_inf_norm * (transpose ? L.t() : L);
		//L(0, 0) = yy(0, 0);
		S = D - L;


		return true;
	}


}




