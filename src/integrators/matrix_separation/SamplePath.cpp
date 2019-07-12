#include "SamplePath.h"
#include <mitsuba/core/lock.h>


MTS_NAMESPACE_BEGIN


mitsuba::Point2 VSensor::getSamplePos() const
{
	return m_samplePath->getPos();
}

mitsuba::Point2i VSensor::getImagePos() const
{
	return Point2i(m_samplePath->getPos());
}


void VSensor::addToPath(const Spectrum &spectrum) const
{
	pool_ref<SamplePath> sp = m_samplePath;
	sp->addLi(spectrum);
}

//////////////////////////////////////////////////////////////////////////


void SamplePath::addLi(const Spectrum &_s)
{
	boost::mutex::scoped_lock lock(m_LiMutex);
	Li += _s;
} 

void SamplePath::setLi(const Spectrum &_s)
{
	boost::mutex::scoped_lock lock(m_LiMutex);
	Li = _s;
}

SamplePath::SamplePath(Stream *stream, InstanceManager *manager) : SerializableObject(stream, manager)
{
	importance = Spectrum(stream);
	Li = Spectrum(stream);
	samplePos = Point2(stream);
	alpha = stream->readFloat();

	unsigned sensor_num = stream->readUInt();
	m_sensors.reserve(sensor_num);
	for (unsigned i = 0; i < sensor_num; ++i)
		m_sensors.push_back(VSensor(stream, manager));

}

void SamplePath::serialize(Stream *stream, InstanceManager *manager) const
{
	importance.serialize(stream);
	Li.serialize(stream);
	samplePos.serialize(stream);
	stream->writeFloat(alpha);

	stream->writeUInt(m_sensors.size());
	for (auto& sensor : m_sensors)
		sensor.serialize(stream, manager);

}

MTS_IMPLEMENT_CLASS_S(SamplePath, false, SerializableObject)
MTS_NAMESPACE_END