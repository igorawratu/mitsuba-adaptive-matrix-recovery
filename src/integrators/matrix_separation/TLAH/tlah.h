#ifndef __TLAH_H_
#define __TLAH_H_

#define ARMA_USE_LAPACK
#define ARMA_USE_BLAS
#define ARMA_USE_NEWARP

//#define ARMA_DONT_USE_LAPACK
//#define ARMA_DONT_USE_BLAS
//#define ARMA_DONT_USE_NEWARP

#include <stdint.h>
#include <armadillo>

#ifdef SINGLE_PRECISION

namespace tlah {
	typedef float Float;
	typedef arma::fmat Fmat;
	typedef arma::sp_fmat sp_Fmat;
	typedef arma::fvec Fvec;
	typedef arma::uvec Uvec;


}

#elif DOUBLE_PRECISION

namespace tlah {
	typedef double Float;
	typedef arma::mat mat;
	typedef arma::sp_mat sp_mat;
}

#else
#error need precision definition
#endif

using tlah::Fmat;
using tlah::sp_Fmat;
using tlah::Fvec;
using tlah::Uvec;
#ifndef Float
using tlah::Float;
#endif


#ifdef _MSC_VER
	#define T_EXPORT __declspec(dllexport)
	#define T_IMPORT __declspec(dllimport)
	#define T_MAY_ALIAS // not supported on Windows
#else
	#define T_EXPORT __attribute__ ((visibility("default")))
	#define T_IMPORT
	#define T_MAY_ALIAS __attribute__ ((__may_alias__))

	#include <stdint.h>

	#define SIZE_T_FMT "%zd"
#endif

#ifdef TLAH_DLL_EXPORT
	#define TLAH_EXPORT T_EXPORT
#else
	#define TLAH_EXPORT T_IMPORT
#endif



namespace tlah {

	bool TLAH_EXPORT PDMS(int &iteration_time, Fmat &L, Fmat &S, const Fmat &D, const arma::uvec &known, uint32_t maximum_rank, uint32_t max_iteration = 100, Float tolerance = 1e-5, Fmat *init_X = NULL, Fmat *init_Y = NULL);


}



#endif