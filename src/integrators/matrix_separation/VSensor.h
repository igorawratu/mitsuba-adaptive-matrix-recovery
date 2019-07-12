#ifndef __VSENSOR_H_
#define __VSENSOR_H_

#include "pluginExport.h"
// SCATTER CUBE CONFIGURE
#if PLUGIN_EXPORT != 35
#define CUBE_RESOLUTION 6
#endif

#define CUBE_RESOLUTION 6

#if PLUGIN_EXPORT == PLUGIN_EXPORT_BDLC
#define MULTIPLE_REPRESENTATIVE 1
#else
#define MULTIPLE_REPRESENTATIVE 1
#endif

#include <mitsuba/render/bsdf.h>
#include <mitsuba/render/shape.h>
#include <mitsuba/core/random.h>
#include "pool_ref.h"
#include "dircone.h"

MTS_NAMESPACE_BEGIN


template<typename T, int axis, int N>
struct element {

};

template<typename T>
struct element<T, 0, 0> {
	Float &operator()(T &v) const { return v.x; }
	const Float &operator()(const T &v) const { return v.x; }
};

template<typename T>
struct element<T, 0, 1> {
	Float &operator()(T &v) const { return v.y; }
	const Float &operator()(const T &v) const { return v.y; }
};

template<typename T>
struct element<T, 0, 2> {
	Float &operator()(T &v) const { return v.z; }
	const Float &operator()(const T &v) const { return v.z; }
};

template<typename T>
struct element<T, 1, 0> {
	Float &operator()(T &v) const { return v.y; }
	const Float &operator()(const T &v) const { return v.y; }
};

template<typename T>
struct element<T, 1, 1> {
	Float &operator()(T &v) const { return v.z; }
	const Float &operator()(const T &v) const { return v.z; }
};

template<typename T>
struct element<T, 1, 2> {
	Float &operator()(T &v) { return v.x; }
	const Float &operator()(const T &v) { return v.x; }
};

template<typename T>
struct element<T, 2, 0> {
	Float &operator()(T &v) const { return v.z; }
	const Float &operator()(const T &v) const { return v.z; }
};

template<typename T>
struct element<T, 2, 1> {
	Float &operator()(T &v) const { return v.x; }
	const Float &operator()(const T &v) const { return v.x; }
};

template<typename T>
struct element<T, 2, 2> {
	Float &operator()(T &v) const { return v.y; }
	const Float &operator()(const T &v) const { return v.y; }
};


inline Spectrum max(const Spectrum &aa, const Spectrum &b) {
	Spectrum a = aa;
	for (int i = 0; i < SPECTRUM_SAMPLES; i++) {
		a[i] = std::max(a[i], b[i]);
	}
	return a;
}

inline Float max(const Float &a, const Float &b) {
	return std::max(a, b);
}



template<typename T, uint32_t N>
struct CubeMapN {

	static const uint32_t resolution = N;
	static const uint32_t count = 6 * N * N;

	T face_maximum[6];
	T value[6][N][N];


	CubeMapN() {  }
	CubeMapN(const CubeMapN<T, N> &a) { memcpy(value, a.value, sizeof(T)* count); memcpy(face_maximum, a.face_maximum, sizeof(T)* 6); }

	CubeMapN(Stream *stream) {
		stream->read(face_maximum, sizeof(T)* 6);
		stream->read(value, sizeof(T)* count);
	}

	void serialize(Stream *stream) const {
		stream->write(face_maximum, sizeof(T)* 6);
		stream->write(value, sizeof(T)* count);
	}

	/*
	//Float &operator()(const Vector &dir) {
	//	const AABB aabb(Point(-1.0f), Point(1.0f));
	//	Ray ray(Point(0.0f), dir, 0.5f);

	//	Float nearT, farT;
	//	aabb.rayIntersect(ray, nearT, farT);

	//	Point p = Point(dir * nearT);
	//	int face, u, v;
	//	AABB tp(Point(0.0f));
	//	tp.expandBy(p);
	//	int la = tp.getLargestAxis();

	//	const Float inv_grid = (Float)N / 2.0f;
	//	if (la == 0) {
	//		face = p.x <= 0.0f ? 0 : 1;
	//		u = (p.y + 1.0f) * inv_grid;
	//		v = (p.z + 1.0f) * inv_grid;
	//	}

	//	if (la == 1) {
	//		face = p.y <= 0.0f ? 2 : 3;
	//		u = (p.x + 1.0f) * inv_grid;
	//		v = (p.z + 1.0f) * inv_grid;
	//	}

	//	if (la == 2) {
	//		face = p.z <= 0.0f ? 4 : 5;
	//		u = (p.x + 1.0f) * inv_grid;
	//		v = (p.y + 1.0f) * inv_grid;
	//	}

	//	u = std::min(u, N - 1);
	//	v = std::min(v, N - 1);

	//	return value[face][u][v];

	//}
	*/



	T evalMax(const AABB &bbox) const {
		
		T result(0.0f);
		
		if (max(face_maximum[0], result) != result) evalMaxWithAABB<0>(result, bbox);
		if (max(face_maximum[1], result) != result) evalMaxWithAABB<1>(result, bbox);
		if (max(face_maximum[2], result) != result) evalMaxWithAABB<2>(result, bbox);
		if (max(face_maximum[3], result) != result) evalMaxWithAABB<3>(result, bbox);
		if (max(face_maximum[4], result) != result) evalMaxWithAABB<4>(result, bbox);
		if (max(face_maximum[5], result) != result) evalMaxWithAABB<5>(result, bbox);

		return result;
	}

	T evalMax(const DirConef &light_cone) const {
		
		DirConef wo_cone;
		wo_cone._dirBox.reset();
		wo_cone._dirBox.expandBy(-light_cone._dirBox.min);
		wo_cone._dirBox.expandBy(-light_cone._dirBox.max);

		// extend to cube
		AABB cube;
		cube.reset();
		for (int i = 0; i < 8; i++) {
			Point p = wo_cone._dirBox.getCorner(i);
			
			Float e = std::max(std::max(fabs(p.x), fabs(p.y)), fabs(p.z));

			if (e < 0.99999f) {
				p /= e;
			}

			cube.expandBy(p);
		}

		T result(0.0f);

		if (max(face_maximum[0], result) != result) evalMaxWithAABB<0>(result, cube);
		if (max(face_maximum[1], result) != result) evalMaxWithAABB<1>(result, cube);
		if (max(face_maximum[2], result) != result) evalMaxWithAABB<2>(result, cube);
		if (max(face_maximum[3], result) != result) evalMaxWithAABB<3>(result, cube);
		if (max(face_maximum[4], result) != result) evalMaxWithAABB<4>(result, cube);
		if (max(face_maximum[5], result) != result) evalMaxWithAABB<5>(result, cube);

		return result;
	}

	void expandBy(const CubeMapN<T, N> &a) {

		for (uint32_t i = 0; i < 6; i++)
			face_maximum[i] = max(a.face_maximum[i], face_maximum[i]);

		for (uint32_t i = 0; i < 6; i++)
		for (uint32_t u = 0; u < N; u++)
		for (uint32_t v = 0; v < N; v++)
			value[i][u][v] = max(a.value[i][u][v], value[i][u][v]);
	}

	T sum() const {

		T s(0.0f);
		for (int i = 0; i < 6; i++) {
			for (int u = 0; u < N; u++) {
				for (int v = 0; v < N; v++) {
					s += value[i][u][v];
				}
			}
		}

		return s;
	}


	// generator
	template<typename G>
	void generate(G &generator) {

		generateAtAxis<G, 0>(generator);
		generateAtAxis<G, 1>(generator);
		generateAtAxis<G, 2>(generator);

	}


private:


	template<int face>
	void evalMaxWithAABB(T &spectrum, const AABB &bbox) const {

		const int axis = face >> 1;
		const int sign = (face & 1) ? 1 : -1;

		typedef TPoint2<Float> FacePoint;
		typedef TAABB<FacePoint> FaceAABB;

		FaceAABB yz_face_bound;
		TPoint2<int> face_grid_min, face_grid_max;
		const Float grid = 2.0f / N;

		element<AABB::PointType, axis, 0> visitor_x;
		element<AABB::PointType, axis, 1> visitor_y;
		element<AABB::PointType, axis, 2> visitor_z;

		// face 0: x = -1; face 1: x = 1; face 2: y = -1; face 3: y = 1; ...
		Float e0 = sign * visitor_x(bbox.min);
		Float e1 = sign * visitor_x(bbox.max);


		if (e0 < 0 && e1 < 0) {
			return;
		}

		yz_face_bound.reset();

		if (e0 > 0) {
			Float e = e0;
			yz_face_bound.expandBy(FacePoint(visitor_y(bbox.min), visitor_z(bbox.min)) / e);
			yz_face_bound.expandBy(FacePoint(visitor_y(bbox.max), visitor_z(bbox.max)) / e);
		}
		
		if (e1 > 0) {
			Float e = e1;
			yz_face_bound.expandBy(FacePoint(visitor_y(bbox.min), visitor_z(bbox.min)) / e);
			yz_face_bound.expandBy(FacePoint(visitor_y(bbox.max), visitor_z(bbox.max)) / e);
		}

		if (e0 * e1 < 0) {
			if (visitor_y(bbox.min) <= 0) {
				yz_face_bound.min.x = -2.0f; // less than -1.0f to avoid numerical error
			}

			if (visitor_y(bbox.max) >= 0) {
				yz_face_bound.max.x = 2.0f;
			}

			if (visitor_z(bbox.min) <= 0) {
				yz_face_bound.min.y = -2.0f;
			}

			if (visitor_z(bbox.max) >= 0) {
				yz_face_bound.max.y = 2.0f;
			}
		}
	
		face_grid_min.x = std::max((int)floor((yz_face_bound.min.x + 1.0f) / grid), 0);
		face_grid_min.y = std::max((int)floor((yz_face_bound.min.y + 1.0f) / grid), 0);

		face_grid_max.x = std::min((int)ceil((yz_face_bound.max.x + 1.0f) / grid), (int)N);
		face_grid_max.y = std::min((int)ceil((yz_face_bound.max.y + 1.0f) / grid), (int)N);

		for (int u = face_grid_min.x; u < face_grid_max.x; u++) {
			for (int v = face_grid_min.y; v < face_grid_max.y; v++) {
				spectrum = max(spectrum, value[face][u][v]);
			}
		}


	}

	template<int face>
	void evalMaxWithCube(T &spectrum, const AABB &cube) const {

		const int axis = face >> 1;
		const int sign = (face & 1) ? 1 : -1;

		element<AABB::PointType, axis, 0> visitor_x;
		element<AABB::PointType, axis, 1> visitor_y;
		element<AABB::PointType, axis, 2> visitor_z;

		typedef TPoint2<Float> FacePoint;
		typedef TAABB<FacePoint> FaceAABB;

		FaceAABB yz_face_bound;
		yz_face_bound.reset();

		if (sign < 0) { // face 0, 2, 4

			if (fabs(visitor_x(cube.min)) > 0.99999f) {
				yz_face_bound.expandBy(visitor_y(cube.min), visitor_z(cube.min));
				yz_face_bound.expandBy(visitor_y(cube.max), visitor_z(cube.max));
			}
			else {
				return;
			}
			
		}
		else { // face 1, 3, 5

			if (fabs(visitor_x(cube.max)) > 0.99999f) {
				yz_face_bound.expandBy(FacePoint(visitor_y(cube.min), visitor_z(cube.min)));
				yz_face_bound.expandBy(FacePoint(visitor_y(cube.max), visitor_z(cube.max)));
			}
			else {
				return;
			}

		}

		TPoint2<int> face_grid_min, face_grid_max;
		const Float grid = 2.0f / N;


		face_grid_min.x = std::min(std::max((int)((yz_face_bound.min.x + 1.0f) / grid), 0), (int)N - 1);
		face_grid_min.y = std::min(std::max((int)((yz_face_bound.min.y + 1.0f) / grid), 0), (int)N - 1);

		face_grid_max.x = std::min(std::max((int)((yz_face_bound.max.x + 1.0f) / grid), 0), (int)N - 1);
		face_grid_max.y = std::min(std::max((int)((yz_face_bound.max.y + 1.0f) / grid), 0), (int)N - 1);

		for (int u = face_grid_min.x; u <= face_grid_max.x; u++) {
			for (int v = face_grid_min.y; v <= face_grid_max.y; v++) {
				spectrum = max(spectrum, value[face][u][v]);
			}
		}


	}


	template<typename G, int axis>
	void generateAtAxis(G &generator) {

		const int face0 = axis << 1;
		const int face1 = face0 + 1;

		element<Vector, axis, 0> visitor_x;
		element<Vector, axis, 1> visitor_y;
		element<Vector, axis, 2> visitor_z;

		Vector dir;
		const Float grid = 2.0f / N;
		const Float half_grid = grid / 2;
		const Float init_grid = -1.0f - half_grid;

		face_maximum[face0] = T(0.0f);
		face_maximum[face1] = T(0.0f);

		visitor_y(dir) = init_grid;
		for (int u = 0; u < N; u++) {
			visitor_y(dir) += grid;
			visitor_z(dir) = init_grid;
			for (int v = 0; v < N; v++) {
				visitor_z(dir) += grid;

				visitor_x(dir) = -1.0f;
				T v0 = generator(dir);
				value[face0][u][v] = v0;
				face_maximum[face0] = max(face_maximum[face0], v0);

				visitor_x(dir) = 1.0f;
				T v1 = generator(dir);
				value[face1][u][v] = v1;
				face_maximum[face1] = max(face_maximum[face1], v1);

			}
		}
	}

#if 0
	Spectrum evalMax_old(const AABB &bbox) const {

		Spectrum m(0.0f);

		Vector dir;
		Ray ray(Point(0.0f), 0.5);
		const Float grid = 2.0f / N;
		const Float init_grid = -1.0f - 0.5f * grid;

		Float nearT, farT;

		if (bbox.min.x <= 0.0f) {
			dir.x = -1.0f;
			dir.y = init_grid;
			for (int u = 0; u < N; u++) {
				dir.y += grid;
				dir.z = init_grid;
				for (int v = 0; v < N; v++) {
					dir.z += grid;
					if (SpectrumLess(m, value[0][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[0][u][v]);
						}
					}

				}
			}
		}


		if (bbox.max.x >= 0.0f) {
			dir.x = 1.0f;
			dir.y = init_grid;
			for (int u = 0; u < N; u++) {
				dir.y += grid;
				dir.z = init_grid;
				for (int v = 0; v < N; v++) {
					dir.z += grid;
					if (SpectrumLess(m, value[1][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[1][u][v]);
						}
					}

				}
			}
		}

		if (bbox.min.y <= 0.0f) {
			dir.y = -1.0f;
			dir.x = init_grid;
			for (int u = 0; u < N; u++) {
				dir.x += grid;
				dir.z = init_grid;
				for (int v = 0; v < N; v++) {
					dir.z += grid;
					if (SpectrumLess(m, value[2][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[2][u][v]);
						}
					}

				}
			}
		}

		if (bbox.max.y >= 0.0f) {
			dir.y = 1.0f;
			dir.x = init_grid;
			for (int u = 0; u < N; u++) {
				dir.x += grid;
				dir.z = init_grid;
				for (int v = 0; v < N; v++) {
					dir.z += grid;
					if (SpectrumLess(m, value[3][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[3][u][v]);
						}
					}

				}
			}
		}


		if (bbox.min.z <= 0.0f) {
			dir.z = -1.0f;
			dir.x = init_grid;
			for (int u = 0; u < N; u++) {
				dir.x += grid;
				dir.y = init_grid;
				for (int v = 0; v < N; v++) {
					dir.y += grid;
					if (SpectrumLess(m, value[4][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[4][u][v]);
						}
					}

				}
			}
		}

		if (bbox.min.z >= 0.0f) {
			dir.z = 1.0f;
			dir.x = init_grid;
			for (int u = 0; u < N; u++) {
				dir.x += grid;
				dir.y = init_grid;
				for (int v = 0; v < N; v++) {
					dir.y += grid;
					if (SpectrumLess(m, value[5][u][v])) {
						ray.setDirection(dir);
						if (bbox.rayIntersect(ray, nearT, farT) && farT > ray.mint) {
							SpectrumExpand(m, value[5][u][v]);
						}
					}

				}
			}
		}

		return m;
	}
#endif
};


typedef CubeMapN<Float, CUBE_RESOLUTION> CubeMap;
// ===========================================================================================================================

template <typename T, uint32_t N> struct TVectorN;
template <typename T, uint32_t N> struct TPointN;

/**
* based on TVectorN<T, N>
*/
template <typename T, uint32_t N> struct TVectorN {
	typedef T          Scalar;
	typedef TPointN<T, N> PointType;
	typedef typename detail::VectorLength<T, std::numeric_limits<T>::is_integer>::type LengthType;

	T data[N];

	/// Number of dimensions
	const static int dim = N;

	/** \brief Construct a new vector without initializing it.
	*
	* This construtor is useful when the vector will either not
	* be used at all (it might be part of a larger data structure)
	* or initialized at a later point in time. Always make sure
	* that one of the two is the case! Otherwise your program will do
	* computations involving uninitialized memory, which will probably
	* lead to a difficult-to-find bug.
	
	*/
#if !defined(MTS_DEBUG_UNINITIALIZED)
	TVectorN<T, N>() { }
#else
	TVectorN<T, N>() { for (int inten = 0; inten < N; inten++) data[inten] = std::numeric_limits<T>::quiet_NaN(); }
#endif

	/// Initialize the vector with the specified X, Y and Z components
	TVectorN<T, N>(T _d[N]) { for (int inten = 0; inten < N; inten++) data[inten] = _d[inten]; }

	/// Initialize all components of the the vector with the specified value
	explicit TVectorN<T, N>(T val) { for (int inten = 0; inten < N; inten++) data[inten] = val; }

	/// Initialize the vector with the components of another vector data structure
	template <typename T2> explicit TVectorN<T, N>(const TVectorN<T2, N> &v) {
		for (int inten = 0; inten < N; inten++) data[inten] = (T)v[inten];
	}

	/// Initialize the vector with the components of a point data structure
	template <typename T2> explicit TVectorN<T, N>(const TPoint4<T2> &p)
	{
		for (int inten = 0; inten < N; inten++) data[inten] = p[inten];
	}

	/// Unserialize a vector from a binary data stream
	explicit TVectorN<T, N>(Stream *stream) {
		for (int inten = 0; inten < N; inten++) {
			data[inten] = stream->readElement<T>();
		}
	}

	/// Add two vectors and return the result
	TVectorN<T, N> operator+(const TVectorN<T, N> &v) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] + v[inten];
		}
		return r;
	}

	/// Subtract two vectors and return the result
	TVectorN<T, N> operator-(const TVectorN<T, N> &v) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] - v[inten];
		}
		return r;
	}

	/// Add another vector to the current one
	TVectorN<T, N>& operator+=(const TVectorN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) {
			data[inten] += v[inten];
		}
		return *this;
	}

	/// Subtract another vector from the current one
	TVectorN<T, N>& operator-=(const TVectorN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) {
			data[inten] -= v[inten];
		}
		return *this;
	}

	/// Multiply the vector by the given scalar and return the result
	TVectorN<T, N> operator*(T f) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] * f;
		}
		return r;
	}

	/// Multiply the vector by the given vector and return the result
	TVectorN<T, N> operator*(TVectorN<T, N> v) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] * v[inten];
		}
		return r;
	}

	/// Multiply the vector by the given scalar
	TVectorN<T, N> &operator*=(T f) {
		for (int inten = 0; inten < N; inten++) {
			data[inten] *= f;
		}
		return *this;
	}

	/// Return a negated version of the vector
	TVectorN<T, N> operator-() const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) r[inten] = -data[inten];
		return r;
	}

	/// Divide the vector by the given scalar and return the result
	TVectorN<T, N> operator/(T f) const {
#ifdef MTS_DEBUG
		if (f == 0)
			SLog(EWarn, "Vector4: Division by zero!");
#endif
		TVectorN<T, N> r;
		T recip = (T)1 / f;
		for (int inten = 0; inten < N; inten++) r[inten] = data[inten] * recip;
		return r;
	}

	/// Divide the vector by the given scalar
	TVectorN<T, N> &operator/=(T f) {
#ifdef MTS_DEBUG
		if (f == 0)
			SLog(EWarn, "Vector4: Division by zero!");
#endif
		T recip = (T)1 / f;
		for (int inten = 0; inten < N; inten++) data[inten] *= recip;
		return *this;
	}

	/// Index into the vector's components
	T &operator[](int intensity) {
		return data[intensity];
	}

	/// Index into the vector's components (const version)
	T operator[](int inten) const {
		return data[inten];
	}

	/// Return the squared 2-norm of this vector
	T lengthSquared() const {
		T f = 0;
		for (int inten = 0; inten < N; inten++) f += data[inten] * data[inten];
		return f;
	}

	/// Return the 2-norm of this vector
	LengthType length() const {
		return (LengthType)std::sqrt((LengthType)lengthSquared());
	}

	/// Return whether or not this vector is identically zero
	bool isZero() const {
		for (int inten = 0; inten < N; inten++)
		{
			if (data[inten] != 0) return false;
		}
		return true;
	}

	/// Equality test
	bool operator==(const TVectorN<T, N> &v) const {
		for (int inten = 0; inten < N; inten++)
		{
			if (data[inten] != v[inten]) return false;
		}
		return true;
	}

	/// Inequality test
	bool operator!=(const TVectorN<T, N> &v) const {
		return !(*this == v);
	}

	/// Serialize this vector to a binary data stream
	void serialize(Stream *stream) const {
		for (int inten = 0; inten < N; inten++) stream->writeElement<T>(data[inten]);
	}

	/// Return a readable string representation of this vector
	std::string toString() const {
		std::ostringstream oss;
		oss << "[";
		if (N > 0) {
			oss << data[0];
		}
		for (int inten = 1; inten < N; inten++)
		{
			oss << ", " << data[inten];
		}
		oss << "]";
		return oss.str();
	}


};

template <typename T, uint32_t N> inline TVectorN<T, N> operator*(T f, const TVectorN<T, N> &v) {
	return v*f;
}

template <typename T, uint32_t N> inline T dot(const TVectorN<T, N> &v1, const TVectorN<T, N> &v2) {
	T f = 0;
	for (int inten = 0; inten < N; inten++) f += v1[inten] * v2[inten];
	return f;
}

template <typename T, uint32_t N> inline T absDot(const TVectorN<T, N> &v1, const TVectorN<T, N> &v2) {
	return std::abs(dot(v1, v2));
}

template <typename T, uint32_t N> inline TVectorN<T, N> abs(const TVectorN<T, N> &v) {
	TVectorN<T, N> r;
	for (int inten = 0; inten < N; inten++) r[inten] = std::abs(v[inten]);
	return r;
}

template <typename T, uint32_t N> inline TVectorN<T, N> normalize(const TVectorN<T, N> &v) {
	return v / v.length();
}

template <typename T, uint32_t N> inline TVectorN<T, N> normalizeStrict(const TVectorN<T, N> &v, const char *errMsg) {
	Float length = v.length();
	if (length == 0)
		SLog(EError, "normalizeStrict(): %s", errMsg);
	return v / length;
}

//template <uint32_t N> inline TVectorN<int, N> TVectorN<int, N>::operator/(int s) const {
//#ifdef MTS_DEBUG
//	if (s == 0)
//		SLog(EWarn, "Vector4i: Division by zero!");
//#endif
//	TVectorN<int, N> r;
//	for (int i = 0; i < N; i++) r[i] = data[i] / s;
//	return r;
//}
//
//template <uint32_t N> inline TVectorN<int, N> &TVectorN<int, N>::operator/=(int s) {
//#ifdef MTS_DEBUG
//	if (s == 0)
//		SLog(EWarn, "Vector4i: Division by zero!");
//#endif
//
//	for (int i = 0; i < N; i++) data[i] /= s;
//	return *this;
//}




/**
* based on TPoint4
*/
template <typename T, uint32_t N> struct TPointN {
	typedef T           Scalar;
	typedef TVectorN<T, N> VectorType;

	T data[N];

	/// Number of dimensions
	const static int dim = N;


#if !defined(MTS_DEBUG_UNINITIALIZED)
	TPointN() { }
#else
	TPointN() { for (int inten = 0; inten < N; inten++) data[inten] = std::numeric_limits<T>::quiet_NaN(); }
#endif

	/// Initialize the point with the specified X, Y and Z components
	TPointN(T *d) { for (int inten = 0; inten < N; inten++) data[inten] = d[inten]; }

	/// Initialize the point with the components of another point
	template <typename T2> explicit TPointN(const TPointN<T2, N> &p)
	{
		for (int inten = 0; inten < N; inten++) data[inten] = (T)p[inten];
	}

	/// Initialize the point with the components of a vector data structure
	template <typename T2> explicit TPointN(const TVectorN<T2, N> &v)
	{
		for (int inten = 0; inten < N; inten++) data[inten] = (T)v[inten];
	}

	/// Initialize all components of the the point with the specified value
	explicit TPointN(T val) { for (int inten = 0; inten < N; inten++) data[inten] = val; }

	/// Unserialize a point from a binary data stream
	explicit TPointN(Stream *stream) {
		for (int inten = 0; inten < N; inten++)
			data[inten] = stream->readElement<T>();
	}

	/// Add a vector to a point and return the result
	TPointN<T, N> operator+(const TVectorN<T, N> &v) const {
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = v[inten] + data[inten];
		}
		return r;
	}

	/// Add two points and return the result (e.g. to compute a weighted position)
	TPointN<T, N> operator+(const TPointN<T, N> &v) const {
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = v[inten] + data[inten];
		}
		return r;
	}

	/// Add a vector to this one (e.g. to compute a weighted position)
	TPointN<T, N>& operator+=(const TVectorN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) data[inten] += v[inten];
		return *this;
	}

	/// Add a point to this one (e.g. to compute a weighted position)
	TPointN<T, N>& operator+=(const TPointN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) data[inten] += v[inten];
		return *this;
	}

	/// Subtract a vector from this point
	TVectorN<T, N> operator-(const TPointN<T, N> &v) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] - v[inten];
		}
		return r;
	}

	/// Subtract two points from each other and return the difference as a vector
	TVectorN<T, N> operator-(const TVectorN<T, N> &p) const {
		TVectorN<T, N> r;
		for (int inten = 0; inten < N; inten++) r[inten] = data[inten] - p[inten];
		return r;
	}

	/// Subtract a vector from this point
	TPointN<T, N>& operator-=(const TPointN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) data[inten] -= v[inten];
		return *this;
	}

	/// Scale the point's coordinates by the given scalar and return the result
	TPointN<T, N> operator*(T f) const {
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) r[inten] = data[inten] * f;
		return r;
	}

	/// Multiply the point by the given point and return the result
	TPointN<T, N> operator*(TPointN<T, N> v) const {
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) {
			r[inten] = data[inten] * v[inten];
		}
		return r;
	}

	/// Scale the point's coordinates by the given scalar
	TPointN<T, N> &operator*=(T f) {
		for (int inten = 0; inten < N; inten++) data[inten] *= f;
		return *this;
	}

	TPointN<T, N> &operator*=(const TPointN<T, N> &v) {
		for (int inten = 0; inten < N; inten++) data[inten] *= v[inten];
		return *this;
	}

	/// Return a version of the point, which has been flipped along the origin
	TPointN<T, N> operator-() const {
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) r[inten] = -data[inten];
		return r;
	}

	/// Divide the point's coordinates by the given scalar and return the result
	TPointN<T, N> operator/(T f) const {
#ifdef MTS_DEBUG
		if (f == 0)
			SLog(EWarn, "Point4: Division by zero!");
#endif
		T recip = (T)1 / f;
		TPointN<T, N> r;
		for (int inten = 0; inten < N; inten++) r[inten] = data[inten] * recip;
		return r;
	}

	/// Divide the point's coordinates by the given scalar
	TPointN<T, N> &operator/=(T f) {
#ifdef MTS_DEBUG
		if (f == 0)
			SLog(EWarn, "Point4: Division by zero!");
#endif
		T recip = (T)1 / f;
		for (int inten = 0; inten < N; inten++) data[inten] *= recip;
		return *this;
	}

	/// Index into the point's components
	T &operator[](int intensity) {
		return (data)[intensity];
	}

	/// Index into the point's components (const version)
	T operator[](int inten) const {
		return (data)[inten];
	}

	/// Return whether or not this point is identically zero
	bool isZero() const {
		for (int inten = 0; inten < N; inten++)
		{
			if (data[inten] != 0) return false;
		}
		return true;
	}

	/// Equality test
	bool operator==(const TPointN<T, N> &v) const {
		for (int inten = 0; inten < N; inten++)
		{
			if (data[inten] != v[inten]) return false;
		}
		return true;
	}

	/// Inequality test
	bool operator!=(const TPointN<T, N> &v) const {
		return !(*this == v);
	}

	/// Serialize this point to a binary data stream
	void serialize(Stream *stream) const {
		for (int inten = 0; inten < N; inten++) stream->writeElement<T>(data[inten]);
	}

	/// Return a readable string representation of this point
	std::string toString() const {
		std::ostringstream oss;
		oss << "[";
		if (N > 0) {
			oss << data[0];
		}
		for (int inten = 1; inten < N; inten++)
		{
			oss << ", " << data[inten];
		}
		oss << "]";
		return oss.str();
	}
};

template <typename T, uint32_t N> inline TPointN<T, N> operator*(T f, const TPointN<T, N> &v) {
	return v*f;
}

template <typename T, uint32_t N> inline TVectorN<T, N> operator*(const TPointN<T, N> &p, const TVectorN<T, N> &v) {
	TVectorN<T, N> r;
	for (int inten = 0; inten < N; inten++) r[inten] = p[inten] * v[inten];
	return r;
}

template <typename T, uint32_t N> inline T distance(const TPointN<T, N> &p1, const TPointN<T, N> &p2) {
	return (p1 - p2).length();
}

template <typename T, uint32_t N> inline TPointN<T, N> abs(const TPointN<T, N> &v) {
	TPointN<T, N> r;
	for (int inten = 0; inten < N; inten++) r[inten] = std::abs(v[inten]);
	return r;
}

template <typename T, uint32_t N> inline T distanceSquared(const TPointN<T, N> &p1, const TPointN<T, N> &p2) {
	return (p1 - p2).lengthSquared();
}

//template <> inline TPointN<int, N> TPointN<int, N>::operator/(int s) const {
//#ifdef MTS_DEBUG
//	if (s == 0)
//		SLog(EWarn, "Point4i: Division by zero!");
//#endif
//	
//	TPointN<int, N> r;
//	for (int i = 0; i < N; i++) r[i] = data[i] / s;
//	return r;
//}
//
//template <> inline TPointN<int, N> &TPointN<int, N>::operator/=(int s) {
//#ifdef MTS_DEBUG
//	if (s == 0)
//		SLog(EWarn, "Point4i: Division by zero!");
//#endif
//	for (int i = 0; i < N; i++) data[i] /= s;
//	return *this;
//}

//////////////////////////////////////////////////////////////////////////

template<typename T, uint32_t N>
struct ProjectableObject {

	typedef TPointN<T, N> SpatialPointType;

	virtual SpatialPointType getSpatialPoint() const = 0;

};

// ================================= VPoint Tree ===========================================


// default builder
template<typename VPointType>
struct VPointTreeBuildItem
{
	typedef typename VPointType::SpatialPointType SpatialPointType;
	typedef typename SpatialPointType::VectorType VectorType;

	VPointTreeBuildItem() : light(NULL) {}
	VPointTreeBuildItem(VPointType *l)
	{
		light = l;
		point = l->getSpatialPoint();
	}
	VPointType                                *light;
	SpatialPointType						   point;

	static SpatialPointType getScale(TAABB<SpatialPointType> &aabb) {
		if (SpatialPointType::dim == 3) {
			return SpatialPointType(1);
		}
		else if (SpatialPointType::dim == 6) {

			SpatialPointType s(1);

			VectorType ext = aabb.getExtents();
			s[3] = s[4] = s[5] = (ext[0] + ext[1] + ext[2]) / 24;
			//s[3] = s[4] = s[5] = 0;// (aabb.max - aabb.min).length() / 800;
			return s;
		}
		else {
			SLog(EError, "Unhandled branch!");
			return SpatialPointType(1);
		}
	}


};



template<typename VPointType>
class VPointTree
{

public:
	typedef typename VPointType::VNodeType VNodeType;
	typedef typename VPointType::SpatialPointType SpatialPointType;
	typedef TAABB<SpatialPointType> AABBType;


	//TODO:
	//protected:
	VNodeType *m_root;
	std::vector<VNodeType> m_nodes;
	std::vector<VPointType> m_leaves;

	VPointTree() : m_root(NULL) { }

	VPointTree(Stream *stream, InstanceManager *manager) {
		unserialize(stream, manager);
	}

	void unserialize(Stream *stream, InstanceManager *manager) {
		m_root = NULL;
		unsigned leaves_num = stream->readUInt();
		m_leaves.reserve(leaves_num);
		for (unsigned i = 0; i < leaves_num; ++i)
			m_leaves.push_back(VPointType(stream, manager));

		unsigned nodes_num = stream->readUInt();
		m_nodes.reserve(nodes_num);
		for (unsigned i = 0; i < nodes_num; ++i)
			m_nodes.push_back(VNodeType(stream, manager, (void*)m_nodes.data(), (void*)m_leaves.data()));

		int root_pos = stream->readInt();
		if (-1 != root_pos) {
			m_root = (VNodeType *)m_nodes.data() + root_pos;
		}
	}

	inline int getIndex(const VNodeType *n) const {
		return n - m_nodes.data();
	}

	inline int getIndex(const VPointType *n) const {
		return n - m_leaves.data();
	}

	//============================= build ==============================================


	void push_back(VPointType &light) { m_leaves.push_back(light); }

	typedef VPointTreeBuildItem<VPointType> BuildItem;
	typedef typename std::vector<BuildItem>::iterator DataIter;

	void serialize(Stream *stream, InstanceManager *manager) const {
		
		stream->writeUInt(m_leaves.size());
		for (auto& leaf : m_leaves)
			leaf.serialize(stream, manager);
		
		stream->writeUInt(m_nodes.size());
		int root_pos = -1, nodes_num = m_nodes.size();
		for (int i = 0; i < nodes_num; ++i) {
			m_nodes[i].serialize(stream, manager, (void*)m_nodes.data(), (void*)m_leaves.data());
			if (((VNodeType *)m_nodes.data()) + i == m_root)
				root_pos = i;
		}
		
		stream->writeInt(root_pos);
	}

	void build(Random *random) {

		if (m_leaves.size() == 0) return;

		// generate spatial point 
		std::vector<BuildItem> v_build_item;
		initBuildItems(v_build_item);

		m_nodes.clear();
		m_nodes.reserve(2 * m_leaves.size());

		m_root = buildSubTree(v_build_item.begin(), v_build_item.end(), random);

	}

	void buildByClusterCenters(Random *random) {
		if (m_leaves.size() == 0) return;

		// generate spatial point 
		std::vector<BuildItem> v_build_item;
		initBuildItems(v_build_item);

		sort(v_build_item.begin(), v_build_item.end(), [](const BuildItem& lhs, const BuildItem& rhs) {
			return lhs.light->clusterCenter < rhs.light->clusterCenter;
		});

		m_nodes.clear();
		m_nodes.reserve(2 * m_leaves.size());

		std::vector<VNodeType*> nodePtrs;
		nodePtrs.reserve(m_leaves.size());
		for (auto leafIter = v_build_item.begin(); leafIter != v_build_item.end(); ) {
			auto nextLeafIter = leafIter;
			while (nextLeafIter != v_build_item.end() && nextLeafIter->light->clusterCenter == leafIter->light->clusterCenter)
				++nextLeafIter;
			VNodeType *subroot = buildSubTree(leafIter, nextLeafIter, random);
			subroot->isSlice = true;
			nodePtrs.push_back(subroot);
			leafIter = nextLeafIter;
		}
		for (int i = 0; i + 1 < nodePtrs.size(); i += 2) {
			m_nodes.emplace_back();
			VNodeType *node = ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;

			node->left = nodePtrs[i];
			node->right = nodePtrs[i + 1];
			updateNode(node, random);

			nodePtrs.push_back(node);
		}
		m_root = ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;
	}

	void buildByPixel(std::vector<VNodeType*>& pixel_nodes, Random *random) {
		if (m_leaves.size() == 0) return;

		// generate spatial point
		std::vector<BuildItem> v_build_item;
		initBuildItems(v_build_item);

		sort(v_build_item.begin(), v_build_item.end(), [](const BuildItem& lhs, const BuildItem& rhs) {
			Point2i lpos = lhs.light->getImagePos(), rpos = rhs.light->getImagePos();
			return lpos.x != rpos.x ? lpos.x < rpos.x : lpos.y < rpos.y;
// 			for (int i = 0; i < 6; ++i)
// 				if (lhs.point[i] != rhs.point[i])
// 					return lhs.point[i] < rhs.point[i];
// 			return true;
		});

		pixel_nodes.clear();
		pixel_nodes.reserve(m_leaves.size());

		m_nodes.clear();
		m_nodes.reserve(2 * m_leaves.size());

		for (auto leafIter = v_build_item.begin(); leafIter != v_build_item.end(); ) {
			auto nextLeafIter = leafIter;
			while (nextLeafIter != v_build_item.end() && nextLeafIter->light->getImagePos() == leafIter->light->getImagePos())
				++nextLeafIter;
			pixel_nodes.push_back(buildSubTree(leafIter, nextLeafIter, random));
			leafIter = nextLeafIter;
		}
		m_root = NULL;
		
		std::vector<VNodeType*> l_pixel_nodes(pixel_nodes);
		for (int i = 0; i + 1 < l_pixel_nodes.size(); i += 2) {
			m_nodes.emplace_back();
			VNodeType *node = ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;

			node->left = l_pixel_nodes[i];
			node->right = l_pixel_nodes[i + 1];
			updateNode(node, random);

			l_pixel_nodes.push_back(node);
		}
		
		m_root = ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;
		
	}
	
	void clear() {
		m_root = NULL;
		m_leaves.clear();
		m_nodes.clear();
	}
private:
	VPointTree<VPointType>(const VPointTree<VPointType> &t) { };
	VPointTree<VPointType>(VPointTree<VPointType> &t) { };
protected:

	VNodeType *makeLeaf(DataIter start) {
		m_nodes.push_back(VNodeType(start->light));
		return ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;
	}

	void updateNode(VNodeType *node, Random *random) {

		node->left->parent = node;
		node->right->parent = node;

		node->merge(random);

	}

	
	void initBuildItems(std::vector<BuildItem>& v_build_item) {

		v_build_item.reserve(m_leaves.size());
		AABBType aabb;

		for (auto itr = m_leaves.begin(); itr != m_leaves.end(); itr++) {
			BuildItem item(&(*itr));
			v_build_item.push_back(item);
			aabb.expandBy(item.point);
		}

		// scale
		SpatialPointType scale = BuildItem::getScale(aabb);
		for (auto itr = v_build_item.begin(); itr != v_build_item.end(); itr++) {
			itr->point *= scale;
		}
	}

	VNodeType *buildSubTree(DataIter start, DataIter end, Random *random) {

		assert((end > start));
		if (end - start == 1)
			return makeLeaf(start);

		AABBType bbox;
		for (DataIter it = start; it != end; it++)
			bbox.expandBy(it->point);

		
		//uint32_t dim = bbox.getLargestAxis();
		//DataIter mid = start + (end - start) / 2;

		//std::nth_element(start, mid, end,
		//	[dim](const BuildItem &d1, const BuildItem &d2)->bool {
		//	return d1.point[dim] == d2.point[dim] ? (d1.light < d2.light) : d1.point[dim] < d2.point[dim];
		//});


		struct SplitOP {
			typename AABBType::Scalar split;
			uint32_t dim;
			bool operator()(const BuildItem &d) {
				return d.point[dim] < split;
			}
		};

		SplitOP op;
		op.dim = bbox.getLargestAxis();
		op.split = 0.5f * (bbox.max[op.dim] + bbox.min[op.dim]);

		DataIter mid = start;
		if (bbox.max[op.dim] - bbox.min[op.dim] > 0.000001f) {
			mid = std::stable_partition(start, end, op);
		}

		if (mid == start || mid == end) {
			mid = start + (end - start) / 2;
		}
		


		m_nodes.emplace_back();
		VNodeType *node = ((VNodeType *)m_nodes.data()) + m_nodes.size() - 1;


		node->left = buildSubTree(start, mid, random);
		node->right = buildSubTree(mid, end, random);

		updateNode(node, random);

		return node;

	}

};




// ======================================== Sensor =============================================

struct VSensorNode;
class SamplePath;

struct VSensor : public ProjectableObject<Float, 6> {

	typedef VSensorNode VNodeType;
	typedef ProjectableObject<Float, 6>::SpatialPointType SpatialPointType;

#if CUBE_RESOLUTION > 0
	CubeMap scattering_cube;
#endif

	Spectrum scattering_normalization; // multiply it to get normalized bsdf
	//Spectrum bsdf_sum;           // the sum of bsdf over hemisphere

	Intersection its;
	Spectrum throughput;

	Spectrum strength;           // the S term of sensor, can only work with normalized bsdf

#if PLUGIN_EXPORT == PLUGIN_EXPORT_BDLC ||PLUGIN_EXPORT == PLUGIN_EXPORT_BDLCMSTEST

	Float Sum_weight_jm2 = 0.0f;
	Float Gamma = FLT_MAX;
	Float Product_spread = 1.0f;


	void configure(Float s_weight, Float gamma, Float product_thread) {

		Sum_weight_jm2 = s_weight;
		Gamma = gamma;
		Product_spread = product_thread;

	}



#endif

	int clusterCenter;
	int depth;  // camera is depth 0

	pool_ref<SamplePath> m_samplePath;

	ref<const Shape> ref_shape;         // refer to shape in "its" for serialization
	ref<const Shape> ref_instance;      // refer to instance in "its" for serialization

	VSensor(SamplePath *_samplePath, const Intersection &_its, int _depth, const Spectrum &_throughput = Spectrum(1.0f)) : 
		its(_its), throughput(_throughput), m_samplePath(_samplePath), depth(_depth) {
		if (its.shape) ref_shape = ref<const Shape>(its.shape);
		if (its.instance) ref_instance = ref<const Shape>(its.instance);

		BSDFSamplingRecord bRec(its, Vector(0.0f, 0.0f, 1.0f));
		const BSDF *bsdf = its.getBSDF();
		
		Spectrum scattering_sum = std::abs(dot(its.shFrame.toWorld(its.wi), its.geoFrame.n)) * (bsdf->getDiffuseReflectance(its) + bsdf->getSpecularReflectance(its));

		for (int i = 0; i < Spectrum::dim; i++) {
			scattering_normalization[i] = scattering_sum[i] > 0.0f ? 1.0f / scattering_sum[i] : 1.0f;
		}

	
#if CUBE_RESOLUTION > 0
		struct CMGenerator {
			const Intersection &its;
			const BSDF *bsdf;
			BSDFSamplingRecord bRec;
			CMGenerator(const Intersection &_its) : its(_its), bsdf(_its.getBSDF()), bRec(_its, Vector(0.0f, 0.0f, 1.0f)) { }

			inline Spectrum operator()(const Vector &dir) {
				bRec.wo = its.toLocal(normalize(dir));
				return bsdf->eval(bRec);
			}
		};


		CMGenerator g(_its);

		CubeMapN<Spectrum, CubeMap::resolution> spectrum_cube;
		spectrum_cube.generate(g);

		// normalize
		//Spectrum scattering_sum = spectrum_cube.sum();
		//for (int i = 0; i < Spectrum::dim; i++) {
		//	scattering_normalization[i] = scattering_sum[i] > 0.0f ? 1.0f / scattering_sum[i] : 1.0f;
		//}

		for (uint32_t i = 0; i < 6; i++) {
			scattering_cube.face_maximum[i] = (spectrum_cube.face_maximum[i] * scattering_normalization).average();
			for (uint32_t u = 0; u < CubeMap::resolution; u++) {
				for (uint32_t v = 0; v < CubeMap::resolution; v++) {
					scattering_cube.value[i][u][v] = (spectrum_cube.value[i][u][v] * scattering_normalization).average();
				}
			}
		}

#endif

		strength = throughput * scattering_sum;
	}

	const VSensor *PreSensor() const; 
	VSensor(Stream *stream, InstanceManager *manager);

	void unserialize(Stream *stream, InstanceManager *manager);
	void serialize(Stream *stream, InstanceManager *manager) const;

	void unserializeIntersection(Stream *stream);
	void serializeIntersection(Stream *stream) const;

	Point2 getSamplePos() const;
	Point2i getImagePos() const;
	void addToPath(const Spectrum &spectrum) const;

	virtual Spectrum evalM(const VSensor *next_sensor) const;
	virtual Spectrum evalM(const Vector &wo) const;
	virtual Spectrum evalD(const VSensor *next_sensor) const;
	virtual Spectrum evalD(const Vector &wo) const;
	virtual Float evalT(const VSensor *sensor) const;

	SpatialPointType getSpatialPoint() const {
		SpatialPointType r;
		r[0] = its.p.x; r[1] = its.p.y; r[2] = its.p.z;
		//r[3] = its.geoFrame.n.x; r[4] = its.geoFrame.n.y; r[5] = its.geoFrame.n.z;
		r[3] = r[4] = r[5] = 0.0f;
		return r;
	}


};

// ======================================== Sensor Node =============================================

struct VSensorNode {


	AABB bbox;
	DirConef normal_cone;

	TAABB<Point2i> sampleBbox;
	CubeMap bsdf_cube;

	uint32_t num_sensor;
	const VSensor *rep_sensor;
	const VSensor *multiple_rep[MULTIPLE_REPRESENTATIVE];

	Spectrum strength;

	bool isSlice = false;

	VSensorNode *left;
	VSensorNode *right;
	VSensorNode *parent;


	VSensorNode() : left(NULL), right(NULL), parent(NULL), rep_sensor(NULL) { num_sensor = 0; }
	VSensorNode(const VSensor *_sensor) : bsdf_cube(_sensor->scattering_cube), 
		bbox(_sensor->its.p), 
		sampleBbox(_sensor->getImagePos()), left(NULL), right(NULL), parent(NULL), num_sensor(1),
		rep_sensor(_sensor) {

		Vector temp(_sensor->its.shFrame.n);
		normal_cone = DirConef(temp);

		strength = rep_sensor->strength;
		for (int i = 0; i < MULTIPLE_REPRESENTATIVE; i++) {
			multiple_rep[i] = rep_sensor;
		}
	}
	
	VSensorNode(Stream *stream, InstanceManager *manager, void *base, void *rep_base);

	virtual void serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const;

	virtual VSensorNode* getUnserializeAddr(Stream *stream, void *base) const;

	virtual void serializeAddress(Stream *stream, void *base, VSensorNode *node) const;


	bool isLeaf() const { return left == NULL && right == NULL; }

	bool isPixel() const {
		return sampleBbox.min == sampleBbox.max;
	}

	void merge(Random *random) {

		VSensorNode *l = (VSensorNode *)(left);
		VSensorNode *r = (VSensorNode *)(right);

		bbox = l->bbox;
		bbox.expandBy(r->bbox);

		normal_cone = DirConef::Union(l->normal_cone, r->normal_cone);

		sampleBbox = l->sampleBbox;
		sampleBbox.expandBy(r->sampleBbox);

		bsdf_cube = l->bsdf_cube;
		bsdf_cube.expandBy(r->bsdf_cube);

		//bsdf_cube.unify();
		
		num_sensor = l->num_sensor + r->num_sensor;
		
		strength = l->strength + r->strength;

		Float rep_split = l->strength.average() / strength.average();

		rep_sensor = random->nextFloat() <= rep_split ? l->rep_sensor : r->rep_sensor;
		
		if (MULTIPLE_REPRESENTATIVE > 0)
			multiple_rep[0] = rep_sensor;

		for (int i = 1; i < MULTIPLE_REPRESENTATIVE; i++) {
			multiple_rep[i] = random->nextFloat() <= rep_split ? l->multiple_rep[i] : r->multiple_rep[i];
		}

	}


};

MTS_NAMESPACE_END
#endif