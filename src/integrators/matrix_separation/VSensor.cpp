#include "VSensor.h"
#include "SamplePath.h"
#include <limits>

MTS_NAMESPACE_BEGIN

#define FLT_MIN std::numeric_limits<float>::min()
#define FLT_MAX std::numeric_limits<float>::max()
#define FLT_EPSILON std::numeric_limits<float>::epsilon()

VSensor::VSensor(Stream *stream, InstanceManager *manager) {

#if CUBE_RESOLUTION > 0
	scattering_cube = CubeMap(stream);
#endif

	scattering_normalization = Spectrum(stream);
	throughput = Spectrum(stream);
	strength = Spectrum(stream);

#if PLUGIN_EXPORT == PLUGIN_EXPORT_BDLC
	
	assert(0); // don't support

#endif

	clusterCenter = stream->readInt();
	depth = stream->readInt();

	m_samplePath = static_cast<SamplePath*>(manager->getInstance(stream));
	ref_shape = static_cast<const Shape*>(manager->getInstance(stream));
	ref_instance = static_cast<const Shape*>(manager->getInstance(stream));

	unserializeIntersection(stream);
}

void VSensor::serialize(Stream *stream, InstanceManager *manager) const {

#if CUBE_RESOLUTION > 0
	scattering_cube.serialize(stream);
#endif

	scattering_normalization.serialize(stream);	
	throughput.serialize(stream);
	strength.serialize(stream);

#if PLUGIN_EXPORT == PLUGIN_EXPORT_BDLC

	assert(0); // don't support

#endif

	stream->writeInt(clusterCenter);
	stream->writeInt(depth);

	manager->serialize(stream, m_samplePath.get());
	manager->serialize(stream, ref_shape.get());
	manager->serialize(stream, ref_instance.get());

	serializeIntersection(stream);
}

void VSensor::unserializeIntersection(Stream *stream) {
	its.t = stream->readFloat();

	its.p = Point(stream);
	its.geoFrame = Frame(stream);
	its.shFrame = Frame(stream);
	its.uv = Point2(stream);
	its.dpdu = Vector(stream);
	its.dpdv = Vector(stream);

	its.dudx = stream->readFloat();
	its.dudy = stream->readFloat();
	its.dvdx = stream->readFloat();
	its.dvdy = stream->readFloat();
	its.time = stream->readFloat();

	its.color = Spectrum(stream);
	its.wi = Vector(stream);
	its.hasUVPartials = stream->readBool();

	its.primIndex = stream->readUInt();
	//its.other = stream->readUInt();

	its.shape = ref_shape.get();
	its.instance = ref_instance.get();
}

void VSensor::serializeIntersection(Stream *stream) const {
	stream->writeFloat(its.t);

	its.p.serialize(stream);
	its.geoFrame.serialize(stream);
	its.shFrame.serialize(stream);
	its.uv.serialize(stream);
	its.dpdu.serialize(stream);
	its.dpdv.serialize(stream);

	stream->writeFloat(its.dudx);
	stream->writeFloat(its.dudy);
	stream->writeFloat(its.dvdx);
	stream->writeFloat(its.dvdy);
	stream->writeFloat(its.time);

	its.color.serialize(stream);
	its.wi.serialize(stream);
	stream->writeBool(its.hasUVPartials);

	stream->writeUInt(its.primIndex);
	//stream->writeUInt(its.other);
}

Spectrum VSensor::evalM(const VSensor *next_sensor) const
{
	Vector wo = next_sensor->its.p - its.p;
	float dist_sqr = dot(wo, wo);

	if (dist_sqr < FLT_EPSILON)
		return Spectrum(FLT_MAX);
	
	wo /= sqrt(dist_sqr);

	Vector local_wo = its.toLocal(wo);
	if (local_wo.z <= 0.0f)
		return Spectrum(FLT_MIN);

	const BSDF *bsdf = its.getBSDF();
	BSDFSamplingRecord bRec(its, local_wo);
	return bsdf->eval(bRec) * scattering_normalization;
}

mitsuba::Spectrum VSensor::evalM(const Vector &wo) const
{

	if (wo.z <= 0.0f)
		return Spectrum(FLT_MIN);

	const BSDF *bsdf = its.getBSDF();
	BSDFSamplingRecord bRec(its, wo);
	return bsdf->eval(bRec) * scattering_normalization;
}

Spectrum VSensor::evalD(const VSensor *next_sensor) const
{
	Vector wo = next_sensor->its.p - its.p;
	float dist_sqr = dot(wo, wo);

	if (dist_sqr < FLT_EPSILON)
		return Spectrum(FLT_MAX);

	wo /= sqrt(dist_sqr);

	Vector local_wo = its.toLocal(wo);
	if (local_wo.z <= 0.0f)
		return Spectrum(FLT_MIN);

	const BSDF *bsdf = its.getBSDF();
	Spectrum scattering_sum = Frame::cosTheta(local_wo) * (bsdf->getDiffuseReflectance(its) + bsdf->getSpecularReflectance(its));

	BSDFSamplingRecord bRec(its, local_wo);
	bRec.reverse();

	return bsdf->eval(bRec) / scattering_sum;
}

mitsuba::Spectrum VSensor::evalD(const Vector &wo) const
{
	Vector local_wo = its.toLocal(wo);
	if (local_wo.z <= 0.0f)
		return Spectrum(FLT_MIN);

	const BSDF *bsdf = its.getBSDF();
	Spectrum scattering_sum = Frame::cosTheta(local_wo) * (bsdf->getDiffuseReflectance(its) + bsdf->getSpecularReflectance(its));

	BSDFSamplingRecord bRec(its, local_wo);
	bRec.reverse(); 

	return bsdf->eval(bRec) / scattering_sum;
}


Float VSensor::evalT(const VSensor *sensor) const
{
	return 1.0f / std::max((its.p - sensor->its.p).lengthSquared(), FLT_MIN);
}

const VSensor * VSensor::PreSensor() const
{
	
	return depth > 1 ? &(m_samplePath->getSensors()[depth - 2]) : nullptr;

}


VSensorNode::VSensorNode(Stream *stream, InstanceManager *manager, void *base, void *rep_base)
{
	bbox = AABB(stream);
	normal_cone._dirBox = TAABB<Point>(stream);
	sampleBbox = TAABB<Point2i>(stream);
	bsdf_cube = CubeMap(stream);

	num_sensor = stream->readUInt();

	int pos = stream->readUInt();
	rep_sensor = -1 != pos ? static_cast<VSensor *>(rep_base) + pos : NULL;

	strength = Spectrum(stream);

	left = getUnserializeAddr(stream, base);
	right = getUnserializeAddr(stream, base);
	parent = getUnserializeAddr(stream, base);
}

void VSensorNode::serialize(Stream *stream, InstanceManager *manager, void *base, void *rep_base) const
{	

	bbox.serialize(stream);
	normal_cone._dirBox.serialize(stream);
	sampleBbox.serialize(stream);
	bsdf_cube.serialize(stream);

	stream->writeUInt(num_sensor);

	stream->writeInt(NULL != rep_sensor ? rep_sensor - static_cast<VSensor *>(rep_base) : -1);

	strength.serialize(stream);
	
	serializeAddress(stream, base, left);
	serializeAddress(stream, base, right);
	serializeAddress(stream, base, parent);
}

VSensorNode* VSensorNode::getUnserializeAddr(Stream *stream, void *base) const
{
	int pos = stream->readInt();
	return -1 != pos ? static_cast<VSensorNode *>(base) + pos : NULL;
}

void VSensorNode::serializeAddress(Stream *stream, void *base, VSensorNode *node) const
{
	stream->writeInt(NULL != node ? static_cast<VSensorNode *>(node)-static_cast<VSensorNode *>(base) : -1);
}

MTS_NAMESPACE_END