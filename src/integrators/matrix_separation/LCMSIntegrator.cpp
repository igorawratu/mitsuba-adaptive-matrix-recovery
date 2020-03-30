#include "pluginExport.h"
#if PLUGIN_EXPORT == PLUGIN_EXPORT_LCMC

#include "ManyLightIntegrator.h"

#include <mitsuba/core/sfcurve.h>
#include <boost/container/vector.hpp>
#include <boost/container/list.hpp>
#include <boost/container/set.hpp>
//#include <boost/container/map.hpp>
#include <queue>

#include <mitsuba/core/timer.h>
#include <mitsuba/core/fstream.h>
#include <iostream>

#include "TLAH/tlah.h"
#include "TLAH/redsvd.h"

using boost::container::vector;
using boost::container::list;
using boost::container::set;
using std::map;

#include <map>

#define SAMPLE_RATIO_ADD 1.0f
#define MAX_AVG_EVAL 0.000001f

#define NORMAL_M_TERM -1

#define RAND_BOARD true
#define LMS_DIMENSION_4

#define SM_THRESHOLD 0.5
#define SM_BOARD 3
#define NOISE_THRESHOLD 0.1
#define DYNAMIC_SAMPLE_RATIO 0
#define ROW_DYNAMIC_SAMPLE true

#define K_NEIGHBOR 3
#define CURVE_COUNT 3

#define CURVE
//#define UV

//#define SAN
//#define CB
//#define SPONZA
//#define SF
//#define PREDICTOR
//#define SF_CLOSE
//#define CARNIVAL
#define CSPONZA

#define spp_M_TERM 0.5
#define M_TERM 0.1

#define MAX_ITERATION 10
#define STOP_ERROR 0.01
#define NEIGHBOR_RANGE 2
#define MAX_EXPAND_COLUMN 500

#define BORDER_REINFORCE 0
#define OUTLINE_REINFORCE 0
#define SINGULAR_REINFORCE 0
//#define SHADOW_REINFORCE 1000 * 1000 * 10
#define SHADOW_REINFORCE 10

#define NEIGHBOR 1
#define SQRT_2PI 2.5066282746310005024147107274575f

#define MAX_CUT_SIZE 400
#define SPARSE_THRESHOLD 0.02

#ifdef SPONZA
#define MAX_ITERATION 10
#define STOP_ERROR 0.01

#define NEIGHBOR_RANGE 1
#define MAX_EXPAND_COLUMN 4000

#define BORDER_REINFORCE 0
#define OUTLINE_REINFORCE 0
#define SINGULAR_REINFORCE 0
//#define SHADOW_REINFORCE 1000 * 1000 * 10
#define SHADOW_REINFORCE 20

#define NEIGHBOR 1
#define SQRT_2PI 2.5066282746310005024147107274575f

#define MAX_CUT_SIZE 500
#define SPARSE_THRESHOLD 0.01

#endif

#ifdef CSPONZA

#define spp_M_TERM -1
#define M_TERM -1

#define MAX_ITERATION 10
#define STOP_ERROR 0.01

#define NEIGHBOR_RANGE 1
#define MAX_EXPAND_COLUMN 5000

#define BORDER_REINFORCE 0
#define OUTLINE_REINFORCE 0
#define SINGULAR_REINFORCE 0
//#define SHADOW_REINFORCE 1000 * 1000 * 10
#define SHADOW_REINFORCE 20

#define MAX_CUT_SIZE 5000
#define SPARSE_THRESHOLD 0

#define NOISE_THRESHOLD 0.2
#define SAMPLE_RATIO_ADD 1
#define ROW_DYNAMIC_SAMPLE true
#define MAX_AVG_EVAL 0.00001f
#define DYNAMIC_SAMPLE_RATIO 0.2

#endif

#ifdef CB
#define MAX_AVG_EVAL 0.00001f
#define spp_M_TERM -99999
#define M_TERM -0.1

#define NEIGHBOR_RANGE 2
#define MAX_EXPAND_COLUMN 4000

#define BORDER_REINFORCE 0
#define OUTLINE_REINFORCE 0
#define SINGULAR_REINFORCE 0
//#define SHADOW_REINFORCE 1000 * 1000 * 10
#define SHADOW_REINFORCE 20
#define SAMPLE_RATIO_ADD 2

#define NEIGHBOR 1
#define SQRT_2PI 2.5066282746310005024147107274575f

#define MAX_CUT_SIZE 50
#define SPARSE_THRESHOLD 0.16

#define MAX_ITERATION 10
#define STOP_ERROR 0.01

#define SM_THRESHOLD 0.5
#define SM_BOARD 3
#define NOISE_THRESHOLD 0.2
#define DYNAMIC_SAMPLE_RATIO 0.2

#define ROW_DYNAMIC_SAMPLE true

#endif

#ifdef SF

#define M_TERM -1
#define spp_M_TERM -1
#define MAX_CUT_SIZE 300
#define SPARSE_THRESHOLD 0.015
#define NOISE_THRESHOLD 0.2
#define SHADOW_REINFORCE 10
#define DYNAMIC_SAMPLE_RATIO 0.2
#define MAX_EXPAND_COLUMN 500
#define ROW_DYNAMIC_SAMPLE true
#define MAX_AVG_EVAL 0.000001f
#define NORMAL_M_TERM -0.97

#define SAMPLE_RATIO_ADD 5

#endif


#ifdef SF_CLOSE

#define M_TERM -0.5
#define spp_M_TERM -1
#define MAX_CUT_SIZE 300
#define SPARSE_THRESHOLD 0.02
#define NOISE_THRESHOLD 0.1
#define SHADOW_REINFORCE 10
#define DYNAMIC_SAMPLE_RATIO 0.2
#define MAX_EXPAND_COLUMN 1000
#define ROW_DYNAMIC_SAMPLE true
#define MAX_AVG_EVAL 0.00001f
#define NORMAL_M_TERM 0//0.97

#define SAMPLE_RATIO_ADD 2

#define MAX_ITERATION 10
#define STOP_ERROR 0.01


//#define M_TERM -0.5
//#define spp_M_TERM -1
//#define MAX_CUT_SIZE 300
//#define SPARSE_THRESHOLD 0.01
//#define NOISE_THRESHOLD 0.1
//#define SHADOW_REINFORCE 10
//#define DYNAMIC_SAMPLE_RATIO 2
//#define MAX_EXPAND_COLUMN 1000
//#define ROW_DYNAMIC_SAMPLE true
//#define MAX_AVG_EVAL 0.000001f
//#define NORMAL_M_TERM 0//0.97
//
//#define SAMPLE_RATIO_ADD 1
//
//#define MAX_ITERATION 20
//#define STOP_ERROR 0.01

#endif

#ifdef SAN

#define M_TERM -1
#define spp_M_TERM -1
#define MAX_CUT_SIZE 500
#define SPARSE_THRESHOLD 0.01
#define NOISE_THRESHOLD 0.2
#define SHADOW_REINFORCE 20
#define DYNAMIC_SAMPLE_RATIO 0.5
#define MAX_EXPAND_COLUMN 2000
#define ROW_DYNAMIC_SAMPLE false
#define MAX_AVG_EVAL 0.00005f
#define MAX_ITERATION 20
#define STOP_ERROR 0.01

#define SAMPLE_RATIO_ADD 1

#endif


#ifdef PREDICTOR

#define M_TERM -0.5
#define spp_M_TERM -1
#define MAX_CUT_SIZE 500
#define SPARSE_THRESHOLD 0.01
#define NOISE_THRESHOLD 0.1
#define SHADOW_REINFORCE 20
#define DYNAMIC_SAMPLE_RATIO 1
#define MAX_EXPAND_COLUMN 2000
#define ROW_DYNAMIC_SAMPLE false
#define MAX_AVG_EVAL 0.00001f
#define NORMAL_M_TERM 0.97

#define MAX_ITERATION 10
#define STOP_ERROR 0.01

#define SAMPLE_RATIO_ADD 1

#endif

#ifdef CARNIVAL

#define M_TERM -1
#define spp_M_TERM -1
#define MAX_CUT_SIZE 500
#define SPARSE_THRESHOLD 0.02
#define NOISE_THRESHOLD 0.2
#define SHADOW_REINFORCE 20
#define DYNAMIC_SAMPLE_RATIO 2
#define MAX_EXPAND_COLUMN 5000
#define ROW_DYNAMIC_SAMPLE false
#define MAX_AVG_EVAL 0.00001f
#define NORMAL_M_TERM 1

#define SAMPLE_RATIO_ADD 1
#endif



// 0.005
//#define INIT_HARD_RATIO 2

//#define EST_HARD_RATIO 1
//#define VIS_HARD_RATIO 1

MTS_NAMESPACE_BEGIN

namespace {
	template <class T>
	inline int
		sign(T a)
	{
		return (a > T(0)) ? 1 : ((a < T(0)) ? -1 : 0);
	}

	struct AOMap {

		std::vector<Point> position;
		std::vector<Point> g_position;
		std::vector<uint32_t> n_sample;

		Point2i offset;
		Vector2i size;

		void init(const TAABB<Point2i> &aabb) {

			size = aabb.getExtents();
			size.x += 1;
			size.y += 1;
			offset = aabb.min;

			position.clear();
			position.resize(size.x * size.y, Point(0.0f));
			g_position.clear();
			g_position.resize(size.x * size.y, Point(0.0f));

			n_sample.clear();
			n_sample.resize(size.x * size.y, 0);
		}

		uint32_t index(const Point2i &p) const {
			Vector2i v = p - offset;
			return v.x + v.y * size.x;
		}
	};


	Point absolute(Point &a) {
		return Point(std::abs(a.x), std::abs(a.y), std::abs(a.z));
	}


	struct Column {

		enum BalanceMode
		{
			BInitial = 0,
			BAfterContract,
			BToExpand, 
			BAfterExpand,
			BAfterRowSample
		};

		enum PredictorSelection : int32_t
		{
			PNONE = -1,
			PNBM = 0,
			PKNN = 1,
			PLMS = 2
		};

		PredictorSelection p_selection = PNONE;

		const VLightNode *lightNode;

		Float pdf;
		Float max_eval;
		Float avg_eval;
		Float sum_est_visibility;


		// expand part
		Float max_est;
		Float min_est; 
		int16_t max_vis;

		Float highest_cost_now;
		Float highest_est_now;
		int16_t highest_vis_now;

		uint32_t expand_times;
		int expand_col_id;

		Float error;
		//Spectrum maxEstimate;

		uint32_t nKnown;
		BalanceMode balance;
		Float accu_error;

		uint32_t n_sample = 0;
		uint32_t n_visible = 0;

		Spectrum spur_intensity;
		Float avg_intensity;

		int col_id;

		struct Elem {
			Float estimate;
			bool estimate_tag;
			int16_t visibility;
			//bool visibility_tag;
			//Float random;
			Spectrum m_rgb;
		};

		vector<Elem> element;
		
		inline Spectrum &m_rgb(uint32_t r) {
			return element[r].m_rgb;
		}

// 		inline Float &random(uint32_t r) {
// 			return element[r].random;
// 		}

		inline Float &estimate(uint32_t r) {
			return element[r].estimate;
			//return _estimate[r];
		}

		inline bool &estimate_tag(uint32_t r) {
			return element[r].estimate_tag;
			//return _estimate_tag[r];c
		}

		inline int16_t &visibility(uint32_t r) {
			return element[r].visibility;
			//return _visibility[r];
		}

// 		inline bool &visibility_tag(uint32_t r) {
// 			return element[r].visibility_tag;
// 			//return _visibility_tag[r];
// 		}

		//vector<Float> _estimate;
		//vector<bool> _estimate_tag;
		//vector<int16_t> _visibility;
		//vector<bool> _visibility_tag;

		//list<uint32_t> known_estimate;
		//list<uint32_t> known_visibility;
		list<uint32_t, PoolAllocator<uint32_t>> known_estimate;
		list<uint32_t, PoolAllocator<uint32_t>> known_visibility;

		//list<uint32_t, boost::fast_pool_allocator<uint32_t>> known_estimate;
		//list<uint32_t, boost::fast_pool_allocator<uint32_t>> known_visibility;

#ifdef UV
		std::map < Float, int, std::less<Float>, PoolAllocator<std::pair<Float, int>>> u_known_sample;
		std::map < Float, int, std::less<Float>, PoolAllocator<std::pair<Float, int>>> v_known_sample;
		
#endif
		typedef std::map < Float, int, std::less<Float>, PoolAllocator<std::pair<Float, int>>>::iterator CROSS_IT;

		typedef std::map < int, int, std::less<int>, PoolAllocator<std::pair<int, int>>>::iterator CURVE_IT;
#ifdef CURVE
		std::vector<std::map < int, int, std::less<int>, PoolAllocator<std::pair<int, int>>>> curve_known_sample;
#endif
		Column(const VLightNode *l, const Spectrum &li_spur, boost::shared_ptr<PoolManager> &pool) : lightNode(l),
			balance(BInitial), known_estimate(PoolAllocator<uint32_t>(pool)), known_visibility(PoolAllocator<uint32_t>(pool)) 
#ifdef UV
			, u_known_sample(PoolAllocator<std::pair<Float, uint32_t>>(pool)), v_known_sample(PoolAllocator<std::pair<Float, uint32_t>>(pool))
#endif
		{

#ifdef CURVE
			for (int i = 0; i < CURVE_COUNT; i++) {
				curve_known_sample.emplace_back(pool);
			}			
#endif	
			min_est = FLT_MAX;
			max_vis = 0;
			sum_est_visibility = accu_error = max_eval = avg_eval = max_est = 0;
			spur_intensity = lightNode->intensity * li_spur;
			avg_intensity = spur_intensity.average();
			error = 0;
			col_id = -1;
			highest_cost_now = highest_est_now = 0;
			highest_vis_now = 0;
			expand_times = 0;
			expand_col_id = 0;
		}
		
		void init(uint32_t n_row) {

			Elem init;
			init.estimate = -1.0f;
			init.visibility = -1;
			init.m_rgb = Spectrum(-1.0f);
			init.estimate_tag = false;
			//init.random = -1;
			//init.visibility_tag = false;		

			element.clear();
			element.resize(n_row, init);


			//_estimate.resize(n_row, -1.0f);
			//_visibility.resize(n_row, -1);
			//_estimate_tag.resize(n_row, false);
			//_visibility_tag.resize(n_row, false);

		}

	};

	struct Row {

		typedef uint32_t CodeType;
		const static CodeType CodeBit = 32;

		const VSensorNode *sensorNode;
		const VSensor *sensor;

		struct spp {
			const VSensor *sensor;
			Spectrum MVTDI;

			spp(const VSensor *s) : sensor{ s }, MVTDI{ 0.0f } { }
		};

		std::vector<spp> spp_sensors;

		CodeType p_code;
		CodeType p_bit;
		Float odd_pdf = 1.0f;
		Float shadow_pdf = 1.0f;
		Float average;

		Float u_coord;
		Float v_coord;

		uint32_t n_visible = 0;
		uint32_t n_sample = 0;

		Point2i hilbert_position;
		int hilbert_order[CURVE_COUNT];

		Row(const VSensorNode *s) : sensorNode(s), sensor(s->rep_sensor) {
			
		}

		void setCode(CodeType p_c, CodeType p_b) {
			p_code = (p_c << (Row::CodeBit - p_b));
			p_bit = p_b;
		}


		bool operator< (const Row &p2) const
		{
			return p_code < p2.p_code;
		}

		static inline CodeType bsr32(CodeType bb)
		{
			static const char msb_256_table[256] =
			{
				0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
				4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
			    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
				6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
				6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
				7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
				7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
				7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
				7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
			};

		    uint32_t result = 0;

			if (bb > 0xFFFF)
			{
				bb >>= 16;
				result += 16;
			}
			if (bb > 0xFF)
			{
				bb >>= 8;
				result += 8;
			}
			
			return (result + msb_256_table[bb]);
		}

		static inline CodeType bsr64(uint64_t bb)
		{
			const uint32_t hb = bb >> 32;
			return hb ? 32 + bsr32((uint32_t)hb) : bsr32((uint32_t)bb);		
		}

		inline CodeType distance(const Row &r) {

			return p_bit - ((CodeBit - 1) - bsr32(p_code ^ r.p_code));

		}
	};


	struct LTMatrix {

	public:
		uint32_t n_row;

		const Scene *m_scene;
		Float m_clamping;
		Float m_cutoff;

	public:
		list<Column, boost::fast_pool_allocator<Column>> columns;

		typedef list<Column, boost::fast_pool_allocator<Column>>::iterator COL_IT;

		//////////////////////////////////////////////////////////////////////////


		struct UVLink {
			Float coord;
			int r_id;
			UVLink(Float c, int r) : coord{ c }, r_id { r } { }

			bool operator<(const UVLink &b) {
				return coord < b.coord;
			}
		};

		std::vector<UVLink> u_link;
		std::vector<UVLink> v_link;


		// curve coord
#ifdef CURVE
		struct CurveLink {
			uint32_t curve_coord;
			int r_id;
			CurveLink(uint32_t c, int r) : curve_coord{ c }, r_id{ r } { }
		};

		std::vector<CurveLink> curve_link[CURVE_COUNT];
#endif

		//////////////////////////////////////////////////////////////////////////

		vector<COL_IT> fix_columns;
		vector<Row> rows;

		vector<uint32_t> row_split;

		Float sum_max_eval;
		Float sum_avg_eval;
		Float sum_error;

		uint32_t n_valid_column;

		Fmat row_distance_map;
		Float row_sigma;

		Spectrum li_spur;
		const VSensorNode *m_slice_root;

		arma::imat pixel_2_row_map;
		Point2i pixel_offset;

		boost::shared_ptr<PoolManager> m_pool_manager;

		LTMatrix(const VSensorNode *slice_root, const Scene *scene, Float clamp, Float cutoff, ref<Random> random) : m_pool_manager(new PoolManager()) {

			m_slice_root = slice_root;
			TAABB<Point2i> image_box = slice_root->sampleBbox;

			pixel_offset = image_box.min;
			Vector2i pixel_size = image_box.getExtents();
			pixel_size.x += 1; pixel_size.y += 1;

			pixel_2_row_map.set_size(pixel_size.x, pixel_size.y);
			pixel_2_row_map.fill(-1);
	

			std::vector<const VSensorNode *> sensor_stack;
			sensor_stack.push_back(slice_root);

			int row_index = 0;

			while (!sensor_stack.empty()) {

				const VSensorNode *cur = sensor_stack.back();
				sensor_stack.pop_back();

				if (cur->isLeaf()) {

					Vector2i p = cur->rep_sensor->getImagePos() - pixel_offset;
		
					if (pixel_2_row_map.at(p.x, p.y) < 0) {
						rows.emplace_back(cur);
						pixel_2_row_map.at(p.x, p.y) = row_index++;
					}
					
					int r = pixel_2_row_map.at(p.x, p.y);
					rows[r].spp_sensors.emplace_back(cur->rep_sensor);
				}
				else {

		
					if (random->nextFloat() < 0.5) {
						sensor_stack.push_back(cur->left);
						sensor_stack.push_back(cur->right);
					}
					else {
						sensor_stack.push_back(cur->right);
						sensor_stack.push_back(cur->left);
					}
				}

			}


			n_row = rows.size();
			m_scene = scene;
			m_clamping = clamp;
			m_cutoff = cutoff;
			sum_error = 0;

			li_spur = Spectrum(1.0f);
			temp_selection.reserve(n_row);
	
			row_distance_map.set_size(n_row, n_row);
			row_distance_map.fill(-1);
		}


		Vector n_directin;
		Vector u_direction;
		Vector v_direction;
		Point o_center;

		//////////////////////////////////////////////////////////////////////////

		struct NBMPredictor {

			uint32_t n_0;
			uint32_t n_1;

			Float priori_0;
			Float priori_1;

			vector<Float> row_likelihood_0;
			vector<Float> row_likelihood_1;
			vector<Float> column_likelihood_0;
			vector<Float> column_likelihood_1;

			uint32_t n_row;
			uint32_t r_threshold;
			uint32_t n_column;
			uint32_t c_threshold;

			void init(LTMatrix &matrix) {
				n_row = matrix.rows.size();
				r_threshold = std::round(n_row * 0.03);
				n_column = matrix.columns.size();
				c_threshold = std::round(n_column * 0.03);

				row_likelihood_0.resize(n_row, 0.0f);
				row_likelihood_1.resize(n_row, 0.0f);

				column_likelihood_0.resize(n_column, 0.0f);
				column_likelihood_1.resize(n_column, 0.0f);
			}

			void initPriori(LTMatrix &matrix) {

				n_0 = 0;
				n_1 = 0;
				uint32_t n_sample = 0;

				for (auto c : matrix.fix_columns) {
					n_sample += c->n_sample;
					n_1 += c->n_visible;
				}

				n_0 = n_sample - n_1;
				
				if (n_0 == 0) {
					n_sample++;
					n_0++;
				}

				if (n_1 == 0) {
					n_sample++;
					n_1++;
				}

				priori_0 = (Float)n_0 / (n_sample);
				priori_1 = 1.0f - priori_0;
			}

			void increasePrioi(uint32_t n_visible, uint32_t n_invisible) {
				n_0 += n_invisible;
				n_1 += n_visible;

				uint32_t n_sample = n_0 + n_1;

				priori_0 = (Float)n_0 / (n_sample);
				priori_1 = 1.0f - priori_0;
			}

			void updateRowLikeliHood(LTMatrix &matrix, uint32_t r) {

				uint32_t ns = matrix.rows[r].n_sample;
				uint32_t nv = matrix.rows[r].n_visible;

				if (nv == 0) {
					nv++;
					ns++;
				}

				//if (ns == nv) {
				//	ns++;
				//}

				row_likelihood_0[r] = (Float)(ns - nv) / n_0;
				row_likelihood_1[r] = (Float)(nv) / n_1;
			}

			void updateColumnLikeliHood(LTMatrix &matrix, uint32_t c) {

				uint32_t ns = matrix.fix_columns[c]->n_sample;
				uint32_t nv = matrix.fix_columns[c]->n_visible;

				if (nv == 0) {
					nv++;
					ns++;
				}

				//if (ns == nv) {
				//	ns++;
				//}

				column_likelihood_0[c] = (Float)(ns - nv) / n_0;
				column_likelihood_1[c] = (Float)(nv) / n_1;
			}

			Float getPosteriorPDF0(uint32_t r, uint32_t c, LTMatrix &matrix) {

				Float r0 = row_likelihood_0[r]; Float r1 = row_likelihood_1[r];
				uint32_t r_sample = matrix.rows[r].n_sample; int off = 1;

				while ((r_sample < r_threshold || off < 2) && off < 4) {
					int rr = r - off;
					//cout << "rr1 " << rr << " " << r_threshold << " " << matrix.fix_columns.size() << " " << endl;
					if (rr >= 0) {		
						//assert(rr >= 0 && rr < n_row);
						//assert(n_row == matrix.rows.size());
						r_sample += matrix.rows[rr].n_sample;
						r0 += row_likelihood_0[rr];
						r1 += row_likelihood_1[rr];
					}

					rr = r + off;
					if (rr < n_row) {
						//cout << "rr2 " << rr << endl;
						//assert(rr >= 0 && rr < n_row);
						//assert(n_row == matrix.rows.size());

						r_sample += matrix.rows[rr].n_sample;
						r0 += row_likelihood_0[rr];
						r1 += row_likelihood_1[rr];
					}

					off++;
				}

				Float c0 = column_likelihood_0[c]; Float c1 = column_likelihood_1[c];

				uint32_t c_sample = matrix.fix_columns[c]->n_sample; off = 1;
				while ((c_sample < c_threshold || off < 2) && off < 4) {

					int cc = c - off;
				
					if (cc >= 0) {
						c_sample += matrix.fix_columns[cc]->n_sample;
						c0 += column_likelihood_0[cc];
						c1 += column_likelihood_1[cc];
					}

					cc = c + off;
				
					if (cc < n_column) {
						c_sample += matrix.fix_columns[cc]->n_sample;
						c0 += column_likelihood_0[cc];
						c1 += column_likelihood_1[cc];
					}

					off++;
				}
		
		
				Float p0 = priori_0 * r0 * c0;
				Float p1 = priori_1 * r1 * c1;

				//cout << (p0 + p1) * n_row * n_column  << endl;

				return p0 / (p0 + p1);
			}

		};

		NBMPredictor m_nbm_predictor;

		void updateNBMPredictor() {

			uint32_t n_column = fix_columns.size();
			uint32_t n_row = rows.size();

			//nbm_predictor.init(*this);
			m_nbm_predictor.initPriori(*this);

			for (uint32_t c = 0; c < n_column; c++) {
				m_nbm_predictor.updateColumnLikeliHood(*this, c);
			}
			for (uint32_t r = 0; r < n_row; r++) {
				m_nbm_predictor.updateRowLikeliHood(*this, r);
			}
		}


		struct	LMSPredictor {

			Fmat W_vectors;
			vector<uint32_t> c_sample;

			uint32_t n_row;
			uint32_t n_column;

			void init(LTMatrix &matrix) {

				n_row = matrix.rows.size();
				n_column = matrix.fix_columns.size();
				c_sample.resize(n_column, 0);

#ifdef LMS_DIMENSION_4
				W_vectors.zeros(4, n_column);
#else 
				W_vectors.zeros(3, n_column);
#endif

			}

			Float P0(uint32_t r, uint32_t c, LTMatrix &matrix) {
				Float *col = W_vectors.colptr(c);
				Float y;
#ifdef LMS_DIMENSION_4
				const Point &p = matrix.rows[r].sensor->its.p;			
				y = col[0] * p.x + col[1] * p.y + col[2] * p.z + col[3];
#else
				y = col[0] * matrix.rows[r].u_coord + col[1] * matrix.rows[r].v_coord + col[2];
#endif
				Float y0 = std::max(0.0f, 0.5f - y);
				Float y1 = std::max(0.0f, y + 0.5f);

				//y0 *= y0;
				//y1 *= y1;

				return y0 / (y0 + y1);
			}

			void UpdateW(uint32_t c, LTMatrix &matrix) {

				LTMatrix::COL_IT &column = matrix.fix_columns[c];
				uint32_t m = column->known_estimate.size();

				if (m == c_sample[c]) {
					return;
				}
				else {
					c_sample[c] = m;
				}

#ifdef LMS_DIMENSION_4
				Fmat x(m, 4);
				x.col(3).fill(1.0f);
#else
				Fmat x(m, 3);
				x.col(2).fill(1.0f);
#endif			
				Fvec y(m);
				y.fill(1.0f);

				uint32_t count = 0;
				for (auto r : column->known_estimate) {

#ifdef LMS_DIMENSION_4
					const Point &p = matrix.rows[r].sensor->its.p;
					x.at(count, 0) = p.x; x.at(count, 1) = p.y; x.at(count, 2) = p.z;
#else
					uint32_t d = 3;
					x.at(count, 0) = matrix.rows[r].u_coord; x.at(count, 1) = matrix.rows[r].v_coord;
#endif
					if (column->visibility(r) <= 0) {
						y.at(count) = -1.0f;
					}
					count++;
				}
	
				W_vectors.col(c) = pinv(x) * y;// solve(x, y);
	
			}

		};

		LMSPredictor m_lms_predictor;

		void updateLMSPredictor() {

			uint32_t n_column = fix_columns.size();

			for (uint32_t c = 0; c < n_column; c++) {
				if (fix_columns[c]->p_selection == Column::PLMS) {
					m_lms_predictor.UpdateW(c, *this);
				}
			}

		}


		//////////////////////////////////////////////////////////////////////////

		void removeEmptyColumn() {
			for (list<Column, boost::fast_pool_allocator<Column>>::iterator column = columns.begin(); column != columns.end(); ) {
				if (column->max_eval == 0.0f) {
					column = columns.erase(column);
				}
				else {
					column++;
				}
			}
		}

		void generateCrossCoordinate() {

			o_center = m_slice_root->bbox.getCenter();
			n_directin = m_slice_root->normal_cone.GetAxis();
	
			int largest_axis = m_slice_root->bbox.getLargestAxis();
			Vector potential_u(0.0f);
			potential_u[largest_axis] = 1.0f;
			Float v = dot(n_directin, potential_u);
			int cc = 0;
			while (std::min(std::fabs(v + 1.0f), std::fabs(v - 1.0f)) < 0.001) {
				potential_u[largest_axis] = 0.0f;
				largest_axis = (largest_axis + 1)% 3;
				potential_u[largest_axis] = 1.0f;
				v = dot(n_directin, potential_u);
				cc++;
				if (cc > 4) {
					cout << v << " " << n_directin.toString() << endl;
				}
			}
		
			u_direction = normalize(cross(potential_u, n_directin));
			v_direction = cross(n_directin, u_direction);

			int n_row = rows.size();

			for (int r = 0; r < n_row; r++) {
				Row &row = rows[r];
				Vector v = row.sensor->its.p - o_center;
				row.u_coord = dot(v, u_direction);
				row.v_coord = dot(v, v_direction);
				
			}
	

			u_link.clear();
			u_link.reserve(n_row);
			v_link.clear();
			v_link.reserve(n_row);
			for (int r = 0; r < n_row; r++) {
				Row &row = rows[r];
				u_link.emplace_back(row.u_coord, r);
				v_link.emplace_back(row.v_coord, r);
			}
			std::sort(u_link.begin(), u_link.end(), [](const UVLink &a, const UVLink &b) { return a.coord < b.coord; });
			std::sort(v_link.begin(), v_link.end(), [](const UVLink &a, const UVLink &b) { return a.coord < b.coord; });

			
			if (0) {
				int find_r = -1;
				for (int r = 0; r < n_row; r++) {
					if (rows[r].sensor->getImagePos() == Point2i(113, 338)) {
						find_r = r;
						break;
					}
				}

				{
					int p = -1;
					for (int r = 0; r < n_row; r++) {
						if (u_link[r].r_id == find_r) {
							cout << rows[u_link[r - 1].r_id].sensor->getImagePos().toString() << " " << rows[u_link[r + 1].r_id].sensor->getImagePos().toString() << endl;
							break;
						}
					}
				}

				{
					int p = -1;
					for (int r = 0; r < n_row; r++) {
						if (v_link[r].r_id == find_r) {
							cout << find_r << " " << n_row << endl;
							cout /*<< rows[v_link[r - 1].r_id].sensor->getImagePos().toString()*/ << " " << rows[v_link[r + 1].r_id].sensor->getImagePos().toString() << endl;
							break;
						}
					}
				}

			}


#ifdef CURVE

			AABB2 uv_bound;
			for (int r = 0; r < n_row; r++) {
				uv_bound.expandBy(Point2(rows[r].u_coord, rows[r].v_coord));
			}

			Vector2 uv_extend = uv_bound.getExtents();
			uv_extend.x = std::max(uv_extend.x, 0.01f);
			uv_extend.y = std::max(uv_extend.y, 0.01f);
			Float y_x = uv_extend.y / uv_extend.x;

			Vector2i hilbert_size;
			hilbert_size[0] = std::ceil(sqrt((Float)2 * n_row / y_x));
			hilbert_size[1] = std::ceil(hilbert_size[0] * y_x);
			hilbert_size[0] += CURVE_COUNT - 1;
			hilbert_size[1] += CURVE_COUNT - 1;
		
			HilbertCurve2D<int32_t> hilbert_curve;
			hilbert_curve.initialize(hilbert_size);

			auto points = hilbert_curve.getPoints();		

			std::vector<int> hilbert_position_2_order(hilbert_size.x * hilbert_size.y, -1);

			for (int i = 0; i < points.size(); i++) {
				Point2i p = points[i];
				hilbert_position_2_order[p.x + p.y * hilbert_size.x] = i;
			}


			std::vector<Point2i> p_hilbert_position;
			p_hilbert_position.reserve(n_row);

			Vector2i scale = hilbert_size;
			scale[0] -= CURVE_COUNT - 1;
			scale[1] -= CURVE_COUNT - 1;
			for (int r = 0; r < n_row; r++) {
				rows[r].hilbert_position.x = std::min((int)(((rows[r].u_coord - uv_bound.min[0]) / uv_extend[0]) * scale[0]), scale[0] - 1);
				rows[r].hilbert_position.y = std::min((int)(((rows[r].v_coord - uv_bound.min[1]) / uv_extend[1]) * scale[1]), scale[1] - 1);
			}

			for (int r = 0; r < n_row; r++) {
				for (int i = 0; i < CURVE_COUNT; i++) {
					rows[r].hilbert_order[i] = hilbert_position_2_order[rows[r].hilbert_position.x + i + (rows[r].hilbert_position.y + i) * hilbert_size.x];
					assert(rows[r].hilbert_order[i] >= 0 && rows[r].hilbert_order[i] < hilbert_size.x * hilbert_size.y);
				}
			}

			for (int i = 0; i < CURVE_COUNT; i++) {
				curve_link[i].clear();
				curve_link[i].reserve(n_row);
			}


			for (int r = 0; r < n_row; r++) {
				Row &row = rows[r];
				for (int i = 0; i < CURVE_COUNT; i++) {
					curve_link[i].emplace_back(row.hilbert_order[i], r);
				}
			}

			for (int i = 0; i < CURVE_COUNT; i++) {
				std::sort(curve_link[i].begin(), curve_link[i].end(), [](const CurveLink &a, const CurveLink &b) { return a.curve_coord < b.curve_coord; });
			}


			//{
			//	int find_r = -1;
			//	for (int r = 0; r < n_row; r++) {
			//		if (rows[r].sensor->getImagePos() == Point2i(192, 675)) {
			//			find_r = r;
			//			break;
			//		}
			//	}
			//	
			//	for (int i = 0; i < CURVE_COUNT; i++) {
			//		int p = -1;
			//		for (int r = 0; r < n_row; r++) {
			//			if (curve_link[i][r].r_id == find_r) {
			//				cout << rows[curve_link[i][r - 1].r_id].sensor->getImagePos().toString() << " " << rows[curve_link[i][r + 1].r_id].sensor->getImagePos().toString() << endl;
			//				break;
			//			}
			//		}
			//	}


			//}

#endif
			
		}


		void crospprojection(uint32_t c) {

#ifdef UV
			auto &column = fix_columns[c];
				// boarder
				column->u_known_sample.insert(std::make_pair(-FLT_MAX, -1));
				column->u_known_sample.insert(std::make_pair(FLT_MAX, -1));

				column->v_known_sample.insert(std::make_pair(-FLT_MAX, -1));
				column->v_known_sample.insert(std::make_pair(FLT_MAX, -1));

				for (auto &sample : column.known_estimate) {
					column->u_known_sample.insert(std::make_pair(rows[sample].u_coord, sample));
					column->v_known_sample.insert(std::make_pair(rows[sample].v_coord, sample));
				}

				assert(column->u_known_sample.size() > 0);
			
#endif

#ifdef CURVE 

				auto &column = fix_columns[c];
				// boarder
				for (int i = 0; i < CURVE_COUNT; i++) {
					column->curve_known_sample[i].insert(std::make_pair(INT_MIN, -1));
					column->curve_known_sample[i].insert(std::make_pair(INT_MAX, -1));
				}

				for (auto &sample : column->known_estimate) {
					for (int i = 0; i < CURVE_COUNT; i++) {
						column->curve_known_sample[i].insert(std::make_pair(rows[sample].hilbert_order[i], sample));
					}
				}

			//	assert(column.curve_known_sample[0].size() > 0);
			
#endif

		}

		void updateColumnAverage() {

			list<Column>::iterator end = columns.end();
			sum_avg_eval = 0.0f;
			for (list<Column>::iterator column = columns.begin(); column != end; column++) {
				column->max_eval = column->max_est * column->max_vis * column->avg_intensity;
				column->avg_eval = column->sum_est_visibility * column->avg_intensity / column->known_estimate.size();
				sum_avg_eval += column->avg_eval;
			}
		}

		void updateColumnID() {
			list<Column>::iterator end = columns.end();
			int c = 0;
			for (list<Column>::iterator column = columns.begin(); column != end; column++, c++) {
				column->col_id = c;
			}
		}

		void fixColumn() {
			fix_columns.clear();
			for (list<Column, boost::fast_pool_allocator<Column>>::iterator column = columns.begin(); column != columns.end(); column++) {
				fix_columns.push_back(column);
			}

			// calculate visibility
			vector<uint32_t> row_vis_count(n_row, 0);
			vector<uint32_t> row_sample_count(n_row, 0);

			for (auto column : fix_columns) {
				column->n_sample = column->known_estimate.size();
				column->n_visible = 0;
				for (auto s : column->known_estimate) {
					row_sample_count[s]++;
					if (column->visibility(s) > 0) {
						column->n_visible++;
						row_vis_count[s]++;
					}
				}
			}

			for (int r = 0; r < n_row; r++) {
				rows[r].n_sample = row_sample_count[r];
				rows[r].n_visible = row_vis_count[r];
			}


		}

		void generateBorderPdf() {

			Point2i offset = m_slice_root->sampleBbox.min;
			Vector2i size = m_slice_root->sampleBbox.max - m_slice_root->sampleBbox.min;
			size.x += 1; size.y += 1;

			arma::imat image_tag(size.x, size.y);
			image_tag.fill(-1);

			for (uint32_t r = 0; r < n_row; r++) {
				Vector2i p = rows[r].sensor->getImagePos() - offset;
				image_tag.at(p.x, p.y) = r;
			}

			for (uint32_t r = 0; r < n_row; r++) {
				Vector2i p = rows[r].sensor->getImagePos() - offset;
				if (p.x == 0 || p.x == size.x - 1 || p.y == 0 || p.y == size.y - 1) {
					rows[r].odd_pdf = BORDER_REINFORCE;
				}
				else if (image_tag.at(p.x - 1, p.y) < 0 || image_tag.at(p.x + 1, p.y) < 0 || image_tag.at(p.x, p.y - 1) < 0 || image_tag.at(p.x, p.y + 1) < 0) {
					rows[r].odd_pdf = BORDER_REINFORCE;
				}
			}

		}

		void generateRowOddPdf(const AOMap &m_ao) {
			return;
			//generateBorderPdf();
			//return;
			// row ao
			uint32_t max_axis = m_slice_root->bbox.getLargestAxis();
			Point length = Point(0.05 * (m_slice_root->bbox.max[max_axis] - m_slice_root->bbox.min[max_axis]));

			//Vector eee = sliceRoot->bbox.getExtents();
			//Point length = Point((eee.x + eee.y + eee.z) / 3);

			//Float op1[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };
			//Float op2[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
			Float op1[9] = { 1, 1, 1, 1, -8, 1, 1, 1, 1 };
			Float op2[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };


			for (vector<Row>::iterator row = rows.begin(); row != rows.end(); row++) {


				Point2i c = row->sensor->getImagePos();
				uint32_t i = m_ao.index(c);

				Point center = m_ao.position[i];
				Point neighbor[9] = { center, center, center, center, center, center, center, center, center };
				AABB valid_bbox(Point(m_ao.position[i] - length), m_ao.position[i] + length);

				int xb = std::max(c.x - 1, m_ao.offset.x), xe = std::min(c.x + 1, m_ao.offset.x + m_ao.size.x);
				int yb = std::max(c.y - 1, m_ao.offset.y), ye = std::min(c.y + 1, m_ao.offset.y + m_ao.size.y);
				int valid = 0;
				for (int x = xb; x < xe; x++) {
					for (int y = yb; y < ye; y++) {
						uint32_t index = m_ao.index(Point2i(x, y));
						if (m_ao.n_sample[index] && valid_bbox.contains(m_ao.position[index])) {
							neighbor[x - (c.x - 1) + 3 * (y - (c.y - 1))] = m_ao.position[index];
						}
					}
				}


				Point lp1(0.0f);
				Point lp2(0.0f);
				for (int i = 0; i < 9; i++) {
					lp1 += neighbor[i] * op1[i];
					//lp2 += neighbor[i] * op2[i];
				}
				row->odd_pdf = (dot(Vector(lp1), row->sensor->its.geoFrame.n));// + fabs(dot(Vector(lp2), row->sensor->its.geoFrame.n));
				//row->odd_pdf *= row->odd_pdf;
				row->odd_pdf = std::max(0.0f, row->odd_pdf * OUTLINE_REINFORCE);
			}


			// row variance pdf compensation
			typedef VSensor::SpatialPointType SP;
			struct  SPop
			{

				TAABB<SP> bbox;


				vector<SP> cluster_sp;
				vector<Float> pdf;

				SPop(uint32_t r) {
					cluster_sp.reserve(r);
					pdf.reserve(r);
				}

				void reset() {
					cluster_sp.clear();
					pdf.clear();
					bbox.reset();
				}

				void push(const VSensor *s) {

					SP r;

					r[0] = s->its.p[0];
					r[1] = s->its.p[1];
					r[2] = s->its.p[2];
					r[3] = s->its.shFrame.n[0];
					r[4] = s->its.shFrame.n[1];
					r[5] = s->its.shFrame.n[2];

					bbox.expandBy(r);
					cluster_sp.push_back(r);
				}



				void process() {

					SP scale = SP(bbox.getExtents());

					// normalize			
					scale[0] = scale[0] > 0.0001 ? 1.0f / scale[0] : 1.0f;
					scale[1] = scale[1] > 0.0001 ? 1.0f / scale[1] : 1.0f;
					scale[2] = scale[2] > 0.0001 ? 1.0f / scale[2] : 1.0f;

					scale[3] = scale[3] > 0.0001 ? 1.0f / scale[3] : 1.0f;
					scale[4] = scale[4] > 0.0001 ? 1.0f / scale[4] : 1.0f;
					scale[5] = scale[5] > 0.0001 ? 1.0f / scale[5] : 1.0f;

					//scale[3] = scale[4] = scale[5] = 1.0f;

					SP center = bbox.getCenter();
					SP average = SP(0.0f);


					for (uint32_t i = 0; i < cluster_sp.size(); i++) {
						cluster_sp[i] = SP(cluster_sp[i] - center) * scale;
						average += cluster_sp[i];
					}
					average /= cluster_sp.size();

					bbox.min = SP(bbox.min - center) * scale;
					bbox.max = SP(bbox.max - center) * scale;

					Float cluster_d = (bbox.getCenter() - average).lengthSquared();
					cluster_d *= SINGULAR_REINFORCE;
					cluster_d = SINGULAR_REINFORCE;

					for (vector<SP>::iterator i = cluster_sp.begin(); i != cluster_sp.end(); i++) {

						SP distance = SP(abs(*i - average));
						Float l = 0;
						for (uint32_t i = 0; i < SP::dim; i++) {
							l = std::max(distance[i], l);
						}
						
						l= pow(l, 2);


						Float lq = (*i - average).lengthSquared();

						pdf.push_back(cluster_d * l);
					}

				}

			};



			SPop op(n_row);
			op.reset();
			for (uint32_t r = 0; r < n_row; r++)
				op.push(rows[r].sensor);

			op.process();
			for (uint32_t r = 0; r < n_row; r++) {
				Float nd = op.pdf[r] + rows[r].odd_pdf;
				rows[r].odd_pdf = (1.0f + nd);
			}

		}

		void generateRowShadowPdf() {

			Float *row_shadow_cost = new Float[n_row];
			Float *row_average = new Float[n_row];
			uint32_t *row_sample_count = new uint32_t[n_row];
			memset(row_shadow_cost, 0, sizeof(Float)* n_row);
			memset(row_average, 0, sizeof(Float)* n_row);
			memset(row_sample_count, 0, sizeof(Float)* n_row);

			sum_error = 0;

			for (list<Column>::iterator column = columns.begin(); column != columns.end(); column++) {

				if (column->max_eval == 0) continue;

				Float intensity = column->spur_intensity.average();
				column->error = 0;

				list<uint32_t>::iterator end = column->known_estimate.end();
				list<uint32_t>::iterator p = column->known_estimate.begin();
				uint32_t current_r = *p;
				p++;
				if (p == end) {
					continue;
				}

				uint32_t next_r = *p;

				Float current_v, next_v;
				current_v = column->estimate(current_r) * column->visibility(current_r);
				next_v = column->estimate(next_r) * column->visibility(next_r);

				Float v = std::abs(current_v - next_v) * intensity;
				//v *= v;
				//v = std::abs(sign(current_v) - sign(next_v)) * column->max_eval;


				Float avg_v = current_v * intensity;

				for (uint32_t r = 0; r < current_r; r++) {
					row_shadow_cost[r] += v;
					row_average[r] += avg_v;
					row_sample_count[r]++;
					column->error += v;
				}
				avg_v = 0.5 * (current_v + next_v) * intensity;

				for (;;) {

					for (uint32_t r = current_r; r < next_r; r++) {
						row_shadow_cost[r] += v;
						row_average[r] += avg_v;
						column->error += v;
						row_sample_count[r]++;
					}

					if (++p == end) break;

					current_v = next_v;
					current_r = next_r;

					next_r = *p;
					next_v = column->estimate(next_r) * column->visibility(next_r);
					v = std::abs(current_v - next_v) * intensity;
					//v *= v;
					//v = std::abs(sign(current_v) - sign(next_v)) * column->max_eval;

					avg_v = 0.5 * (current_v + next_v) * intensity;
				}
				avg_v = next_v * intensity;
				for (uint32_t r = next_r; r < n_row; r++) {
					row_shadow_cost[r] += v;
					row_average[r] += avg_v;
					column->error += v;
					row_sample_count[r]++;
				}
				column->error /= n_row;
				sum_error += (column->error) ;		
			
			} // end columns

			sum_error = (sum_error);
			//sum_error /= sum_avg_eval;
			//sum_error *= 1e8 * 10;
			//sum_error = std::max(sum_error - 1.0f, 0.0f);
			//sum_error = std::min(sum_error, 30.0f);

			Float mmax = 0; Float mmin = FLT_MAX;
			Float s_avg = 0;
			for (uint32_t r = 0; r < n_row; r++) {
				s_avg += row_average[r];
				mmax = std::max(mmax, row_shadow_cost[r]);
				mmin = std::min(mmin, row_shadow_cost[r]);
			}
			s_avg /= n_row;

			for (uint32_t r = 0; r < n_row; r++) {
				row_shadow_cost[r] = row_shadow_cost[r] / sum_avg_eval;
			}

			int filter = std::min((int)n_row, 20);
			Float s = row_shadow_cost[0];
			for (int i = 1; i < filter; i++) {
				s += row_shadow_cost[i];
			}

			for (int r = 0; r < n_row; r++) {

				int b = r - filter;
				if (b >= 0) s -= row_shadow_cost[b];
				int e = r + filter;
				if (e < n_row) s += row_shadow_cost[e];

				rows[r].shadow_pdf = s / (1 + std::min(e, (int)n_row - 1) - std::max(0, b));
			}

			for (int r = 0; r < n_row; r++) {
				rows[r].shadow_pdf *= SHADOW_REINFORCE;
			}

			delete[]row_shadow_cost;
			delete[]row_sample_count;
			delete[]row_average;
		}

#if 0
		void generateRowShadowPdf() {

			Float *row_shadow_cost = new Float[n_row];
			Float *row_average = new Float[n_row];
			uint32_t *row_sample_count = new uint32_t[n_row];
			memset(row_shadow_cost, 0, sizeof(Float)* n_row);
			memset(row_average, 0, sizeof(Float)* n_row);
			memset(row_sample_count, 0, sizeof(Float)* n_row);

			for (list<Column>::iterator column = columns.begin(); column != columns.end(); column++) {

				Float intensity = column->spur_intensity.average();
				list<uint32_t>::iterator end = column->known_estimate.end();
				Float last_v, current_v, next_v;
				list<uint32_t>::iterator p = column->known_estimate.begin();
				uint32_t current_r = *p;
				uint32_t last_r = 0;
				uint32_t next_r;
				//last_v = current_v = column->estimate(current_r) * column->visibility(current_r) > 0 ? 1 : 0;
				last_v = current_v = column->estimate(current_r) * column->visibility(current_r);
				p++;
				for ( ; p != end; p++) {

					next_r = *p;
					//next_v = column->estimate(next_r) * column->visibility(next_r) > 0 ? 1 : 0;
					next_v = column->estimate(next_r) * column->visibility(next_r);
					
					Float v = std::abs(current_v - last_v) + std::abs(current_v - next_v);
					v *= intensity;


					for (int i = last_r; i < next_r; i++) {
						row_shadow_cost[i] += v;
					}

					//row_shadow_cost[current_r] += v;
					
					row_average[current_r] += current_v * intensity;
					row_sample_count[current_r]++;
					

					last_v = current_v;
					current_v = next_v;
					last_r = current_r;
					current_r = next_r;

				}
	
				Float v = std::abs(current_v - last_v);
				v *= intensity;
				for (int i = last_r; i < next_r; i++) {
					row_shadow_cost[i] += v;
				}
				//row_shadow_cost[current_r] += v;
				
				row_average[current_r] += current_v * intensity;
				row_sample_count[current_r]++;
				

			}

			Float sum = 0;
			Float max = 0;
			for (uint32_t r = 0; r < n_row; r++) {
				sum += row_shadow_cost[r];
				max = std::max(row_shadow_cost[r], max);
			}

			max /= 1000;

			int filter = 16;
			for (int r = 0; r < n_row; r++) {

				int b = std::max(0, r - filter);
				int e = std::min(r + filter, (int)n_row - 1);
				Float s = 0;
				Float m = 0;
				for (int i = b; i < e; i++) {
					s += row_shadow_cost[i];
					m = std::max(row_shadow_cost[i], m);
				}

				rows[r].shadow_pdf = row_shadow_cost[r];

			}
	
			for (uint32_t r = 0; r < n_row; r++) {
				rows[r].shadow_pdf = SHADOW_REINFORCE * (rows[r].shadow_pdf) / sum_max_eval;
				rows[r].average = row_average[r] / row_sample_count[r];// / row_sample_count[r];
				//rows[r].shadow_pdf = row_shadow_cost[r] / sum_avg_eval;
			}




			delete[]row_shadow_cost;
			delete[]row_sample_count;
			delete[]row_average;
		}
#endif

		inline Float getRow3DDistance(uint32_t r_small, uint32_t r_large) {
			if (r_small > r_large) {
				uint32_t t = r_small;
				r_small = r_large;
				r_large = t;
			}

			Float &t = row_distance_map.at(r_small, r_large);
			if (t < 0) { // initialize the distance
				Float dist = distance(rows[r_small].sensorNode->rep_sensor->its.p, rows[r_large].sensorNode->rep_sensor->its.p);
				//Float dist = (rows[r_small].sensorNode->rep_sensor->its.p - rows[r_large].sensorNode->rep_sensor->its.p).lengthSquared();
				t = dist;
			}

			return t;
		}
	
		inline Float getRowDistance(uint32_t r_small, uint32_t r_large) {
	
			//return 1.0f;
			return r_large - r_small;
		//	return (r_small - r_large) *  (r_small - r_large);
			//return 1.0f;// row_distance_map.at(0, r_large);
			//Float d = (Float)rows[r_small].distance(rows[r_large]);
			//return d;

			Float &t = row_distance_map.at(r_small, r_large);
			//return t;

			if (t < 0) { // initialize the distance

				Float dist = distance(rows[r_small].sensorNode->rep_sensor->its.p, rows[r_large].sensorNode->rep_sensor->its.p);
				t = dist;
				//t = std::exp(-(dist * dist) / (2 * row_sigma * row_sigma)) / (SQRT_2PI * row_sigma);

			}

			return t;
		}

		// local parameter
		struct NeighborSearcher {

			const static uint32_t buffer_length = 500 * NEIGHBOR;

			uint32_t neighbor[buffer_length];
			Float pdf[buffer_length];

			uint32_t p_begin;
			uint32_t p_end;
			uint32_t p_center;

			Float pdf_sum;

			const VSensorNode *last_parent;

			list<uint32_t>::iterator e_iter;
			list<uint32_t>::iterator end_iter;
			list<uint32_t>::iterator p_iter;

			inline void newColumn(Column &column) {

				p_begin = p_center = p_end = 0;
				e_iter = p_iter = column.known_estimate.begin();
				end_iter = column.known_estimate.end();
				uint32_t n = NEIGHBOR;
				while (e_iter != end_iter && n--) {
					neighbor[p_end++] = *e_iter++;
				}
				last_parent = NULL;
				
			}

			inline void oneStep() {

				if (e_iter == end_iter) {
					p_begin = std::min(p_begin + 1, buffer_length - NEIGHBOR);
					p_center = std::min(p_center + 1, buffer_length - 1);
				}
				else if (p_end != buffer_length) {
					p_center++;
					p_begin = p_center >= NEIGHBOR ? p_center - NEIGHBOR : 0;
					neighbor[p_end++] = *e_iter++;
				}
				else {
					
					uint32_t l = p_end - p_begin - 1;

					for (uint32_t i = 0; i < l; i++) {
						neighbor[i] = neighbor[p_begin + 1 + i];
					}
					p_end = p_end - p_begin;
					neighbor[p_end - 1] = *e_iter++;
					p_center = p_center - p_begin;
					p_begin = 0;

				}

			}

			inline void locate(uint32_t r) {
				
				while (p_iter != end_iter && r > *p_iter) {
				
					if (e_iter == end_iter) {
						p_center = std::min(p_center + 1, p_end);
						p_begin = std::min(p_begin + 1, p_center - NEIGHBOR);					
					}
					else if (p_end != buffer_length) {
						p_center++;
						p_begin = p_center >= NEIGHBOR ? p_center - NEIGHBOR : 0;
						neighbor[p_end++] = *e_iter++;
					}
					else {
						uint32_t l = p_end - p_begin - 1;


						for (uint32_t i = 0; i < l; i++) {
							neighbor[i] = neighbor[p_begin + 1 + i];
						}
						p_end = p_end - p_begin;
						neighbor[p_end - 1] = *e_iter++;
						p_center = p_center - p_begin;
						p_begin = 0;

					}

					p_iter++;					
				}
			
			}
		};
		
		NeighborSearcher neighbor_searcher;


		inline Float mergeNeighborInRowSequence(uint32_t r, list<Column>::iterator &column) {


			neighbor_searcher.locate(r);
			Float w_sum = 0, e = 0;
			for (uint32_t i = neighbor_searcher.p_begin; i < neighbor_searcher.p_end; i++) {
				uint32_t n = neighbor_searcher.neighbor[i];
				Float w = 1.0f / getRowDistance(n, r);
				e += w * column->estimate(n) * column->visibility(n);
				//e += column->estimate(n) * column->visibility(n);
				w_sum += w;	
			}

			return e / w_sum;
			return e / (neighbor_searcher.p_end - neighbor_searcher.p_begin);

		}

		inline Float mergeNeighborInRowSequenceFull(uint32_t r, list<Column>::iterator &column) {

			Float e = 0; Float w_sum = 0;
			for (list<uint32_t>::iterator i = column->known_estimate.begin();  i != column->known_estimate.end(); i++) {
				
				uint32_t n = *i;
				Float w = getRowDistance(n, r);
				e += w * column->estimate(n) * column->visibility(n);
				//e += column->estimate(n) * column->visibility(n);
				w_sum += w;
			}

			return e / w_sum;
			return e / (neighbor_searcher.p_end - neighbor_searcher.p_begin);

		}

		inline Float debug_mergeNeighborInRowSequence(uint32_t r, list<Column>::iterator &column) {


			neighbor_searcher.locate(r);

			for (uint32_t i = neighbor_searcher.p_begin; i < neighbor_searcher.p_end; i++) {
				cout << neighbor_searcher.neighbor[i] << endl;
			}

			Float w_sum = 0, e = 0;
			for (uint32_t i = neighbor_searcher.p_begin; i < neighbor_searcher.p_end; i++) {
				uint32_t n = neighbor_searcher.neighbor[i];
				Float w = 1.0f / getRowDistance(n, r);
				cout << w << endl;
				e += w * column->estimate(n) * column->visibility(n);
				w_sum += w;

			}
			e /= w_sum;
			cout << e << endl;

			return e / w_sum;// (neighbor_searcher.p_end - neighbor_searcher.p_begin);

		}

		inline uint32_t selectRandomNeighbor(uint32_t r, vector<uint32_t> &known_est, uint32_t ptr, Float random) {

			// estimate noise
			int lower_p = ptr;

			uint32_t neighbor[2 * NEIGHBOR];
			Float pdf[2 * NEIGHBOR];
			uint32_t n_neighbor = 0;

			// find random close estimate and copy it
			int upper_p = lower_p - 1;
			int end = std::max(upper_p - NEIGHBOR, -1);
			for (; upper_p > end; upper_p--) {

				neighbor[n_neighbor] = known_est[upper_p];
				pdf[n_neighbor] = 1.0f / getRowDistance(known_est[upper_p], r);

				n_neighbor++;
			}

			end = std::min(lower_p + NEIGHBOR, (int)known_est.size());
			for (; lower_p < end; lower_p++) {

				neighbor[n_neighbor] = known_est[lower_p];
				pdf[n_neighbor] = 1.0f / getRowDistance(r, known_est[lower_p]);

				n_neighbor++;
			}

			for (int i = 1; i < n_neighbor; i++) {
				pdf[i] += pdf[i - 1];
			}

			Float t = random * pdf[n_neighbor - 1];

			int select = 0;
			for (; select < n_neighbor - 1 && pdf[select] < t; select++) {}

			return neighbor[select];
		}

		inline uint32_t selectRandomNeighbor(uint32_t r, list<uint32_t>::iterator &b_iter, list<uint32_t>::iterator &e_iter, list<uint32_t>::iterator &ptr, Float random) {

			// estimate noise
			uint32_t neighbor[2 * NEIGHBOR];
			Float pdf[2 * NEIGHBOR];
			uint32_t n_neighbor = 0;

			// find random close estimate and copy it
			list<uint32_t>::iterator upper_p = ptr;
			for (int i = 0; i < NEIGHBOR && upper_p != b_iter; i++) {
				upper_p--;

				neighbor[n_neighbor] = *upper_p;
				pdf[n_neighbor] = 1.0f / getRowDistance(*upper_p, r);
				n_neighbor++;
			}

			list<uint32_t>::iterator lower_p = ptr;
			for (int i = 0; i < NEIGHBOR && lower_p != e_iter; i++) {

				neighbor[n_neighbor] = *lower_p;
				pdf[n_neighbor] = 1.0f / getRowDistance(*lower_p, r);
				 
				n_neighbor++;
				lower_p++;
			}


			for (int i = 1; i < n_neighbor; i++) {
				pdf[i] += pdf[i - 1];
			}

			Float t = random * pdf[n_neighbor - 1];

			int select = 0;
			for (; select < n_neighbor - 1 && pdf[select] < t; select++) {}

			return neighbor[select];
		}
		
	
	private:
		vector<uint32_t> temp_selection;

	public:

		//inline void sampleElement(uint32_t r, list<Column>::iterator &column, Float &e, int16_t &visibility, ref<Random> &random) const {

		//	Spectrum estimate = column->lightNode->eval(*(rows[r].sensor), m_clamping);
		//	Float e_avg = estimate.average();
		//	if (e_avg <= m_cutoff) {
		//		e_avg = 0;
		//		estimate = Spectrum(0.0f);
		//	}
		//	else {
		//		estimate *= rows[r].sensor->bsdf_normalization; // smooth out texture detail, normalize rgb channel
		//	}
		//	e = estimate.average();

		//	if (e_avg > 0) {
		//		Ray ray = column->lightNode->shadowRay(*(rows[r].sensorNode->rep_sensor));
		//		visibility = !m_scene->rayIntersect(ray);
		//	}

		//}

		inline void sampleElement(uint32_t r, list<Column>::iterator &column, Float &e, Spectrum &m_rgb, int16_t &visibility, ref<Random> &random) const {

			const VLightNode *lightNode = column->lightNode;
	
		
			uint32_t deep = 0;
			while (!lightNode->isLeaf() && deep++ < 0) {
				Float ran = random->nextFloat();
				Float left = lightNode->left->intensity.average();
				Float right = lightNode->right->intensity.average();
				Float q = left / (left + right);
				if (ran < q) {
					lightNode = lightNode->left;
				}
				else {
					lightNode = lightNode->right;
				}
			}

			m_rgb = lightNode->evalMTD(rows[r].sensor, m_clamping);
			e = m_rgb.getLuminance();
			if (e <= m_cutoff) {
				e = 0;
				m_rgb = Spectrum(0.0f);
			}
			else {
				m_rgb /= e;
			}
		

			if (e > 0) {
				Ray ray = column->lightNode->shadowRay(rows[r].sensorNode->rep_sensor);
				visibility = !m_scene->rayIntersect(ray);
			}

		}


		template<typename VECTOR>
		inline void sampleColumn_kernel(VECTOR &elements, list<Column>::iterator &column, ref<Random> &random) {

		
			Float max_eval = 0;
			int max_row = -1;
			column->known_estimate.clear();
			column->known_visibility.clear();
			column->max_est = 0;
			column->min_est = FLT_MAX;
			column->max_vis = 0;
			column->sum_est_visibility = 0;

			for (typename VECTOR::iterator itr = elements.begin(); itr != elements.end(); itr++) {

				uint32_t r = *itr;

				Float e;
				Spectrum m_rgb;
				int16_t v = -1;
				sampleElement(r, column, e, m_rgb, v, random);

				column->estimate(r) = e;
				column->m_rgb(r) = m_rgb;
				column->estimate_tag(r) = true;
				column->known_estimate.push_back(r);
				column->max_est = std::max(column->max_est, e);
				column->min_est = std::min(column->min_est, e);

				if (v >= 0) {

					column->visibility(r) = v;
					//column->visibility_tag(r) = true;
					column->known_visibility.push_back(r);

				}

				if (v > 0) {
					column->max_vis = 1;
					column->sum_est_visibility += e;

					if (e > max_eval) {
						max_eval = e;
						max_row = r;
					}
				}
			}

	
			column->avg_eval = (column->sum_est_visibility / column->known_estimate.size()) * column->avg_intensity;
			column->max_eval = max_row >= 0 ? column->estimate(max_row) * column->avg_intensity : 0;

		}

		inline void sampleColumn(list<Column>::iterator &column, ref<Random> &random) {
			static int count = 0;
			temp_selection.clear();
			

#ifndef INIT_HARD_RATIO

			uint32_t a = 0;
			for (vector<uint32_t>::iterator itr = row_split.begin(); itr != row_split.end(); itr++) {
				Float nn = random->nextFloat();
		
				uint32_t r = a + nn * (*itr - a);	
				temp_selection.push_back(std::min(r, *itr - 1));
				a = *itr;
			}
#else 
			for (uint32_t i = 0; i < n_row; i++) {
				if (random->nextFloat() <= INIT_HARD_RATIO) { temp_selection.push_back(i); }
				else cout << "bug" << i << endl;
			}

#endif		
			
			sampleColumn_kernel(temp_selection, column, random);
			//cout << column->known_estimate.size() << endl;
			return;

			//////////////////////////////////////////////////////////////////////////
			column->known_estimate.clear();
			column->known_visibility.clear();

			Float column_sum_est_visibility = 0;
			Float max_eval = 0;
			int max_row = -1;


			for (vector<uint32_t>::iterator itr = temp_selection.begin(); itr != temp_selection.end(); itr++) {

				uint32_t r = *itr;

				Float e;
				Spectrum m_rgb;
				int16_t v = -1;
				sampleElement(r, column, e, m_rgb, v, random);
				
				column->estimate(r) = e;
				column->m_rgb(r) = m_rgb;
				column->estimate_tag(r) = true;
				column->known_estimate.push_back(r);
				column->max_est = std::max(column->max_est, e);

				if (v >= 0) {

					column->visibility(r) = v;
					//column->visibility_tag(r) = true;
					column->known_visibility.push_back(r);
					
				}

				if (v > 0) {
					column->max_vis = 1;
					column_sum_est_visibility += e;

					if (e > max_eval) {
						max_eval = e;
						max_row = r;
					}
				}
			}


			column->sum_est_visibility = column_sum_est_visibility;
			column->avg_eval = (column_sum_est_visibility / column->known_estimate.size()) * column->avg_intensity;
			column->max_eval = max_row >= 0 ? column->estimate(max_row) * column->avg_intensity : 0;

		}
	
		list<Column>::iterator mergeColumn(list<Column>::iterator &sibling_column, list<Column>::iterator &column, ref<Random> &random, Float i_threshold, Float e_threshold) {

			const VLightNode *p_node = column->lightNode->parent;
			list<Column>::iterator &rep_column = p_node->rep_light == column->lightNode->rep_light ? column : sibling_column;

			Spectrum p_spur_intensity = p_node->intensity * li_spur;
			Float p_intensity = p_spur_intensity.average();
			Float p_scale = p_intensity / rep_column->avg_intensity;

			Float rep_max_eval = rep_column->max_eval * p_scale;
			if (rep_max_eval >= i_threshold || column->max_eval + sibling_column->max_eval >= i_threshold) {
				return ++column;
			}

			Float error_threshold = e_threshold - column->accu_error - sibling_column->accu_error;
			if (error_threshold < 0) {
				return ++column;
			}

			bool merge = true;		
			Float merge_error = 0;
			
			if (column->max_eval == 0 || sibling_column->max_eval == 0) {

				if (column->max_est * column->avg_intensity > i_threshold || sibling_column->max_est * column->avg_intensity > i_threshold) {
					merge = false;
				}

				//list<Column>::iterator visible_column, invisible_column;
				//if (column->max_eval == 0) {
				//	visible_column = sibling_column;
				//	invisible_column = column;
				//}
				//else {
				//	visible_column = column;
				//	invisible_column = sibling_column;
				//}

				//if (rep_column == visible_column) {
				//	merge_error = std::abs(rep_max_eval - rep_column->max_eval);
				//	merge = merge_error < error_threshold;
				//}
				//else {
				//	merge_error = visible_column->max_eval;
				//	merge = merge_error < error_threshold;
				//}

			}
			else {
				
				list<uint32_t>::iterator c_itr = column->known_estimate.begin();
				list<uint32_t>::iterator c_end = column->known_estimate.end();
				list<uint32_t>::iterator s_itr = sibling_column->known_estimate.begin();
				list<uint32_t>::iterator s_end = sibling_column->known_estimate.end();

				Float c_last_eval, c_current_eval, s_last_eval, s_current_eval;
				Float &p_eval = p_node->rep_light == column->lightNode->rep_light ? c_current_eval : s_current_eval;
				c_current_eval = c_last_eval = column->estimate(*c_itr) * column->visibility(*c_itr) * column->avg_intensity;
				s_current_eval = s_last_eval = sibling_column->estimate(*s_itr) * sibling_column->visibility(*s_itr) * sibling_column->avg_intensity;

				while (c_itr != c_end || s_itr != s_end) {

					if (s_itr == s_end || *c_itr < *s_itr) {
						c_current_eval = column->estimate(*c_itr) * column->visibility(*c_itr) * column->avg_intensity;
						Float t = p_eval * p_scale - c_current_eval;
						Float error = std::max(std::abs(t - s_current_eval), std::abs(t - s_last_eval));
						if (error > merge_error) {
							merge_error = error;
							if (merge_error > error_threshold) {
								merge = false;
								break;
							}
						}

						c_last_eval = c_current_eval;
						c_itr++;			
					}
					else {
						s_current_eval = sibling_column->estimate(*s_itr) * sibling_column->visibility(*s_itr) * sibling_column->avg_intensity;
						Float t = p_eval * p_scale - s_current_eval;
						Float error = std::max(std::abs(t - c_current_eval), std::abs(t - c_last_eval));
						if (error > merge_error) {
							merge_error = error;
							if (merge_error > error_threshold) {
								merge = false;
								break;
							}
						}

						s_last_eval = s_current_eval;
						s_itr++;
					}
				}

			} // end merge judgment

			if (merge) {

				rep_column->accu_error = merge_error + column->accu_error + sibling_column->accu_error;

				rep_column->lightNode = p_node;
				rep_column->spur_intensity = p_spur_intensity;
				rep_column->avg_intensity = p_intensity;

				rep_column->max_eval *= p_scale;
				rep_column->avg_eval *= p_scale;

				rep_column->balance = Column::BAfterContract;

				if (column == rep_column) {
					columns.erase(sibling_column);
				}
				else {
					columns.erase(column);
				}

				return rep_column; // search last one

			}
			else {

				return ++column;
			}

		}


		list<Column>::iterator contractColumn(list<Column>::iterator &column, ref<Random> &random, Float threshold) {

			const VLightNode *p_node = column->lightNode->parent;
			if (!p_node) return ++column;

			list<Column>::iterator sibling_column = column;
			if (column->lightNode == p_node->left) {
				sibling_column++;
				if (sibling_column == columns.end() || sibling_column->lightNode != p_node->right) return ++column;
			}
			else {
				sibling_column--;
				if (sibling_column->lightNode != p_node->left) return ++column;
			}
			
			if (!(sibling_column->balance == Column::BAfterContract || sibling_column->balance == Column::BInitial)) {
				return ++column;
			}

			Float error_threshold = threshold * sum_avg_eval - column->accu_error - sibling_column->accu_error;
			Float merge_error = 0;
			list<Column>::iterator rep_column = p_node->rep_light == column->lightNode->rep_light ? column : sibling_column;

			bool merge = true;
			Spectrum p_spur_intensity = p_node->intensity * li_spur;
			Float p_scale = p_spur_intensity.average() / rep_column->spur_intensity.average();
			Float rep_max_eval = rep_column->max_eval * p_scale;

			if (error_threshold < 0) {
				merge = false;
			}
			else if (column->max_eval < error_threshold && sibling_column->max_eval < error_threshold && rep_max_eval < error_threshold) {
				merge_error = std::abs(rep_max_eval - (column->max_eval + sibling_column->max_eval));
				// the maximum eval is less than error_threshold, do nothing
			}
			else if (column->max_eval == 0 || sibling_column->max_eval == 0) {

				list<Column>::iterator visible_column, invisible_column;
				if (column->max_eval == 0) {
					visible_column = sibling_column;
					invisible_column = column;
				}
				else {
					visible_column = column;
					invisible_column = sibling_column;
				}

				if (rep_column == visible_column) {
					merge_error = std::abs(rep_max_eval - rep_column->max_eval);
					merge = merge_error < error_threshold;
				}
				else {
					merge_error = visible_column->max_eval;
					merge = merge_error < error_threshold;
				}
				
			}
			else {
				merge = false;

				//Float c_intensity = column->spur_intensity.average();
				//Float s_intensity = sibling_column->spur_intensity.average();
				//Float p_intensity = c_intensity + s_intensity;

				//// 1
				//{
				//	list<Column>::iterator &c_column = column;
				//	list<uint32_t>::iterator c_itr = column->known_estimate.begin();
				//	list<uint32_t>::iterator c_end = column->known_estimate.end();
				//	Float c_i = c_intensity;
				//	
				//	list<Column>::iterator &s_column = sibling_column;
				//	list<uint32_t>::iterator s_itr = sibling_column->known_estimate.begin();
				//	list<uint32_t>::iterator s_begin = s_itr;
				//	list<uint32_t>::iterator s_end = sibling_column->known_estimate.end();
				//	Float s_i = s_intensity;

				//	Float c_eval, s_eval;
				//	Float &p_eval = c_column == rep_column ? c_eval : s_eval;

				//	for (; c_itr != c_end; c_itr++) {

				//		uint32_t r = *c_itr;
				//		while (s_itr != s_end && *s_itr < r) {
				//			s_itr++;
				//		}
				//

				//		uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

				//		c_eval = c_column->estimate(r) * c_column->visibility(r);
				//		s_eval = s_column->estimate[rep_r] * s_column->visibility[rep_r];
				//		assert(c_eval < 1);
				//		assert(s_eval < 1);

				//		Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
				//		if (error > merge_error) {
				//			merge_error = error;
				//			if (merge_error >= error_threshold) {
				//				merge = false;
				//				break;
				//			}
				//		}

				//	}
				//}

				//// 2
				//if (merge) {

				//	list<Column>::iterator &c_column = sibling_column;
				//	list<uint32_t>::iterator c_itr = sibling_column->known_estimate.begin();
				//	list<uint32_t>::iterator c_end = sibling_column->known_estimate.end();
				//	Float c_i = s_intensity;

				//	list<Column>::iterator &s_column = column;
				//	list<uint32_t>::iterator s_itr = column->known_estimate.begin();
				//	list<uint32_t>::iterator s_begin = s_itr;
				//	list<uint32_t>::iterator s_end = column->known_estimate.end();
				//	Float s_i = c_intensity;

				//	Float c_eval, s_eval;
				//	Float &p_eval = c_column == rep_column ? c_eval : s_eval;

				//	for (; c_itr != c_end; c_itr++) {

				//		uint32_t r = *c_itr;
				//		while (s_itr != s_end && *s_itr < r) {
				//			s_itr++;
				//		}

				//		uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

				//		c_eval = c_column->estimate(r) * c_column->visibility(r);
				//		s_eval = s_column->estimate[rep_r] * s_column->visibility[rep_r];
				//		assert(c_eval < 1);
				//		assert(s_eval < 1);

				//		Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
				//		if (error > merge_error) {
				//			merge_error = error;
				//			if (merge_error >= error_threshold) {
				//				merge = false;
				//				break;
				//			}
				//		}

				//	}

				//}		

			} // end merge judgment
			
			if (merge) {
				
				if (column != rep_column) {
					sibling_column = column;
					column = rep_column;
				}
				
				column->accu_error = merge_error + column->accu_error + sibling_column->accu_error;
				column->lightNode = p_node;
				column->spur_intensity = p_spur_intensity;

				column->max_eval *= p_scale;
				column->avg_eval *= p_scale;

				columns.erase(sibling_column);
				column->balance = Column::BAfterContract;
				
			}
			else {
				
				if (column->balance == Column::BInitial) {
					column->balance = Column::BToExpand;
				}

				if (sibling_column->balance == Column::BInitial) {
					sibling_column->balance = Column::BToExpand;
				}
				
				column++;
				if (column == sibling_column) {
					column++;
				}
			}
			

			return column;
		}


		list<Column>::iterator splitColumn(list<Column>::iterator &column, ref<Random> &random, uint32_t &shadoe_count) {

			const VLightNode *p_node = column->lightNode;


			list<Column>::iterator sibling_column = column;
			const VLightNode *c_node, *s_node;
			if (p_node->rep_light == p_node->left->rep_light) {
				c_node = p_node->left;
				s_node = p_node->right;
				sibling_column++;
				
			}
			else {
				c_node = p_node->right;
				s_node = p_node->left;
			}


			sibling_column = columns.insert(sibling_column, Column(s_node, li_spur, m_pool_manager));
			sibling_column->init(n_row);


			// sample sibling column
			//list<uint32_t, PoolAllocator<uint32_t>> c_known = column->known_estimate;
			//sampleColumn_random(c_known, column, random);
			sampleColumn(sibling_column, random);
			

			Float p_max_eval = column->max_eval;
			Float p_avg_eval = column->avg_eval;
			Float p_intensity = column->avg_intensity;

			column->lightNode = c_node;
			column->spur_intensity = column->lightNode->intensity * li_spur;
			column->avg_intensity = column->spur_intensity.average();

			Float p_scale = p_intensity / column->avg_intensity;
			column->max_eval = p_max_eval / p_scale;
			column->avg_eval = p_avg_eval / p_scale;
			
			column->balance = Column::BAfterExpand;
			sibling_column->balance = Column::BAfterExpand;

			column->error = column->lightNode->errorBoundSMTDI(m_slice_root).average() / n_row;
			sibling_column->error = sibling_column->lightNode->errorBoundSMTDI(m_slice_root).average() / n_row;

			column->expand_times++;
			sibling_column->expand_times = column->expand_times;
			sibling_column->expand_col_id = column->expand_col_id;

			return sibling_column;
		}

		list<Column>::iterator expandColumn(list<Column>::iterator column, ref<Random> random, uint32_t &shadoe_count, Float error_threshold) {

			if (column->balance == Column::BAfterContract || column->balance == Column::BAfterExpand) {
				return ++column;
			}

			if (column->lightNode->isLeaf()) {
				column->balance = Column::BAfterExpand;
				return ++column;
			}

			Float est_max_eval = column->max_eval;
		
			static Float neighbor_scale = 2;
			if (est_max_eval == 0 || 1) {

				Float search_left = neighbor_scale * column->avg_intensity;
				list<Column>::iterator ii = column;
				list<Column>::iterator begin = columns.begin();

				ii--;
				while (1) {
					est_max_eval = std::max(est_max_eval, ii->max_eval * column->avg_intensity / ii->avg_intensity);
					if (ii->max_eval > 0 && 0) {
						est_max_eval = column->max_est * column->avg_intensity;
						break;
					}
					else {
						search_left -= ii->avg_intensity;
						if (search_left < 0 || ii == begin) {
							break;
						}
						ii--;
					}
				}
			}

			if (est_max_eval == 0 || 1) {

				Float search_left = neighbor_scale * column->avg_intensity;
				list<Column>::iterator ii = column;
				list<Column>::iterator end = columns.end();

				ii++;
				while (1) {
					est_max_eval = std::max(est_max_eval, ii->max_eval * column->avg_intensity / ii->avg_intensity);
					if (ii->max_eval > 0 && 0) {
						est_max_eval = column->max_est * column->avg_intensity;
						break;
					}
					else {
						search_left -= ii->avg_intensity;
						ii++;
						if (search_left < 0 || ii == end) {
							break;
						}
					}
				}

				
			}

			//est_max_eval = column->max_est * column->avg_intensity;
			

			if (est_max_eval < error_threshold) {
				column->balance = Column::BAfterExpand;
				return ++column;
			}


		
			uint32_t sc = 0;

			const VLightNode *p_node = column->lightNode;
	
			list<Column>::iterator sibling_column = column;
			const VLightNode *c_node, *s_node;
			if (p_node->rep_light == p_node->left->rep_light) {
				c_node = p_node->left;
				s_node = p_node->right;
				sibling_column++;
			}
			else {
				c_node = p_node->right;
				s_node = p_node->left;
			}
			

			sibling_column = columns.insert(sibling_column, Column(s_node, li_spur, m_pool_manager));
			sibling_column->init(rows.size());		

			// sample sibling column
			sampleColumn(sibling_column, random);		

			Float p_max_eval = column->max_eval;
			Float p_avg_eval = column->avg_eval;
			Spectrum p_spur_intensity = column->spur_intensity;
			

			column->lightNode = c_node;	
			column->spur_intensity = column->lightNode->intensity * li_spur;
			column->avg_intensity = column->spur_intensity.average();
			Float p_scale = (p_spur_intensity).average() / column->spur_intensity.average();
			column->max_eval = p_max_eval / p_scale;	
			column->avg_eval = p_avg_eval / p_scale;
			//cout << column->spur_intensity.average() << " " << sibling_column->spur_intensity.average() << endl;
			return p_node->left == c_node ? column : sibling_column;
			

			//////////////////////////////////////////////////////////////////////////

			// -column->accu_error - sibling_column->accu_error;
			Float merge_error = 0;
			list<Column>::iterator &rep_column = column;

			bool merge = true;
			Float rep_max_eval = rep_column->max_eval * p_scale;

			if (column->max_eval < error_threshold && sibling_column->max_eval < error_threshold && p_max_eval < error_threshold) {
				merge_error = std::abs(rep_max_eval - (column->max_eval + sibling_column->max_eval));
				// the maximum eval is less than error_threshold, do nothing
			}
			else if (column->max_eval == 0 || sibling_column->max_eval == 0) {

				list<Column>::iterator visible_column, invisible_column;
				if (column->max_eval == 0) {
					visible_column = sibling_column;
					invisible_column = column;
				}
				else {
					visible_column = column;
					invisible_column = sibling_column;
				}

				if (rep_column == visible_column) {
					merge_error = std::abs(rep_max_eval - rep_column->max_eval);
					merge = merge_error < error_threshold;
				}
				else {
					merge_error = visible_column->max_eval;
					merge = merge_error < error_threshold;
				}

			}
			else {

				Float c_intensity = column->spur_intensity.average();
				Float s_intensity = sibling_column->spur_intensity.average();
				Float p_intensity = c_intensity + s_intensity;

				// 1
				{
					list<Column>::iterator &c_column = column;
					list<uint32_t>::iterator c_itr = column->known_estimate.begin();
					list<uint32_t>::iterator c_end = column->known_estimate.end();
					Float c_i = c_intensity;

					list<Column>::iterator &s_column = sibling_column;
					list<uint32_t>::iterator s_itr = sibling_column->known_estimate.begin();
					list<uint32_t>::iterator s_begin = s_itr;
					list<uint32_t>::iterator s_end = sibling_column->known_estimate.end();
					Float s_i = s_intensity;

					Float c_eval, s_eval;
					Float &p_eval = c_column == rep_column ? c_eval : s_eval;

					for (; c_itr != c_end; c_itr++) {

						uint32_t r = *c_itr;
						while (s_itr != s_end && *s_itr < r) {
							s_itr++;
						}

						uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

						c_eval = c_column->estimate(r) * c_column->visibility(r);
						s_eval = s_column->estimate(rep_r) * s_column->visibility(rep_r);


						if (s_column->estimate(rep_r) < 0 || c_column->estimate(r) < 0) {
							cout << s_column->estimate(rep_r) << " " << c_column->estimate(r) << endl;
							SLog(EError, "slice %u, %u", rows[r].sensor->getImagePos().x, rows[r].sensor->getImagePos().y);
						}
					

						Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
						if (error > merge_error) {
							merge_error = error;
							if (merge_error >= error_threshold) {
								merge = false;
								break;
							}
						}

					}
				}

				// 2
				if (merge) {

					list<Column>::iterator &c_column = sibling_column;
					list<uint32_t>::iterator c_itr = sibling_column->known_estimate.begin();
					list<uint32_t>::iterator c_end = sibling_column->known_estimate.end();
					Float c_i = s_intensity;

					list<Column>::iterator &s_column = column;
					list<uint32_t>::iterator s_itr = column->known_estimate.begin();
					list<uint32_t>::iterator s_begin = s_itr;
					list<uint32_t>::iterator s_end = column->known_estimate.end();
					Float s_i = c_intensity;

					Float c_eval, s_eval;
					Float &p_eval = c_column == rep_column ? c_eval : s_eval;

					for (; c_itr != c_end; c_itr++) {

						uint32_t r = *c_itr;
						while (s_itr != s_end && *s_itr < r) {
							s_itr++;
						}

						uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

						c_eval = c_column->estimate(r) * c_column->visibility(r);
						s_eval = s_column->estimate(rep_r) * s_column->visibility(rep_r);
						assert(c_eval < 1);
						assert(s_eval < 1);

						Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
						if (error > merge_error) {
							merge_error = error;
							if (merge_error >= error_threshold) {
								merge = false;
								break;
							}
						}
					}

				}

			} // end merge judgment

			if (column->max_eval > 0.0001 || sibling_column->max_eval > 0.0001) {
				merge = false;
			}

			if (merge) {
				
				columns.erase(sibling_column);

				column->lightNode = p_node;
				column->spur_intensity = p_spur_intensity;
				column->max_eval = p_max_eval;
				column->max_eval = p_avg_eval;

				column->balance = Column::BAfterExpand;		
				column++;

			}
			else {

				column->balance = Column::BAfterExpand;
				sibling_column->balance = Column::BAfterExpand;

				column++;
				if (column == sibling_column) {
					column++;
				}
			}
			shadoe_count += sc;
			return column;
		}

		list<Column>::iterator expandColumn_new(list<Column>::iterator column, ref<Random> random, uint32_t &shadoe_count, Float error_threshold) {

			if (column->balance == Column::BAfterContract || column->balance == Column::BAfterExpand) {
				return ++column;
			}

			if (column->lightNode->isLeaf()) {
				column->balance = Column::BAfterExpand;
				return ++column;
			}
	
			uint32_t sc = 0;

			const VLightNode *p_node = column->lightNode;

			list<Column>::iterator sibling_column = column;
			const VLightNode *c_node, *s_node;
			if (p_node->rep_light == p_node->left->rep_light) {
				c_node = p_node->left;
				s_node = p_node->right;
				sibling_column++;
			}
			else {
				c_node = p_node->right;
				s_node = p_node->left;
			}


			sibling_column = columns.insert(sibling_column, Column(s_node, li_spur, m_pool_manager));
			sibling_column->init(rows.size());

			// sample sibling column
			sampleColumn(sibling_column, random);

			Float p_max_eval = column->max_eval;
			Float p_avg_eval = column->avg_eval;
			Spectrum p_spur_intensity = column->spur_intensity;


			column->lightNode = c_node;
			column->spur_intensity = column->lightNode->intensity * li_spur;
			column->avg_intensity = column->spur_intensity.average();
			Float p_scale = (p_spur_intensity).average() / column->spur_intensity.average();
			column->max_eval = p_max_eval / p_scale;
			column->avg_eval = p_avg_eval / p_scale;
			//cout << column->spur_intensity.average() << " " << sibling_column->spur_intensity.average() << endl;
			return p_node->left == c_node ? column : sibling_column;


			//////////////////////////////////////////////////////////////////////////

			// -column->accu_error - sibling_column->accu_error;
			Float merge_error = 0;
			list<Column>::iterator &rep_column = column;

			bool merge = true;
			Float rep_max_eval = rep_column->max_eval * p_scale;

			if (column->max_eval < error_threshold && sibling_column->max_eval < error_threshold && p_max_eval < error_threshold) {
				merge_error = std::abs(rep_max_eval - (column->max_eval + sibling_column->max_eval));
				// the maximum eval is less than error_threshold, do nothing
			}
			else if (column->max_eval == 0 || sibling_column->max_eval == 0) {
				merge_error = std::max(column->max_est * column->avg_intensity, sibling_column->max_eval * sibling_column->avg_eval);

				//list<Column>::iterator visible_column, invisible_column;
				//if (column->max_eval == 0) {
				//	visible_column = sibling_column;
				//	invisible_column = column;
				//}
				//else {
				//	visible_column = column;
				//	invisible_column = sibling_column;
				//}

				//if (rep_column == visible_column) {
				//	merge_error = std::abs(rep_max_eval - rep_column->max_eval);
				//	merge = merge_error < error_threshold;
				//}
				//else {
				//	merge_error = visible_column->max_eval;
				//	merge = merge_error < error_threshold;
				//}

			}
			else {

				Float c_intensity = column->spur_intensity.average();
				Float s_intensity = sibling_column->spur_intensity.average();
				Float p_intensity = c_intensity + s_intensity;

				// 1
				{
					list<Column>::iterator &c_column = column;
					list<uint32_t>::iterator c_itr = column->known_estimate.begin();
					list<uint32_t>::iterator c_end = column->known_estimate.end();
					Float c_i = c_intensity;

					list<Column>::iterator &s_column = sibling_column;
					list<uint32_t>::iterator s_itr = sibling_column->known_estimate.begin();
					list<uint32_t>::iterator s_begin = s_itr;
					list<uint32_t>::iterator s_end = sibling_column->known_estimate.end();
					Float s_i = s_intensity;

					Float c_eval, s_eval;
					Float &p_eval = c_column == rep_column ? c_eval : s_eval;

					for (; c_itr != c_end; c_itr++) {

						uint32_t r = *c_itr;
						while (s_itr != s_end && *s_itr < r) {
							s_itr++;
						}

						uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

						c_eval = c_column->estimate(r) * c_column->visibility(r);
						s_eval = s_column->estimate(rep_r) * s_column->visibility(rep_r);


						if (s_column->estimate(rep_r) < 0 || c_column->estimate(r) < 0) {
							cout << s_column->estimate(rep_r) << " " << c_column->estimate(r) << endl;
							SLog(EError, "slice %u, %u", rows[r].sensor->getImagePos().x, rows[r].sensor->getImagePos().y);
						}


						Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
						if (error > merge_error) {
							merge_error = error;
							if (merge_error >= error_threshold) {
								merge = false;
								break;
							}
						}

					}
				}

				// 2
				if (merge) {

					list<Column>::iterator &c_column = sibling_column;
					list<uint32_t>::iterator c_itr = sibling_column->known_estimate.begin();
					list<uint32_t>::iterator c_end = sibling_column->known_estimate.end();
					Float c_i = s_intensity;

					list<Column>::iterator &s_column = column;
					list<uint32_t>::iterator s_itr = column->known_estimate.begin();
					list<uint32_t>::iterator s_begin = s_itr;
					list<uint32_t>::iterator s_end = column->known_estimate.end();
					Float s_i = c_intensity;

					Float c_eval, s_eval;
					Float &p_eval = c_column == rep_column ? c_eval : s_eval;

					for (; c_itr != c_end; c_itr++) {

						uint32_t r = *c_itr;
						while (s_itr != s_end && *s_itr < r) {
							s_itr++;
						}

						uint32_t rep_r = (s_itr != s_end && *s_itr == r) ? r : selectRandomNeighbor(r, s_begin, s_end, s_itr, random->nextFloat());

						c_eval = c_column->estimate(r) * c_column->visibility(r);
						s_eval = s_column->estimate(rep_r) * s_column->visibility(rep_r);
						assert(c_eval < 1);
						assert(s_eval < 1);

						Float error = std::abs(c_eval * c_intensity + s_eval * s_intensity - p_eval * p_intensity);
						if (error > merge_error) {
							merge_error = error;
							if (merge_error >= error_threshold) {
								merge = false;
								break;
							}
						}
					}

				}

			} // end merge judgment

			if (merge_error > error_threshold) {
				merge = false;
			}

			if (merge) {

				columns.erase(sibling_column);

				column->lightNode = p_node;
				column->spur_intensity = p_spur_intensity;
				column->max_eval = p_max_eval;
				column->max_eval = p_avg_eval;

				column->balance = Column::BAfterExpand;
				sibling_column->balance = Column::BAfterExpand;
				column++;

			}
			else {

				//column->balance = Column::BAfterExpand;
				//sibling_column->balance = Column::BAfterExpand;

				if (p_node->left == column->lightNode) {
					return column;
				}
				else {
					return sibling_column;
				}
			}
			shadoe_count += sc;
			return column;
		}


		void copyVisibility(list<Column>::iterator &source, list<Column>::iterator &target, ref<Random> &random) {

			list<uint32_t>::iterator s_itr = source->known_estimate.begin();
			list<uint32_t>::iterator se_itr = source->known_estimate.end();

			list<uint32_t>::iterator t_itr = target->known_estimate.begin(); 
			list<uint32_t>::iterator te_itr = target->known_estimate.end();

			Float t_intensity = target->spur_intensity.average();

			while (s_itr != se_itr) {

				while (t_itr != te_itr && *t_itr <= *s_itr) t_itr++;

				uint32_t r = *s_itr;
				if (!target->estimate_tag(r) && source->visibility(r) > 0) {
			
					Float e;
					Spectrum m_rgb;
					int16_t v = -1;

					sampleElement(r, target, e, m_rgb, v, random);

					target->m_rgb(r) = m_rgb;
					target->estimate(r) = e;
					target->estimate_tag(r) = true;
					target->known_estimate.insert(t_itr, r);
					target->max_est = std::max(target->max_est, e);

					if (v >= 0) {

						target->visibility(r) = v;
						//target->visibility_tag(r) = true;
						target->max_eval = std::max(target->max_eval, e * v * t_intensity);
						target->max_vis = std::max(v, target->max_vis);
					}

				}

				s_itr++;
			}

		}

		void searchColumnNeighbor(list<Column>::iterator &column, list<Column>::iterator &begin, list<Column>::iterator &end, ref<Random> &random) {
		
			if (column != begin) {
				list<Column>::iterator before = column;
				before--;
				if (before->max_eval == 0) {
					copyVisibility(column, before, random);
					if (before->max_eval > 0) {
						searchColumnNeighbor(before, begin, end, random);
					}
				}
			}


			list<Column>::iterator after = column;
			after++;
			if (after != end && after->max_eval == 0) {
				copyVisibility(column, after, random);
			}

		}

		void spreadVisibility(ref<Random> &random) {

			list<Column>::iterator column = columns.begin();
			list<Column>::iterator begin = column;
			list<Column>::iterator end = columns.end();
			
			while (column != end) {

				if (column->max_eval > 0.0f)
					searchColumnNeighbor(column, begin, end, random);
				column++;
			}

		}
	};


}

class LCMSIntegrator : public ManyLightIntegrator {

	struct SliceLightRecord {

		const VSensorNode *slice;
		Float slice_avg_eval;

		std::vector<const VLightNode *> visible_lights;
		std::vector<Float> visible_eval;

		std::vector<const VLightNode *> invisible_lights;

		SliceLightRecord(const VSensorNode *s) : slice{ s } { slice_avg_eval = 0; }

		void recordInvisibleLight(LTMatrix &matrix) {

			for (auto column : matrix.columns) {
				if (column.max_eval == 0) {
					invisible_lights.push_back(column.lightNode);
				}
			}
		}

		void recordVisibleLight(LTMatrix &matrix) {

			slice_avg_eval = matrix.sum_avg_eval;
			for (auto column : matrix.columns) {
				visible_lights.push_back(column.lightNode);
				visible_eval.push_back(column.max_eval);
			}
		}
	};



public:
	LCMSIntegrator(const Properties &props) : ManyLightIntegrator(props) {

		m_sliceSize = props.getInteger("sliceSize", 512);
		m_maxCutsize = props.getInteger("maxCutsize", 1000);
		m_error_threshold = props.getFloat("errorThreshold", 0.02);
		m_sample_ratio = props.getFloat("sampleRatio", 0.01);
		m_denoise_threshold = props.getFloat("denoiseThreshold", 0.005);
		m_minimum_sample = props.getInteger("minimumSample", 1);
		m_singular_threshold = props.getFloat("singularThreshold", 0.01);
		m_sc_mutex = new Mutex();
		
	//	m_clamping = 0.2;


/*#ifdef MAX_CUT_SIZE
		m_maxCutsize = MAX_CUT_SIZE;
#endif*/

/*#ifdef SPARSE_THRESHOLD
		m_singular_threshold = SPARSE_THRESHOLD;
#endif*/

		m_standard_column_size = 1000;
		m_postprocess_slice = false;
		m_randomFlag = RAND_BOARD;
	}


	LCMSIntegrator(Stream *stream, InstanceManager *manager) : ManyLightIntegrator(stream, manager) { }


	//////////////////////////////////////////////////////////////////////////

	virtual bool preprocess(const Scene *scene, RenderQueue *queue, const RenderJob *job,
		int sceneResID, int sensorResID, int samplerResID) {

		bool success = ManyLightIntegrator::preprocess(scene, queue, job, sceneResID, sensorResID, samplerResID);

		if (!success) return success;

		//TAABB<Point2i> p_aabb = m_sensor_map->m_sensor_tree.m_root->sampleBbox;
		//m_ao.init(p_aabb);

		//for (std::vector<VSensor>::iterator itr = m_sensor_map->m_sensor_tree.m_leaves.begin(); itr != m_sensor_map->m_sensor_tree.m_leaves.end(); itr++) {
		//	uint32_t index = m_ao.index(itr->getImagePos());
		//	m_ao.n_sample[index]++;
		//	m_ao.position[index] += itr->its.p;
		//}

		//uint32_t e = m_ao.n_sample.size();
		//for (uint32_t i = 0; i < e; i++) {
		//	if (m_ao.n_sample[i]) m_ao.position[i] /= m_ao.n_sample[i];
		//}


		// GRADIENT
		//for (int r = m_ao.offset.x; r < m_ao.offset.x + m_ao.size.x; r++)
		//for (int u = m_ao.offset.y; u < m_ao.offset.y + m_ao.size.y; u++) {

		//	Point2i c(r, u);
		//	uint32_t i = m_ao.index(c);
		//	if (m_ao.n_sample[i] == 0) continue;

		//	//Float op[9] = { -1, -2, -1, 0, 0, 0, 1, 2, 1 };
		//	//Float op2[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
		//	Float op[9] = { 1, 1, 1, 1, -8, 1, 1, 1, 1 };
		//	Float op2[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

		//	Point center = m_ao.position[i];
		//	Point neighbor[9];
		//	for (int i = 0; i < 9; i++) {
		//		neighbor[i] = center;
		//	}


		//	
		//	int xb = std::max(c.x - 1, m_ao.offset.x), xe = std::min(c.x + 1, m_ao.offset.x + m_ao.size.x);
		//	int yb = std::max(c.y - 1, m_ao.offset.y), ye = std::min(c.y + 1, m_ao.offset.y + m_ao.size.y);
		//	int valid = 0;
		//	for (int x = xb; x < xe; x++) {
		//		for (int y = yb; y < ye; y++) {
		//			uint32_t index = m_ao.index(Point2i(x, y));
		//			if (m_ao.n_sample[index]) {
		//				neighbor[x - (c.x - 1) + 3 * (y - (c.y - 1))] = m_ao.position[index];
		//				//cout << x - c.x + 3 * (y - c.y) << " " << m_ao.position[index].toString() << endl;
		//			}
		//		}
		//	}

		//	Point lp(0.0f);
		//	Point lp2(0.0f);
		//	for (int i = 0; i < 9; i++) {
		//		lp += neighbor[i] * op[i];
		//		lp2 += neighbor[i] * op2[i];
		//		//cout << neighbor[i].toString() << endl;
		//	}
		//	//cout << lp.toString() << endl;
		//	m_ao.g_position[i] = absolute(lp) + absolute(lp2);
		//	//row->odd_pdf = Vector(lp + lp2).length();
		//}

		m_shadow_count = 0;


		return true;
	}

	void initMatrix(LTMatrix &matrix, const Scene *scene, const VSensorNode *sliceRoot, uint32_t row_block, uint32_t &shadow_count, int max_cutsize) const {

		// row
		int n_row = matrix.n_row;
		{

			matrix.row_sigma = matrix.m_slice_root->bbox.getBSphere().radius / 4;

			struct NodeItem {

				const VSensorNode * node;

				Row::CodeType p_code;
				Row::CodeType p_bit;
				uint32_t row_begin;

				Float split_cost;

				NodeItem(const VSensorNode *n, Row::CodeType p_c, Row::CodeType p_b) :
					node(n), p_code(p_c), p_bit(p_b) { 
				}

				NodeItem(const VSensorNode *n, Row::CodeType p_c, Row::CodeType p_b, uint32_t r_b, Float length_scale = 0) : 
					node(n), p_code(p_c), p_bit(p_b), row_begin(r_b) {
					if (node->isLeaf()) {
						split_cost = 0; 
					} 
					else {
					
						Float f_cost, l_cost, r_cost, s;
						s = 1.0 + (1.0f - node->normal_cone.GetAngleCos()) * 10;
				
						f_cost = s * (node->bbox.max - node->bbox.min).length();

						split_cost = f_cost;
					}
				}

				bool operator<(const NodeItem &n) {
					return split_cost < n.split_cost;
				}
			};

			matrix.row_split.clear();
			matrix.row_split.reserve(row_block);
			bool heap = true; // at least 2 // row_block > 1 ? true : false;

			Vector extend = sliceRoot->bbox.getExtents();
			Float scale = (extend.x + extend.y + extend.z) / 3;


			vector<NodeItem> node_stack;
			node_stack.reserve(n_row);
			node_stack.push_back(NodeItem(sliceRoot, 0, 0, 0, scale));
					
			//std::push_heap(lightcut.begin(), lightcut.end());
			while (node_stack.size() > 0) {

				if (heap) {					
					std::pop_heap(node_stack.begin(), node_stack.end());
				}			
				
		
				NodeItem item = node_stack.back();
				node_stack.pop_back();
				const VSensorNode *n = item.node;		

				if (n->isLeaf()) {
					if (item.p_bit > Row::CodeBit) 	Log(EError, "Singular Slice");

					//Point2i p =
					Vector2i p = n->rep_sensor->getImagePos() - matrix.pixel_offset;
					int r = matrix.pixel_2_row_map.at(p.x, p.y);

					if (matrix.rows[r].sensorNode == n) {
						matrix.rows[r].setCode(item.p_code, item.p_bit);
					}		
				}
				else {

					if (heap) {
						matrix.row_split.push_back(item.row_begin + n->left->num_sensor);
						if (matrix.row_split.size() >= row_block - 1) {
							heap = false;
						}
					}

					if (heap) {
						node_stack.push_back(NodeItem(n->left, item.p_code << 1, item.p_bit + 1, item.row_begin, scale));
						std::push_heap(node_stack.begin(), node_stack.end());

						node_stack.push_back(NodeItem(n->right, (item.p_code << 1) | 1, item.p_bit + 1, item.row_begin + n->left->num_sensor, scale));
						std::push_heap(node_stack.begin(), node_stack.end());
					}
					else {
						node_stack.push_back(NodeItem(n->left, item.p_code << 1, item.p_bit + 1));
						node_stack.push_back(NodeItem(n->right, (item.p_code << 1) | 1, item.p_bit + 1));
					}

				}
			}
			
			while (matrix.row_split.size() > matrix.n_row) {
				matrix.row_split.pop_back();
			}
			std::sort(matrix.row_split.begin(), matrix.row_split.end());
			while (matrix.row_split.size() >= matrix.n_row) {
				matrix.row_split.pop_back();
			}
			matrix.row_split.push_back(n_row);
			
			
			// change row split
			if (1) {

				Float row_per_block = (Float)matrix.rows.size() / (matrix.row_split.size());
				for (int i = 0; i < matrix.row_split.size() - 1; i++) {
					matrix.row_split[i] = std::round(row_per_block * (i + 1));
				}

			}

			std::sort(matrix.rows.begin(), matrix.rows.end());
			matrix.pixel_2_row_map.fill(-1);



			//matrix.generateRowOddPdf(m_ao);
		}


		// column
		{
			matrix.columns.clear();


			struct LightcutHeapItem
			{
				LightcutHeapItem(const uint8_t &pb, const uint64_t &pc, const VLightNode *node, const Spectrum &ub, const Spectrum &est = Spectrum(0.0f)) : _node(node), _estimate(est), _upperBound(ub), p_bit(pb), p_code(pc)
				{
					_e = _upperBound.average();
				}

				bool operator< (const LightcutHeapItem &p2) const
				{
					return _e == p2._e ? (_node < p2._node) : _e < p2._e;
				}


				const VLightNode   *_node;
				Spectrum     _upperBound;
				Float           _e;
				Spectrum        _estimate;

				uint8_t p_bit;
				uint64_t p_code;

				inline uint64_t getMyCode() {
					return p_code << (64 - p_bit);
				}

				inline uint64_t getLeftChildCode() {
					return p_code << (64 - p_bit);
				}

				inline uint64_t getRightChildCode() {
					uint64_t shift = 64 - p_bit;
					return (p_code << shift) | (1 << (shift - 1));
				}
				//list<const VLightNode *>::iterator insert_point;
			};

			struct LightcutCol {

				LightcutCol() { }
				LightcutCol(const VLightNode *n, const uint64_t &p, const Float &e = 1) : node(n), p_code(p), error(e) {}

				const VLightNode *node;
				uint64_t p_code;
				Float error;

				bool operator< (const LightcutCol &p2) const
				{
					return p_code < p2.p_code;
				}

			};



			uint32_t sc = 0;

			vector<LightcutHeapItem> lightcut;		
			lightcut.reserve(max_cutsize);

			Spectrum bound;
			//Spectrum totalEstL, estimate, bound;
			//totalEstL = Spectrum(0.0f);

			
			{
				const VPointTree<VOrientedLight> &ori_tree = m_vpl_map->m_oriented_tree;
				if (ori_tree.m_root) {

					//estimate = sliceRoot->weight * eval(ori_tree.m_root, sliceRoot, scene, sc);
					bound = ori_tree.m_root->errorBoundSMTDI(sliceRoot);
					//totalEstL += ori_tree.m_root->intensity * estimate;

					lightcut.push_back(LightcutHeapItem(2, 0, ori_tree.m_root, bound/*, estimate*/));
					std::push_heap(lightcut.begin(), lightcut.end());

				}
			}

			{
				const VPointTree<VOmniLight> &omni_tree = m_vpl_map->m_omni_tree;
				if (omni_tree.m_root) {

					//estimate = sliceRoot->weight * eval(omni_tree.m_root, sliceRoot, scene, sc);
					bound = omni_tree.m_root->errorBoundSMTDI(sliceRoot);
					//totalEstL += omni_tree.m_root->intensity * estimate;

				
					lightcut.push_back(LightcutHeapItem(2, 1, omni_tree.m_root, bound/*, estimate*/));
					std::push_heap(lightcut.begin(), lightcut.end());
				}
			}

			{
				const VPointTree<VDirectionalLight> &dir_tree = m_vpl_map->m_directional_tree;
				if (dir_tree.m_root) {

					//estimate = sliceRoot->weight * eval(dir_tree.m_root, sliceRoot, scene, sc);
					bound = dir_tree.m_root->errorBoundSMTDI(sliceRoot);
					//totalEstL += dir_tree.m_root->intensity * estimate;

					
					lightcut.push_back(LightcutHeapItem(2, 2, dir_tree.m_root, bound/*, estimate*/));
					std::push_heap(lightcut.begin(), lightcut.end());
				}
			}

			std::vector<LightcutCol> lightcut_cols;
			lightcut_cols.reserve(max_cutsize);

			while (lightcut.size() > 0 && lightcut.size() + lightcut_cols.size() < max_cutsize) {
	
				std::pop_heap(lightcut.begin(), lightcut.end());
				LightcutHeapItem cutItem = lightcut.back();


				//if (!totalEstL.isZero()) {
				//	Float error = cutItem._upperBound.isZero() ? 0.0f : (cutItem._upperBound / totalEstL).average();
				//	if (error < m_error_threshold) {
				//		break;
				//	}
				//}


				lightcut.pop_back();
				 
				const VLightNode *node = cutItem._node;

				if (node->isLeaf()) {
					lightcut_cols.push_back(LightcutCol(node, cutItem.getMyCode(), cutItem._upperBound.average()));
					continue;
				}
				

				//Spectrum leftEst = (node->rep_light == node->left->rep_light) ? cutItem._estimate : sliceRoot->weight * eval(node->left, sliceRoot, scene, sc);
				//Spectrum lL = node->left->intensity * leftEst;


				//Spectrum rightEst = (node->rep_light == node->right->rep_light) ? cutItem._estimate : sliceRoot->weight * eval(node->right, sliceRoot, scene, sc);
				//Spectrum rL = node->right->intensity * rightEst;

				//totalEstL -= node->intensity * cutItem._estimate;
				//totalEstL += lL + rL;


				if (node->left->isLeaf()) {
					bound = node->left->errorBoundSMTDI(sliceRoot);
					lightcut_cols.push_back(LightcutCol(node->left, cutItem.getLeftChildCode(), bound.average()));
				}
				else {
					bound = node->left->errorBoundSMTDI(sliceRoot);

					lightcut.push_back(LightcutHeapItem(cutItem.p_bit + 1, (cutItem.p_code << 1), node->left, bound/*, leftEst*/));
					std::push_heap(lightcut.begin(), lightcut.end());
				}
	
				if (node->right->isLeaf()) {
					bound = node->right->errorBoundSMTDI(sliceRoot);
					lightcut_cols.push_back(LightcutCol(node->right, cutItem.getRightChildCode(), bound.average()));
				}
				else {
					bound = node->right->errorBoundSMTDI(sliceRoot);

					lightcut.push_back(LightcutHeapItem(cutItem.p_bit + 1, (cutItem.p_code << 1) | 1, node->right, bound/*, rightEst*/));
					std::push_heap(lightcut.begin(), lightcut.end());
				}
	
			}
			

			for (vector<LightcutHeapItem>::iterator itr = lightcut.begin(); itr != lightcut.end(); itr++) {
				lightcut_cols.push_back(LightcutCol(itr->_node, itr->getMyCode(), itr->_upperBound.average()));
			}

			std::sort(lightcut_cols.begin(), lightcut_cols.end());

	
			for (std::vector<LightcutCol>::iterator itr = lightcut_cols.begin(); itr != lightcut_cols.end(); itr++) {
		
				matrix.columns.emplace_back(itr->node, matrix.li_spur, matrix.m_pool_manager);
				matrix.columns.back().init(n_row);
				matrix.columns.back().error = itr->error / n_row;
				
			}
			
		
			shadow_count += sc;


		} 
		

		
	}

#if 0
	bool old_sparseSample(LTMatrix &matrix, const Scene *scene, Float ratio, ref<Random> random, uint32_t &shadow_count) const {

		int n_row = matrix.row.size();
		int n_column = matrix.column.size();		

		uint32_t n_shadow = 0;
		uint32_t n_est = 0;

		Float sum_pdf = 0;
		for (list<Column>::iterator i = matrix.column.begin(); i != matrix.column.end(); i++) {
			sum_pdf += i->pdf;
		}
		if (sum_pdf == 0) sum_pdf = 1;


		// sample estimate
		int minSample = 5;
		int maxSample = std::max(std::min(10, n_row), n_row / 10);

		Float parameter = ratio * n_row * n_column / sum_pdf;
		Float sum_eval = 0;

		for (list<Column>::iterator column = matrix.column.begin(); column != matrix.column.end(); column++) {

			int n_sample = math::clamp((int)(column->pdf * parameter), minSample, maxSample);
#ifdef EST_HARD_RATIO
			n_sample = math::clamp((int)(n_row * EST_HARD_RATIO), 0, n_row);
#endif

			column->known_estimate.clear();
			list<uint32_t>::iterator it = column->known_estimate.begin();

			Float max_eval = 0;

			if (n_sample >= n_row * 0.5) {
				Float threshold = (Float)n_sample / n_row;
				for (int r = 0; r < n_row; r++) {
					if (random->nextFloat() <= threshold) {				

						Spectrum e = column->lightNode->eval(*(matrix.row[r].sensor), m_clamping);
						if (e.average() <= m_cutoff) {
							e = Spectrum(0.0f);
						}
						else {
							e *= matrix.row[r].sensor->bsdf_normalize_term; // smooth out detail texture
						}
						

						column->estimate(r) = e;
						it = column->known_estimate.insert(it, r);
						column->estimate_tag(r) = true;

						max_eval = std::max(max_eval, (e * column->lightNode->spur_intensity).average());

					}
				}

			}
			else {	
				Float step = (Float)n_row / n_sample;
				Float ptr = 0.0f;
				int a = std::round(ptr), b;
				while (a < n_row) {

					ptr += step;
					b = std::round(ptr);

					int r = a + random->nextFloat() * (b - a);
					a = b;
					r = std::min(r, n_row - 1);
	

					Spectrum e = column->lightNode->eval(*(matrix.row[r].sensor), m_clamping);
					if (e.average() <= m_cutoff) {
						e = Spectrum(0.0f);
					}
					else {
						e *= matrix.row[r].sensor->bsdf_normalize_term; // smooth out detail texture
					}
					

					column->estimate(r) = e;
					it = column->known_estimate.insert(it, r);
					column->estimate_tag(r) = true;

					max_eval = std::max(max_eval, (e * column->lightNode->spur_intensity).average());
					
				}
			}

			n_est += column->known_estimate.size();
			column->max_eval = max_eval;
			sum_eval += max_eval;
		}

		//////////////////////////////////////////////////////////////////////////

		// sample visibility
		minSample = 1;
		maxSample = std::max(std::min(10, n_row), n_row / 10);

		n_est = 0;
		parameter = ratio * n_row * n_column / sum_eval;
		int c = 0;
		for (list<Column>::iterator column = matrix.column.begin(); column != matrix.column.end(); column++, c++) {

			if (column->max_eval == 0) {
				n_est += column->known_estimate.size();
				continue;
			}

			int n_sample = math::clamp((int)(column->max_eval * parameter), minSample, maxSample);
#ifdef VIS_HARD_RATIO
			n_sample = math::clamp((int)(n_row * VIS_HARD_RATIO), minSample, n_row);
#endif
			column->known_visibility.clear();
			list<uint32_t>::iterator it = column->known_visibility.begin();

			if (n_sample >= n_row * 0.5) {
				Float threshold = (Float)n_sample / n_row;
				for (int r = 0; r < n_row; r++) {

				
					if (random->nextFloat() <= threshold) {
						
						Spectrum e;
						if (column->known_estimate.insert(r).second) {
							e = column->lightNode->eval(*(matrix.row[r].sensorNode->rep_sensor), m_clamping);
							if (e.average() <= m_cutoff) {
								e = Spectrum(0.0f);
							}
							else {
								e *= matrix.row[r].sensor->bsdf_normalize_term; // smooth out detail texture
							}				

							column->estimate(r) = e;
						}
						else {
							e = column->estimate(r);
						}

	
						if (!e.isZero()) {
							Ray ray = column->lightNode->shadowRay(*(matrix.row[r].sensorNode->rep_sensor));
							column->visibility(r) = !scene->rayIntersect(ray);
							it = column->known_visibility.insert(it, r);
							column->visibility_tag(r) = true;
						}
						
					}
				}
			}
			else {
				Float step = (Float)n_row / n_sample;
				Float ptr = 0.0f;
				int a = std::round(ptr), b;
				while (a < n_row) {

					ptr += step;
					b = std::round(ptr);
				
					int r = a + random->nextFloat() * (b - a);
					a = b;
					r = std::min(r, n_row - 1);

					Spectrum e;
					if (column->estimate_tag(r)) {
						e = column->estimate(r);
					}
					else {

						column->known_estimate.insert(r);
						e = column->lightNode->eval(*(matrix.row[r].sensorNode->rep_sensor), m_clamping);
						if (e.average() <= m_cutoff) {
							e = Spectrum(0.0f);
						}
						else {
							e *= matrix.row[r].sensor->bsdf_normalize_term; // smooth out detail texture
						}
						column->estimate(r) = e;
					}


					if (!e.isZero()) {
						Ray ray = column->lightNode->shadowRay(*(matrix.row[r].sensorNode->rep_sensor));
						column->visibility(r) = !scene->rayIntersect(ray);
						it = column->known_visibility.insert(it, r);
						column->visibility_tag(r) = true;
					}		
			
				}

			}

			n_shadow += column->known_visibility.size();
			n_est += column->known_estimate.size();
		}

		shadow_count += n_shadow;
		cout << n_est << " " << n_shadow << endl;
		return true;
	}
#endif


	void sparseSample(LTMatrix &matrix, const Scene *scene, Float ratio, ref<Random> random, uint32_t &shadow_count) const {

		Float sum_avg_eval = 0;
		Float sum_max_eval = 0;
		uint32_t sc = 0;

		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++) {		
			
			matrix.sampleColumn(column, random);

			sum_avg_eval += column->avg_eval;
			sum_max_eval += column->max_eval;
			sc += column->known_visibility.size();

			
		} // end column

		//////////////////////////////////////////////////////////////////////////
		matrix.sum_avg_eval = sum_avg_eval;
		matrix.sum_max_eval = sum_max_eval;
		shadow_count += sc;

	}

	inline void getNeighbor(list<Column>::iterator &b, list<Column>::iterator &e, list<Column>::iterator &begin, list<Column>::iterator &end, Float range) const {

		Float b_range = range;
		while (b_range > 0 && b != begin) {
			b--;
			b_range -= b->avg_intensity;
		}

		Float e_range = range;
		e++;
		while (e_range > 0 && e != end) {			
			e_range -= e->avg_intensity;
			e++;
		}
		
	}




	void expandMatrix(LTMatrix &matrix, Float threshold, ref<Random> &random, uint32_t &shadow_count) const {

		
		
		struct ExpandItem {

			Float cost;
			const VLightNode *node;
			list<Column>::iterator column;

			ExpandItem(list<Column>::iterator &c) : node(c->lightNode), column(c), cost(c->highest_cost_now) { }

			bool operator<(const ExpandItem &b) const {
				return cost < b.cost;
			}
		};

		std::priority_queue<ExpandItem> expand_queue;

		// init 
		list<Column>::iterator b, e; 
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++) {		

			if (column->lightNode->isLeaf()) continue;
			b = e = column; 
			Float max_est = 0;
			int16_t max_vis = column->max_vis;

			auto matcol_beg = matrix.columns.begin();
			auto matcol_end = matrix.columns.end();

			getNeighbor(b, e, matcol_beg, matcol_end, (max_vis > 0 ? 1 : 2) * NEIGHBOR_RANGE * column->avg_intensity);

			while (b != e) {
				max_est = std::max(b->max_est, max_est);
				max_vis = std::max(b->max_vis, max_vis);
				b++;
			}

			if (b != matrix.columns.end()) {
				max_est = std::max(b->max_est, max_est);
				max_vis = std::max(b->max_vis, max_vis);
			}

			column->highest_vis_now = max_vis;
			column->highest_est_now = max_est;
			column->highest_cost_now = std::min(column->error, column->avg_intensity * max_est * max_vis);
			if (column->highest_cost_now > threshold) expand_queue.push(ExpandItem(column));
		}

		while (matrix.columns.size() < m_maxCutsize && !expand_queue.empty()) {

			ExpandItem current = expand_queue.top();
			expand_queue.pop();


			if (current.column->lightNode != current.node) continue; // has been split by higher cost
			list<Column>::iterator &current_column = current.column;
			Float p_intensity = current_column->avg_intensity;

			
			list<Column>::iterator sibling_column = matrix.splitColumn(current_column, random, shadow_count);
			

			Float max_est = std::max(current_column->max_est, sibling_column->max_est);
			int16_t max_vis = std::max(current_column->max_vis, sibling_column->max_vis);

			if (current.node->left == current_column->lightNode) {
				b = current_column; e = sibling_column;
			}
			else {
				b = sibling_column; e = current_column;
			}



			Float vis_componsate = current_column->max_vis == 0 && max_vis > 0 ? 2 : 1; // found a visible column in dark region, announce it
			auto matcol_beg = matrix.columns.begin();
			auto matcol_end = matrix.columns.end();
			getNeighbor(b, e, matcol_beg, matcol_end, std::max(current_column->avg_intensity, sibling_column->avg_intensity) * vis_componsate * NEIGHBOR_RANGE);
			//getNeighbor(b, e, begin, end, p_intensity * NEIGHBOR_RANGE);		



			Float hold_max_est = max_est; int16_t hold_max_vis = max_vis;
			int count = 0;
			while (b != e) {
				
			
				max_est = std::max(b->max_est, max_est);
				max_vis = std::max(b->max_vis, max_vis);

				if (hold_max_est > b->highest_est_now || hold_max_vis > b->highest_vis_now) {

					b->highest_est_now = std::max(b->highest_est_now, hold_max_est);
					b->highest_vis_now = std::max(b->highest_vis_now, hold_max_vis);

					Float v = b->avg_intensity * b->highest_est_now * b->highest_vis_now;
					if (v > b->highest_cost_now) {
						b->highest_cost_now = std::min(b->error, v);
						if (b->highest_cost_now > threshold && !b->lightNode->isLeaf()) expand_queue.push(ExpandItem(b));
					}

				}
				
				b++;
			}

			if (b != matrix.columns.end()) {

				max_est = std::max(b->max_est, max_est);
				max_vis = std::max(b->max_vis, max_vis);

				if (hold_max_est > b->highest_est_now || hold_max_vis > b->highest_vis_now) {

					b->highest_est_now = std::max(b->highest_est_now, hold_max_est);
					b->highest_vis_now = std::max(b->highest_vis_now, hold_max_vis);

					Float v = b->avg_intensity * b->highest_est_now * b->highest_vis_now;
					if (v > b->highest_cost_now) {
						b->highest_cost_now = std::min(b->error, v);
						if (b->highest_cost_now > threshold && !b->lightNode->isLeaf()) expand_queue.push(ExpandItem(b));
					}

				}
			}
	

			current_column->highest_est_now = max_est;
			current_column->highest_vis_now = max_vis;
			current_column->highest_cost_now = std::min(current_column->error, max_est * max_vis * current_column->avg_intensity);
			if (current_column->highest_cost_now > threshold && !current_column->lightNode->isLeaf()) expand_queue.push(ExpandItem(current_column));


			sibling_column->highest_est_now = max_est;
			sibling_column->highest_vis_now = max_vis;
			sibling_column->highest_cost_now = std::min(sibling_column->error, max_est * max_vis * sibling_column->avg_intensity);
			if (sibling_column->highest_cost_now > threshold && !sibling_column->lightNode->isLeaf()) expand_queue.push(ExpandItem(sibling_column));

			
		}
	

	}

	void contractMatrix(LTMatrix &matrix, Float intensity_threshold, Float error_threshold, ref<Random> &random, uint32_t &shadow_count) const {

		list<Column>::iterator last_column = matrix.columns.begin();
		list<Column>::iterator column = last_column; column++;
		

		while (column != matrix.columns.end()) {

			{
				int c = 0;
				list<Column>::iterator jj = column;

			}
			
			if (last_column == matrix.columns.begin() || last_column->lightNode->parent != column->lightNode->parent || last_column->lightNode->parent == NULL) {
				last_column = column;
				column++;
				continue;
			}

			column = matrix.mergeColumn(last_column, column, random, intensity_threshold, error_threshold);
			last_column = column;
			if (last_column == matrix.columns.begin()) {
				column++;
			}
			else {
				last_column--;
			}
		}
	}

	void balanceMatrix(LTMatrix &matrix, ref<Random> random, uint32_t &shadow_count, Float sample_ratio, Float singular_threshold) const {

		matrix.updateColumnID();
		for (list<Column>::iterator itr = matrix.columns.begin(); itr != matrix.columns.end(); itr++) {
			itr->expand_col_id = itr->col_id;
		}
		// expand
		//cout << matrix.columns.size() << endl;
		Float intensity_threshold = std::max(matrix.sum_avg_eval * singular_threshold, MAX_AVG_EVAL);
		Float error_threshold = intensity_threshold;

		expandMatrix(matrix, intensity_threshold, random, shadow_count);
		matrix.spreadVisibility(random);


		matrix.generateRowShadowPdf(); // row pdf
		int n_valid_column = 0;
		for (list<Column>::iterator itr = matrix.columns.begin(); itr != matrix.columns.end(); itr++)
		{
			if (itr->max_eval > 0) n_valid_column++;
		}

		//if (0) { // supplement sample
		//	uint32_t n_row = matrix.rows.size();
		//	Float parameter = n_row * n_valid_column * sample_ratio / matrix.sum_max_eval;

		//	Fvec row_ratio(n_row);
		//	for (uint32_t r = 0; r < n_row; r++) {
		//		row_ratio.at(r) = std::max(0.0f, matrix.rows[r].shadow_pdf);
		//		//row_ratio.at(r) = std::min(row_ratio.at(r), (Float)SHADOW_REINFORCE);

		//		//Float &ra = matrix.rows[r].shadow_pdf;
		//		//if (ra < SHADOW_REINFORCE) {
		//		//	row_ratio.at(r) = 0.5 * ra * ra;
		//		//}
		//		//else {
		//		//	row_ratio.at(r) = SHADOW_REINFORCE * (ra - 0.5 * SHADOW_REINFORCE);
		//		//}
		//		//row_ratio(r) *= 50;
		//	}

		//	if (max(row_ratio) > 0) {
		//		Fvec ratio;
		//		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); ) {
		//			//if (column->max_eval == 0) column->max_eval = column->highest_cost_now;
		//			

		//			Float column_ratio = (column->max_eval == 0 ? column->highest_vis_now * column->max_est : column->max_eval) * parameter / n_row;
		//			if (column->max_eval == 0) column_ratio = 0;
		//			if (column_ratio == 0) {
		//				column++;
		//				continue;
		//			}

		//			if (column->balance != Column::BAfterRowSample) {
		//				ratio = row_ratio * column_ratio;
		//				supplementColumn(column, matrix, ratio, random);
		//				column->balance = Column::BAfterRowSample;
		//			}


		//			if (column->max_eval > intensity_threshold && !column->lightNode->isLeaf()) {
		//				column = matrix.splitColumn(column, random, shadow_count);
		//			}
		//			else {
		//				column++;
		//			}
		//			
		//		}
		//	}
		//}


		// contract
	//	cout << matrix.columns.size() << endl;
		matrix.updateColumnAverage();
		
		/*int n_valid = 0;
		for (auto column : matrix.columns) {
			if (column.max_eval > 0) {
				n_valid++;
			}
		}

		if (n_valid > 50) {
			intensity_threshold = std::max(matrix.sum_avg_eval * singular_threshold, MAX_AVG_EVAL);
			error_threshold = std::max(0.1f * intensity_threshold, MAX_AVG_EVAL);

			contractMatrix(matrix, intensity_threshold, error_threshold, random, shadow_count);
		}*/

		//cout << matrix.columns.size() << endl;

	}

	void copyVisibility(list<Column>::iterator &source_column, list<Column>::iterator &dst_column, LTMatrix &matrix, ref<Random> &random) const {
		
		uint32_t n_row = matrix.rows.size();

		list<Column>::iterator &c_column = source_column;
		list<uint32_t>::iterator c_itr = c_column->known_estimate.begin();
		list<uint32_t>::iterator c_end = c_column->known_estimate.end();
		Float c_i = c_column->spur_intensity.average();

		list<Column>::iterator &s_column = dst_column;
		list<uint32_t>::iterator s_itr = s_column->known_estimate.begin();
		list<uint32_t>::iterator s_begin = s_itr;
		list<uint32_t>::iterator s_end = s_column->known_estimate.end();
		Float s_i = s_column->spur_intensity.average();


		for (; c_itr != c_end; c_itr++) {

			uint32_t r = *c_itr;
			while (s_itr != s_end && *s_itr < r) {
				s_itr++;
			}

			if (s_itr != s_end && *s_itr == r) {
				continue;
			}

			bool copy = false;
			list<uint32_t>::iterator e = s_itr;
			list<uint32_t>::iterator b = s_itr;
			if (b != s_begin) b--;
			if (e == s_end && e != s_begin) {
				e--;
			}

			if (sign(c_column->visibility(*c_itr)) != sign(s_column->visibility(*e)) && 
				sign(c_column->visibility(*c_itr)) != sign(s_column->visibility(*b))) { 

				Float e;
				int16_t v = -1;
				Spectrum m_rgb;
				matrix.sampleElement(r, s_column, e, m_rgb, v, random);
				
				s_column->estimate(r) = e;
				s_column->m_rgb(r) = m_rgb;
				s_column->estimate_tag(r) = true;
				s_column->known_estimate.insert(s_itr, r);


				if (v >= 0) {
					s_column->visibility(r) = v;
					//s_column->visibility_tag(r) = true;
					s_column->max_eval = std::max(s_column->max_eval, e * v * s_i);
					s_column->sum_est_visibility += e;
				}
								
			}

		}

	}


	//void supplementColumn(list<Column>::iterator &column, LTMatrix &matrix, Fvec &row_ratio, ref<Random> &rr) const {

	//	uint32_t n_row = matrix.rows.size();

	//	Float intensity = column->spur_intensity.average();

	//	list<uint32_t>::iterator v_itr = column->known_visibility.begin();
	//	list<uint32_t>::iterator v_end = column->known_visibility.end();
	//	list<uint32_t>::iterator e_itr = column->known_estimate.begin();
	//	list<uint32_t>::iterator e_end = column->known_estimate.end();
	//	bool test = false;

	//	for (uint32_t r = 0; r < n_row; r++) {
	//		if (column->estimate_tag(r)) continue;
	//	
	//		Float &ra = column->random(r);
	//		ra = rr->nextFloat();
	//		if (ra > row_ratio[r]) {
	//			ra = (ra - row_ratio[r]) / (1.0f - row_ratio[r]);
	//			continue;
	//		}

	//		Float e;
	//		Spectrum m_rgb;
	//		int16_t v = -1;
	//		matrix.sampleElement(r, column, e, m_rgb, v, rr);

	//		if (column->max_eval == 0 && e * v != 0) {
	//			test = true;
	//		}

	//		column->m_rgb(r) = m_rgb;
	//		column->estimate(r) = e;
	//		column->estimate_tag(r) = true;

	//		while (e_itr != e_end && *e_itr < r) {
	//			e_itr++;
	//		}


	//		column->known_estimate.insert(e_itr, r);

	//		if (v < 0) continue;
	//		column->max_eval = std::max(column->max_eval, intensity * e);
	//		column->visibility(r) = v;
	//		column->sum_est_visibility += e;

	//		while (v_itr != v_end && *v_itr < r) {
	//			v_itr++;
	//		}
	//		column->known_visibility.insert(v_itr, r);

	//	}
	//}


	//void supplementSample(vector<uint32_t> &special, LTMatrix &matrix, ref<Random> random, uint32_t &shadow_count, Float singular_threshold) const {
	//	return;
	//	uint32_t sc = 0;
	//	uint32_t n_row = matrix.rows.size();

	//	matrix.sum_avg_eval = 0;
	//	matrix.n_valid_column = 0;
	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++) {
	//		matrix.sum_avg_eval += column->avg_eval;
	//		if (column->max_eval > 0) matrix.n_valid_column++;
	//	}

	//	


	//	//{

	//	//	Fvec row_ratio(n_row);
	//	//	for (uint32_t r = 0; r < n_row; r++) {
	//	//		row_ratio.at(r) = matrix.rows[r].odd_pdf;
	//	//	}

	//	//	Float parameter = n_row * m_standard_column_size * m_sample_ratio / matrix.sum_max_eval;

	//	//	vector<list<Column>::iterator> dark_columns;
	//	//	Fvec ratio; uint32_t valid_c = 0;
	//	//	Float sum_column_ratio = 0;
	//	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++) {

	//	//		if (column->max_eval == 0){
	//	//			dark_columns.push_back(column);
	//	//		}
	//	//		else {
	//	//			
	//	//			Float *column_random = pre_elem_randoms.colptr(column->col_id);
	//	//			for (uint32_t r = 0; r < n_row; r++) column_random[r] = random->nextFloat();
	//	//			continue;
	//	//			Float column_ratio = std::max((column->max_eval * parameter - column->known_estimate.size()), 1.0f) / n_row;
	//	//			ratio = row_ratio * column_ratio;
	//	//			sum_column_ratio += column_ratio;

	//	//			supplementColumn(column, matrix, ratio, column_random, random);
	//	//		}
	//	//	}

	//	//	return;
	//	//	matrix.spreadVisibility(random);
	//	//	for (uint32_t i = 0; i < dark_columns.size(); i++) {
	//	//		list<Column>::iterator &column = dark_columns[i];
	//	//		if (column->max_eval > 0) {

	//	//			Float *column_random = pre_elem_randoms.colptr(column->col_id);
	//	//			for (uint32_t r = 0; r < n_row; r++) column_random[r] = random->nextFloat();

	//	//			Float column_ratio = std::max((column->max_eval * parameter - column->known_estimate.size()), 1.0f) / n_row;
	//	//			ratio = row_ratio * column_ratio;
	//	//			ratio.fill(10);

	//	//			supplementColumn(column, matrix, ratio, column_random, random);

	//	//		}
	//	//	}
	//	//}
	//	//return;
	//	//matrix.spreadVisibility(random);
	//	matrix.generateRowShadowPdf();
	//	
	//	{
	//		Float parameter = n_row * m_standard_column_size * m_sample_ratio / matrix.sum_max_eval;

	//		Fvec row_ratio(n_row);
	//		for (uint32_t r = 0; r < n_row; r++) {
	//			row_ratio.at(r) = std::max(0.0f, matrix.rows[r].shadow_pdf);
	//			row_ratio.at(r) = std::min(row_ratio.at(r), (Float)SHADOW_REINFORCE);
	//		}	

	//		if (max(row_ratio) > 0) {
	//			Fvec ratio;
	//			for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++) {
	//				//if (column->max_eval == 0) column->max_eval = column->highest_cost_now;
	//				//if (column->max_eval == 0) continue;

	//				Float column_ratio = (column->max_eval == 0 ? column->highest_cost_now : column->max_eval) * parameter / n_row;
	//				if (column_ratio == 0) continue;


	//				ratio = row_ratio * column_ratio;
	//				
	//				supplementColumn(column, matrix, ratio, random);				
	//			}
	//		}
	//	}


	//}

	void writeOutMatrix(LTMatrix &matrix, const Scene *scene) const {

		int nRow = matrix.rows.size();
		int nCol = matrix.columns.size();

		Fmat est_mat(nRow, nCol);
		Fmat vis_mat(nRow, nCol);

		int c = 0;
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, c++) {

			for (uint32_t r = 0; r < nRow; r++) {

				est_mat(r, c) = (column->estimate(r) * column->spur_intensity).average();

				Ray ray = column->lightNode->shadowRay(matrix.rows[r].sensorNode->rep_sensor);
				if (scene->rayIntersect(ray)) {
					vis_mat(r, c) = 0;
				}
				else {
					vis_mat(r, c) = 1;
				}
			}

		}


		est_mat.save("est.txt", arma::raw_ascii);
		vis_mat.save("vis.txt", arma::raw_ascii);

	}

	void pfm(Fmat &D, const char *filename, Float thread = 0.5) const {

		ref<Bitmap> bitmap = new Bitmap(Bitmap::ERGB, Bitmap::EFloat32, Vector2i(D.n_cols, D.n_rows), 3);
		ref<FileStream> stream = new FileStream(filename, FileStream::ETruncWrite);
	

		for (int y = 0; y < D.n_rows; y++) {
			for (int x = 0; x < D.n_cols; x++) {
				Spectrum s(0.0f);
				if (D(y, x) > thread / 2) {
					s[1] = D(y, x) - thread;
				}
				else if (D(y, x) >= 0) {				
					s[0] = D(y, x);				
				}
				else {
					s[2] = -D(y, x);
				}
				bitmap->setPixel(Point2i(x, y), s);
			}
		}

		bitmap->write(Bitmap::EPFM, stream);
	}

	//void disperseNoise(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

	//	uint32_t n_row = matrix.rows.size();
	//	uint32_t n_column = matrix.columns.size();
	//	
	//	D.set_size(n_row, n_column);
	//	known.set_size(n_row * n_column);		

	//	Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
	//	uint32_t cc = 0;
	//	Uvec::size_type n_known = 0;
	//	int32_t c = -1;
	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

	//		if (column->max_eval == 0) {
	//			continue;
	//		}

	//		if (column->max_eval > singular_threshold) {
	//			//continue;
	//			//assert(column->lightNode->isLeaf());
	//			
	//			for (uint32_t r = 0; r < n_row; r++) {

	//				if (!column->estimate_tag(r)) {
	//					Float e;
	//					int16_t v = -1;
	//					Spectrum m_rgb;
	//					matrix.sampleElement(r, column, e, m_rgb, v, random);
	//					column->estimate(r) = e;
	//					column->m_rgb(r) = m_rgb;
	//					column->visibility(r) = v;
	//					
	//					//	column->estimate_tag(r) = true;
	//				}
	//			}

	//			singular_column.push_back(column);

	//			continue;
	//		}


	//		sparse_column.push_back(column);
	//		c++;
	//		Fmat::size_type p = c * n_row;

	//		bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;
	//		//m_term = true;

	//		for (auto nr : column->known_estimate) {
	//			known.at(n_known++) = p + nr;
	//		}
	//	
	//		Float intensity = column->spur_intensity.average();
	//		if (column->known_estimate.size() == n_row) {
	//			for (uint32_t r = 0; r < n_row; r++) {					
	//				D.at(p++) = intensity * column->estimate(r) * column->visibility(r);
	//			}
	//			continue;
	//		}
	//		

	//		list<uint32_t>::iterator ii = column->known_estimate.begin();
	//	
	//		uint32_t begin = *ii;
	//		Float f = intensity * column->estimate(begin) * column->visibility(begin);
	//		for (uint32_t r = 0; r < begin; r++) {		
	//			Float v = f;
	//			column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();
	//			if (m_term && f > 0) {
	//				v = intensity * column->estimate(r);
	//			}

	//			D.at(p++) = v;
	//		}

	//		D.at(p++) = f;

	//		uint32_t last_ii = begin;
	//		list<uint32_t>::iterator ee = column->known_estimate.end();
	//		ee--;
	//		uint32_t end = *ee;
	//		ii++;

	//		for (uint32_t r = begin + 1; r < end; r++) {

	//			if (*ii == r) {
	//				D.at(p++) = intensity * column->estimate(r) * column->visibility(r);
	//				last_ii = *ii;
	//				ii++;
	//			}
	//			else {
	//				Float d0 = matrix.getRowDistance(last_ii, r);
	//				Float d1 = matrix.getRowDistance(r, *ii);

	//				
	//				uint32_t rep_r = random->nextFloat() < (Float)d0 / (d0 + d1) ? last_ii : *ii;
	//				Float v;
	//				if (m_term) {
	//					v = column->estimate(rep_r) * column->visibility(rep_r);
	//					column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

	//					if (v > 0) {
	//						v = column->estimate(r);
	//					}
	//				}
	//				else {
	//					v = column->estimate(rep_r) * column->visibility(rep_r);
	//				}
	//	
	//				//D.at(p++) = 0.5 * intensity * (column->estimate(last_ii) * column->visibility(last_ii) + column->estimate(*ii) * column->visibility(*ii));
	//				D.at(p++) = intensity * v;
	//			}
	//		}

	//		
	//		if (end < n_row) {
	//			f = intensity * column->estimate(end) * column->visibility(end);
	//			D.at(p++) = f;		

	//			for (uint32_t r = end + 1; r < n_row; r++) {

	//				Float v = f;
	//				column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();
	//				if (m_term && f > 0) {
	//					v = intensity * column->estimate(r);
	//				}

	//				D.at(p++) = v;
	//			}
	//		}

	//	}


	//	D.resize(n_row, sparse_column.size());	
	//	known.resize(n_known);


	//}



#ifdef UV
	void disperseNoise_UV(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {
		
		uint32_t n_row = matrix.rows.size();
		uint32_t n_column = matrix.columns.size();

		D.set_size(n_row, n_column);
		known.set_size(n_row * n_column);

		vector<LTMatrix::UVLink> row_cluster[4];
		for (int i = 0; i < 4; i++) {
			row_cluster[i].reserve(n_row);
		}
		std::vector<int> r_tag; r_tag.resize(n_row);

		Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
		uint32_t cc = 0;
		Uvec::size_type n_known = 0;
		int32_t c = -1;
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

			if (column->max_eval == 0) {
				continue;
			}

			if (column->max_eval > singular_threshold) {

				for (uint32_t r = 0; r < n_row; r++) {

					if (!column->estimate_tag(r)) {
						Float e;
						int16_t v = -1;
						matrix.sampleElement(r, column, e, v, random);
						column->estimate(r) = e;
						column->visibility(r) = v;
					}
				}

				singular_column.push_back(column);

				continue;
			}


			sparse_column.push_back(column);
			c++;
			Fmat::size_type p = c * n_row;
			Float intensity = column->spur_intensity.average();

			if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {
				for (uint32_t r = 0; r < n_row; r++) {
					int pp = p + r;
					known.at(n_known++) = pp;
					D.at(pp) = intensity * column->estimate(r) * column->visibility(r);
				}
				continue;
			}

			for (auto nr : column->known_estimate) {
				int pp = p + nr;
				known.at(n_known++) = pp;
				D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
			}


			bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;
	
			//////////////////////////////////////////////////////////////////////////


			for (int i = 0; i < 4; i++) {
				row_cluster[i].clear();
			}

			std::fill(r_tag.begin(), r_tag.end(), -1);
			for (int r = 0; r < n_row; r++) {

				int ur = matrix.u_link[r].r_id;
				if (column->estimate_tag(ur)) continue;

				int ran = random->nextFloat() * (4);
				if (ran > 1) {
					r_tag[ur] = ran;
				}
				else {
					row_cluster[ran].push_back(matrix.u_link[r]);
				}

			}

			for (int r = 0; r < n_row; r++) {
				int vr = matrix.v_link[r].r_id;

				int ran = r_tag[vr];
				if (ran < 0) {
					continue;
				}
				else {
					row_cluster[ran].push_back(matrix.v_link[r]);
				}
			}

			auto posSearch = [&](vector<LTMatrix::UVLink> &cluster, Column::CROSS_IT &it) {
				int n = cluster.size();
				for (int i = 0; i < n; i++) {

					while (it->first < cluster[i].coord) it++;

					int r = cluster[i].r_id;
					//int rep_r = getClosetNeighbor(matrix, it, cluster[i].r_id, cluster[i].coord, true);

					//if (r == 0) {
					//	cout << matrix.getRow3DDistance(r, rep_r) << " " << matrix.rows[rep_r].sensor->getImagePos().toString() << endl;
					//}

					if (it->first == FLT_MAX) it--;
					if (it->first == -FLT_MAX) it++;
					int rep_r = it->second;

					Float v;
					if (m_term) {
						v = column->estimate(rep_r) * column->visibility(rep_r);
						column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

						if (v > 0) {
							v = column->estimate(r);
						}
					}
					else {
						v = column->estimate(rep_r) * column->visibility(rep_r);
					}			

					D.at(p + r) = intensity * v;
				}
			};

			auto negSearch = [&](vector<LTMatrix::UVLink> &cluster, Column::CROSS_IT &it) {

				it--;
				int n = cluster.size();
				for (int i = n - 1; i >= 0; i--) {

					while (it->first > cluster[i].coord) it--;

					int r = cluster[i].r_id;
					//int rep_r = getClosetNeighbor(matrix, it, cluster[i].r_id, cluster[i].coord, false);

					//if (r == 0) {
					//	cout << matrix.getRow3DDistance(r, rep_r) << " " << matrix.rows[rep_r].sensor->getImagePos().toString() << endl;
					//}

					if (it->first == FLT_MAX) it--;
					if (it->first == -FLT_MAX) it++;
					int rep_r = it->second;

					Float v;
					if (m_term) {
						v = column->estimate(rep_r) * column->visibility(rep_r);
						column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

						if (v > 0) {
							v = column->estimate(r);
						}
					}
					else {
						v = column->estimate(rep_r) * column->visibility(rep_r);
					}

				
					D.at(p + r) = intensity * v;
				}

				

			};


			posSearch(row_cluster[0], column->u_known_sample.begin());
			negSearch(row_cluster[1], column->u_known_sample.end());
			posSearch(row_cluster[2], column->v_known_sample.begin());
			negSearch(row_cluster[3], column->v_known_sample.end());


		}

		D.resize(n_row, sparse_column.size());
		known.resize(n_known);
	
	}

	void disperseNoise_UV_new(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

		uint32_t n_row = matrix.rows.size();
		uint32_t n_column = matrix.columns.size();

		D.set_size(n_row, n_column);
		known.set_size(n_row * n_column);

		struct Item {
			int rep_row[4];
			Float pdf[4];
		};

		std::vector<Item> r_tag; r_tag.resize(n_row);

		Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
		uint32_t cc = 0;
		Uvec::size_type n_known = 0;
		int32_t c = -1;
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

			if (column->max_eval == 0) {
				continue;
			}

			if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

				for (uint32_t r = 0; r < n_row; r++) {

					if (!column->estimate_tag(r)) {
						Float e;
						int16_t v = -1;
						matrix.sampleElement(r, column, e, v, random);
						column->estimate(r) = e;
						column->visibility(r) = v;
					}
				}

				singular_column.push_back(column);

				continue;
			}


			sparse_column.push_back(column);
			c++;
			Fmat::size_type p = c * n_row;
			Float intensity = column->spur_intensity.average();

			//if (column->known_estimate.size() == n_row) {

			//	for (uint32_t r = 0; r < n_row; r++) {
			//		int pp = p + r;
			//		known.at(n_known++) = pp;
			//		D.at(pp) = intensity * column->estimate(r) * column->visibility(r);
			//	}
			//	continue;
			//}

			for (auto nr : column->known_estimate) {
				int pp = p + nr;
				known.at(n_known++) = pp;
				D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
			}


			bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;

			//////////////////////////////////////////////////////////////////////////

			if (1) { // u
				Column::CROSS_IT lower_bound_it = column->u_known_sample.begin();
				Column::CROSS_IT before_it = lower_bound_it;
				lower_bound_it++;

				for (int cr = 0; cr < n_row; cr++) {

					int r = matrix.u_link[cr].r_id;
					if (column->estimate_tag(r)) continue;

					Float u_coord = matrix.rows[r].u_coord;
					while (u_coord > lower_bound_it->first) {
						before_it++;
						lower_bound_it++;
					}

					if (lower_bound_it->second >= 0) {
						int rep_r = getClosetNeighbor(matrix, lower_bound_it, r, u_coord, true);
						r_tag[r].rep_row[0] = rep_r;
						r_tag[r].pdf[0] = 1.0f / matrix.getRow3DDistance(r, rep_r);
					}
					else {
						r_tag[r].rep_row[0] = -1;
						r_tag[r].pdf[0] = 0;
					}

					if (before_it->second >= 0) {
						int rep_r = getClosetNeighbor(matrix, before_it, r, u_coord, false);
						r_tag[r].rep_row[1] = rep_r;
						r_tag[r].pdf[1] = 1.0f / matrix.getRow3DDistance(r, rep_r);
					}
					else {
						r_tag[r].rep_row[1] = -1;
						r_tag[r].pdf[1] = 0;
					}
				}

			}
			

			if (1) { // v
				Column::CROSS_IT lower_bound_it = column->v_known_sample.begin();
				Column::CROSS_IT before_it = lower_bound_it;
				lower_bound_it++;

				for (int cr = 0; cr < n_row; cr++) {

					int r = matrix.v_link[cr].r_id;
					if (column->estimate_tag(r)) continue;

					Float v_coord = matrix.rows[r].v_coord;
					while (v_coord > lower_bound_it->first) {
						before_it++;
						lower_bound_it++;
					}

					if (lower_bound_it->second >= 0) {
						int rep_r = getClosetNeighbor(matrix, lower_bound_it, r, v_coord, true);
						r_tag[r].rep_row[2] = rep_r;
						r_tag[r].pdf[2] = 1.0f / matrix.getRow3DDistance(r, rep_r);
					}
					else {
						r_tag[r].rep_row[2] = -1;
						r_tag[r].pdf[2] = 0;
					}

					if (before_it->second >= 0) {
						int rep_r = getClosetNeighbor(matrix, before_it, r, v_coord, false);
						r_tag[r].rep_row[3] = rep_r;
						r_tag[r].pdf[3] = 1.0f / matrix.getRow3DDistance(r, rep_r);
					}
					else {
						r_tag[r].rep_row[3] = -1;
						r_tag[r].pdf[3] = 0;
					}
				}
			}

			for (int r = 0; r < n_row; r++) {

				if (column->estimate_tag(r)) continue;

				Float pdf_sum = r_tag[r].pdf[0] + r_tag[r].pdf[1] + r_tag[r].pdf[2] + r_tag[r].pdf[3];
				Float ran = random->nextFloat() * pdf_sum;

	

				Float s = 0;
				int select = -1;
				for (int i = 0; i < 4; i++) {
					s += r_tag[r].pdf[i];
					if (ran <= s) {
						select = i;
						break;
					}
				}
				assert(select >= 0);
				int rep_r = r_tag[r].rep_row[select];
				assert(rep_r >= 0);

				Float v;
				if (m_term) {
					v = column->estimate(rep_r) * column->visibility(rep_r);
					column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

					if (v > 0) {
						v = column->estimate(r);
					}
				}
				else {
					v = column->estimate(rep_r) * column->visibility(rep_r);
				}


				D.at(p + r) = intensity * v;
			}

		}

		D.resize(n_row, sparse_column.size());
		known.resize(n_known);
	}

	void disperseNoise_UV_k(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

		uint32_t n_row = matrix.rows.size();
		uint32_t n_column = matrix.columns.size();

		D.set_size(n_row, n_column);
		known.set_size(n_row * n_column);

		struct Item {
			int rep_row[4];
			Float pdf[4];
		};

		std::vector<KNeighbor> r_neighbor(n_row);

		Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
		uint32_t cc = 0;
		Uvec::size_type n_known = 0;
		int32_t c = -1;
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

			//if (column->max_eval == 0) {
			//	assert(0);
			//	continue;
			//}

			if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

				for (uint32_t r = 0; r < n_row; r++) {

					if (!column->estimate_tag(r)) {
						Float e;
						int16_t v = -1;
						matrix.sampleElement(r, column, e, v, random);
						column->estimate(r) = e;
						column->visibility(r) = v;
					}
				}

				singular_column.push_back(column);

				continue;
			}


			sparse_column.push_back(column);
			c++;
			Fmat::size_type p = c * n_row;
			Float intensity = column->spur_intensity.average();

			for (auto nr : column->known_estimate) {
				int pp = p + nr;
				known.at(n_known++) = pp;
				D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
			}


			bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;



			if (1) { // u
				Column::CROSS_IT lower_bound_it = column->u_known_sample.begin();
				Column::CROSS_IT before_it = lower_bound_it;
				lower_bound_it++;

				for (int cr = 0; cr < n_row; cr++) {


					int r = matrix.u_link[cr].r_id;
					if (column->estimate_tag(r)) continue;

					//if (c == 258 && r == 51) {

					//}
					//else {
					//	r_neighbor[r].distance[0] = 1.0f;
					//	r_neighbor[r].r_id[0] = 0;
					//	continue;
					//}


					r_neighbor[r].init();

					Float u_coord = matrix.rows[r].u_coord;
					while (u_coord > lower_bound_it->first) {
						before_it++;
						lower_bound_it++;
					}

					
					//cout << r << " " << lower_bound_it->first << " " << lower_bound_it->second << endl;		
					getKClosetNeighbor(r_neighbor[r], matrix, lower_bound_it, r, u_coord);
	
				}

			}


			if (1) { // v
				Column::CROSS_IT lower_bound_it = column->v_known_sample.begin();
				Column::CROSS_IT before_it = lower_bound_it;
				lower_bound_it++;

				for (int cr = 0; cr < n_row; cr++) {

					int r = matrix.v_link[cr].r_id;
					if (column->estimate_tag(r)) continue;

					Float v_coord = matrix.rows[r].v_coord;
					while (v_coord > lower_bound_it->first) {
						before_it++;
						lower_bound_it++;
					}
	
					getKClosetNeighbor(r_neighbor[r], matrix, lower_bound_it, r, v_coord);
	
				}
			}

			for (int r = 0; r < n_row; r++) {

				if (column->estimate_tag(r)) continue;

				int rep_r = r_neighbor[r].select(random);

				Float v;
				if (m_term) {
					v = column->estimate(rep_r) * column->visibility(rep_r);
					column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

					if (v > 0) {
						v = column->estimate(r);
					}
				}
				else {
					v = column->estimate(rep_r) * column->visibility(rep_r);
				}


				D.at(p + r) = intensity * v;
			}

		}

		D.resize(n_row, sparse_column.size());
		known.resize(n_known);
	}


#endif

#ifdef CURVE

	//void disperseNoise_CURVE(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

	//	uint32_t n_row = matrix.rows.size();
	//	uint32_t n_column = matrix.columns.size();

	//	D.set_size(n_row, n_column);
	//	known.set_size(n_row * n_column);


	//	std::vector<Float> r_tag;
	//	r_tag.resize(n_row);

	//	Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
	//	uint32_t cc = 0;
	//	Uvec::size_type n_known = 0;
	//	int32_t c = -1;
	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

	//		if (column->max_eval == 0) {
	//			continue;
	//		}

	//		if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

	//			for (uint32_t r = 0; r < n_row; r++) {

	//				if (!column->estimate_tag(r)) {
	//					Float e;
	//					int16_t v = -1;
	//					matrix.sampleElement(r, column, e, v, random);
	//					column->estimate(r) = e;
	//					column->visibility(r) = v;
	//					column->estimate_tag(r) = true;
	//				}
	//			}

	//			singular_column.push_back(column);

	//			continue;
	//		}


	//		sparse_column.push_back(column);
	//		c++;
	//		Fmat::size_type p = c * n_row;
	//		Float intensity = column->spur_intensity.average();


	//		bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;

	//		for (auto nr : column->known_estimate) {
	//			int pp = p + nr;
	//			known.at(n_known++) = pp;
	//			D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
	//		}

	//		//////////////////////////////////////////////////////////////////////////


	//		for (int r = 0; r < n_row; r++) {
	//			//r_tag[r] = column->estimate_tag(r) ? -1 : random->nextFloat() * (CURVE_COUNT << 1);
	//			r_tag[r] = column->estimate_tag(r) ? -1 : random->nextFloat() * CURVE_COUNT;
	//		}
	//		
	//		for (int i = 0; i < CURVE_COUNT; i++) {

	//			Column::CURVE_IT lower_bound_it = column->curve_known_sample[i].begin();
	//			lower_bound_it++;
	//			int before = lower_bound_it->second;
	//			int after = before;

	//			for (int cr = 0; cr < n_row; cr++) {

	//				int r = matrix.curve_link[i][cr].r_id;
	//				//if (r_tag[r] < 0 || (int)r_tag[r] != i) continue;
	//				if (r_tag[r] < 0 || (int)r_tag[r] != i) continue;

	//				int curve_coord = matrix.rows[r].hilbert_order[i];
	//				while (curve_coord > lower_bound_it->first) {
	//					before = after;
	//					lower_bound_it++;
	//					after = lower_bound_it->second;
	//					if (lower_bound_it->first == INT_MAX) after = before;
	//				}

	//				Float d0 = matrix.getRow3DDistance(r, before);
	//				Float d1 = matrix.getRow3DDistance(r, after);
	//				int rep_r = (r_tag[r] - i) < d0 / (d0 + d1) ? after : before;
	//				//int rep_r = (r_tag[r] - i) < 0.5 ? before : after;

	//				Float v;
	//				if (m_term) {
	//					v = column->estimate(rep_r) * column->visibility(rep_r);
	//					column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

	//					if (v > 0) {
	//						v = column->estimate(r);
	//					}
	//				}
	//				else {
	//					v = column->estimate(rep_r) * column->visibility(rep_r);
	//				}

	//				D.at(p + r) = intensity * v;

	//			}

	//		}

	//	}

	//	D.resize(n_row, sparse_column.size());
	//	known.resize(n_known);

	//}

	//void disperseNoise_CURVE_new(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

	//	uint32_t n_row = matrix.rows.size();
	//	uint32_t n_column = matrix.columns.size();

	//	D.set_size(n_row, n_column);
	//	known.set_size(n_row * n_column);


	//	struct Item {
	//		int rep_row[CURVE_COUNT * 2];
	//		Float pdf[CURVE_COUNT * 2];
	//	};

	//	std::vector<Item> r_tag; r_tag.resize(n_row);

	//	Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
	//	uint32_t cc = 0;
	//	Uvec::size_type n_known = 0;
	//	int32_t c = -1;
	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

	//		if (column->max_eval == 0) {
	//			continue;
	//		}

	//		if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

	//			for (uint32_t r = 0; r < n_row; r++) {

	//				if (!column->estimate_tag(r)) {
	//					Float e;
	//					int16_t v = -1;
	//					matrix.sampleElement(r, column, e, v, random);
	//					column->estimate(r) = e;
	//					column->visibility(r) = v;
	//				}
	//			}

	//			singular_column.push_back(column);

	//			continue;
	//		}


	//		sparse_column.push_back(column);
	//		c++;
	//		Fmat::size_type p = c * n_row;
	//		Float intensity = column->spur_intensity.average();



	//		bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;

	//		for (auto nr : column->known_estimate) {
	//			int pp = p + nr;
	//			known.at(n_known++) = pp;
	//			D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
	//		}

	//		//////////////////////////////////////////////////////////////////////////

	//		for (int i = 0; i < CURVE_COUNT; i++) {

	//			Column::CURVE_IT lower_bound_it = column->curve_known_sample[i].begin();
	//			int before = lower_bound_it->second;
	//			lower_bound_it++;
	//			int after = lower_bound_it->second;

	//			for (int cr = 0; cr < n_row; cr++) {

	//				int r = matrix.curve_link[i][cr].r_id;
	//				//if (r_tag[r] < 0 || (int)r_tag[r] != i) continue;
	//				if (column->estimate_tag(r)) continue;

	//				int curve_coord = matrix.rows[r].hilbert_order[i];
	//				while (curve_coord > lower_bound_it->first) {
	//					before = after;
	//					lower_bound_it++;
	//					after = lower_bound_it->second;
	//				}
	//				



	//				int n = i << 1;
	//				if (before >= 0) {
	//					r_tag[r].rep_row[n] = before;
	//					r_tag[r].pdf[n] = 1.0f / matrix.getRow3DDistance(r, before);
	//	
	//				}
	//				else {
	//
	//					r_tag[r].rep_row[n] = -1;
	//					r_tag[r].pdf[n] = 0.0f;
	//				}

	//				n++;;
	//				if (after >= 0) {
	//					r_tag[r].rep_row[n] = after;
	//					r_tag[r].pdf[n] = 1.0f / matrix.getRow3DDistance(r, after);
	//				}
	//				else {
	//					r_tag[r].rep_row[n] = -1;
	//					r_tag[r].pdf[n] = 0.0f;
	//				}				

	//			}		

	//		}

	//		for (int r = 0; r < n_row; r++) {

	//			if (column->estimate_tag(r)) continue;

	//			Float pdf_sum = 0;
	//			for (int i = 0; i < CURVE_COUNT << 1; i++) {
	//				pdf_sum += r_tag[r].pdf[i];
	//			}
	//			Float ran = random->nextFloat() * pdf_sum;

	//			Float s = 0;
	//			int select = -1;
	//			for (int i = 0; i < CURVE_COUNT << 1; i++) {
	//				s += r_tag[r].pdf[i];
	//				if (ran <= s) {
	//					select = i;
	//					break;
	//				}
	//			}
	//			assert(select >= 0);
	//			int rep_r = r_tag[r].rep_row[select];

	//			assert(rep_r >= 0);

	//			Float v;
	//			if (m_term) {
	//				v = column->estimate(rep_r) * column->visibility(rep_r);
	//				column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

	//				if (v > 0) {
	//					v = column->estimate(r);
	//				}
	//			}
	//			else {
	//				v = column->estimate(rep_r) * column->visibility(rep_r);
	//			}


	//			D.at(p + r) = intensity * v;
	//		}

	//	}

	//	D.resize(n_row, sparse_column.size());
	//	known.resize(n_known);
	//	//assert(min(min(D)) >= 0);
	//}

	//void disperseNoise_CURVE_k(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random) const {

	//	uint32_t n_row = matrix.rows.size();
	//	uint32_t n_column = matrix.columns.size();

	//	D.set_size(n_row, n_column);
	//	known.set_size(n_row * n_column);

	//	std::vector<KNeighbor> r_neighbor(n_row);

	//	Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
	//	uint32_t cc = 0;
	//	Uvec::size_type n_known = 0;
	//	int32_t c = -1;
	//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

	//		if (column->max_eval == 0) {
	//			continue;
	//		}

	//		if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

	//			for (uint32_t r = 0; r < n_row; r++) {

	//				if (!column->estimate_tag(r)) {
	//					Float e;
	//					int16_t v = -1;
	//					matrix.sampleElement(r, column, e, v, random);
	//					column->estimate(r) = e;
	//					column->visibility(r) = v;
	//				}
	//			}

	//			singular_column.push_back(column);

	//			continue;
	//		}


	//		sparse_column.push_back(column);
	//		c++;
	//		Fmat::size_type p = c * n_row;
	//		Float intensity = column->spur_intensity.average();



	//		bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;

	//		for (auto nr : column->known_estimate) {
	//			int pp = p + nr;
	//			known.at(n_known++) = pp;
	//			D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
	//		}

	//		//////////////////////////////////////////////////////////////////////////

	//		for (int i = 0; i < CURVE_COUNT; i++) {

	//			Column::CURVE_IT lower_bound_it = column->curve_known_sample[i].begin();
	//			lower_bound_it++;

	//			for (int cr = 0; cr < n_row; cr++) {

	//				int r = matrix.curve_link[i][cr].r_id;
	//				//if (r_tag[r] < 0 || (int)r_tag[r] != i) continue;
	//				if (column->estimate_tag(r)) continue;

	//				if (i == 0) {
	//					r_neighbor[r].init();
	//				}
	//				

	//				int curve_coord = matrix.rows[r].hilbert_order[i];
	//				while (curve_coord > lower_bound_it->first) {
	//					lower_bound_it++;
	//				}

	//				getKClosetNeighbor(r_neighbor[r], matrix, lower_bound_it, r);			
	//			}

	//		}

	//		for (int r = 0; r < n_row; r++) {

	//			if (column->estimate_tag(r)) continue;

	//			
	//			int rep_r = r_neighbor[r].select(random);
	//			assert(rep_r >= 0);

	//			Float v;
	//			if (m_term) {
	//				v = column->estimate(rep_r) * column->visibility(rep_r);
	//				column->estimate(r) = (column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping)).average();

	//				if (v > 0) {
	//					v = column->estimate(r);
	//				}
	//			}
	//			else {
	//				v = column->estimate(rep_r) * column->visibility(rep_r);
	//			}


	//			D.at(p + r) = intensity * v;
	//		}

	//	}

	//	D.resize(n_row, sparse_column.size());
	//	known.resize(n_known);
	//	
	//}
	
	void disperseNoise_predictor(Fmat &D, arma::uvec &known, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, LTMatrix &matrix, ref<Random> &random)
	{

		uint32_t n_row = matrix.rows.size();
		uint32_t n_column = matrix.columns.size();


		std::vector<bool> row_m_term(n_row);
		Vector n_avg(0.0f);
		for (int r = 0; r < n_row; r++) {
			n_avg += matrix.rows[r].sensor->its.shFrame.n;
		}
		n_avg = normalize(n_avg);

		for (int r = 0; r < n_row; r++) {
			Float v = dot(matrix.rows[r].sensor->its.shFrame.n, n_avg);
			row_m_term[r] = v < NORMAL_M_TERM;

			if (r > 0) {
				if (dot(matrix.rows[r].sensor->its.shFrame.n, matrix.rows[r - 1].sensor->its.shFrame.n) < NORMAL_M_TERM) {
					row_m_term[r] = true;
				}
			}
			if (r < n_row -1) {
				if (dot(matrix.rows[r].sensor->its.shFrame.n, matrix.rows[r + 1].sensor->its.shFrame.n) < NORMAL_M_TERM) {
					row_m_term[r] = true;
				}
			}
			
			//if (matrix.rows[r].sensor->getImagePos() == Point2i(103, 604)) {
			//	row_m_term[r] = true;
			//	cout << r << " " << v << endl;
			//}
			//if (matrix.rows[r].sensor->getImagePos() == Point2i(103, 603)) {
			//	row_m_term[r] = true;
			//	cout << r << " " << v << endl;
			//}
			//if (matrix.rows[r].sensor->getImagePos() == Point2i(104, 604)) {
			//	row_m_term[r] = true;
			//	cout << r << " " << v << endl;
			//}
			//if (matrix.rows[r].sensor->getImagePos() == Point2i(104, 603)) {
			//	row_m_term[r] = true;
			//	cout << r << " " << v << endl;
			//}
		}

		//system("pause");

		
		D.set_size(n_row, n_column);
		known.set_size(n_row * n_column);

		std::vector<KNeighbor> r_neighbor(n_row);

		Float singular_threshold = 2 * m_singular_threshold * matrix.sum_avg_eval;
		uint32_t cc = 0;
		Uvec::size_type n_known = 0;
		int32_t c = -1;
		for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, cc++) {

			if (column->max_eval == 0) {
				continue;
			}

			if (column->max_eval > singular_threshold || column->known_estimate.size() >= 0.8 * n_row) {

				for (uint32_t r = 0; r < n_row; r++) {

					if (!column->estimate_tag(r)) {
						Float e;
						int16_t v = -1;
						Spectrum m_rgb;
						matrix.sampleElement(r, column, e, m_rgb, v, random);
						if (e > column->max_est) column->max_est = e;
						if (e < column->min_est) column->min_est = e;
						column->estimate(r) = e;
						column->m_rgb(r) = m_rgb;
						column->visibility(r) = v;
						column->estimate_tag(r) = true;
					}
				}

				singular_column.push_back(column);

				continue;
			}


			sparse_column.push_back(column);
			c++;
			Fmat::size_type p = c * n_row;
			Float intensity = column->spur_intensity.average();


			bool m_term = ((column->max_est - column->min_est) / std::max(1e-6f, column->min_est)) > M_TERM;
			Float mid_m = 0.5 * (column->max_est + column->min_est);
			Spectrum mid_m_rgb = column->m_rgb(*(column->known_estimate.begin()));

			for (auto nr : column->known_estimate) {
				int pp = p + nr;
				known.at(n_known++) = pp;
				D.at(pp) = column->estimate(nr) * column->visibility(nr) * intensity;
			}

			//////////////////////////////////////////////////////////////////////////
			if (column->p_selection == Column::PKNN) {
				for (int i = 0; i < CURVE_COUNT; i++) {

					Column::CURVE_IT lower_bound_it = column->curve_known_sample[i].begin();
					lower_bound_it++;

					for (int cr = 0; cr < n_row; cr++) {

						int r = matrix.curve_link[i][cr].r_id;
						//if (r_tag[r] < 0 || (int)r_tag[r] != i) continue;
						if (column->estimate_tag(r)) continue;

						if (i == 0) {
							r_neighbor[r].init();
						}


						int curve_coord = matrix.rows[r].hilbert_order[i];
						while (curve_coord > lower_bound_it->first) {
							lower_bound_it++;
						}

						getKClosetNeighbor(r_neighbor[r], matrix, lower_bound_it, r);
					}

				}

				for (int r = 0; r < n_row; r++) {

					if (column->estimate_tag(r)) continue;

					int rep_r = r_neighbor[r].select(random);
					assert(rep_r >= 0);

					Float v;
					if (m_term || row_m_term[r]) {
						v = column->estimate(rep_r) * column->visibility(rep_r);
						Spectrum e_rgb = column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping);
						Float e = e_rgb.average();
						column->estimate(r) = e;
						column->m_rgb(r) = e == 0 ? e_rgb : e_rgb / e;

						if (v > 0) {
							v = column->estimate(r);
						}
					}
					else {
						v = column->estimate(rep_r) * column->visibility(rep_r);
						column->estimate(r) = column->estimate(rep_r);
						column->m_rgb(r) = column->m_rgb(rep_r);
					}


					D.at(p + r) = intensity * v;
				}
			}
			else if (column->p_selection == Column::PNBM) {

				for (int r = 0; r < n_row; r++) {

					if (column->estimate_tag(r)) continue;

					if (m_term || row_m_term[r]) {
		
						Spectrum e_rgb = column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping);
						Float e = e_rgb.average();
						column->estimate(r) = e;
						column->m_rgb(r) = e == 0 ? e_rgb : e_rgb / e;

						if (e > column->max_est) column->max_est = e;
						if (e < column->min_est) column->min_est = e;
					}
					else {
						column->estimate(r) = mid_m;
						column->m_rgb(r) = mid_m_rgb;
					}
					
					Float v = column->estimate(r);
					if (random->nextFloat() < matrix.m_nbm_predictor.getPosteriorPDF0(r, c, matrix)) {
						v = 0.0f;
					}

					D.at(p + r) = intensity * v;

				}

			}
			else if(column->p_selection == Column::PLMS) {

				for (int r = 0; r < n_row; r++) {

					if (column->estimate_tag(r)) continue;

					if (m_term || row_m_term[r]) {

						Spectrum e_rgb = column->lightNode->evalMTD((matrix.rows[r].sensor), m_clamping);
						Float e = e_rgb.average();
						column->estimate(r) = e;
						column->m_rgb(r) = e == 0 ? e_rgb : e_rgb / e;

						if (e > column->max_est) column->max_est = e;
						if (e < column->min_est) column->min_est = e;
					}
					else {
						column->estimate(r) = mid_m;
						column->m_rgb(r) = mid_m_rgb;
					}


					Float v = column->estimate(r);
					if (random->nextFloat() < matrix.m_lms_predictor.P0(r, c, matrix)) {
						v = 0.0f;
					}

					D.at(p + r) = intensity * v;

				}
			}
			else {
				assert(0);
			}

		}

		D.resize(n_row, sparse_column.size());
		known.resize(n_known);
	}


#endif

	void fillResult_spp(LTMatrix &matrix, Fmat &recoveredResult, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, SliceWorkResult *result, const Scene *scenem, ref<Random> &random) const
	{
		uint32_t sc = 0;
		int n_row = recoveredResult.n_rows;
		int n_col = recoveredResult.n_cols;

		//for (uint32_t c = 0; c < sparse_column.size(); c++) {
		//	list<Column>::iterator &column = sparse_column[c];
		//	Float avg_intensity = column->spur_intensity.average();

		//	for (int r = 0; r < n_row; r++) {
		//		if (recoveredResult.at(r, c) > 0.5 * column->estimate(r) * avg_intensity) {
		//			recoveredResult.at(r, c) = column->estimate(r) * avg_intensity;
		//		}
		//		else {
		//			recoveredResult.at(r, c) = 0;
		//		}
		//	}

		//}


		// sparse
		for (uint32_t c = 0; c < sparse_column.size(); c++) {
		
			list<Column>::iterator &column = sparse_column[c];
			Float avg_intensity = column->spur_intensity.average();
			Spectrum rgb = column->spur_intensity / avg_intensity;
			

			if (column->max_est - column->min_est < spp_M_TERM * std::max(1e-6f, column->min_est)) { // use nearby

				for (uint32_t r = 0; r < n_row; r++) {

					Float &rEst = recoveredResult(r, c);
					if (rEst > 0) {
						
						Spectrum v = rEst * rgb * column->m_rgb(r);

						for (auto &spp : matrix.rows[r].spp_sensors) {
	
							spp.MVTDI += v;
						}
					}
				}

			}
			else { // accurate
				
				for (uint32_t r = 0; r < n_row; r++) {
					
					Float &rEst = recoveredResult(r, c);
					if (rEst > 0 && column->estimate(r) > 0) {
						Float scale = rEst / (column->avg_intensity * column->estimate(r));
						scale = std::min(1.0f, scale);

						for (auto &spp : matrix.rows[r].spp_sensors) {


							if (spp.sensor == matrix.rows[r].sensor && column->estimate_tag(r)) {
								spp.MVTDI += scale * column->estimate(r) * column->m_rgb(r) * column->spur_intensity;
							}
							else {
								spp.MVTDI += scale * column->lightNode->evalMTD(spp.sensor, m_clamping) * column->spur_intensity;
							}
						}
					}

				}
			}
		
		}


		// singular
		for (uint32_t c = 0; c < singular_column.size(); c++) {

			list<Column>::iterator &column = singular_column[c];
			Spectrum rgb = column->spur_intensity;

			if (column->max_est - column->min_est < spp_M_TERM * column->min_est) {

				for (uint32_t r = 0; r < n_row; r++) {

					Float rEst = column->estimate(r) * column->visibility(r);
					if (rEst > 0) {
						Spectrum v = rEst * rgb * column->m_rgb(r);
						for (auto &spp : matrix.rows[r].spp_sensors) {
							spp.MVTDI += v;
						}
					}
				}
			}
			else {

				for (uint32_t r = 0; r < n_row; r++) {

					if (column->visibility(r) > 0) {
						for (auto &spp : matrix.rows[r].spp_sensors) {

							if (spp.sensor == matrix.rows[r].sensor && column->estimate_tag(r)) {
								spp.MVTDI += column->estimate(r) * column->m_rgb(r) * rgb;
							}
							else {
								spp.MVTDI += (column->lightNode->evalMTD(spp.sensor, m_clamping)) * column->spur_intensity;
							}
						}
					}

				}
			}
		}

		//////////////////////////////////////////////////////////////////////////
		//ref<Random> random = new Random(matrix.m_slice_root->sampleBbox.min.x + matrix.m_slice_root->sampleBbox.min.y);
		//Spectrum rs;
		//rs[0] = random->nextFloat(); rs[1] = random->nextFloat(); rs[2] = random->nextFloat();

		//Fvec row_smple(n_row, arma::fill::zeros);
		//for (uint32_t c = 0; c < sparse_column.size(); c++) {
		//	list<Column>::iterator column = sparse_column[c];
		//	for (uint32_t r = 0; r < n_row; r++) {
		//		row_smple(r) += column->estimate_tag(r);
		//	}
		//}

		//for (uint32_t c = 0; c < singular_column.size(); c++) {
		//	list<Column>::iterator column = singular_column[c];
		//	for (uint32_t r = 0; r < n_row; r++) {
		//		row_smple(r) += column->estimate_tag(r);
		//	}
		//}

		Float a[3];
		a[0] = random->nextFloat();
		a[1] = random->nextFloat();
		a[2] = random->nextFloat();
		Spectrum ranColor(a);

		//int cc = 0;
		//int count = 0;
		//for (int i = 0; i < n_col; i++) {
		//	if (sparse_column[i]->p_selection == Column::PLMS) {
		//		cc = i;
		//		count++;
		//		if (count == 20)
		//			break;
		//	}
		//}

		//cout << cc << endl;

		//////////////////////////////////////////////////////////////////////////

		//std::vector < std::pair < const VSensorNode*, int > > sliceNodes;
		//m_sensor_map->calAdjSlices(matrix.m_slice_root, sliceNodes, m_sliceSize);

		Spectrum off_spur = 1.0f / matrix.li_spur;
		for (uint32_t r = 0; r < n_row; r++) {

			for (auto &spp : matrix.rows[r].spp_sensors) {

				//cout << (spp.MVTDI).toString() << endl;
				result->put(spp.sensor, spp.sensor->strength * off_spur * spp.MVTDI);			
			}

			//Spectrum rs;

			//rs[0] = rs[1] = rs[2] = 0.0f; rs[1] = 1.0f; rs[0] = 1.0f - matrix.m_lms_predictor.P0(r, cc, matrix);
			//result->put(matrix.rows[r].sensor, rs.abs());

			//result->put(matrix.rows[r].sensor, ranColor);

			//rs[0] = std::abs(matrix.rows[r].u_coord);  rs[1] = std::abs(matrix.rows[r].v_coord); rs[2] = sliceNodes.size();
			//result->put(matrix.rows[r].sensor, rs.abs());

			//rs[0] = row_smple[r]; rs[1] = matrix.rows[r].shadow_pdf; rs[2] = matrix.sum_avg_eval;
			//result->put(matrix.rows[r].sensor, rs.abs());
		}

	}



	void fillResult(LTMatrix &matrix, Fmat &recoveredResult, vector<list<Column>::iterator> &sparse_column, vector<list<Column>::iterator> &singular_column, SliceWorkResult *result, const Scene *scenem, ref<Random> &random) const
	{
		uint32_t sc = 0;
		int n_row = recoveredResult.n_rows;
		int n_col = recoveredResult.n_cols;

		std::vector<Spectrum> sparse_result(n_row, Spectrum(0.0f));	
		for (uint32_t c = 0; c < sparse_column.size(); c++) {

			list<Column>::iterator &column = sparse_column[c];
			Spectrum rgb = column->spur_intensity / column->spur_intensity.average();
			Float intensity = rgb.average();

			for (uint32_t r = 0; r < n_row; r++) {

				Float &rEst = recoveredResult(r, c);
				//Float rEst = column->estimate(r);
				if (rEst > 0) {
					//assert(column->estimate(r) >= 0);
					//sparse_result[r] += column->lightNode->evalMTD(matrix.rows[r].sensor, m_clamping);
					sparse_result[r] += rEst * rgb;
				}
			}
		}



		std::vector<Spectrum> singular_result(n_row, Spectrum(0.0f));
		for (uint32_t c = 0; c < singular_column.size(); c++) {

			list<Column>::iterator &column = singular_column[c];
			Spectrum rgb = column->spur_intensity;

			for (uint32_t r = 0; r < n_row; r++) {

				Float rEst = column->estimate(r) * column->visibility(r);

				if (rEst > 0) {
					singular_result[r] += rEst * rgb;
				}
			}
		}




	
		
		//////////////////////////////////////////////////////////////////////////
		//ref<Random> random = new Random(matrix.m_slice_root->sampleBbox.min.x + matrix.m_slice_root->sampleBbox.min.y);
		//Spectrum rs;
		//rs[0] = random->nextFloat(); rs[1] = random->nextFloat(); rs[2] = random->nextFloat();

		//Fvec row_smple(n_row, arma::fill::zeros);
		//for (uint32_t c = 0; c < sparse_column.size(); c++) {
		//	list<Column>::iterator column = sparse_column[c];
		//	for (uint32_t r = 0; r < n_row; r++) {
		//		row_smple(r) += column->estimate_tag(r);
		//	}
		//}

		//for (uint32_t c = 0; c < singular_column.size(); c++) {
		//	list<Column>::iterator column = singular_column[c];
		//	for (uint32_t r = 0; r < n_row; r++) {
		//		row_smple(r) += column->estimate_tag(r);
		//	}
		//}

		Float a[3];
		a[0] = random->nextFloat();
		a[1] = random->nextFloat();
		a[2] = random->nextFloat();
		Spectrum ranColor(a);

		//int cc = 0;
		//int count = 0;
		//for (int i = 0; i < n_col; i++) {
		//	if (sparse_column[i]->p_selection == Column::PLMS) {
		//		cc = i;
		//		count++;
		//		if (count == 20)
		//			break;
		//	}
		//}

		//cout << cc << endl;

		//////////////////////////////////////////////////////////////////////////
		
		//std::vector < std::pair < const VSensorNode*, int > > sliceNodes;
		//m_sensor_map->calAdjSlices(matrix.m_slice_root, sliceNodes, m_sliceSize);

		Spectrum off_spur = 1.0f / matrix.li_spur;
		for (uint32_t r = 0; r < n_row; r++) {
			//if ( r <= 239) continue;
			
			result->put(matrix.rows[r].sensor, matrix.rows[r].sensor->strength * off_spur * (singular_result[r] + sparse_result[r]));
			continue;


			Spectrum rs;

			//rs[0] = rs[1] = rs[2] = 0.0f; rs[1] = 1.0f; rs[0] = 1.0f - matrix.m_lms_predictor.P0(r, cc, matrix);
			//result->put(matrix.rows[r].sensor, rs.abs());
			
			//result->put(matrix.rows[r].sensor, ranColor);

			//rs[0] = std::abs(matrix.rows[r].u_coord);  rs[1] = std::abs(matrix.rows[r].v_coord); rs[2] = sliceNodes.size();
			//result->put(matrix.rows[r].sensor, rs.abs());

			//rs[0] = row_smple[r]; rs[1] = matrix.rows[r].shadow_pdf; rs[2] = matrix.sum_avg_eval;
			//result->put(matrix.rows[r].sensor, rs.abs());
		}

	}

	//void denoise(LTMatrix &matrix, Fmat &L, Fmat &S, vector<list<Column>::iterator> &sparse_column, uint32_t &shadow_count, ref<Random> &random) const {

	//	uint32_t sc = 0;
	//	S = arma::abs(S);

	//	uint32_t n_row = L.n_rows;
	//	uint32_t n_col = L.n_cols;
	//	
	//	
	//	arma::Row<Float> S_col_maximu = arma::max(S);

	//	struct NoiseCol {

	//		Float max_noise;
	//		list<Column>::iterator column;
	//		uint32_t c_id;
	//		
	//		bool operator<(NoiseCol &b) {
	//			return max_noise > b.max_noise;
	//		}
	//	};

	//	vector<NoiseCol> noise_column(n_col);	
	//	for (uint32_t i = 0; i < n_col; i++) {
	//		noise_column[i].max_noise = S_col_maximu.at(i);
	//		noise_column[i].column = sparse_column[i];
	//		noise_column[i].c_id = i;
	//	}

	//	std::sort(noise_column.begin(), noise_column.end());

	//	Fvec row_eval = arma::sum(L, 1);

	//	Float min_row_eval = arma::min(row_eval);
	//	Uvec index, p_index; //Fvec s_col;
	//	uint32_t fix_column = 0;
	//	for (uint32_t i = 0; i < n_col; i++) {

	//		if (noise_column[i].max_noise < m_denoise_threshold * min_row_eval) break;

	//		list<Column>::iterator column = noise_column[i].column;
	//	 
	//		index = arma::find(S.col(noise_column[i].c_id) / row_eval >= m_denoise_threshold);
	//		//index = arma::find(S.col(noise_column[i].col_id)  >= 0.001);
	//		Fvec vv = S.col(noise_column[i].c_id) / row_eval;
	//		//cout << vv(index) << endl;
	//		//index.set_size(n_row);
	//		//for (int i = 0; i < n_row; i++) {
	//		//	index(i) = i;
	//		//}
	//		//////////////////////////////////////////////////////////////////////////

	//		if (index.n_elem == 0) continue;
	//		fix_column++;

	//		Float intensity = column->spur_intensity.average();

	//		uint32_t c_head = noise_column[i].c_id * n_row;
	//	//	s_col = S(index + c_head);

	//		list<uint32_t>::iterator b_itr = column->known_visibility.begin();
	//		list<uint32_t>::iterator e_itr = column->known_visibility.end();
	//		list<uint32_t>::iterator itr = b_itr;
	//		while (itr != e_itr && *itr < index.at(0)) {
	//			itr++;
	//		}
	//		//list<uint32_t>::iterator itr = column->known_visibility.upper_bound(index.at(0)); // equal or greater

	//		uint32_t upper_bound = 0;	
	//		uint32_t r_pass_bark = 0;
	//		
	//		for (uint32_t j = 0; j < index.n_elem; j++) {
	//		
	//			uint32_t c_r = index.at(j);
	//			if (c_r < r_pass_bark) {
	//				continue;
	//			}
	//			else {
	//				//if (itr != e_itr && *itr < c_r)
	//				//	itr++;
	//				//if (itr != e_itr && *itr < c_r)
	//				//	itr = column->known_visibility.upper_bound(c_r); // equal or greater
	//				while (itr != e_itr && *itr < c_r) {
	//					itr++;
	//				}
	//			}

	//			r_pass_bark = itr == e_itr ? n_row : *itr + 1;

	//			// search upper bound
	//			list<uint32_t>::iterator ut = itr;
	//			uint32_t k = NEIGHBOR * 100;
	//			for (; ut != b_itr && k; ut--, k--) { }
	//			if (k == 0) {
	//				upper_bound = std::max(*ut, upper_bound);
	//			}

	//			uint32_t lower_bound = n_row - 1;
	//			list<uint32_t>::iterator lt = itr;
	//			k = NEIGHBOR * 2 - 1 + (*itr == c_r);	
	//			for (; lt != e_itr && k; lt++, k--) { }
	//			if (k == 0) {
	//				lower_bound = std::min(*lt, lower_bound);
	//			}
	//			
	//			// for update
	//			for (uint32_t r = upper_bound; r <= lower_bound; r++) {
	//				uint32_t cc = r + c_head;
	//				if (column->estimate_tag(r)) {
	//					L.at(cc) = intensity * column->estimate(r) * column->visibility(r);
	//					continue;
	//				}

	//				Float e;
	//				int16_t v = -1;
	//				matrix.sampleElement(r, column, e, v, random);
	//				if (v >= 0) {
	//					sc++;
	//					e *= v;
	//				}

	//				
	//				row_eval.at(r) -= L.at(cc);
	//				L.at(cc) = e * intensity;
	//				row_eval.at(r) += L.at(cc);
	//			}

	//			upper_bound = lower_bound + 1;
	//			if (upper_bound >= n_row) break;
	//		}
	//		
	//	}
	//	//cout << "fix column " << fix_column << endl;
	//	shadow_count += sc;

	//}

	//void denoise_2(LTMatrix &matrix, Fmat &L, Fmat &S, vector<list<Column>::iterator> &sparse_column, uint32_t &shadow_count, ref<Random> &random) const {
	//	//return;
	//	uint32_t sc = 0;
	//	S = arma::abs(S);

	//	uint32_t n_row = L.n_rows;
	//	uint32_t n_col = L.n_cols;

	//	arma::Row<Float> S_col_maximu = arma::max(S);	

	//
	//	Fvec vv;
	//	Uvec index, p_index; //Fvec s_col;
	//	uint32_t fix_column = 0;
	//	for (uint32_t i = 0; i < n_col; i++) {

	//		list<Column>::iterator column = sparse_column[i];

	//		Float C = 0.5f * column->max_eval;
	//		vv = abs(S.col(i));

	//		index = arma::find(vv > C);
	//		if (index.n_elem == 0) continue;

	//		Float intensity = column->avg_intensity;
	//		
	//		uint32_t c_head = i * n_row;
	//		
	//		list<uint32_t>::iterator b_itr = column->known_estimate.begin();
	//		list<uint32_t>::iterator e_itr = column->known_estimate.end();
	//		list<uint32_t>::iterator itr = b_itr;
	//		while (itr != e_itr && *itr < index.at(0)) {
	//			itr++;
	//		}
	//		

	//		uint32_t upper_bound = 0;
	//		uint32_t r_pass_bark = 0;

	//		for (uint32_t j = 0; j < index.n_elem; j++) {

	//			uint32_t c_r = index.at(j);
	//			if (c_r < r_pass_bark) {
	//				continue;
	//			}
	//			else {
	//				//if (itr != e_itr && *itr < c_r)
	//				//	itr++;
	//				//if (itr != e_itr && *itr < c_r)
	//				//	itr = column->known_visibility.upper_bound(c_r); // equal or greater
	//				while (itr != e_itr && *itr < c_r) {
	//					itr++;
	//				}
	//			}

	//			r_pass_bark = itr == e_itr ? n_row : *itr + 1;

	//			// search upper bound
	//			list<uint32_t>::iterator ut = itr;
	//			uint32_t k = NEIGHBOR;
	//			for (; ut != b_itr && k; ut--, k--) {}
	//			if (k == 0) {
	//				upper_bound = std::max(*ut, upper_bound);
	//			}

	//			uint32_t lower_bound = n_row - 1;
	//			list<uint32_t>::iterator lt = itr;
	//			k = NEIGHBOR - 1 + (*itr == c_r);
	//			for (; lt != e_itr && k; lt++, k--) {}
	//			if (k == 0) {
	//				lower_bound = std::min(*lt, lower_bound);
	//			}

	//			// for update
	//			for (uint32_t r = upper_bound; r <= lower_bound; r++) {
	//				uint32_t cc = r + c_head;
	//				if (column->estimate_tag(r)) {

	//					L.at(cc) = intensity * column->estimate(r) * column->visibility(r);
	//					continue;
	//				}

	//				Float e;
	//				int16_t v = -1;
	//				matrix.sampleElement(r, column, e, v, random);
	//				if (v >= 0) {
	//					sc++;
	//					e *= v;
	//				}
	//
	//				L.at(cc) =  e * intensity;
	//	
	//			}

	//			upper_bound = lower_bound + 1;
	//			if (upper_bound >= n_row) break;
	//		}

	//	}
	//	//cout << "fix column " << fix_column << endl;
	//	shadow_count += sc;

	//}

	//void denoise_green(LTMatrix &matrix, Fmat &L, Fmat &S, Fmat &D, vector<list<Column>::iterator> &sparse_column, uint32_t &shadow_count, ref<Random> &random, Fmat &green) const {
	//	//return;
	//	uint32_t sc = 0;

	//	uint32_t n_row = L.n_rows;
	//	uint32_t n_col = L.n_cols;

	//	Fvec row_sum = arma::sum(D, 1);
	//	arma::Row<Float> column_sum = arma::sum(D, 0);

	//	Fvec r_row_sum = arma::sum(L, 1);
	//	arma::Row<Float> r_column_sum = arma::sum(L, 0);
	//
	//	vector<bool> valid_column(n_col, false);
	//	vector<bool> valid_row(n_row, false);

	//	struct BGCount {
	//		uint32_t blue_point = 0;
	//		uint32_t green_point = 0;
	//		BGCount() = default;
	//	};

	//	vector<BGCount> column_count(n_col);
	//	vector<BGCount> row_count(n_row);

	//	uint32_t p = 0;
	//	for (uint32_t c = 0; c < n_col; c++) {
	//		for (uint32_t r = 0; r < n_row; r++) {
	//			Float m = green.at(p);
	//			if (m > 0) {
	//				column_count[c].green_point++;
	//				row_count[r].green_point++;
	//			}
	//			else if (m < 0) {
	//				column_count[c].blue_point++;
	//				row_count[r].blue_point++;
	//			}
	//			p++;
	//		}
	//	}



	//	Uvec find;
	//	find = arma::find(arma::abs(r_row_sum - row_sum) / row_sum > 0.1);
	//	for (auto f : find) {
	//		//cout << "row " << f << " " << r_row_sum(f) << " " << row_sum(f) << " " << row_count[f].green_point << " " << row_count[f].blue_point << " " << endl;
	//		valid_row[f] = true;
	//	}
	//
	//	find = arma::find(arma::abs(r_column_sum - column_sum) / column_sum > 0.1);
	//	for (auto f : find) {
	//		//cout << "column " << f << endl;
	//		valid_column[f] = true;
	//	}



	//	uint32_t fix_column = 0;
	//	for (uint32_t c = 0; c < n_col; c++) {

	//		list<Column>::iterator &column = sparse_column[c];
	//		uint32_t c_head = c * n_row;

	//		list<uint32_t>::iterator b_itr = column->known_estimate.begin();
	//		list<uint32_t>::iterator e_itr = column->known_estimate.end();
	//		list<uint32_t>::iterator itr = b_itr;
	//		uint32_t for_now = 0;

	//		for (uint32_t r = 0; r < n_row; r++) {

	//			if (green.at(r, c) == 0.0f) {
	//				continue;
	//			}

	//			if (!(valid_column[c] && valid_row[r])) {
	//				continue;
	//			}

	//			while (itr != e_itr && *itr < r) {
	//				itr++;
	//			}

	//			uint32_t upper_bound = 0;
	//			list<uint32_t>::iterator ti = itr;
	//			if (ti != b_itr) {
	//				ti--;
	//			}
	//			if (*ti < r) {
	//				upper_bound = *ti + 1;
	//			}

	//			uint32_t lower_bound = n_row - 1;
	//			if (itr != e_itr && *itr == r) {
	//				itr++;
	//			}
	//			if (itr != e_itr) {
	//				lower_bound = *itr - 1;
	//			}

	//			upper_bound = std::max(for_now, upper_bound);
	//			for_now = std::max(for_now, lower_bound + 1);

	//			// for update
	//			for (uint32_t rr = upper_bound; rr <= lower_bound; rr++) {
	//				uint32_t cc = rr + c_head;
	//
	//			
	//				if (column->estimate_tag(rr)) {
	//					L.at(cc) = column->avg_intensity * column->estimate(rr) * column->visibility(rr);
	//				}
	//				else {
	//					Float e;
	//					int16_t v = -1;
	//					matrix.sampleElement(rr, column, e, v, random);
	//					if (v >= 0) {
	//						sc++;
	//						e *= v;
	//					}
	//					L.at(cc) = e * column->avg_intensity;
	//				}
	//			}

	//		}
	//	}
	//	//cout << "fix column " << fix_column << endl;
	//	shadow_count += sc;

	//}

	void boxFilter(Fmat &T, Fmat &S, int n) const {

		int n_row = T.n_rows;
		int n_col = T.n_cols;
		
		Fmat scan_columns = S;
		for (int c = 0; c < n_col; c++) {
			Float *ptr = scan_columns.colptr(c);
			for (int r = 1; r < n_row; r++) {
				ptr[r] += ptr[r - 1];
			}
		}

		n = std::min(std::min(n_row, n_col) - 1, n);

		int b_ptr = 0; int e_ptr = n + 1; 
		int acc_col = n + 1;
		Fvec sum_col = arma::sum(scan_columns.cols(0, e_ptr), 1);

		for (int c = 0; c < n_col; c++) {

			if (sum_col.max() > 0.0f) {
				
				for (int r = 0; r < n_row; r++) {

					int c_ptr = r - n;
					int d_ptr = std::min(r + n, n_row - 1);

					if (c_ptr >= 0) {
						Float v = sum_col.at(d_ptr) - sum_col.at(c_ptr);
	
						int acc_row = d_ptr - c_ptr + 1;
						T.at(r, c) = v / (acc_row * acc_col);
					}
					else
					{
						Float v = sum_col.at(d_ptr);
						assert(v >= 0);
						int acc_row = d_ptr + 1;
						T.at(r, c) = v / (acc_row * acc_col);
					}
				}
			}

			if (e_ptr < n_col - 1) {
				e_ptr++;
				sum_col += scan_columns.col(e_ptr);
				acc_col++;
			}

			if (b_ptr <= c - n) {
				sum_col -= scan_columns.col(b_ptr);
				b_ptr++;
				acc_col--;
			}

		}

	}

	void denoise_filter(LTMatrix &matrix, Fmat &L, Fmat &S, Fmat &D, vector<list<Column>::iterator> &sparse_column, uint32_t &shadow_count, ref<Random> &random, Fmat &green) const {
		//return;
		uint32_t sc = 0;

		uint32_t n_row = L.n_rows;
		uint32_t n_col = L.n_cols;

		Fmat noise_map(n_row, n_col);
		boxFilter(noise_map, green, SM_BOARD);

		//Uvec f;
		//for (uint32_t c = 0; c < n_col; c++) {

		//	f = arma::find(noise_map.col(c) > NOISE_THRESHOLD);
		//	
		//	if (f.n_elem == 0) continue;

		//	list<Column>::iterator &column = sparse_column[c];
		//	for (auto r : f) {

		//		if (column->estimate_tag(r)) {
		//			L.at(r, c) = column->avg_intensity * column->estimate(r) * column->visibility(r);
		//		}
		//		else {
		//			Float e;
		//			int16_t v = -1;
		//			matrix.sampleElement(r, column, e, v, random);
		//			if (v >= 0) {
		//				sc++;
		//				e *= v;
		//			}
		//			L.at(r, c) = e * column->avg_intensity;
		//		}

		//	}
		//}

		uint32_t fix_column = 0;
		for (uint32_t c = 0; c < n_col; c++) {

			list<Column>::iterator &column = sparse_column[c];
			uint32_t c_head = c * n_row;

			list<uint32_t>::iterator b_itr = column->known_estimate.begin();
			list<uint32_t>::iterator e_itr = column->known_estimate.end();
			list<uint32_t>::iterator itr = b_itr;
			uint32_t for_now = 0;

			for (uint32_t r = 0; r < n_row; r++) {

				if (green.at(r, c) == 0.0f || noise_map.at(r, c) < NOISE_THRESHOLD) {
					continue;
				}


				while (itr != e_itr && *itr < r) {
					itr++;
				}

				uint32_t upper_bound = 0;
				list<uint32_t>::iterator ti = itr;
				if (ti != b_itr) {
					ti--;
				}
				if (*ti < r) {
					upper_bound = *ti + 1;
				}

				uint32_t lower_bound = n_row - 1;
				if (itr != e_itr && *itr == r) {
					itr++;
				}
				if (itr != e_itr) {
					lower_bound = *itr - 1;
				}

				upper_bound = std::max(for_now, upper_bound);
				for_now = std::max(for_now, lower_bound + 1);

				// for update
				for (uint32_t rr = upper_bound; rr <= lower_bound; rr++) {
					uint32_t cc = rr + c_head;
					if (noise_map.at(rr, c) < NOISE_THRESHOLD) {
						continue;
					}

					if (column->estimate_tag(rr)) {
						L.at(cc) = column->avg_intensity * column->estimate(rr) * column->visibility(rr);
					}
					else {
						Float e;
						int16_t v = -1;
						Spectrum m_rgb;
						matrix.sampleElement(rr, column, e, m_rgb, v, random);
						column->m_rgb(r) = m_rgb;
						column->estimate(r) = e;
						column->visibility(r) = v;
						if (v >= 0) {
							sc++;
							e *= v;
						}
						L.at(cc) = e * column->avg_intensity;
					}
				}

			}
		}
		//cout << "fix column " << fix_column << endl;
		shadow_count += sc;

	}


	void blurNoise(LTMatrix &matrix, Fmat &L, Fmat &S, vector<list<Column>::iterator> &valid_column, uint32_t &shadow_count) const {
		//return;
		uint32_t m = S.n_rows;
		uint32_t n = S.n_cols;
		int filter = 4;
		for (uint32_t c = 0; c < n; c++) {

			//Float v = arma::mean(S.col(c));
			
			//L.col(c) += v;
			Float intensity = valid_column[c]->spur_intensity.average();
			Fvec col = S.col(c);

			for (int r = 0; r < m; r++)
			{
				int b = std::max(0, r - filter);
				int e = std::min((int)m - 1, r + filter);

				L(r, c) += intensity * matrix.mergeNeighborInRowSequence(r, valid_column[c]);
				continue;

				Float w_sum = 0; Float v_sum = 0; Float max_w = 0;
				for (int i = b; i < e; i++) {

					if (r == i) continue;

					Float d = matrix.getRowDistance(r, i);
					Float w = 1.0f / d;
					max_w = std::max(max_w, w);
					v_sum += col(i) * w;
					w_sum += w;
				}
				v_sum += max_w * 2 * col(r);
				w_sum += 2 * max_w;
				L(r, c) += v_sum / w_sum;

				Float v = arma::mean(col.rows(std::max(0, r - filter), std::min((int)m - 1, r + filter)));
				L(r, c) += v;


				

			}

		}


	}

#ifdef UV
	void searchFourNeighbor(int neighbor[4], LTMatrix &matrix, Column::CROSS_IT &u_it, Column::CROSS_IT &v_it, int r_center) const {
		
		auto seachU = [&](Float u_center, Column::CROSS_IT &c_it, int off) {

			{   // u +
				
				Column::CROSS_IT u_pos = c_it;
				Float min_dist = FLT_MAX;
				while (u_pos->first != FLT_MAX && (u_pos->first - u_center) < min_dist) {
		
					Float distance = matrix.getRow3DDistance(r_center, u_pos->second);
					if (distance < min_dist) {
						min_dist = distance;
						neighbor[off] = u_pos->second;
					}
					u_pos++;
				}
			}

			{    // u -
				Column::CROSS_IT u_pos = c_it;
				u_pos--;
				Float min_dist = FLT_MAX;
				while (u_pos->first != -FLT_MAX && (u_center - u_pos->first) < min_dist) {
					Float distance = matrix.getRow3DDistance(r_center, u_pos->second);
					if (distance < min_dist) {
						min_dist = distance;
						neighbor[off + 1] = u_pos->second;
					}
					u_pos--;
				}
			}

		};

		
		seachU(matrix.rows[r_center].u_coord, u_it, 0);
		seachU(matrix.rows[r_center].v_coord, v_it, 2);

	}

	
	int getClosetNeighbor(LTMatrix &matrix, Column::CROSS_IT it, int r_center, Float u_center, bool pos) const {

		auto searchPos = [&](Column::CROSS_IT &u_it) {
			 // u +
			Float min_dist = FLT_MAX;
			int neighbor = -1;
			while (u_it->first != FLT_MAX && (u_it->first - u_center) < min_dist) {

				Float distance = matrix.getRow3DDistance(r_center, u_it->second);
				if (distance < min_dist) {
					min_dist = distance;
					neighbor = u_it->second;
				}
				u_it++;
			}

			return neighbor;
		};

		auto searchNeg = [&](Column::CROSS_IT &u_it) {
			// u -
			Float min_dist = FLT_MAX;
			int neighbor = -1;
			while (u_it->first != -FLT_MAX && (u_center - u_it->first) < min_dist) {

				Float distance = matrix.getRow3DDistance(r_center, u_it->second);
				if (distance < min_dist) {
					min_dist = distance;
					neighbor = u_it->second;
				}
				u_it--;
			}

			return neighbor;
		};

		if (pos) {
			return it->first != FLT_MAX ? searchPos(it) : searchNeg(--it);
		}
		else {
			return it->first != -FLT_MAX ? searchNeg(it) : searchPos(++it);
		}
		
	}

#endif

	struct KNeighbor {

		int r_id[K_NEIGHBOR];
		Float distance[K_NEIGHBOR];

		void init() {
			for (int i = 0; i < K_NEIGHBOR; i++) {
				r_id[i] = -1; distance[i] = FLT_MAX;
			}
		}


		void push(Float d, int r) {

			int ptr = 0;
			while (d > distance[ptr] && ptr < K_NEIGHBOR - 1) {
				ptr++;
			}

			if (d <= distance[ptr] && r_id[ptr] != r) {

				for (int i = K_NEIGHBOR - 2; i >= ptr; i--) {
					r_id[i + 1] = r_id[i];
					distance[i + 1] = distance[i];
				}

				r_id[ptr] = r;
				distance[ptr] = d;
			}


		}

		Float getHeadDistance() {
			return distance[K_NEIGHBOR - 1];
		}

		int select(ref<Random> &random) {

			if (1) {
				Float pdf[K_NEIGHBOR];
				Float pdf_sum = 0;
				for (int i = 0; i < K_NEIGHBOR; i++) {
					pdf[i] = r_id[i] >= 0 ? 1.0f / distance[i] : 0;
					pdf_sum += pdf[i];
				}

				Float ran = random->nextFloat() * pdf_sum;

				Float s = 0;
				int select = -1;
				for (int i = 0; i < K_NEIGHBOR; i++) {
					s += pdf[i];
					if (ran <= s) {
						select = i;
						break;
					}
				}
				assert(select >= 0);
				int rep_r = r_id[select];
				assert(rep_r >= 0);

				return rep_r;

			}
			else {
				int n = K_NEIGHBOR - 1;
				while (n >= 0 && r_id[n] < 0) {
					n--;
				}

				assert(n >= 0);
				int rep_r = r_id[(int)(random->nextFloat() * (n + 1))];
				assert(rep_r >= 0);
				return rep_r;
			}

		}
	};


	void getKClosetNeighbor(KNeighbor &k_neighbor, LTMatrix &matrix, Column::CROSS_IT it, int r_center, Float u_center) const {

		auto searchPos = [&](Column::CROSS_IT &u_it) {
			// u +
			Float min_dist = k_neighbor.getHeadDistance();

			while (u_it->first != FLT_MAX && (u_it->first - u_center) < min_dist) {

				Float i_dist = matrix.getRow3DDistance(r_center, u_it->second);
				if (i_dist < min_dist) {
					k_neighbor.push(i_dist, u_it->second);
					min_dist = k_neighbor.getHeadDistance();
				}
				u_it++;
			}
		};


		auto searchNeg = [&](Column::CROSS_IT &u_it) {
			// u -
			Float min_dist = k_neighbor.getHeadDistance();
			while (u_it->first != -FLT_MAX && (u_center - u_it->first) < min_dist) {

				Float i_dist = matrix.getRow3DDistance(r_center, u_it->second);
				if (i_dist < min_dist) {
					k_neighbor.push(i_dist, u_it->second);
					min_dist = k_neighbor.getHeadDistance();
				}
				u_it--;
			}
		};


		if (it->first != FLT_MAX) {
			searchPos(it);
		}

		it--;
		if (it->first != -FLT_MAX) {
			searchNeg(it);
		}

	}


	void getKClosetNeighbor(KNeighbor &k_neighbor, LTMatrix &matrix, Column::CURVE_IT it, int r_center) const {

		auto searchPos = [&](Column::CURVE_IT &u_it) {
			// u +
			Float min_dist = k_neighbor.getHeadDistance();
			int count = 0;
			while (u_it->first != INT_MAX && count++ < K_NEIGHBOR) {
				Float i_dist = matrix.getRow3DDistance(r_center, u_it->second);
				if (i_dist < min_dist) {
					k_neighbor.push(i_dist, u_it->second);
				}
				u_it++;
			}
		};


		auto searchNeg = [&](Column::CURVE_IT &u_it) {
			// u -
			Float min_dist = k_neighbor.getHeadDistance();
			int count = 0;
			while (u_it->first != INT_MIN && count++ < K_NEIGHBOR) {

				Float i_dist = matrix.getRow3DDistance(r_center, u_it->second);
				if (i_dist < min_dist) {
					k_neighbor.push(i_dist, u_it->second);
				}
				u_it--;
			}
		};


		if (it->first != INT_MAX) {
			searchPos(it);
		}

		it--;
		if (it->first != INT_MIN) {
			searchNeg(it);
		}

	}

	bool dynamicSampleRow(std::vector<int> &suggest_r, LTMatrix &matrix, int r, int r_sample, Float rho, ref<Random> random) const {
		Row &row = matrix.rows[r];
		int n_column = matrix.columns.size();

		int error_sample = 0;
		int correct_sample = 0;

		for (int i = 0; i < r_sample; i++) {
	
			int c = random->nextFloat() * (n_column);
			int count = 0;

			while (matrix.fix_columns[c]->estimate_tag(r)) {
				correct_sample++;
				c = (c + 1) % n_column;
				if (count++ == n_column) {
					return false;
				}
			}



			LTMatrix::COL_IT column = matrix.fix_columns[c];

			int select = -1;
#ifdef UV
			int ran = random->nextFloat() * (4);
			select = ran;

			Column::CROSS_IT u_it = column->u_known_sample.lower_bound(row.u_coord);
			Column::CROSS_IT v_it = column->v_known_sample.lower_bound(row.v_coord);

			if (0) {
				int neighbor[4] {-1, -1, -1, -1};
				searchFourNeighbor(neighbor, matrix, u_it, v_it, r);

				select = ran;
				while (neighbor[select] < 0) select = (select + 1) % 4;

				select = neighbor[select];

			}
			else if (0) {
				if (select < 2) {
					if (select == 0) {
						select = getClosetNeighbor(matrix, u_it, r, row.u_coord, true);
					}
					else {
						Column::CROSS_IT v = u_it;
						v--;
						select = getClosetNeighbor(matrix, v, r, row.u_coord, false);
					}

				}
				else {

					if (select == 2) {
						select = getClosetNeighbor(matrix, v_it, r, row.v_coord, true);
					}
					else {
						Column::CROSS_IT v = v_it;
						v--;
						select = getClosetNeighbor(matrix, v, r, row.v_coord, false);
					}
				}
			}
			else {
				KNeighbor neighbor;
				neighbor.init();
				getKClosetNeighbor(neighbor, matrix, u_it, r, row.u_coord);
				getKClosetNeighbor(neighbor, matrix, v_it, r, row.v_coord);
				select = neighbor.select(random);
				
			}


			column->u_known_sample.insert(u_it, std::make_pair(row.u_coord, r));
			column->v_known_sample.insert(v_it, std::make_pair(row.v_coord, r));
#endif

#ifdef CURVE
			Column::CURVE_IT its[CURVE_COUNT];
			for (int i = 0; i < CURVE_COUNT; i++) {
				its[i] = column->curve_known_sample[i].lower_bound(row.hilbert_order[i]);
			}

			if (0) {

				int r_select = random->nextFloat() * (CURVE_COUNT * 2);
				Column::CURVE_IT &r_it = its[r_select / 2];
				if (r_select % 2 == 1) { // 0 : pos, 1 : neg
					r_it--;
				}

				if (r_it->first == INT_MIN) { r_it++; }
				if (r_it->first == INT_MAX) { r_it--; }
				select = r_it->second;

			}
			else {
				KNeighbor neighbor;
				neighbor.init();

				for (int i = 0; i < CURVE_COUNT; i++) {
					getKClosetNeighbor(neighbor, matrix, its[i], r);
				}
				select = neighbor.select(random);
			}
			

			for (int i = 0; i < CURVE_COUNT; i++) {
				column->curve_known_sample[i].insert(its[i], std::make_pair(row.hilbert_order[i], r));
			}
#endif
			Float e;
			int16_t v = -1;
			Spectrum m_rgb;
			matrix.sampleElement(r, column, e, m_rgb, v, random);
			Float actual = e * v;

			column->sum_est_visibility += actual;
			column->estimate(r) = e;
			column->m_rgb(r) = m_rgb;
			column->estimate_tag(r) = true;
			column->known_estimate.push_back(r);
			column->max_est = std::max(column->max_est, e);
			column->min_est = std::min(column->min_est, e);

			if (v >= 0) {
				column->visibility(r) = v;
				//column->visibility_tag(r) = true;
				column->known_visibility.push_back(r);
			}

			Float guess = column->estimate(select) * column->visibility(select);
			if (sign(guess) != sign(actual)) {

				error_sample++;

				int nr = findSuggestRow(matrix, r, select, random);
				if (nr >= 0) {
					suggest_r.push_back(nr);
				}
			}
		}

		return (Float)error_sample / (r_sample + correct_sample) > rho;
	}

	int findSuggestRow(LTMatrix &matrix, int r, int rep_r, ref<Random> random) const {

		Float u_0 = matrix.rows[r].u_coord;
		Float u_1 = matrix.rows[rep_r].u_coord;
		if (u_0 > u_1) {
			std::swap(u_0, u_1);
		}

		Float v_0 = matrix.rows[r].v_coord;
		Float v_1 = matrix.rows[rep_r].v_coord;
		if (v_0 > v_1) {
			std::swap(v_0, v_1);
		}

		LTMatrix::UVLink search(u_0 + random->nextFloat() * (u_1 - u_0), -1);
		std::vector<LTMatrix::UVLink>::iterator it = std::lower_bound(matrix.u_link.begin(), matrix.u_link.end(), search);

		int find_sr = -1;
		{
			std::vector<LTMatrix::UVLink>::iterator sit = it;

			while (sit != matrix.u_link.end()) {
				if (sit->r_id != r && sit->r_id != rep_r) {
					Float v = matrix.rows[it->r_id].v_coord;
					if (v >= v_0 && v <= v_1) {
	
						find_sr = it->r_id;
						break;
					}
				}

				sit++;
			}
		}

		if (find_sr < 0 && it != matrix.u_link.begin()) {

			std::vector<LTMatrix::UVLink>::iterator sit = it;
			sit--;
			while (1) {

				if (sit->r_id != r && sit->r_id != rep_r) {
					Float v = matrix.rows[it->r_id].v_coord;
					if (v >= v_0 && v <= v_1) {
						find_sr = it->r_id;
						break;
					}
				}

				if (sit == matrix.u_link.begin()) {
					break;
				}
				sit--;
			}

		}

		return find_sr;

	}


	bool dynamicSampleColumn(std::vector<int> &suggest_r, LTMatrix &matrix, int c, int r_sample, Float rho, ref<Random> random) const {

		auto &column = matrix.fix_columns[c];
		int n_row = matrix.rows.size();		
		int error_sample = 0;

		std::vector<int> sample_r = suggest_r;
		suggest_r.clear();

		int remain_r = r_sample - sample_r.size();
		Float step = (Float)n_row / remain_r;

		Float q = 0.0f;
		for (int sam = 0; sam < remain_r; sam++) {
			uint32_t r = (q + random->nextFloat() * step);
			q += step;
			r = std::min((int)r, n_row - 1);
			sample_r.push_back(r);
		}


		for (int mn = 0; mn < sample_r.size(); mn++) {			
			int r = sample_r[mn];


			int count = 0;
			while (column->estimate_tag(r)) {
				r = (r + 1) % n_row;
				assert(r < matrix.rows.size() && r >= 0);
				if (count++ == n_row) {
					return false;
				}
			}

			Row &row = matrix.rows[r];
			int select = -1;

#ifdef UV
			int ran = random->nextFloat() * (4);
			select = ran;

			Column::CROSS_IT u_it = column->u_known_sample.lower_bound(row.u_coord);
			Column::CROSS_IT v_it = column->v_known_sample.lower_bound(row.v_coord);

			if (0) {
				int neighbor[4] {-1, -1, -1, -1};
				searchFourNeighbor(neighbor, matrix, u_it, v_it, r);

				select = ran;
				while (neighbor[select] < 0) select = (select + 1) % 4;

				select = neighbor[select];

			}
			else if (0) {
				if (select < 2) {
					if (select == 0) {
						select = getClosetNeighbor(matrix, u_it, r, row.u_coord, true);
					}
					else {
						Column::CROSS_IT v = u_it;
						v--;
						select = getClosetNeighbor(matrix, v, r, row.u_coord, false);
					}

				}
				else {

					if (select == 2) {
						select = getClosetNeighbor(matrix, v_it, r, row.v_coord, true);
					}
					else {
						Column::CROSS_IT v = v_it;
						v--;
						select = getClosetNeighbor(matrix, v, r, row.v_coord, false);
					}
				}
			}
			else {
				KNeighbor neighbor;
				neighbor.init();
				getKClosetNeighbor(neighbor, matrix, u_it, r, row.u_coord);
				getKClosetNeighbor(neighbor, matrix, v_it, r, row.v_coord);
				select = neighbor.select(random);
				
			}


			column->u_known_sample.insert(u_it, std::make_pair(row.u_coord, r));
			column->v_known_sample.insert(v_it, std::make_pair(row.v_coord, r));
#endif

#ifdef CURVE
			Column::CURVE_IT its[CURVE_COUNT];
			for (int i = 0; i < CURVE_COUNT; i++) {
				its[i] = column->curve_known_sample[i].lower_bound(row.hilbert_order[i]);
			}



			int r_select = random->nextFloat() * (CURVE_COUNT * 2);
			Column::CURVE_IT &r_it = its[r_select / 2];
			if (r_select % 2 == 1) { // 0 : pos, 1 : neg
				r_it--;
			}

			if (r_it->first == INT_MIN) { r_it++; }
			if (r_it->first == INT_MAX) { r_it--; }
			select = r_it->second;

			for (int i = 0; i < CURVE_COUNT; i++) {
				column->curve_known_sample[i].insert(its[i], std::make_pair(row.hilbert_order[i], r));
			}
#endif

			Float e;
			Spectrum m_rgb;
			int16_t v = -1;
			matrix.sampleElement(r, column, e, m_rgb, v, random);
			Float actual = e * v;

			column->sum_est_visibility += actual;
			column->estimate(r) = e;
			column->m_rgb(r) = m_rgb;
			column->estimate_tag(r) = true;
			column->known_estimate.push_back(r);
			column->max_est = std::max(column->max_est, e);
			column->min_est = std::min(column->min_est, e);

			if (v >= 0) {
				column->visibility(r) = v;
				//column->visibility_tag(r) = true;
				column->known_visibility.push_back(r);
			}

			Float guess = column->estimate(select) * column->visibility(select);
			if (sign(guess) != sign(actual)) {
				error_sample++;
				
				int nr = findSuggestRow(matrix, r, select, random);
				if (nr >= 0) {
					suggest_r.push_back(nr);
				}
			}

		}


		return (Float)error_sample / r_sample > rho;
	}

	bool predictorSampleRow(LTMatrix &matrix, int r, int r_sample, Float rho, ref<Random> random) const {
		Row &row = matrix.rows[r];
		int n_column = matrix.columns.size();

		int error_sample = 0;
		int correct_sample = 0;
		uint32_t n_new_sample = 0;
		uint32_t n_new_visible = 0;

		for (int i = 0; i < r_sample; i++) {

			int c = random->nextFloat() * (n_column);
			int count = 0;

			while (matrix.fix_columns[c]->estimate_tag(r)) {
				correct_sample++;
				c = (c + 1) % n_column;
				if (count++ == n_column) {
					return false;
				}
			}

			LTMatrix::COL_IT column = matrix.fix_columns[c];

			Float e;
			int16_t v = -1;
			Spectrum m_rgb;
			matrix.sampleElement(r, column, e, m_rgb, v, random);
			Float actual = e * v;

			n_new_sample++;
			column->n_sample++;
			matrix.rows[r].n_sample++;

			column->sum_est_visibility += actual;
			column->m_rgb(r) = m_rgb;
			column->estimate(r) = e;
			column->estimate_tag(r) = true;
			column->known_estimate.push_back(r);
			column->max_est = std::max(column->max_est, e);
			column->min_est = std::min(column->min_est, e);

			if (v >= 0) {
				column->visibility(r) = v;
				//column->visibility_tag(r) = true;
				column->known_visibility.push_back(r);

				if (v == 1) {
					column->n_visible++;
					matrix.rows[r].n_visible++;
					n_new_visible++;
				}

			}


			if (column->p_selection == Column::PKNN) {

				int select = -1;
#ifdef UV
				int ran = random->nextFloat() * (4);
				select = ran;

				Column::CROSS_IT u_it = column->u_known_sample.lower_bound(row.u_coord);
				Column::CROSS_IT v_it = column->v_known_sample.lower_bound(row.v_coord);

				if (0) {
					int neighbor[4] {-1, -1, -1, -1};
					searchFourNeighbor(neighbor, matrix, u_it, v_it, r);

					select = ran;
					while (neighbor[select] < 0) select = (select + 1) % 4;

					select = neighbor[select];

				}
				else if (0) {
					if (select < 2) {
						if (select == 0) {
							select = getClosetNeighbor(matrix, u_it, r, row.u_coord, true);
						}
						else {
							Column::CROSS_IT v = u_it;
							v--;
							select = getClosetNeighbor(matrix, v, r, row.u_coord, false);
						}

					}
					else {

						if (select == 2) {
							select = getClosetNeighbor(matrix, v_it, r, row.v_coord, true);
						}
						else {
							Column::CROSS_IT v = v_it;
							v--;
							select = getClosetNeighbor(matrix, v, r, row.v_coord, false);
						}
					}
				}
				else {
					KNeighbor neighbor;
					neighbor.init();
					getKClosetNeighbor(neighbor, matrix, u_it, r, row.u_coord);
					getKClosetNeighbor(neighbor, matrix, v_it, r, row.v_coord);
					select = neighbor.select(random);

				}


				column->u_known_sample.insert(u_it, std::make_pair(row.u_coord, r));
				column->v_known_sample.insert(v_it, std::make_pair(row.v_coord, r));
#endif

#ifdef CURVE
				Column::CURVE_IT its[CURVE_COUNT];
				for (int i = 0; i < CURVE_COUNT; i++) {
					its[i] = column->curve_known_sample[i].lower_bound(row.hilbert_order[i]);
				}

				if (0) {

					int r_select = random->nextFloat() * (CURVE_COUNT * 2);
					Column::CURVE_IT &r_it = its[r_select / 2];
					if (r_select % 2 == 1) { // 0 : pos, 1 : neg
						r_it--;
					}

					if (r_it->first == INT_MIN) { r_it++; }
					if (r_it->first == INT_MAX) { r_it--; }
					select = r_it->second;

				}
				else {
					KNeighbor neighbor;
					neighbor.init();

					for (int i = 0; i < CURVE_COUNT; i++) {
						getKClosetNeighbor(neighbor, matrix, its[i], r);
					}
					select = neighbor.select(random);
				}


				for (int i = 0; i < CURVE_COUNT; i++) {
					column->curve_known_sample[i].insert(its[i], std::make_pair(row.hilbert_order[i], r));
				}
#endif

				Float guess = column->estimate(r) * column->visibility(select);
				if (sign(guess) != sign(actual)) {
					error_sample++;
				}
			}
			else if (column->p_selection == Column::PNBM) {

				matrix.m_nbm_predictor.updateColumnLikeliHood(matrix, c);
				matrix.m_nbm_predictor.updateRowLikeliHood(matrix, r);

				Float p0 = matrix.m_nbm_predictor.getPosteriorPDF0(r, c, matrix);
				Float guess = column->estimate(r);
				if (random->nextFloat() < p0) {
					guess = 0;
				}

				if (sign(guess) != sign(actual)) {
					error_sample++;
				}
			}
			else if (column->p_selection == Column::PLMS) {


				Float p0 = matrix.m_lms_predictor.P0(r, c, matrix);
// 
// 				if (std::abs(p0 - 0.5) < 0.2) {
// 					matrix.m_lms_predictor.UpdateW(c, matrix);
// 				}

				Float guess = column->estimate(r);
				if (random->nextFloat() < p0) {
					guess = 0;
				}

				if (sign(guess) != sign(actual)) {
					error_sample++;
				}
			}
			else {
				assert(0);
			}

			matrix.m_nbm_predictor.increasePrioi(n_new_visible, n_new_sample - n_new_visible);
		
		}

		return (Float)error_sample / (r_sample + correct_sample) > rho;
	}


	template<int n_predictor>
	bool predictorSamplingColumn(LTMatrix &matrix, int c, int r_sample, Float rho, ref<Random> random) const {

		auto &column = matrix.fix_columns[c];
		matrix.m_nbm_predictor.updateColumnLikeliHood(matrix, c);

		if (n_predictor >= 2 || column->p_selection == Column::PLMS) {
			matrix.m_lms_predictor.UpdateW(c, matrix);
		}


		uint32_t n_new_sample = 0;
		uint32_t n_new_visible = 0;

		int n_row = matrix.rows.size();
		int error_sample = 0;

		std::vector<int> sample_r;
		sample_r.reserve(r_sample);

		int remain_r = r_sample - sample_r.size();
		Float step = (Float)n_row / remain_r;

		Float q = 0.0f;
		for (int sam = 0; sam < remain_r; sam++) {
			uint32_t r = (q + random->nextFloat() * step);
			q += step;
			r = std::min((int)r, n_row - 1);
			sample_r.push_back(r);
		}

		uint32_t n_error_sample[n_predictor];
		for (int i = 0; i < n_predictor; i++) {
			n_error_sample[i] = 0;
		}

		
		//if (n_predictor > 1) {
		//	n_error_sample[2] = n_row;
		//}

		for (int mn = 0; mn < sample_r.size(); mn++) {
			int r = sample_r[mn];
			Float random_float = random->nextFloat();

			int count = 0;
			while (column->estimate_tag(r)) {
				r = (r + 1) % n_row;
				assert(r < matrix.rows.size() && r >= 0);
				if (count++ == n_row) {
					return false;
				}
			}

			Row &row = matrix.rows[r];
			int select = -1;

			Float e;
			int16_t v = -1;
			Spectrum m_rgb;
			matrix.sampleElement(r, column, e, m_rgb, v, random);
			Float actual = e * v;

			n_new_sample++;
			column->n_sample++;
			matrix.rows[r].n_sample++;

			column->sum_est_visibility += actual;
			column->estimate(r) = e;
			column->m_rgb(r) = m_rgb;
			column->estimate_tag(r) = true;
			column->known_estimate.push_back(r);

			column->max_est = std::max(column->max_est, e);
			column->min_est = std::min(column->min_est, e);

			if (v >= 0) {
				column->visibility(r) = v;
				//column->visibility_tag(r) = true;
				//column->known_visibility.push_back(r);

				if (v == 1) {
					column->n_visible++;
					matrix.rows[r].n_visible++;
					n_new_visible++;
				}
			}


			uint32_t p_idx = 0;
			if (n_predictor > 1 || column->p_selection == Column::PNBM) {
			
				matrix.m_nbm_predictor.updateRowLikeliHood(matrix, r);
			
				Float p0 = matrix.m_nbm_predictor.getPosteriorPDF0(r, c, matrix);
			
				Float guess = column->estimate(r);
				if (random_float < p0) {
					guess = 0;
				}

				if (sign(guess) != sign(actual)) {
					n_error_sample[p_idx++]++;
				}

			}
	
			if (n_predictor >= 2 || column->p_selection == Column::PKNN) {

#ifdef UV
				int ran = random->nextFloat() * (4);
				select = ran;

				Column::CROSS_IT u_it = column->u_known_sample.lower_bound(row.u_coord);
				Column::CROSS_IT v_it = column->v_known_sample.lower_bound(row.v_coord);

				if (0) {
					int neighbor[4] {-1, -1, -1, -1};
					searchFourNeighbor(neighbor, matrix, u_it, v_it, r);

					select = ran;
					while (neighbor[select] < 0) select = (select + 1) % 4;

					select = neighbor[select];

				}
				else if (0) {
					if (select < 2) {
						if (select == 0) {
							select = getClosetNeighbor(matrix, u_it, r, row.u_coord, true);
						}
						else {
							Column::CROSS_IT v = u_it;
							v--;
							select = getClosetNeighbor(matrix, v, r, row.u_coord, false);
						}

					}
					else {

						if (select == 2) {
							select = getClosetNeighbor(matrix, v_it, r, row.v_coord, true);
						}
						else {
							Column::CROSS_IT v = v_it;
							v--;
							select = getClosetNeighbor(matrix, v, r, row.v_coord, false);
						}
					}
				}
				else {
					KNeighbor neighbor;
					neighbor.init();
					getKClosetNeighbor(neighbor, matrix, u_it, r, row.u_coord);
					getKClosetNeighbor(neighbor, matrix, v_it, r, row.v_coord);
					select = neighbor.select(random);

				}


				column->u_known_sample.insert(u_it, std::make_pair(row.u_coord, r));
				column->v_known_sample.insert(v_it, std::make_pair(row.v_coord, r));
#endif

#ifdef CURVE
				Column::CURVE_IT its[CURVE_COUNT];
				for (int i = 0; i < CURVE_COUNT; i++) {
					its[i] = column->curve_known_sample[i].lower_bound(row.hilbert_order[i]);
				}

				int r_select = random->nextFloat() * (CURVE_COUNT * 2);
				Column::CURVE_IT &r_it = its[r_select / 2];
				if (r_select % 2 == 1) { // 0 : pos, 1 : neg
					r_it--;
				}

				if (r_it->first == INT_MIN) { r_it++; }
				if (r_it->first == INT_MAX) { r_it--; }
				select = r_it->second;

				for (int i = 0; i < CURVE_COUNT; i++) {
					column->curve_known_sample[i].insert(its[i], std::make_pair(row.hilbert_order[i], r));
				}
#endif

				Float guess = column->estimate(r) * column->visibility(select);
				if (sign(guess) != sign(actual)) {
					n_error_sample[p_idx++]++;
				}

			}

	
			if (n_predictor >= 3 || column->p_selection == Column::PLMS) {

				Float p0 = matrix.m_lms_predictor.P0(r, c, matrix);
				Float guess = column->estimate(r);
				if (random_float < p0) {
					guess = 0;
				}

				if (sign(guess) != sign(actual)) {
					n_error_sample[p_idx++]++;
				}

			}

		}

		matrix.m_nbm_predictor.increasePrioi(n_new_visible, n_new_sample - n_new_visible);

		if (n_predictor > 1) {

			Column::PredictorSelection min_selection = Column::PNONE;
			int min_count = INT_MAX;

			for (int i = 0; i < n_predictor; i++) {
				if (n_error_sample[i] < min_count) {
					min_selection = (Column::PredictorSelection)i;
					min_count = n_error_sample[i];
				}
			}

			assert(min_selection != Column::PNONE);
			//min_selection = Column::PKNN;
			column->p_selection = min_selection;


			return (Float)min_count / r_sample > rho;
		}
		else {
			return (Float)n_error_sample[0] / r_sample > rho;
		}


		
	}

	void dynamicSampling(LTMatrix &matrix, ref<Random> random, uint32_t &shadow_count, Float sample_ratio, Float rho, Float singular_threshold) const {

		int n_column = matrix.fix_columns.size();
		int n_row = matrix.rows.size();

		// cross coordinate 
		matrix.generateCrossCoordinate();

		matrix.m_nbm_predictor.init(matrix);
		matrix.updateNBMPredictor();

		matrix.m_lms_predictor.init(matrix);

		int r_sample = 2;
		/// protector selection ===========
		Float max_error_ratio = 0;
		Float sum_error_ratio = 0;
		for (auto row : matrix.rows) {
			max_error_ratio = std::max(max_error_ratio, row.shadow_pdf);
			sum_error_ratio += row.shadow_pdf;
		}

		int c_sample = std::max((int)2, (int)(sample_ratio * n_row));
		sum_error_ratio /= n_row;

		if (max_error_ratio < 1) { // smooth region, use Bayesian for smooth columns
			
			for (int c = 0; c < n_column; c++) {
				LTMatrix::COL_IT &column = matrix.fix_columns[c];
				if (column->n_visible == 0 || column->known_estimate.size() == column->n_visible) {
					column->p_selection = Column::PNBM;
				}
				else {

					matrix.crospprojection(c);

					if (predictorSamplingColumn<3>(matrix, c, c_sample, rho, random)) {

						while (predictorSamplingColumn<1>(matrix, c, c_sample, rho, random)) {

						}
					}
					
				}
			}			
		}
		else {

			
			sum_error_ratio *= DYNAMIC_SAMPLE_RATIO;
			sample_ratio *= (1.0f + sum_error_ratio);

			vector<int> active_column;
			active_column.reserve(n_column);

			vector<int> random_column;
			random_column.reserve(n_column);
			for (int c = 0; c < n_column; c++) {
				random_column.push_back(c);
			}

			for (int p = 0; p < n_column; p++) {
				int random_c = random->nextFloat() * (random_column.size());
				int c = random_column[random_c];

				LTMatrix::COL_IT &column = matrix.fix_columns[c];
				if (0 && column->n_visible == 0 || column->known_estimate.size() == column->n_visible) {
					column->p_selection = Column::PNBM;
				}
				else {

					matrix.crospprojection(c);
					
					if (predictorSamplingColumn<3>(matrix, c, c_sample, rho, random)) {
						active_column.push_back(c);			
					}

				}

				random_column[random_c] = random_column.back();
				random_column.pop_back();
			}

			assert(random_column.size() == 0);

			Float test_ratio = sample_ratio;
			// sample columns
			if (1) {
				
				while (!active_column.empty()) {

					int random_c = random->nextFloat() * (active_column.size());
					int c = active_column[random_c];
			
					if (predictorSamplingColumn<1>(matrix, c, c_sample, rho, random)) {

					}
					else {
						active_column[random_c] = active_column.back();
						active_column.pop_back();
					}
				}

			}
			//rho *= 0.1;
			// sample rows
			if (ROW_DYNAMIC_SAMPLE) {

				r_sample = std::max((int)2, (int)(test_ratio * matrix.columns.size()));
				
				vector<int> active_row;
				active_row.reserve(n_row);
				vector<int> random_row;
				random_row.reserve(n_row);

				for (int r = 0; r < n_row; r++) {
					random_row.push_back(r);
				}
				for (int p = 0; p < n_row; p++) {
					int random_r = random->nextFloat() * random_row.size();//(p + 1);

					int r = random_row[random_r];

 					if (predictorSampleRow(matrix, r, r_sample, rho, random)) {
						active_row.push_back(r);
					}

					random_row[random_r] = random_row.back();
					random_row.pop_back();
				}
				assert(random_row.size() == 0);

				//r_sample *= 2;
				while (!active_row.empty()) {

					int random_r = random->nextFloat() * (active_row.size());
					int r = active_row[random_r];
					if (predictorSampleRow(matrix, r, r_sample, rho, random)) {

					}
					else {
						active_row[random_r] = active_row.back();
						active_row.pop_back();
					}
				}
			}
		}

		{

			for (int r = 0; r < n_row; r++) {
				if (matrix.rows[r].n_sample == 0) {
					while (predictorSampleRow(matrix, r, r_sample, rho, random)) {

					}
				}
			}
		}
		
		matrix.updateNBMPredictor();
		matrix.updateLMSPredictor();

		for (int i = 0; i < n_column; i++) {
			
			LTMatrix::COL_IT &column = matrix.fix_columns[i];
			//column->known_estimate.clear();
			list<uint32_t, PoolAllocator<uint32_t>>::iterator itr = column->known_estimate.begin();
			int count = 0;
			for (int r = 0; r < n_row; r++) {
				if (column->estimate_tag(r)) {
					count++;
					*itr = r;
					itr++;
					//column->known_estimate.push_back(r);
				}
			}

			assert(itr == column->known_estimate.end());
			
		}
	}

	bool find(const VSensorNode *n, Point2i p) {
		if (n->isLeaf()) {
			return n->sampleBbox.contains(p);
		}
		else {
			return find(n->left, p) || find(n->right, p);
		}
	}

	void Clear(Fmat &D, vector<list<Column>::iterator> &valid_column, LTMatrix &matrix) {

		for (int i = 0; i < valid_column.size(); i++) {

			list<Column>::iterator c = valid_column[i];

			for (list<uint32_t>::iterator it = c->known_estimate.begin(); it != c->known_estimate.end(); it++) {	
				D.at(*it, i) = -1;				
			}

		}
	}

	void Blue(Fmat &D, Fmat &S, vector<list<Column>::iterator> &sparse_column) {

		for (int i = 0; i < D.n_cols; i++) {

			Float max_eval = sparse_column[i]->max_eval;
			Float min_eval = sparse_column[i]->min_est * sparse_column[i]->avg_intensity;
			//bool m_term = ((sparse_column[i]->max_est - sparse_column[i]->min_est) / std::max(1e-6f, sparse_column[i]->min_est)) > M_TERM;
			for (int r = 0; r < D.n_rows; r++) {
				
				if (S(r, i) > SM_THRESHOLD * std::max(D(r, i), max_eval)) {
					if (sparse_column[i]->estimate_tag(r)) {
						S(r, i) = 999;
					}
					else {
						S(r, i) *= -1;
					}

				}
			}

		}
	}

	void getTrue(Fmat &D, vector<list<Column>::iterator> &sparse_column, LTMatrix &matrix, ref<Random> &random) {
		for (int c = 0; c < D.n_cols; c++) {
			list<Column>::iterator column = sparse_column[c];
			Float intensity = column->avg_intensity;
			for (int r = 0; r < D.n_rows; r++) {
				if (column->estimate_tag(r)) {
					D.at(r, c) = column->estimate(r) * column->visibility(r) * intensity;
				}
				else {
					Float e;
					int16_t v;
					Spectrum m_rgb;
					matrix.sampleElement(r, column, e, m_rgb, v, random);
					D.at(r, c) = e * v * intensity;
				}
			}

		}
	}


	void drawGreenPicture(Fmat &green, LTMatrix &matrix, vector<list<Column>::iterator> &sparse_columns, Fmat &S, Fmat &L, Fmat &D, ref<Random> &random) const {

		int n_row = S.n_rows;
		int n_col = S.n_cols;

		S = arma::abs(S);
		green.zeros(n_row, n_col);

		// draw blue
		for (int c = 0; c < n_col; c++) {
			list<Column>::iterator &column = sparse_columns[c];			
			Float max_eval = column->max_eval;
			for (int r = 0; r < n_row; r++) {
				if (S.at(r, c) > SM_THRESHOLD * std::max(D(r, c), max_eval)) {
					green.at(r, c) = 1.0f;  // blue
					if (column->estimate_tag(r)) { // green
						green.at(r, c) = 1.0f;
					}

					//green.at(r, c) = -1.0f;  // blue
					//if (column->estimate_tag(r)) { // green
					//	green.at(r, c) = 1.0f;
					//}
				}
			}
		}



		return;

		//std::vector<Point2i> green_record;
		//green_record.reserve(n_row * n_col);

		//auto turnPoint = [&](int ir, int ic, list<Column>::iterator &column) {

		//	bool extend = false;
	
		//	Float ture_value = -1; Float e; int16_t v;
		//	if (column->estimate_tag(ir)) { // already known
		//		e = column->estimate(ir);
		//		v = column->visibility(ir);
		//		ture_value = e * v * column->avg_intensity;
		//	}
		//	else { // unknown point
		//		e;
		//		v = -1;
		//		matrix.sampleElement(ir, column, e, v, random);

		//		column->estimate(ir) = e;
		//		column->estimate_tag(ir) = true;
		//		column->visibility(ir) = v;

		//		ture_value = e * v * column->avg_intensity;

		//	}


		//	if (std::abs(ture_value - L.at(ir, ic)) > SM_THRESHOLD * e) {
		//		green.at(ir, ic) = 2.0f; // it's dark green
		//		extend = true;
		//	}
		//	else {
		//		green.at(ir, ic) = 1.0f; // turn to dim green
		//	}

		//	L.at(ir, ic) = ture_value;

		//	return extend;
		//};

		//auto spreadNearby = [&](int r, int c, list<Column>::iterator &column) {
		//	// up
		//	int end = std::max((int)0, (int)r - SM_BOARD);
		//	for (int ir = r - 1; ir >= end; ir--) {
		//		if (turnPoint(ir, c, column)) {
		//			end = std::max((int)0, (int)ir - SM_BOARD);
		//		}
		//	}

		//	// down
		//	int end_1 = std::min(n_row - 1, (int)r + SM_BOARD);
		//	for (int ir = r + 1; ir <= end_1; ir++) {
		//		if (turnPoint(ir, c, column)) {
		//			end_1 = std::min(n_row - 1, (int)ir + SM_BOARD);;
		//		}
		//	}

	
		//	// extend to nearby columns
		//	int nc = std::min(c + SM_BOARD, n_col - 1);
		//	for (int i = std::max((int)0, c - SM_BOARD); i <= nc; i++) {
		//		
		//		if (i == c) continue;

		//		for (int rr = end; rr <= end_1; rr++) {
		//			if (green.at(rr, i) <= 0.0f) {
		//				green_record.emplace_back(rr, i);
		//			}
		//		}

		//	}

		//};



		//for (int c = 0; c < n_col; c++) {
		//	list<Column>::iterator &column = sparse_columns[c];
 
		//	Float m_eval = column->max_eval * 0.1;
		//	for (auto r : column->known_estimate) {

		//		if (green.at(r, c) < 0.0f) { // known and blue point
		//			//cout << r << " " << c << endl;
		//		
		//			green.at(r, c) = 2.0f;	
		//			L.at(r, c) = column->estimate(r) * column->visibility(r) * column->avg_intensity;
		//			spreadNearby(r, c, column);		

		//			
		//		}
		//	}
		//}



		//
		////return;
		//// test dim green
		//while (!green_record.empty()) {

		//	Point2i record = green_record.back();
		//	green_record.pop_back();

		//	int r = record.x;
		//	int c = record.y;


		//	if (green.at(r, c) > 0) { continue; } // already green
		//
		//	if (turnPoint(r, c, sparse_columns[c])) { // need to expand
		//		spreadNearby(r, c, sparse_columns[c]);
		//	}

		//}
	
	}


	void colorpfm(Fmat &D, const char *filename, vector<list<Column>::iterator> &sparse_column, LTMatrix &matrix) const {

		ref<Bitmap> bitmap = new Bitmap(Bitmap::ERGB, Bitmap::EFloat32, Vector2i(D.n_cols, D.n_rows), 3);
		ref<FileStream> stream = new FileStream(filename, FileStream::ETruncWrite);

		for (int c = 0; c < D.n_cols; c++) {
			list<Column>::iterator &column = sparse_column[c];

			for (int r = 0; r < D.n_rows; r++) {
				Spectrum v;
				if (D.at(r, c) > 0) {
					 v = column->estimate(r) * column->m_rgb(r) * matrix.rows[r].sensor->strength * column->spur_intensity;
				}
				else {
					v = Spectrum(0.0f);
				}
				bitmap->setPixel(Point2i(c, r), v);
			}
		}
		

		bitmap->write(Bitmap::EPFM, stream);
	}

	void test(SliceLightRecord &record, uint32_t &shadowCount, const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *sn, SliceWorkResult *result, ref<Timer> t0) {
		//Log(EWarn, "init slice [%u %u %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		//t0->start();
	
		int max_cutsize = m_maxCutsize;
		std::srand(0);
		Float singular_thresholding = m_singular_threshold;
		Float sample_ratio = m_sample_ratio;
		sample_ratio *= SAMPLE_RATIO_ADD;
		bool special = false; 
#ifdef CB
		if (//sn->sampleBbox.contains(Point2i(1, 197)) ||
			//sn->sampleBbox.contains(Point2i(346, 381)) ||
			TAABB<Point2i>(Point2i(748, 1508), Point2i(880, 1560)).overlaps(sn->sampleBbox) ||
			//TAABB<Point2i>(Point2i(847, 1291), Point2i(935, 1342)).overlaps(slice_root->sampleBbox) ||
			//TAABB<Point2i>(Point2i(459, 312), Point2i(463, 314)) == (slice_root->sampleBbox) ||
			//TAABB<Point2i>(Point2i(1081, 1312), Point2i(1120, 1345)).overlaps(slice_root->sampleBbox) ||
			//TAABB<Point2i>(Point2i(1176, 1296), Point2i(1259, 1361)).overlaps(slice_root->sampleBbox) ||
			//TAABB<Point2i>(Point2i(1155, 1275), Point2i(1178, 1290)).overlaps(slice_root->sampleBbox) ||
			//TAABB<Point2i>(Point2i(982, 1551), Point2i(1047, 1584)).overlaps(slice_root->sampleBbox) ||
			0) {
			special = true;
			//	sample_ratio *= 5;
		}
	//	if (!special) return;
#endif
#ifdef SPONZA
		if (//sn->sampleBbox.contains(Point2i(706, 603)) ||
			//sn->sampleBbox.contains(Point2i(100, 100)) ||
			//TAABB<Point2i>(Point2i(893, 1496), Point2i(214, 173)) == (sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(893, 1496), Point2i(1005, 1571)).overlaps(sn->sampleBbox) ||
			0) {
			special = true;
			//	sample_ratio *= 5;
		}
		//if (!special) return;
#endif//

#ifdef SF
		if (sn->sampleBbox.contains(Point2i(103, 604)) ||
			//sn->sampleBbox.contains(Point2i(127, 492)) ||
			//TAABB<Point2i>(Point2i(893, 1496), Point2i(214, 173)) == (sn->sampleBbox) ||
			//TAABB<Point2i>(Point2i(426, 101), Point2i(587, 556)) == (sn->sampleBbox) ||
			//TAABB<Point2i>(Point2i(280, 360), Point2i(320, 380)).overlaps(sn->sampleBbox) ||
			0) {
			special = true;
			//	sample_ratio *= 5;
		}
		//if (!special) return;

#endif

#ifdef SAN
		if (//sn->sampleBbox.contains(Point2i(706, 603)) ||
			//sn->sampleBbox.contains(Point2i(127, 492)) ||
			TAABB<Point2i>(Point2i(415, 369), Point2i(502, 441)) == (sn->sampleBbox) ||
			//TAABB<Point2i>(Point2i(221, 146), Point2i(420, 164)).overlaps(sn->sampleBbox) ||
			//TAABB<Point2i>(Point2i(0, 60), Point2i(116, 155)).overlaps(sn->sampleBbox) ||
			0) {
			special = true;
			//	sample_ratio *= 5;
		}
		//if (!special) return;


#endif

/*#ifdef CSPONZA
		if (//sn->sampleBbox.contains(Point2i(706, 603)) ||
			//sn->sampleBbox.contains(Point2i(100, 100)) ||
			//TAABB<Point2i>(Point2i(893, 1496), Point2i(214, 173)) == (sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(504, 744), Point2i(818, 912)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1060, 985), Point2i(1408, 1155)).overlaps(sn->sampleBbox) ||
			0) {
			special = true;
			//	sample_ratio *= 5;
			max_cutsize = 600;
		}
		//if (!special) return;
#endif//*/

#ifdef CARNIVAL

		if (TAABB<Point2i>(Point2i(505, 703), Point2i(597, 750)).overlaps(sn->sampleBbox) || //
			0) {
			max_cutsize = 1000;
			special = true;
		}

		if (TAABB<Point2i>(Point2i(388, 498), Point2i(1110, 700)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1216, 684), Point2i(1368, 944)).overlaps(sn->sampleBbox) ||
			0) {
				max_cutsize = 2000;
				special = true;
		}

		if (TAABB<Point2i>(Point2i(1038, 624), Point2i(1087, 661)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1001, 670), Point2i(1033, 706)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1052, 692), Point2i(1065, 716)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(325, 637), Point2i(374, 655)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(482, 795), Point2i(757, 952)).overlaps(sn->sampleBbox) || // ma
			TAABB<Point2i>(Point2i(810, 796), Point2i(923, 952)).overlaps(sn->sampleBbox) || // ma2
			TAABB<Point2i>(Point2i(954, 882), Point2i(1020, 910)).overlaps(sn->sampleBbox) || // ma3
			TAABB<Point2i>(Point2i(212, 641), Point2i(1056, 749)).overlaps(sn->sampleBbox) || 
			TAABB<Point2i>(Point2i(974, 828), Point2i(1182, 971)).overlaps(sn->sampleBbox) ||
			0) {
			max_cutsize = 3000;
			special = true;
		}

		if (TAABB<Point2i>(Point2i(781, 860), Point2i(818, 901)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1186, 876), Point2i(1212, 922)).overlaps(sn->sampleBbox) ||
			TAABB<Point2i>(Point2i(1033, 672), Point2i(1067, 717)).overlaps(sn->sampleBbox) ||
			0) {
			max_cutsize = 4000;
			sample_ratio *= 2;
		}

	//	if (!special) return;

#endif

		const VSensorNode *slice_root = sn;
		//Log(EWarn, "coming in slice [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);

		// init column
		ref<Random> random = new Random(uint64_t(0));
		LTMatrix matrix(slice_root, scene, m_clamping, m_cutoff, random);
		uint32_t n_init_sample = std::round(matrix.n_row * sample_ratio);
	
		n_init_sample = std::max(n_init_sample, std::min(matrix.n_row, m_minimum_sample));
		initMatrix(matrix, scene, slice_root, n_init_sample, shadowCount, max_cutsize);
		
  		//bool find = false;
  		//for (int r = 0; r < matrix.rows.size(); r++) {
  		//	if (//matrix.rows[r].sensor->getImagePos() == Point2i(283, 476)
  		//		//matrix.rows[r].sensor->getImagePos() == Point2i(100, 100)
				//matrix.rows[r].sensor->getImagePos() == Point2i(675, 494) ||
  		//		//matrix.rows[r].sensor->getImagePos() == Point2i(24, 85)
  		//		//matrix.rows[r].sensor->getImagePos() == Point2i(749, 1234) || matrix.rows[r].sensor->getImagePos() == Point2i(739, 1234)
  		//		0) {

				////if (slice_root->num_sensor == 875) {
				//	cout << "row is " << r << "sample is " << n_init_sample << endl;
				//	cout << "num sensor is " << slice_root->num_sensor << endl;
				//	cout << matrix.rows[r].sensor->getImagePos().toString() << " ";
				//	cout << matrix.rows[r].sensor->its.p.toString() << " " << matrix.rows[r].sensor->its.shFrame.n.toString() << endl;
				//	find = true;
				////}

  		//	}
  		//}
  		//if (!find) {
  		//	return;
  		//}
		
		//cout << slice_root->bbox.toString() << endl;
		//cout << slice_root->rep_sensor->its.p.toString() << endl;
		//Log(EWarn, "init slice [%u %u %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		sparseSample(matrix, scene, sample_ratio, random, shadowCount);

		if (matrix.columns.size() == 0 || matrix.rows.size() == 0 || matrix.sum_avg_eval == 0) {
			//Log(EWarn, "An empty slice [%u %u, %u, %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
			return;
		}

		Float e_sum = 0;
		//int c = 0;


		//cout << e_sum << endl;
		//{
		//	Float sum(0.0f);
		//	uint32_t c = 0;
		//	for (list<Column>::iterator column = matrix.columns.begin(); column != matrix.columns.end(); column++, c++) {
		//	
		//		sum += column->max_eval;
		//	}
		//	cout << sum<< " " << matrix.sum_max_eval << endl;
		//}
		//Log(EWarn, "balance slice [%u %u %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		//randomSample(matrix, random);
		balanceMatrix(matrix, random, shadowCount, sample_ratio, singular_thresholding);
		record.recordInvisibleLight(matrix);
		int n_column_before = matrix.columns.size();
		//matrix.removeEmptyColumn();
		
		if (matrix.columns.size() == 0) {

			return;
		}

		matrix.fixColumn();

		//std::cout << matrix.columns.size() << std::endl;
		
		dynamicSampling(matrix, random, shadowCount, sample_ratio, 0.1, singular_thresholding);

#if 0
		{
			uint32_t n_sample = 0;
			for (auto column : matrix.fix_columns) {
				n_sample += column->known_estimate.size();
			}
			Float ratio = (Float)n_sample / (matrix.n_row *n_column_before);
			Spectrum color(ratio);
			for (auto row : matrix.rows) {
				for (auto spp : row.spp_sensors) {
					result->put(spp.sensor, color);
				}
			}
		}
		return;
#endif
		

		record.recordVisibleLight(matrix);

	
		matrix.updateColumnID();
		Float s = matrix.sum_avg_eval;
		matrix.updateColumnAverage();
		
		
		Fmat D;
		Uvec known;
		vector<list<Column>::iterator> sparse_column;		
		sparse_column.reserve(matrix.columns.size());
		vector<list<Column>::iterator> singular_column;
		singular_column.reserve(matrix.columns.size());

// 		Fmat prob(matrix.n_row, matrix.fix_columns.size());
// 		for (int c = 0; c < matrix.fix_columns.size(); c++) {
// 			for (int r = 0; r < matrix.n_row; r++) {
// 				prob(r, c) = 1.0f - matrix.m_nbm_predictor.getPosteriorPDF0(r, c, matrix);
// 			}
// 		}
// 		cout << prob.max() << " " << prob.min() << endl;
// 		pfm(prob, "prob.pfm", 99);
		//return;

		//randomSample(matrix, random);
		
#ifdef UV
		//disperseNoise_UV_new(D, known, sparse_column, singular_column, matrix, random);
		disperseNoise_UV_k(D, known, sparse_column, singular_column, matrix, random);
#endif // UV
#ifdef CURVE
		//disperseNoise_CURVE_new(D, known, sparse_column, singular_column, matrix, random);
		////disperseNoise_CURVE_k(D, known, sparse_column, singular_column, matrix, random);
#endif
		//disperseNoise(D, known, sparse_column, singular_column, matrix, random);

		disperseNoise_predictor(D, known, sparse_column, singular_column, matrix, random);


		//disperseNoise(D, known, sparse_column, singular_column, matrix, random);


		//sp_Fmat s_D(D);
		//Fmat X, Y, L;
		//if (!tlah::NMFC(X,Y, s_D, 5, 20, 0.01)) {
		//	return;
		//	Log(EError, "Failed to recover at slice [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		//}
		//L = X * Y;
		//getTrue(D, sparse_column, matrix, random);
		//known.resize(1);
		//return;
		//Log(EWarn, "begin separation [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		Fmat S, L;
		int iteration_time = 0;
		if (std::min(D.n_rows, D.n_cols) < 8) {
			//Log(EWarn, "A small slice");
			L = D; S.zeros(D.n_rows, D.n_cols);
		}
		else if (!tlah::PDMS(iteration_time, L, S, D, known, 8, MAX_ITERATION, STOP_ERROR)) {
			Log(EError, "Failed to recover at slice [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
			return;
		}


#if 0
		{
			Fmat LL = L;

			Fmat sample = D;

			int n_row = matrix.rows.size();
			for (uint32_t c = 0; c < sparse_column.size(); c++) {
				list<Column>::iterator &column = sparse_column[c];
				Float avg_intensity = column->spur_intensity.average();

				for (int r = 0; r < n_row; r++) {
					if (LL.at(r, c) > 0.5 * column->estimate(r) * avg_intensity) {
						LL.at(r, c) = column->estimate(r) * avg_intensity;
					}
					else {
						LL.at(r, c) = 0;
					}

					if (!column->estimate_tag(r)) {
						sample.at(r, c) = 0;
					}
					else {
						sample.at(r, c) = (1.0f);
					}

				}

			}

			Fmat SS = D - LL;

			//colorpfm(sample, "color_sample.pfm", sparse_column, matrix);
			pfm(sample, "black_sample.pfm", 99);

			colorpfm(D, "color_DD.pfm", sparse_column, matrix);
			pfm(D, "black_DD.pfm", 99);

			colorpfm(LL, "color_LL.pfm", sparse_column, matrix);
			pfm(LL, "black_LL.pfm", 99);

			colorpfm(SS, "color_SS.pfm", sparse_column, matrix);
			pfm(SS, "black_SS.pfm", 99);


			Fmat RR = LL;
			getTrue(RR, sparse_column, matrix, random);
			colorpfm(RR, "color_RR.pfm", sparse_column, matrix);
			pfm(RR, "black_RR.pfm", 99);

			Fmat green_picture;
			drawGreenPicture(green_picture, matrix, sparse_column, S, L, D, random);
			denoise_filter(matrix, LL, SS, D, sparse_column, shadowCount, random, green_picture);

			pfm(green_picture, "green.pfm", 99);

			colorpfm(LL, "color_final.pfm", sparse_column, matrix);
			pfm(LL, "black_final.pfm", 99);

			return;
		}
#endif


		if (std::min(D.n_rows, D.n_cols) >= 8) {
			Fmat green_picture;
			drawGreenPicture(green_picture, matrix, sparse_column, S, L, D, random);
			//pfm(green_picture, "ratio.pfm");
			denoise_filter(matrix, L, S, D, sparse_column, shadowCount, random, green_picture);

		}

		// error detection
#if 0
		{ 

			Fmat RL = D;
			getTrue(RL, sparse_column, matrix, random);

			//pfm(RL, "D-ref.pfm");


			//pfm(L, "L.pfm");
			//S = abs(S);

			//Blue(D, S, sparse_column);
			//pfm(S, "S.pfm");
			//Clear(D, sparse_column, matrix);
			//pfm(D, "D.pfm");



			Float e0 = norm(RL - L, "fro") / norm(RL, "fro");
			Fvec l_v = arma::sum(L, 1);
			Fvec rl_v = arma::sum(RL, 1);
			Float e1 = norm(rl_v - l_v, "fro") / norm(rl_v, "fro");

			Spectrum c(0.0f);

			e0 = std::min(1.0f, e0);
			e1 = std::min(1.0f, e1);
			c[0] = e0;
			c[1] = e1;
			c[2] = iteration_time;

			if (norm(RL, "fro") == 0) {
				c[0] = c[1] = 0;
			}

			

			//cout << e0 << " " << e1 << " " << iteration_time << endl;

			for (auto &row : matrix.rows) {
				for (auto &s : row.spp_sensors) {
					result->put(s.sensor, c);
				}
			}

			return;
		}
#endif


#if 0
		{
			if (std::min(D.n_rows, D.n_cols) < 8) {
				Spectrum c(0.1);

				for (auto &row : matrix.rows) {
					for (auto &s : row.spp_sensors) {
						result->put(s.sensor, c);
					}
				}
				return;

			}
			//getTrue(D, sparse_column, matrix, random);
			//D = D.rows(400, 800);
			//D = D.cols(0, 99);
			//{

			//	Fmat qd(D.n_rows, D.n_cols * 2);

			//	for (int i = 0; i < D.n_cols; i++) {
			//		qd.col(i * 2) = D.col(i);
			//		qd.col(i * 2 + 1) = D.col(i);
			//	}

			//	//	D = qd;

			//}
			Fmat U, V;
			Fvec S;
			int rr = 0;
			{
				arma::svd_econ(U, S, V, L);
				Uvec f = arma::find(S > 0.01 * sum(S));
				rr = f.n_elem;
			}
			
			

			//tlah::redsvd(U, S, V, D, 32);
			arma::svd_econ(U, S, V, D);

			int r = rr;

			r = std::max(1, r);
			
			//system("pause");


			U = U.cols(0, r - 1);
			V = V.cols(0, r - 1);
			//U = U.rows(0, 25);
			Fmat SS = U * U.t();

			Float mu = 1.0f / (std::log10((Float)std::max(D.n_rows, D.n_cols)));
			mu *= mu;

			mu *= (0.5 * std::min(D.n_rows, D.n_cols) / r);

			Float mu1 = mu * r / U.n_rows;

			int coherence = 0;
			for (int i = 0; i < U.n_rows; i++) {
				Float n = norm(U.row(i), 2);
				n *= n;
				s += n;
				//if (n > mu1)
				//cout << i << " " << n << " " << U.row(i) << endl;
				//if (i % 100 == 0) {
				//	system("pause");
				//}
				if (n > mu1) coherence++;

			}

			//r = std::min(D.n_rows, D.n_cols);
			coherence = 0;
			Float mu2 = mu * r / V.n_rows;
			for (int i = 0; i < V.n_rows; i++) {
				Float n = norm(V.row(i), 2);
				n *= n;
				s += n;

				if (n > mu2) coherence++;
			}

			Float p = (Float)coherence / sparse_column.size();

			Spectrum c(p);

			for (auto &row : matrix.rows) {
				for (auto &s : row.spp_sensors) {
					result->put(s.sensor, c);
				}
			}

		}

		return;

		//cout << D.n_rows << " " << D.n_cols << endl;
		//D.save("debug.txt", arma::raw_ascii);
		//return;
#endif



		//

		//denoise_2(matrix, L, S, sparse_column, shadowCount, random);
		

		//int n_valid = 0; Float sumin = 0;
		//for (int i = 0; i < D.n_cols; i++) {
		//	if (sparse_column[i]->max_eval > 0) sumin += sparse_column[i]->avg_intensity;

		//	D.at(0, i) = -sparse_column[i]->max_eval / matrix.sum_avg_eval;
		//	D.at(1, i) = -sparse_column[i]->error;
		//	D.at(3, i) = -sparse_column[i]->max_eval;

		//	Float v = sparse_column[i]->expand_times;
		//	D.at(5, i) = -v;
		//	D.at(10, i) = -sparse_column[i]->avg_intensity / 10;
		//	D.at(15, i) = -sparse_column[i]->expand_col_id;// : -sparse_column[i]->col_id;
		//	D.at(20, i) = -sparse_column[i]->highest_cost_now;
		//	D.at(21, i) = -sparse_column[i]->max_eval;
		//	D.at(22, i) = -sparse_column[i]->max_est * sparse_column[i]->avg_intensity;
		//	if (sparse_column[i]->max_eval > 0) {
		//		n_valid++;
		//	}
		//}
		//cout << "sum visible intensity " << sumin << endl;
		//cout << "num valid " << n_valid << endl;



// 		Fmat D_split = D;
// 		for (int c = 0; c < sparse_column.size(); c++) {
// 			if (sparse_column[c]->p_selection == Column::PKNN) {
// 				D_split.col(c) *= -1;
// 			}
// 			else if (sparse_column[c]->p_selection == Column::PLMS) {
// 				D_split.col(c) += 1;
// 			}
// 		}
// 		pfm(D_split, "D-split.pfm");
// 
		//Fmat RL = L;
		//getTrue(RL, sparse_column, matrix, random);
		//L = RL;


		//pfm(RL, "D-ref.pfm");

		//Uvec f = arma::find(arma::abs(RL - D) > 0);
		//cout << "error ratio " << (Float)f.n_elem / D.n_elem << endl;

		//pfm(L, "L.pfm");
		//S = abs(S);

		//Blue(D, S, sparse_column);
		//pfm(S, "S.pfm");
		//Clear(D, sparse_column, matrix);
		//pfm(D, "D.pfm");

		//{

		//	singular_column.clear();

		//	for (int i = 0; i < L.n_elem; i++) {
		//		if (random->nextFloat() > 0.02) {

		//			L.at(i) = 0;
		//		}
		//	}

		//}




		//Log(EWarn, "begin fill up [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		Fmat &recoveredMat = L;
		if (special) recoveredMat = D;
		//fillResult(matrix, recoveredMat, sparse_column, singular_column, result, scene, random);
		fillResult_spp(matrix, recoveredMat, sparse_column, singular_column, result, scene, random);

		//int cc = 0;
		//int qq = 0;
		//for (auto &ll : sparse_column) {

		//	const VLightNode *l = ll->lightNode;
		//	for (auto &row : matrix.rows) {
		//		for (auto &s : row.spp_sensors) {
		//			uint32_t f;
		//			Spectrum v = evalMTDV(l, s.sensor, scene, f) * s.sensor->strength  *ll->spur_intensity;
		//			result->put(s.sensor, v);
	
		//		}
		//	}
		//	//cc++;
		//}

		

		//t0->stop();
	}

	

	void renderSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *result) 
	{
		
		//if (TAABB<Point2i>(Point2i(280, 400), Point2i(400, 450)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(158, 597), Point2i(352, 643)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(559, 734), Point2i(584, 768)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(361, 288), Point2i(414, 304)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(681, 606), Point2i(742, 627)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(25, 38), Point2i(65, 113)).overlaps(slice_root->sampleBbox) ||
		//	//TAABB<Point2i>(Point2i(581, 191), Point2i(625, 225)).overlaps(slice_root->sampleBbox) ||
		//	0)
		//{

		//}
		//else {
		//	return;
		//}

		ref<Timer> t0 = new Timer(false);
		ref<Timer> t1 = new Timer();
		//Log(EWarn, "in slice [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);
		SliceLightRecord record(slice_root);
		uint32_t shadow_count;
		test(record, shadow_count, scene, sensor, sampler, stop, slice_root, result, t0);

		UniqueLock lock(m_resultMutex);
		m_shadow_count += shadow_count;

		//m_slice_record.insert(std::make_pair(slice_root, record));
		//return;
		//Log(EWarn, "out of slice [%u %u, %u %u]", slice_root->sampleBbox.min.x, slice_root->sampleBbox.min.y, slice_root->sampleBbox.max.x, slice_root->sampleBbox.max.y);

		//cout << t1->getMicrosecondsSinceStart() << " " << (Float)t0->getMicroseconds() / t1->getMicrosecondsSinceStart() << " " << slice_root->sampleBbox.toString() << endl;
	}

// 
// 	void postprocessSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *result)
// 	{
// 		return;
// 		bool special = false;
// 		if (slice_root->sampleBbox.contains(Point2i(1022, 1567)) ||
// 			//slice_root->sampleBbox.contains(Point2i(307, 266)) ||
// 			//TAABB<Point2i>(Point2i(937, 1532), Point2i(1120, 1597)).overlaps(sn->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(847, 1291), Point2i(935, 1342)).overlaps(slice_root->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(459, 312), Point2i(463, 314)) == (slice_root->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(1081, 1312), Point2i(1120, 1345)).overlaps(slice_root->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(1176, 1296), Point2i(1259, 1361)).overlaps(slice_root->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(1155, 1275), Point2i(1178, 1290)).overlaps(slice_root->sampleBbox) ||
// 			//TAABB<Point2i>(Point2i(982, 1551), Point2i(1047, 1584)).overlaps(slice_root->sampleBbox) ||
// 			0) {
// 			special = true;
// 			//	sample_ratio *= 5;
// 		}
// 		//if (!special) return;
// 
// 		//cout << slice_root->sampleBbox.toString() << endl;
// 
// 
// 		//return;
// 		uint32_t sc = 0;
// 
// 		std::map<const VSensorNode *, SliceLightRecord>::iterator it = m_slice_record.find(slice_root);
// 		assert(it != m_slice_record.end());
// 		SliceLightRecord &my_record = it->second;
// 		
// 
// 		std::vector<const VSensor *> sensors;
// 		sensors.reserve(slice_root->num_sensor);
// 
// 		std::vector<const VSensorNode *> stack;
// 		stack.reserve(slice_root->num_sensor);
// 		stack.push_back(slice_root);
// 
// 		while (!stack.empty()) {
// 			const VSensorNode *cur = stack.back();
// 			stack.pop_back();
// 
// 			if (cur->isLeaf()) {
// 				sensors.push_back(cur->rep_sensor);
// 			}
// 			else {
// 				stack.push_back(cur->left);
// 				stack.push_back(cur->right);
// 			}
// 		}
// 
// 		assert(sensors.size() == slice_root->num_sensor);
// 	
// 		if (my_record.invisible_lights.size() == 0) {
// 			return;
// 		}
// 
// 		std::vector < std::pair < const VSensorNode*, int > > adj_slice;
// 		m_sensor_map->calAdjSlices(slice_root, adj_slice, m_sliceSize);
// 		for (auto s : adj_slice) {
// 			//cout << s.first->sampleBbox.toString() << endl;
// 		}
// 
// 		//cout << adj_slice.size() << " " << my_record.invisible_lights.size() << endl;
// 
// 		std::vector<bool> vpl_to_test(m_vpl_map->vplSize(), false);
// 
// 		for (auto lightNode : my_record.invisible_lights) {
// 			vpl_to_test[m_vpl_map->getIndex(lightNode->rep_light)] = true;
// 		}
// 		
// 		std::vector<Spectrum> eval(sensors.size(), Spectrum(0.0f));
// 
// 		int l_count = 0;
// 		int v_count = 0;
// 		for (auto adj : adj_slice) {
// 
// 			const VSensorNode *vs = adj.first;
// 			std::map<const VSensorNode *, SliceLightRecord>::iterator it = m_slice_record.find(vs);
// 			assert(it != m_slice_record.end());
// 
// 			SliceLightRecord &the_record = it->second;
// 			for (auto lightNode : the_record.visible_lights) {
// 
// 				int l_id = m_vpl_map->getIndex(lightNode->rep_light);
// 				
// 				if (!vpl_to_test[l_id]) continue;
// 
// 				l_count++;
// 				Float m = 0;
// 				for (int r = 0; r < sensors.size(); r++) {
// 
// 					Spectrum v = evalMTDV(lightNode, sensors[r], scene, sc);
// 					m = std::max(v.average(), m);
// 					eval[r] += v * lightNode->intensity;
// 
// 				}
// 
// 				if (m > 0) {
// 					v_count++;
// 				}
// 
// 				vpl_to_test[l_id] = false;
// 			}
// 
// 		}
// 		//cout << adj_slice.size() << " " << l_count << " " << v_count << endl;
// 		//return;
// 		//for (auto lightNode : my_record.invisible_lights) {
// 
// 		//	for (int r = 0; r < sensors.size(); r++) {
// 		//		Spectrum v = evalMTDV(lightNode, sensors[r], scene, sc);
// 		//		eval[r] += v * lightNode->intensity;
// 		//	}
// 
// 		//}
// 
// 
// 		for (int r = 0; r < sensors.size(); r++) {
// 			result->put(sensors[r], eval[r] * sensors[r]->strength);
// 		}
// 
// 
// 	}


	MTS_DECLARE_CLASS()
protected:
	virtual ~LCMSIntegrator() {

	}

	
	uint32_t m_maxCutsize;
	Float m_error_threshold;
	Float m_sample_ratio;
	Float m_denoise_threshold;
	Float m_singular_threshold;
	uint32_t m_minimum_sample;

	uint32_t m_standard_column_size;

	ref<Mutex> m_sc_mutex;
	uint32_t m_shadow_count;
	

	std::map<const VSensorNode *, SliceLightRecord> m_slice_record;

	AOMap m_ao;
};


MTS_IMPLEMENT_CLASS(LCMSIntegrator, false, Integrator)
MTS_EXPORT_PLUGIN(LCMSIntegrator, "Lightcut matrix separation integrator");
MTS_NAMESPACE_END

#endif
