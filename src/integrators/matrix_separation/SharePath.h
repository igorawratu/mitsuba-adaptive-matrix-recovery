//#ifndef __MANY_LIGHT_SHARE_PATH_H_
//#define __MANY_LIGHT_SHARE_PATH_H_
//
//
//#include <mitsuba/render/bsdf.h>
//#include <boost/thread/mutex.hpp>
//#include <boost/pool/singleton_pool.hpp>
//#include <list>
//
//
//#include "VSensor.h"
//
//MTS_NAMESPACE_BEGIN
//
//
//class MTS_EXPORT SharePath : public SerializableObject {
//
//public:
//
//	SharePath(Stream *stream, InstanceManager *manager);
//
//	virtual void serialize(Stream *stream, InstanceManager *manager) const;
//
//
//	virtual void decRef(bool autoDeallocate = false) {
//
//		Object::decRef(false);
//		if (getRefCount() == 0) {
//			this->~SharePath();
//			boost::singleton_pool<SamplePath, sizeof(SamplePath)>::free(this);
//		}
//
//	}
//
//	bool hit_background = false;
//
//	MTS_DECLARE_CLASS()
//protected:
//	SamplePath() : Li(0.0f) { m_sensors.clear(); m_sensors.reserve(1); };
//
//	Spectrum importance;
//	Spectrum Li;
//	Point2 samplePos;
//	Float alpha;
//	std::vector<VSensor> m_sensors;
//
//	boost::mutex m_LiMutex;
//
//
//private:
//	virtual ~SharePath() { };
//
//	friend boost::object_pool<SamplePath>;
//};
//
//typedef boost::singleton_pool<SamplePath, sizeof(SamplePath)> PathPool;
//
//
//MTS_NAMESPACE_END
//#endif