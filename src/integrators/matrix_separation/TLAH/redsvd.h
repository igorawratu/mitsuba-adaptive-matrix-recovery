#ifndef __TLAH_REDSVD_H_
#define __TLAH_REDSVD_H_

/*
This is a partial svd package based on REDSVD.
*/

#include <random>
#include <armadillo>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace tlah {

	namespace util {

		const Float SVD_EPS = 0.000001f;


		inline void sampleTwoGaussian(Float& f1, Float& f2){
			Float v1 = (Float)(std::rand() + 1.f) / ((Float)RAND_MAX + 2.f);
			Float v2 = (Float)(std::rand() + 1.f) / ((Float)RAND_MAX + 2.f);
			Float len = std::sqrt(-2.f * std::log(v1));
			f1 = len * std::cos(2.f * M_PI * v2);
			f2 = len * std::sin(2.f * M_PI * v2);
		}

		template<typename Mat>
		void sampleGaussianMat(Mat &mat) {
			for (int i = 0; i < mat.n_rows; ++i){
				int j = 0;
				for (; j + 1 < mat.n_cols; j += 2){
					Float f1, f2;
					sampleTwoGaussian(f1, f2);
					mat.at(i, j) = f1;
					mat.at(i, j + 1) = f2;
				}
				for (; j < mat.n_cols; j++){
					Float f1, f2;
					sampleTwoGaussian(f1, f2);
					mat.at(i, j) = f1;
				}
			}
		}

		template<typename Mat>
		void processGramSchmidt(Mat& mat){
			for (int i = 0; i < mat.n_cols; ++i){
				for (int j = 0; j < i; ++j){
					Float r = arma::dot(mat.col(i), mat.col(j));
					mat.col(i) -= r * mat.col(j);
				}
				Float norm = arma::norm(mat.col(i), 2);
				if (norm < SVD_EPS){
					for (int k = i; k < mat.n_cols; ++k){
						mat.col(k).zeros();
					}
					return;
				}
				mat.col(i) *= (1.f / norm);
			}
		}

	}


	
	template<typename Mat, typename Vec>
	bool redsvd(Mat &U, Vec &S, Mat &V, const Mat &A, const uint32_t rank) {

		if (A.n_cols == 0 || A.n_rows == 0) return false;
		int r = (rank < A.n_rows) ? rank : A.n_rows;
		r = (r < A.n_cols) ? r : A.n_cols;

		// Gaussian Random Matrix for A^T
		Mat O(A.n_rows, r);
		util::sampleGaussianMat(O);

		// Compute Sample Matrix of A^T
		Mat Y = A.t() * O;

		// Orthonormalize Y
		util::processGramSchmidt(Y);

		// Range(B) = Range(A^T)
		Mat B = A * Y;

		// Gaussian Random Matrix
		Mat P(B.n_cols, r);
		util::sampleGaussianMat(P);

		// Compute Sample Matrix of B
		Mat Z = B * P;

		// Orthonormalize Z
		util::processGramSchmidt(Z);

		// Range(C) = Range(B)
		Mat C = Z.t() * B;

		bool success = arma::svd_econ(U, S, V, C);

		U = Z * U;
		V = Y * V;

		return success;
	}
}


#endif