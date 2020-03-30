#ifndef __MANY_LIGHT_INTEGRATOR_H_
#define __MANY_LIGHT_INTEGRATOR_H_

#include <mitsuba/render/renderproc.h>
#include <mitsuba/core/statistics.h>
#include <mitsuba/core/plugin.h>
#include <mitsuba/render/integrator.h>
#include <mitsuba/render/scene.h>
#include <mitsuba/render/vpl.h>
#include <mutex>
#include <vector>

#include "BlockSenseProcess.h"
#include "SliceProcess.h"
#include "VPointMap.h"

MTS_NAMESPACE_BEGIN

class VSensorBlock;
class VPLGenerateWork;
class VPLGenerateResult;

class ManyLightIntegrator : public Integrator {
public:
	ManyLightIntegrator(const Properties &props);
	ManyLightIntegrator(Stream *stream, InstanceManager *manager);

	virtual ref<ParallelProcess> getSenseProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID);
	virtual ref<ParallelProcess> getVPLProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID);
	virtual ref<ParallelProcess> getRenderProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID);
	virtual ref<ParallelProcess> getPostProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID);

	/* return the non-vpl lighting */
	virtual void collectVSensor(const RayDifferential &ray, RadianceQueryRecord &rRec, 
		SamplePath *samplePath, Sampler* sampler) const;

	//virtual Spectrum Li2(const RayDifferential &ray,
	//	RadianceQueryRecord &rRec) const;
	virtual void bindUsedResources(ParallelProcess *) const;

	virtual void generateVPL(const Scene *scene, const Sensor *sensor, Sampler *sampler, Random *random, const VPLGenerateWork *vWork, VPLGenerateResult *vResult);

	virtual bool preprocess(const Scene *scene, RenderQueue *queue, const RenderJob *job,
		int sceneResID, int sensorResID, int samplerResID);

	virtual void postprocess(const Scene *scene, RenderQueue *queue, const RenderJob *job,
		int sceneResID, int sensorResID, int samplerResID);

	virtual void cancel();
	virtual void wakeup(ConfigurableObject *parent,
		std::map<std::string, SerializableObject *> &params);

	virtual bool render(Scene *scene,
		RenderQueue *queue, const RenderJob *job,
		int sceneResID, int sensorResID, int samplerResID);

	virtual void senseBlock(const Scene *scene, const Sensor *sensor,
		Sampler *sampler, VSensorBlock *block, const bool &stop,
		const std::vector< TPoint2<uint8_t> > &points);

	virtual void renderSlice(const Scene *scene, const Sensor *sensor,
		Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *slice_result);

	virtual void clusterSlice(const Scene *scene, const Sensor *sensor,
		Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *slice_result);

	virtual void postprocessSlice(const Scene *scene, const Sensor *sensor,
		Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *slice_result);
	ref<VSensorMap> getSensorMap() { return m_sensor_map; }

	virtual const VPointTree<VSensor> &getSensor() { return m_sensor_map->m_sensor_tree; }
	virtual const std::vector<const VSensorNode *> &getSliceRoot() { return m_sensor_map->msr(); }

	const std::vector < Intersection > &getClusterCenters() { return m_clusterCenters; }

	virtual void put(const VSensorBlock *sensor_block);
	virtual void put(const VPLGenerateResult *vplResult);

	void divideSlices();

	MTS_DECLARE_CLASS()
		
protected:
	virtual void fixSensor(const Scene *scene);

	void initSensorLeaves();
	virtual void buildSensorMap(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID);


	// non-vpl Li
	virtual Spectrum nvLi(const RayDifferential &ray, RadianceQueryRecord &rRec, bool scattered, uint32_t &shadow_count) const;
	

	virtual inline Spectrum evalMTDV(const VLight *light, const VSensor *sensor, const Scene *scene, uint32_t &shadow_count) const {

		Spectrum estimate = light->evalMTD(sensor, m_clamping);

		/*if (estimate.average() <= m_cutoff) {
			estimate = Spectrum(0.0f);
		}
		else */{
			Ray r = light->shadowRay(sensor);
			shadow_count++;
			if (scene->rayIntersect(r)) {
				estimate = Spectrum(0.0f);
			}
		}

		return estimate;
	}



	virtual inline Spectrum evalMTDV(const VLightNode *l_node, const VSensorNode *s_node, const Scene *scene, uint32_t &shadow_count) const {
		return evalMTDV(l_node->rep_light, s_node->rep_sensor, scene, shadow_count);
	}



	virtual inline Spectrum evalMTD(const VLightNode *lightNode, const VSensorNode *sensorNode) const {
		return lightNode->evalMTD(sensorNode->rep_sensor, 0.0f);
	}

	void initClustersByKmeans(const Scene *scene);
	void initClustersByRandom();

	virtual ~ManyLightIntegrator();
	
	bool m_kmeansFlag;
	bool m_randomFlag;
	int m_iterationNum;
	int m_clusterNum;
	std::vector < Intersection > m_clusterCenters;
	Spectrum m_luminance;

	ref<VLightMap> m_vpl_map;
	int m_vplmap_id = -1;
	size_t m_vpl_offset;
	size_t m_vpl_path_count = 0;

	ref<VSensorMap> m_sensor_map;
	int m_sensormap_id = -1;

	ref<Random> m_random;

	bool m_strictNormals;
	bool m_hideEmitters;

	int m_vplMaxDepth;
	int m_vpsMaxDepth;

	Float m_cutoff;
	Float m_clamping;
	int m_vpl_count;
	int m_sliceSize;
	bool m_cancel;
	
	ref<Mutex> m_resultMutex;
	/// Used to temporarily cache a parallel process while it is in operation
	ref<ParallelProcess> m_process;

	std::string m_backGroundFile;

	bool m_haveVplMapBuffer;
	bool m_serializeVplMap;
	std::string m_vplMapBufferName;
	ref<Stream> m_vplMapStream;
	ref<InstanceManager> m_vplMapInstanceManager;

	bool m_haveSensorMapBuffer;
	bool m_serializeSensorMap;
	std::string m_sensorMapBufferName;
	ref<Stream> m_sensorMapStream;
	ref<InstanceManager> m_sensorMapInstanceManager;

	int m_sliceSamplerResID = -1;

	bool m_postprocess_slice = false;
	std::vector<VPL> vpls_;
	std::mutex vpl_mutex_;
};


MTS_NAMESPACE_END
#endif