#ifndef __MANY_LIGHT_SAMPLE_PATH_H_
#define __MANY_LIGHT_SAMPLE_PATH_H_


#include <mitsuba/render/bsdf.h>
#include <boost/thread/mutex.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <list>


#include "VSensor.h"

MTS_NAMESPACE_BEGIN


class MTS_EXPORT SamplePath : public SerializableObject {

public:

	SamplePath(Stream *stream, InstanceManager *manager);

	virtual void serialize(Stream *stream, InstanceManager *manager) const;


	void addLi(const Spectrum &_s);
	void setLi(const Spectrum &_s);
	Spectrum getLi() const { return Li; }

	void emplaceSensor(const Intersection &its, const Spectrum &throughput)
	{
		m_sensors.emplace_back(this, its, m_sensors.size() + 1, throughput);
	}

	void popSensor() {
		m_sensors.pop_back();
	}


	const std::vector<VSensor> &getSensors() const {
		return m_sensors;
	}
	void clearSensors() { m_sensors.clear(); }
	void copySensors(const SamplePath &s) { 
		m_sensors = s.m_sensors; 
		for (std::vector<VSensor>::iterator itr = m_sensors.begin(); itr != m_sensors.end(); itr++) {
			itr->m_samplePath = this;
		}

	}


	void setImportance(const Spectrum &_s) { importance = _s;  }
	Spectrum getImportance() const { return importance;  }
	void setPos(Point2 p) { samplePos = p; }
	Point2 getPos() const  { return samplePos;  }
	void setAlpha(Float a) { alpha = a;  }
	Float getAlpha() const { return alpha;  }

	
	static SamplePath *create() {
		SamplePath *p = (SamplePath *)boost::singleton_pool<SamplePath, sizeof(SamplePath)>::malloc();
		return new (p)SamplePath();
	}

	virtual void decRef(bool autoDeallocate = false) {
	
		Object::decRef(false);
		if (getRefCount() == 0) {
			this->~SamplePath();
			boost::singleton_pool<SamplePath, sizeof(SamplePath)>::free(this);
		}

	}

	bool hit_background = false;

	MTS_DECLARE_CLASS()
protected:
	SamplePath() : Li(0.0f) { m_sensors.clear(); m_sensors.reserve(1); };

	Spectrum importance;
	Spectrum Li;
	Point2 samplePos;
	Float alpha;
	std::vector<VSensor> m_sensors;

	boost::mutex m_LiMutex;


private:
	virtual ~SamplePath() { };

	friend boost::object_pool<SamplePath>;
};

typedef boost::singleton_pool<SamplePath, sizeof(SamplePath)> PathPool;


MTS_NAMESPACE_END
#endif