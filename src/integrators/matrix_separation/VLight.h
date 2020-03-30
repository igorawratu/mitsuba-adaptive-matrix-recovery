#ifndef __VLIGHT_H_
#define __VLIGHT_H_


#include <mitsuba/render/vpl.h>
#include "VSensor.h"
#include <time.h>
#include <limits>

MTS_NAMESPACE_BEGIN

#define FLT_MIN std::numeric_limits<float>::min()
#define FLT_MAX std::numeric_limits<float>::max()

inline Point::Scalar getDiagonal(const AABB& box) {
	Point::Scalar result = 0;
	for (int i = 0; i < Point::dim; ++i) {
		Point::Scalar value = box.max[i] - box.min[i];
		result += value * value;
	}
	return std::sqrt(result);
}


inline bool overlaps(const AABB &a, const AABB &b) {
	for (int i = 0; i < Point::dim; ++i)
		if (a.max[i] < b.min[i] || a.min[i] > b.max[i])
			return false;
	return true;
}

inline Float sqr(Float x) {
	return x * x;
}

struct VOmniLightNode;
struct VOrientedLightNode;
struct VDirectionalLightNode;


struct VLight
{
	Spectrum intensity;
	VLight(const Spectrum &_intensity) : intensity(_intensity) { } 

	VLight(Stream *stream, InstanceManager *manager) {
		intensity = Spectrum(stream);
	}

	virtual Spectrum evalMTD(const VSensor *sensor, Float clamp) const = 0;
	virtual void evalRawMTD(Spectrum &d_M, Spectrum &s_M, Float &TD, const VSensor *sensor, Float clamp) const = 0;
	virtual Spectrum evalMaterial(const VSensor *sensor) const = 0;
	virtual Float evalDirection(const VSensor *sensor) const = 0;
	virtual Float evalTransit(const VSensor *sensor) const = 0;

	virtual void evalBSDFandG(Spectrum &bsdf, Float &G, const VSensor *sensor) const = 0;

	virtual void serialize(Stream *stream, InstanceManager *manager) const {
		intensity.serialize(stream);
	}
	
	virtual Ray shadowRay(const VSensor *sensor) const = 0;
	virtual ~VLight() { }



	virtual bool hasEmitter() { return false; }
};


struct VOrientedLight : public VLight, public ProjectableObject<Float, 6>
{
	typedef VOrientedLightNode VNodeType;
	typedef ProjectableObject<Float, 6>::SpatialPointType SpatialPointType;


	Point position;
	Vector direction;

	VOrientedLight(const Spectrum &_intensity, const Point &_position, const Vector &_direction) :
		VLight(_intensity), position(_position), direction(_direction) { }

	VOrientedLight(Stream *stream, InstanceManager *manager) : VLight(stream, manager) {
		position = Point(stream);
		direction = Vector(stream);
	}

	SpatialPointType getSpatialPoint() const {
		SpatialPointType r;
		r[0] = position.x; r[1] = position.y; r[2] = position.z;
		r[3] = direction.x; r[4] = direction.y; r[5] = direction.z;
		return r; 
	}

	virtual void serialize(Stream *stream, InstanceManager *manager) const {
		VLight::serialize(stream, manager);
		position.serialize(stream);
		direction.serialize(stream);
	}

	virtual void evalBSDFandG(Spectrum &bsdf, Float &G, const VSensor *sensor) const {
		const Intersection &its = sensor->its;

		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			G = FLT_MIN;
			bsdf = Spectrum(FLT_MIN);
			return;
		}

		wo /= sqrt(dist_sqr);
		Float cosi = std::max(FLT_MIN, dot(its.geoFrame.n, wo));
		G = cosi * std::max(FLT_MIN, dot(direction, -wo)) / (dist_sqr);
		
		BSDFSamplingRecord bRec(its, wo);
		bsdf = its.getBSDF()->eval(bRec) / cosi;
	}

	virtual Spectrum evalMTD(const VSensor *sensor, Float clamp) const
	{
		/*const Intersection &its = sensor->its;

		Vector wo = position - its.p;
		Float dist_sqr = dot(wo, wo);
		if (dist_sqr == 0) return Spectrum(clamp);
		Float T = 1.0f / dist_sqr;
		
		wo /= sqrt(dist_sqr);

		Float D = std::max(dot(-wo, direction), 0.0f) * INV_PI; // standard VPL don't do this, but we need to normalize D term
	
		Vector local_wo = its.toLocal(wo);
		const BSDF *bsdf = its.getBSDF();
		BSDFSamplingRecord bRec(its, local_wo);
		Spectrum M = bsdf->eval(bRec) * sensor->scattering_normalization; // normalize M term

		Float TD = T * D;
		if (clamp > 0.0f) { // clamp
			Float m = M.max() * TD / clamp;
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}

		return TD * M;*/

		//Changing their light calculation to be in line with mine
		const Intersection& its = sensor->its;
		Vector wi = position - its.p;
		float d = std::max(wi.length(), clamp);
		wi = normalize(wi);	

		float atten = 1.f / (d * d);
		float light_bsdf = std::max(dot(-wi, direction), 0.0f) * INV_PI;

		const BSDF *bsdf = its.getBSDF();
		BSDFSamplingRecord brec(its, its.toLocal(wi));
		Spectrum sensor_bsdf = bsdf->eval(brec) * sensor->scattering_normalization;

		Spectrum contrib = sensor_bsdf * atten * light_bsdf;

		Spectrum light_energy(0.f);

		if(its.isEmitter()){
			light_energy = its.Le(its.n);
		}

		return contrib;
	}

	virtual void evalRawMTD(Spectrum &d_M, Spectrum &s_M, Float &TD, const VSensor *sensor, Float clamp) const
	{
		const Intersection &its = sensor->its;

		Vector wo = position - its.p;
		Float dist_sqr = dot(wo, wo);
		Float T;
		if (dist_sqr == 0) {
			T = FLT_MAX;
			wo = its.shFrame.n;
		}
		else {
			T = 1.0f / dist_sqr;
			wo /= sqrt(dist_sqr);
		}

		
		Float D = std::max(dot(-wo, direction), 0.0f) * INV_PI; // standard VPL don't do this, but we need to normalize D term

		Vector local_wo = its.toLocal(wo);
		const BSDF *bsdf = its.getBSDF();

		BSDFSamplingRecord bRec(its, local_wo);
		bRec.typeMask = BSDF::EDiffuseReflection;
		d_M = bsdf->eval(bRec);
		bRec.typeMask = BSDF::EGlossyReflection;
		s_M = bsdf->eval(bRec);


		TD = T * D;
		if (clamp > 0.0f) { // clamp
			Float m = ((d_M + s_M) * sensor->scattering_normalization).max() * TD / clamp;
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}

	}

	virtual Float evalTransit(const VSensor *sensor) const {
		return 1.0f / std::max((position - sensor->its.p).lengthSquared(), FLT_MIN);
	}

	virtual Spectrum evalMaterial(const VSensor *sensor) const {
		const Intersection &its = sensor->its;


		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			return Spectrum(FLT_MAX);
		}

		wo /= sqrt(dist_sqr);

		Vector local_wo = its.toLocal(wo);
		const BSDF *bsdf = its.getBSDF();
		BSDFSamplingRecord bRec(its, local_wo);
		return bsdf->eval(bRec) * sensor->scattering_normalization; 
	}

	virtual Float evalDirection(const VSensor *sensor) const {
		const Intersection &its = sensor->its;


		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			return (0.0f);
		}

		wo /= sqrt(dist_sqr);

		return (std::max(dot(-wo, direction), 0.0f) * INV_PI);
	}

	virtual Ray shadowRay(const VSensor *sensor) const
	{
		const Intersection &its = sensor->its;
		Vector d(position - its.p);

		Float length = d.length();
		return Ray(its.p, length > 0 ? d / length : Vector(its.geoFrame.n), Epsilon, length * (1.0f - ShadowEpsilon), its.time);
	}
};

struct VOmniLight : public VLight, public ProjectableObject<Float, 3>
{
	typedef VOmniLightNode VNodeType;
	typedef ProjectableObject<Float, 3>::SpatialPointType SpatialPointType;

	Point position;
	ref<const Emitter> emitter;

	VOmniLight(const Spectrum &_intensity, const Point &_position, const Emitter *_emitter) : VLight(_intensity), position(_position), emitter(_emitter) {	
		Ray ray;
		Spectrum spectrum = emitter->sampleRay(ray, Point2(0.5, 0.5), Point2(0.5, 0.5), 0);
		spectrum /= spectrum.max();
		intensity *= spectrum;
		
	}

	VOmniLight(Stream *stream, InstanceManager *manager) : VLight(stream, manager) {
		position = Point(stream);
		emitter = static_cast<Emitter*>(manager->getInstance(stream));
	}

	SpatialPointType getSpatialPoint() const { return SpatialPointType((Float *)(&position)); }

	virtual void serialize(Stream *stream, InstanceManager *manager) const {
		VLight::serialize(stream, manager);
		position.serialize(stream);
		manager->serialize(stream, emitter.get());
	}

	virtual void evalBSDFandG(Spectrum &bsdf, Float &G, const VSensor *sensor) const {
		const Intersection &its = sensor->its;

		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			G = FLT_MIN;
			bsdf = Spectrum(FLT_MIN);
			return;
		}

		wo /= sqrt(dist_sqr);
		Float cosi = std::max(FLT_MIN, dot(its.geoFrame.n, wo));
		G = cosi / (dist_sqr);

		BSDFSamplingRecord bRec(its, wo);
		bsdf = its.getBSDF()->eval(bRec) / cosi;
	}
	
	virtual Spectrum evalMTD(const VSensor *sensor, Float clamp) const
	{
		/*const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();
		
		Vector wo = position - its.p;
		Float dist_sqr = dot(wo, wo);
		if (dist_sqr == 0) return Spectrum(clamp);
		Float T = 1.0f / dist_sqr;
		
		wo /= sqrt(dist_sqr);

		BSDFSamplingRecord bRec(its, its.toLocal(wo));

		PositionSamplingRecord pRec(its, ESolidAngle);
		DirectionSamplingRecord dRec(-wo, ESolidAngle);

		Float D = emitter->evalDirection(dRec, pRec).max();

		Spectrum M = bsdf->eval(bRec) * sensor->scattering_normalization;

		Float TD = T * D;
		if (clamp > 0.0f) { // clamp
			Float m = M.max() * (TD / clamp);
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}

		return TD * M;*/

		const Intersection& its = sensor->its;
		Vector wi = position - its.p;
		float d = std::max(wi.length(), clamp);
		wi = normalize(wi);	

		float atten = 1.f / (d * d);

		const BSDF *bsdf = its.getBSDF();
		BSDFSamplingRecord brec(its, its.toLocal(wi));
		Spectrum sensor_bsdf = bsdf->eval(brec) * sensor->scattering_normalization;

		Spectrum contrib = sensor_bsdf * atten;

		return contrib;

	}

	virtual void evalRawMTD(Spectrum &d_M, Spectrum &s_M, Float &TD, const VSensor *sensor, Float clamp) const {

		const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();

		Vector wo = position - its.p;
		Float dist_sqr = dot(wo, wo);
		Float T;
		if (dist_sqr == 0) {
			T = FLT_MAX;
			wo = its.shFrame.n;
		}
		else {
			T = 1.0f / dist_sqr;
			wo /= sqrt(dist_sqr);
		}


		PositionSamplingRecord pRec(its, ESolidAngle);
		DirectionSamplingRecord dRec(-wo, ESolidAngle);

		Float D = emitter->evalDirection(dRec, pRec).max();

		BSDFSamplingRecord bRec(its, its.toLocal(wo));
		bRec.typeMask = BSDF::EDiffuseReflection;
		d_M = bsdf->eval(bRec);

		bRec.typeMask = BSDF::EGlossyReflection;
		s_M = bsdf->eval(bRec);

		TD = T * D;
		if (clamp > 0.0f) { // clamp
			Float m = ((d_M + s_M) * sensor->scattering_normalization).max() * TD / clamp;
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}

	}

	virtual bool hasEmitter() { return true; }

	virtual Float evalTransit(const VSensor *sensor) const {
		return (1.0f / std::max(FLT_MIN, (position - sensor->its.p).lengthSquared()));
	}

	virtual Spectrum evalMaterial(const VSensor *sensor) const {

		const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();

		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			return Spectrum(FLT_MAX);
		}

		wo /= sqrt(dist_sqr);
		BSDFSamplingRecord bRec(its, its.toLocal(wo));

		return bsdf->eval(bRec) * sensor->scattering_normalization;
	}

	virtual Float evalDirection(const VSensor *sensor) const {
		const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();

		Vector wo = position - its.p;
		float dist_sqr = dot(wo, wo);

		if (dist_sqr == 0)
		{
			return (FLT_MAX);
		}

		PositionSamplingRecord pRec(its, ESolidAngle);
		DirectionSamplingRecord dRec(pRec.n, ESolidAngle);

		return (emitter->evalDirection(dRec, pRec).max());
	}

	virtual Ray shadowRay(const VSensor *sensor) const
	{
		const Intersection &its = sensor->its;
		Vector d(position - its.p);

		Float length = d.length();
		return Ray(its.p, length > 0 ? d / length : Vector(its.geoFrame.n), Epsilon, length * (1.0f - ShadowEpsilon), its.time);
	}
};

struct VDirectionalLight : public VLight, public ProjectableObject<Float, 3>
{
	typedef VDirectionalLightNode VNodeType;
	typedef ProjectableObject<Float, 3>::SpatialPointType SpatialPointType;

	Vector direction;

	VDirectionalLight(const Spectrum &_intensity, const Vector &_direction) : VLight(_intensity), direction(_direction) { }

	VDirectionalLight(Stream *stream, InstanceManager *manager) : VLight(stream, manager) {
		direction = Vector(stream);
	}

	SpatialPointType getSpatialPoint() const {
		SpatialPointType r;
		r[0] = direction.x;   r[1] = direction.y;   r[2] = direction.z;
		return r;
	}

	virtual void serialize(Stream *stream, InstanceManager *manager) const {
		VLight::serialize(stream, manager);
		direction.serialize(stream);
	}

	virtual void evalBSDFandG(Spectrum &bsdf, Float &G, const VSensor *sensor) const {
		const Intersection &its = sensor->its;

		Vector wo = -direction;
		Float cosi = std::max(FLT_MIN, dot(its.geoFrame.n, wo));
		G = 1.0f;

		BSDFSamplingRecord bRec(its, wo);
		bsdf = its.getBSDF()->eval(bRec) / cosi;
	}

	virtual Spectrum evalMTD(const VSensor *sensor, Float clamp) const
	{
		/*const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();
		
		//Float T = 1.0f;
		//Float D = 1.0f;

		Vector wo = -direction;

		BSDFSamplingRecord bRec(its, its.toLocal(wo));
		Spectrum M = bsdf->eval(bRec) * sensor->scattering_normalization;

		Float TD = 1.0f;

		if (clamp > 0.0f) { // clamp
			Float m = M.max() * TD / clamp; 
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}

		return TD * M;*/

		const Intersection& its = sensor->its;
		Vector wi = normalize(-direction);

		const BSDF *bsdf = its.getBSDF();
		BSDFSamplingRecord brec(its, its.toLocal(wi));
		Spectrum sensor_bsdf = bsdf->eval(brec) * sensor->scattering_normalization;

		Spectrum contrib = sensor_bsdf;

		return contrib;
	}

	virtual void evalRawMTD(Spectrum &d_M, Spectrum &s_M, Float &TD, const VSensor *sensor, Float clamp) const {

		const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();

		//Float T = 1.0f;
		//Float D = 1.0f;

		Vector wo = -direction;

		BSDFSamplingRecord bRec(its, its.toLocal(wo));
		bRec.typeMask = BSDF::EDiffuseReflection;
		d_M = bsdf->eval(bRec);

		bRec.typeMask = BSDF::EGlossyReflection;
		s_M = bsdf->eval(bRec);

		TD = 1.0f;

		if (clamp > 0.0f) { // clamp
			Float m = ((d_M + s_M) * sensor->scattering_normalization).max() * TD / clamp;
			if (m > 1.0f) {
				TD *= (1.0f / m);
			}
		}
	}

	virtual Float evalTransit(const VSensor *sensor) const {
		return 1.0f;
	}


	virtual Spectrum evalMaterial(const VSensor *sensor) const {
		const Intersection &its = sensor->its;
		const BSDF *bsdf = its.getBSDF();

		//Float T = 1.0f;
		//Float D = 1.0f;

		Vector wo = -direction;

		BSDFSamplingRecord bRec(its, its.toLocal(wo));
		return bsdf->eval(bRec) * sensor->scattering_normalization;
	}

	virtual Float evalDirection(const VSensor *sensor) const {
		return 1.0f;
	}

	virtual Ray shadowRay(const VSensor *sensor) const
	{
		const Intersection &its = sensor->its;
		return Ray(its.p, -direction, its.time);
	}
};

// =============================== Light Node ===========================================


inline Float cosBoundB2P(const AABB &bbox, const DirConef &cone, const Point &p) {

	Float cosBound = 1.0f;
	Float cosHalfAngle = cone.GetAngleCos();
	if (cosHalfAngle <= 0.0f)
		cosBound = 1.0f;
	else {

		Vector axis = cone.GetAxis();
		Float cos = dot(Vector(0.0f, 0.0f, 1.0f), axis);

		Transform mt;
		if (std::abs(cos - 1.0f) < 1e-6) {
			mt = Transform::rotate(Vector(1.0f, 0.0f, 0.0f), 0.0f);
		}
		else if (std::abs(cos + 1.0f) < 1e-6) {
			mt = Transform::rotate(Vector(1.0f, 0.0f, 0.0f), -180.0f);
		}
		else {
			Vector vv = normalize(cross(Vector(0.0f, 0.0f, 1.0f), axis));
			mt = Transform::rotate(vv, radToDeg(-acosf(cos)));
		}

		AABB xbox;
		for (int i = 0; i < 8; i++) {
			xbox.expandBy(mt.transformAffine(Point(p - bbox.getCorner(i))));
		}

		
		Point &xm = xbox.min;
		Point &xM = xbox.max;

		Float cosTheta;
		if (xM.z > 0)
		{
			Float minx2, miny2;
			if (xm.x * xM.x <= 0)
				minx2 = 0.0f;
			else
				minx2 = std::min(xm.x * xm.x, xM.x * xM.x);

			if (xm.y * xM.y <= 0)
				miny2 = 0.0f;
			else
				miny2 = std::min(xm.y * xm.y, xM.y * xM.y);
			Float maxz2 = xM.z * xM.z;
			cosTheta = xM.z / sqrt(minx2 + miny2 + maxz2);
		}
		else
			cosTheta = 0;

		if (cosTheta > cosHalfAngle)
			cosBound = 1.0f;
		else
		{
			float sinHalfAngle = sqrt(1 - cosHalfAngle * cosHalfAngle);
			float sinTheta = sqrt(1 - cosTheta * cosTheta);
			cosBound = math::clamp(cosTheta * cosHalfAngle + sinTheta * sinHalfAngle, 0.0f, 1.0f);
		}
	}

	return cosBound;
}

struct VLightNode {

	typedef uint32_t IndexType;

	Spectrum intensity;
	VLight *rep_light;

	VLightNode *left;
	VLightNode *right;
	VLightNode *parent;

	VLightNode() : rep_light(NULL) { }
	VLightNode(const Spectrum &_intensity, VLight *_rep_light) : intensity(_intensity), rep_light(_rep_light), left(NULL), right(NULL), parent(NULL) { }

	template<typename DerivativeNodeType, typename DerivativeLeafType>
	VLightNode(Stream *stream, InstanceManager *manager, DerivativeNodeType *base, DerivativeLeafType *rep_base) {
		intensity = Spectrum(stream);

		int pos[4];
		stream->readIntArray(pos, 4);

		rep_light = -1 != pos[0] ? static_cast<VLight *>(rep_base+pos[0]) : NULL;
	
	
		left = deserializeAddress(pos[1], base);
		right = deserializeAddress(pos[2], base);
		parent = deserializeAddress(pos[3], base);
	}



private:
	template<typename DerivativeNodeType>
	inline DerivativeNodeType *deserializeAddress(int pos, DerivativeNodeType *base) {
		return -1 != pos ? base + pos : NULL;
	}

	inline void serializeAddress(Stream *stream, char *base, char *node, size_t s) const {
		stream->writeInt(NULL != node ? (node - base) / s : -1);
	}

	template<typename DerivativeNodeType>
	inline void serializeAddress_t(Stream *stream, DerivativeNodeType *base, DerivativeNodeType *node) const {
		stream->writeInt(NULL != node ? (node - base) : -1);
	}

protected:
	// faster implementation, use it only when you known what happen
	template<typename DerivativeNodeType, typename DerivativeLeafType>
	void serialize_t(Stream *stream, InstanceManager *manager, DerivativeNodeType *base, DerivativeLeafType *rep_base) const {

		intensity.serialize(stream);

		stream->writeInt(NULL != rep_light ? (static_cast<DerivativeLeafType *>(rep_light)-rep_base) : -1);

		serializeAddress_t(stream, base, static_cast<DerivativeNodeType *>(left));
		serializeAddress_t(stream, base, static_cast<DerivativeNodeType *>(right));
		serializeAddress_t(stream, base, static_cast<DerivativeNodeType *>(parent));
	}


public:
	// generalized interface
	virtual void serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const {

		size_t light_size = sizeof(*(this->rep_light));
		size_t node_size = sizeof(*this);

		intensity.serialize(stream);
	
		stream->writeInt(NULL != rep_light ? ((char *)rep_light - (char *)rep_base) / light_size : -1);

		serializeAddress(stream, static_cast<char *>(base), reinterpret_cast<char *>(left), node_size);
		serializeAddress(stream, static_cast<char *>(base), reinterpret_cast<char *>(right), node_size);
		serializeAddress(stream, static_cast<char *>(base), reinterpret_cast<char *>(parent), node_size);
	}

	
	bool isLeaf() const { return left == NULL && right == NULL; }

	virtual void merge(Random *random) = 0;
	virtual Spectrum evalMTD(const VSensor *sensor, Float clamp) const = 0;
	//virtual Spectrum errorBoundSMTDI(const VSensor &sensor, Float dist_clamp) const = 0;
	virtual Spectrum errorBoundSMTDI(const VSensor *sensor) const = 0;
	virtual Spectrum errorBoundSMTDI(const VSensorNode *sensor_node, bool *compare_flag = NULL) const = 0;

	virtual Ray shadowRay(const VSensor *sensor) const {
		return rep_light->shadowRay(sensor);
	}

	virtual ~VLightNode() { }
};



struct VOrientedLightNode : public VLightNode  {

	AABB bbox;
	DirConef cone;

	VOrientedLightNode() { }
	VOrientedLightNode(VOrientedLight *_light) : VLightNode(_light->intensity, _light), bbox(_light->position), cone(_light->direction) { }

	VOrientedLightNode(Stream *stream, InstanceManager *manager, void *base, void *rep_base) : VLightNode(stream, manager, static_cast<VOrientedLightNode *>(base), static_cast<VOrientedLight *>(rep_base)) {
		bbox = AABB(stream);
		cone._dirBox = TAABB<Point>(stream);
	}

	virtual void serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const {
		VLightNode::serialize_t(stream, manager, static_cast<VOrientedLightNode *>(base), static_cast<VOrientedLight *>(rep_base));
		bbox.serialize(stream);
		cone._dirBox.serialize(stream);
	}



	void merge(Random *random) {
		VOrientedLightNode *l = (VOrientedLightNode *)(left);
		VOrientedLightNode *r = (VOrientedLightNode *)(right);

		bbox = l->bbox;
		bbox.expandBy(r->bbox);

		cone = DirConef::Union(l->cone, r->cone);

		float selection = random->nextFloat();
		float total_intensity = l->intensity.getLuminance() + r->intensity.getLuminance();
		intensity = selection <= (l->intensity.getLuminance() / total_intensity) ? 
			l->intensity / l->intensity.getLuminance() * total_intensity :
			r->intensity / r->intensity.getLuminance() * total_intensity;
		//intensity = l->intensity + r->intensity;
		rep_light = selection <= (l->intensity.getLuminance() / total_intensity) ? l->rep_light : r->rep_light;
	}

	Spectrum evalMTD(const VSensor *sensor, Float clamp) const {
		return rep_light->evalMTD(sensor, clamp);
	}

	Spectrum errorBoundSMTDI(const VSensor *sensor) const {

		const Intersection &its = sensor->its;
		Point p = its.p;
		Point bp;
		bp.x = math::clamp(p.x, bbox.min[0], bbox.max[0]);
		bp.y = math::clamp(p.y, bbox.min[1], bbox.max[1]);
		bp.z = math::clamp(p.z, bbox.min[2], bbox.max[2]);

		Float lenSqr = std::max((bp - p).lengthSquared(), 1e-6f); // avoid zero
		Float T_bound = 1.0f / lenSqr;

		Float D_bound = cosBoundB2P(bbox, cone, p) * INV_PI;  // standard VPL don't do this, but we need to normalize D term

		Float M_bound = 0;
		if (D_bound > 0) {
			AABB shift_aabb(Point(bbox.min - p), Point(bbox.max - p));
			M_bound = sensor->scattering_cube.evalMax(shift_aabb);
		}
		
		return sensor->strength * intensity * (M_bound * T_bound * D_bound);

	}

	Spectrum errorBoundSMTDI(const VSensorNode *sensorNode, bool *compare_flag = NULL) const {

		const AABB &ltBBox = bbox;
		const AABB &gpBBox = sensorNode->bbox;

		Float lenSqr = std::max(ltBBox.squaredDistanceTo(gpBBox), 1e-6f); // avoid zero
		Float T_bound = 1.0f / lenSqr;

		//AABB tbbox(Point(gpBBox.min - ltBBox.max), Point(gpBBox.max - ltBBox.min));
		AABB shift_aabb(Point(ltBBox.min - gpBBox.max), Point(ltBBox.max - gpBBox.min));
		Float D_bound = cosBoundB2P(shift_aabb, cone, Point(0.0f)) * INV_PI;  // standard VPL don't do this, but we need to normalize D term
		
		Float M_bound = D_bound > 0 ? sensorNode->bsdf_cube.evalMax(shift_aabb) : 0;


		if (compare_flag) {
			if (overlaps(ltBBox, gpBBox)) {
				*compare_flag = getDiagonal(ltBBox) > getDiagonal(gpBBox);
			} else {
				float lightcutHeur = 0.0f;
				float sensornodeHeur = 0.0f;
				float lengSqr = ltBBox.squaredDistanceTo(gpBBox);
				if (!isLeaf()) {
					float lmin = reinterpret_cast<VOrientedLightNode *>(left)->bbox.squaredDistanceTo(gpBBox);
					float rmin = reinterpret_cast<VOrientedLightNode *>(right)->bbox.squaredDistanceTo(gpBBox);

					float redDist = std::min(lmin, rmin) / lenSqr;
					float redMat = (1.0f - cone.GetAngleCos()) * getDiagonal(ltBBox) / sqrt(lenSqr);
					lightcutHeur = redDist * redMat;
				}
				if (!sensorNode->isLeaf()) {
					float lmin = ltBBox.squaredDistanceTo(sensorNode->left->bbox);
					float rmin = ltBBox.squaredDistanceTo(sensorNode->right->bbox);

					float redDist = std::min(lmin, rmin) / lenSqr;
					float redMat = (1.0f - sensorNode->normal_cone.GetAngleCos()) * getDiagonal(gpBBox) / sqrt(lenSqr);

					sensornodeHeur = redDist * redMat;
				}
				*compare_flag = sensornodeHeur <= lightcutHeur;
			}
		}

		return sensorNode->strength * intensity * (M_bound * T_bound * D_bound);
	}

};

struct VOmniLightNode : public VLightNode  {

	AABB bbox;

	VOmniLightNode() { }
	VOmniLightNode(VOmniLight *_light) : VLightNode(_light->intensity, _light), bbox(_light->position) { }

	VOmniLightNode(Stream *stream, InstanceManager *manager, void *base, void *rep_base) : VLightNode(stream, manager, static_cast<VOmniLightNode *>(base), static_cast<VOmniLight *>(rep_base)) {

		bbox = AABB(stream);
	}

	virtual void serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const {
		VLightNode::serialize_t(stream, manager, static_cast<VOmniLightNode *>(base), static_cast<VOmniLight *>(rep_base));
		bbox.serialize(stream);
	}



	void merge(Random *random) {
		VOmniLightNode *l = (VOmniLightNode *)(left);
		VOmniLightNode *r = (VOmniLightNode *)(right);

		bbox = l->bbox;
		bbox.expandBy(r->bbox);


		intensity = l->intensity + r->intensity;
		rep_light = random->nextFloat() <= (l->intensity.average() / intensity.average()) ? l->rep_light : r->rep_light;
	}

	Spectrum evalMTD(const VSensor *sensor, Float clamp) const {
		return rep_light->evalMTD(sensor, clamp);
	}

	Spectrum errorBoundSMTDI(const VSensor *sensor) const {

		const Intersection &its = sensor->its;
		Point p = its.p;
		Point bp;
		bp.x = math::clamp(p.x, bbox.min[0], bbox.max[0]);
		bp.y = math::clamp(p.y, bbox.min[1], bbox.max[1]);
		bp.z = math::clamp(p.z, bbox.min[2], bbox.max[2]);

		Float lenSqr = std::max((bp - p).lengthSquared(), 1e-6f); // avoid zero
		Float T_bound = 1.0f / lenSqr;

		Float D_bound = INV_FOURPI;

		AABB shift_aabb(Point(bbox.min - p), Point(bbox.max - p));
		Float M_bound = sensor->scattering_cube.evalMax(shift_aabb);

		return (sensor->strength * intensity) * (D_bound * T_bound * M_bound);

	}

	Spectrum errorBoundSMTDI(const VSensorNode *sensorNode, bool *compare_flag = NULL) const {

		const AABB &ltBBox = bbox;
		const AABB &gpBBox = sensorNode->bbox;

		Float lenSqr = std::max(ltBBox.squaredDistanceTo(gpBBox), 1e-6f); // avoid zero
		Float T_bound = 1.0f / lenSqr;

		Float D_bound = INV_FOURPI;
		
		//AABB shift_aabb(Point(gpBBox.max - ltBBox.min), Point(gpBBox.min - ltBBox.max));
		AABB shift_aabb(Point(ltBBox.min - gpBBox.max), Point(ltBBox.max - gpBBox.min));
		Float M_bound = sensorNode->bsdf_cube.evalMax(shift_aabb);

		if (compare_flag) {
			if (overlaps(ltBBox, gpBBox)) {
				*compare_flag = getDiagonal(ltBBox) > getDiagonal(gpBBox);
			} else {
				float lightcutHeur = 0.0f;
				float sensornodeHeur = 0.0f;
				float lengSqr = ltBBox.squaredDistanceTo(gpBBox);
				if (!isLeaf()) {
					float lmin = reinterpret_cast<VOmniLightNode *>(left)->bbox.squaredDistanceTo(gpBBox);
					float rmin = reinterpret_cast<VOmniLightNode *>(right)->bbox.squaredDistanceTo(gpBBox);
					lightcutHeur = std::min(lmin, rmin) / lenSqr;
				}
				if (!sensorNode->isLeaf()) {
					float lmin = ltBBox.squaredDistanceTo(sensorNode->left->bbox);
					float rmin = ltBBox.squaredDistanceTo(sensorNode->right->bbox);
					sensornodeHeur = std::min(lmin, rmin) / lenSqr;
				}
				*compare_flag = sensornodeHeur <= lightcutHeur;
			}
		}

		return sensorNode->strength * intensity * (D_bound * T_bound * M_bound);
	}

};

struct VDirectionalLightNode : public VLightNode  {

	DirConef cone;

	VDirectionalLightNode() { }
	VDirectionalLightNode(VDirectionalLight *_light) : VLightNode(_light->intensity, _light), cone(_light->direction) { }

	VDirectionalLightNode(Stream *stream, InstanceManager *manager, void *base, void *rep_base) : VLightNode(stream, manager, static_cast<VDirectionalLightNode *>(base), static_cast<VDirectionalLight *>(rep_base)) {
		cone._dirBox = TAABB<Point>(stream);
	}

	virtual void serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const {
		VLightNode::serialize_t(stream, manager, static_cast<VDirectionalLightNode *>(base), static_cast<VDirectionalLight *>(rep_base));
		cone._dirBox.serialize(stream);
	}



	void merge(Random *random) {

		VDirectionalLightNode *l = (VDirectionalLightNode *)(left);
		VDirectionalLightNode *r = (VDirectionalLightNode *)(right);

		cone = DirConef::Union(l->cone, r->cone);

		intensity = l->intensity + r->intensity;
		rep_light = random->nextFloat() <= (l->intensity.average() / intensity.average()) ? l->rep_light : r->rep_light;
	}

	Spectrum evalMTD(const VSensor *sensor, Float clamp) const {
		return rep_light->evalMTD(sensor, clamp);
	}

	Spectrum errorBoundSMTDI(const VSensor *sensor) const {

		Spectrum SM = sensor->strength * sensor->scattering_cube.evalMax(cone);
		//Float TD = 1.0f;

		return SM * intensity;

	}

	Spectrum errorBoundSMTDI(const VSensorNode *sensorNode, bool *compare_flag = NULL) const {

		Spectrum SM = sensorNode->strength * sensorNode->bsdf_cube.evalMax(cone);

		if (compare_flag) {
			const AABB &ltBBox = cone.getBoundingBox();
			const AABB &gpBBox = sensorNode->bbox;
			if (overlaps(ltBBox, gpBBox)) {
				*compare_flag = getDiagonal(ltBBox) > getDiagonal(gpBBox);
			} else {
				Float lightcutHeur = 0.0f;
				Float sensornodeHeur = 0.0f;
				Float length = ltBBox.distanceTo(gpBBox);

				if (!isLeaf())
					lightcutHeur = getDiagonal(ltBBox) / length;

				if (!sensorNode->isLeaf())
					sensornodeHeur = getDiagonal(gpBBox) / length;

				*compare_flag = sensornodeHeur <= lightcutHeur;
			}
		}
		
		//Float TD = 1.0f;

		return SM * intensity;
	}

};




MTS_NAMESPACE_END
#endif