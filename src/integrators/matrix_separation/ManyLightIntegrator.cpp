#include <mitsuba/core/fstream.h>
#include <mitsuba/core/mstream.h>
#include <mitsuba/core/statistics.h>
#include <mitsuba/core/qmc.h>

#include <queue>
#include <unordered_map>
#include <fstream>

#include "ManyLightIntegrator.h"
#include "pluginExport.h"
#include "SliceProcess.h"
#include "VPLGenerateProcess.h"


MTS_NAMESPACE_BEGIN

 
ManyLightIntegrator::ManyLightIntegrator(const Properties &props) : Integrator(props) {

	/**
	* This parameter specifies the action to be taken when the geometric
	* and shading normals of a surface don't agree on whether a ray is on
	* the front or back-side of a surface.
	*
	* When \c strictNormals is set to \c false, the shading nwdormal has
	* precedence, and rendering proceeds normally at the risk of
	* introducing small light leaks (this is the default).
	*
	* When \c strictNormals is set to \c true, the random walk is
	* terminated when encountering such a situation. This may
	* lead to silhouette darkening on badly tesselated meshes.
	*/
	m_strictNormals = props.getBoolean("strictNormals", false);

	/**
	* When this flag is set to true, contributions from directly
	* visible emitters will not be included in the rendered image
	*/
	m_hideEmitters = props.getBoolean("hideEmitters", false);
	

	/* Max. depth (expressed as path length) */
	m_vplMaxDepth = props.getInteger("vplMaxDepth", 10);
	m_vpsMaxDepth = props.getInteger("vpsMaxDepth", 5);

	m_backGroundFile = props.getString("backGroundFile", "");

	/* Relative clamping factor (0=no clamping, 1=full clamping) */
	//m_clamping = props.getFloat("clamping", 0.02f);
	m_cutoff = props.getFloat("cutoff", 0.0f);
	m_vpl_count = props.getInteger("vplCount", 1024);
	m_sliceSize = props.getInteger("sliceSize", 16);

	m_random = new Random();

	m_resultMutex = new Mutex();
	
	m_serializeVplMap = props.getBoolean("serializeVplMap", false);
	m_serializeSensorMap = props.getBoolean("serializeSensorMap", false);

	m_iterationNum = props.getInteger("iterationNum", 100);
	m_kmeansFlag = props.getBoolean("kmeansFlag", false);
	m_randomFlag = props.getBoolean("randomFlag", false);

	if (m_serializeVplMap) {
		m_vplMapBufferName = props.getString("vplMapBuffer", "vpl_map.buffer");

		if (std::ifstream(m_vplMapBufferName)) {	
			m_haveVplMapBuffer = true;
			m_vplMapStream = new FileStream(fs::path(m_vplMapBufferName));
		}
		else {
			m_haveVplMapBuffer = false;
			m_vplMapStream = new FileStream(fs::path(m_vplMapBufferName), FileStream::ETruncWrite);
		}

		m_vplMapInstanceManager = new InstanceManager();
	}
	

	if (m_serializeSensorMap) {
		m_sensorMapBufferName = props.getString("sensorMapBuffer", "sensor_map.buffer");

		if (std::ifstream(m_sensorMapBufferName)) {
			m_haveSensorMapBuffer = true;
			m_sensorMapStream = new FileStream(fs::path(m_sensorMapBufferName));
		}
		else {
			m_haveSensorMapBuffer = false;
			m_sensorMapStream = new FileStream(fs::path(m_sensorMapBufferName), FileStream::ETruncWrite);
		}

		m_sensorMapInstanceManager = new InstanceManager();
	}

	if (m_sliceSamplerResID == -1) {

		std::string s_slice_sampler = props.getString("sliceSampler", "independent");
		int slice_sampleCount = props.getInteger("sliceSampleCount", 4);
		int slice_Dimension = props.getInteger("sliceDimension", 4);

		Properties p(s_slice_sampler);
		p.setInteger("sampleCount", slice_sampleCount);
		p.setInteger("dimension", slice_Dimension);

		ref<Sampler> slice_sampler = static_cast<Sampler *> (PluginManager::getInstance()->createObject(MTS_CLASS(Sampler), p));
		slice_sampler->configure();

		ref<Scheduler> sched = Scheduler::getInstance();

		/* Create a sampler instance for everycore */
		std::vector<SerializableObject *> samplers(sched->getCoreCount());
		for (size_t i = 0; i < sched->getCoreCount(); ++i) {
			ref<Sampler> clonedSampler = slice_sampler->clone();
			clonedSampler->incRef();
			samplers[i] = clonedSampler.get();
		}
		m_sliceSamplerResID = sched->registerMultiResource(samplers);
		for (size_t i = 0; i < sched->getCoreCount(); ++i)
			samplers[i]->decRef();
	
	}

}

/// Unserialize from a binary data stream
ManyLightIntegrator::ManyLightIntegrator(Stream *stream, InstanceManager *manager)
: Integrator(stream, manager) {
	//TODO
	SLog(EError, "Unfinished region");
}

float calculateMinDistance(const Scene *scene, const std::vector<VPL>& vpls, float clamping){
	float acc_near = 0.f;
	float acc_far = 0.f;
	std::uint32_t lights_processed = 0;

	for(std::uint32_t i = 0; i < vpls.size(); ++i){
		bool surface_emitter = vpls[i].type == ESurfaceVPL;
		float near = std::numeric_limits<float>::max();
		float far = std::numeric_limits<float>::min();

		for(int k = 0; k < 128; ++k){
			if(vpls[i].type == EDirectionalEmitterVPL){
				continue;
			}

			Point2 sample(sample02(k));

			Ray ray(vpls[i].its.p,
				surface_emitter ? vpls[i].its.shFrame.toWorld(warp::squareToCosineHemisphere(sample))
				: warp::squareToUniformSphere(sample), 0);

			Float t;
			ConstShapePtr shape;
			Normal n;
			Point2 uv;

			float acc = 0.f;
			bool hit = false;

			for(std::uint8_t j = 0; j < 5; ++j){
				if(!scene->rayIntersect(ray, t, shape, n, uv)){
					break;
				}

				acc += t;
				if (!(shape->getBSDF()->getType() & BSDF::ETransmission)) {
					hit = true;
					break;
				}

				ray.o = ray(t);
			}
			
			if (hit && acc > 0.f) {
				near = std::min(near, acc);
				far = std::max(far, acc);
			}

		}

		if(near >= far){
			BSphere bsphere(scene->getKDTree()->getAABB().getBSphere());
			Float min_dist = 0;

			if ((vpls[i].type == ESurfaceVPL || vpls[i].type == EPointEmitterVPL) &&
					!bsphere.contains(vpls[i].its.p))
				min_dist = (bsphere.center - vpls[i].its.p).length() - bsphere.radius;

			far = min_dist + bsphere.radius * 2.25f;
			near = std::max(min_dist - 0.25f * bsphere.radius, far * 1e-5f);
		}
		else{
			near = std::max(near / 1.5f, far * 1e-5f);
			far *= 1.5f;
		}

		acc_near += near;
		acc_far += far;
		lights_processed++;
	}

	acc_near /= (float)lights_processed;
	acc_far /= (float)lights_processed;

	return acc_near + (acc_far - acc_near) * clamping;
}


size_t generateVPLs(const Scene *scene, size_t offset, size_t count, int max_depth, bool prune, 
	std::vector<VPL> &vpls) {
	if (max_depth <= 1)
		return 0;

	Properties props("halton");
    props.setInteger("scramble", 0);
    Sampler *sampler = static_cast<Sampler *> (PluginManager::getInstance()->
        createObject(MTS_CLASS(Sampler), props));
    sampler->configure();
    sampler->generate(Point2i(0));

	const Sensor *sensor = scene->getSensor();
	Float time = sensor->getShutterOpen() + sensor->getShutterOpenTime() * sampler->next1D();

	const Frame standard_frame(Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1));
	int retries = 0;

	while (vpls.size() < count) {
		sampler->setSampleIndex(++offset);
		//offset++;

		if (vpls.empty() && ++retries > 10000) {
			return 0;
		}

		PositionSamplingRecord point_sample(time);
		DirectionSamplingRecord direction_sample;
		Spectrum weight = scene->sampleEmitterPosition(point_sample, sampler->next2D());

		size_t start = vpls.size();

		const Emitter *emitter = static_cast<const Emitter *>(point_sample.object);

		//samples an emitter, adds it to the vpl vector, and then extracts a point and direction sample to be used later for additional
		//vpl generation
		if (!emitter->isEnvironmentEmitter()) {
			EVPLType type = emitter->needsDirectionSample() ? ESurfaceVPL : EPointEmitterVPL;
			VPL vpl(type, weight);
			vpl.its.p = point_sample.p;
			vpl.its.time = time;
			vpl.its.shFrame = point_sample.n.isZero() ? standard_frame : Frame(point_sample.n);
			vpl.emitter = emitter;
			vpls.push_back(vpl);

			if(type == ESurfaceVPL){
				weight *= emitter->sampleDirection(direction_sample, point_sample, sampler->next2D());
			}
		}
		else {
			DirectSamplingRecord direct_sample(scene->getKDTree()->getAABB().getCenter(), point_sample.time);

			Spectrum direct_sample_weight = emitter->sampleDirect(direct_sample, sampler->next2D())	/ scene->pdfEmitterDiscrete(emitter);

			if (direct_sample_weight.isZero())
				continue;

			VPL vpl(EDirectionalEmitterVPL, direct_sample_weight);
			vpl.its.p = Point(0.0);
			vpl.its.time = time;
			vpl.its.shFrame = Frame(-direct_sample.d);
			vpl.emitter = emitter;
			vpls.push_back(vpl);
			direction_sample.d = -direct_sample.d;

			Point2 offset = warp::squareToUniformDiskConcentric(sampler->next2D());
			Vector transformed_offset = Frame(direct_sample.d).toWorld(Vector(offset.x, offset.y, 0));
			BSphere bounding_sphere = scene->getKDTree()->getAABB().getBSphere();
			point_sample.p = bounding_sphere.center + (transformed_offset - direction_sample.d) * bounding_sphere.radius;
			weight = direct_sample_weight * M_PI * bounding_sphere.radius * bounding_sphere.radius;
		}

		int depth = 2; 
		Ray ray(point_sample.p, direction_sample.d, time);
		Intersection its;
		//generates vpls from additional bounces
		while (!weight.isZero() && (depth < max_depth || max_depth == -1) && vpls.size() < count) {
			if (!scene->rayIntersect(ray, its))
				break;

			const BSDF *bsdf = its.getBSDF();
			BSDFSamplingRecord bsdf_sample(its, sampler, EImportance);
			Spectrum bsdf_sample_weight = bsdf->sample(bsdf_sample, sampler->next2D());
			if (bsdf_sample_weight.isZero())
				break;

			float approx_albedo = std::fmin(0.95f, bsdf_sample_weight.max());

			//roulette
            if (sampler->next1D() > approx_albedo){
				break;
			}
			
			weight *= bsdf_sample_weight;

			VPL vpl(ESurfaceVPL, weight);
			vpl.its = its;

			if (BSDF::getMeasure(bsdf_sample.sampledType) == ESolidAngle) {
				vpls.push_back(vpl);
			}

			Vector wi = -ray.d, wo = its.toWorld(bsdf_sample.wo);

			ray = Ray(its.p, wo, 0.0f);

			float wi_dot_geometricnormal = dot(its.geoFrame.n, wi);
			float wo_dot_geometricnormal = dot(its.geoFrame.n, wo);
			if (wi_dot_geometricnormal * Frame::cosTheta(bsdf_sample.wi) <= 0 || 
				wo_dot_geometricnormal * Frame::cosTheta(bsdf_sample.wo) <= 0) {
				break;
			}

			++depth;
		}

		size_t end = vpls.size();
		float lights_added = end - start;
		for (size_t i = start; i < end; ++i){
			vpls[i].emitterScale = 1.0f / lights_added;
		}
	}

	return offset;
}

void ManyLightIntegrator::generateVPL(const Scene *scene, const Sensor *sensor, Sampler *sampler, Random *random, const VPLGenerateWork *vWork, VPLGenerateResult *vResult)
{
	std::vector<VPL> result;
	vResult->offset = generateVPLs(scene, vWork->m_offset, vWork->m_numVPL, vWork->m_maxDepth, false, result);
	{
		std::lock_guard<std::mutex> lock(vpl_mutex_);
		vpls_.insert(vpls_.end(), result.begin(), result.end());
	}

	vResult->path_count = vResult->offset - vWork->m_offset;
	vResult->m_vpls.clear();
	vResult->m_vpls.resize(result.size(), *result.begin());
	std::copy(result.begin(), result.end(), vResult->m_vpls.begin());

	//vResult->m_vpls.insert(vResult->m_vpls.end(), result.begin(), result.end());

}


bool ManyLightIntegrator::preprocess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID)
{


	Integrator::preprocess(scene, queue, job, sceneResID, sensorResID, samplerResID);

	ref<Scheduler> sched = Scheduler::getInstance();
	int integratorResID = sched->registerResource(this);


	if (!(scene->getSensor()->getType() & Sensor::EProjectiveCamera))
		Log(EError, "The VPL integrator requires a projective camera "
		"(e.g. perspective/thinlens/orthographic/telecentric)!");

	// vpl
	if (m_serializeVplMap && m_haveVplMapBuffer) {
		
		size_t s = m_vplMapStream->getSize();
		uint8_t *ptr = (uint8_t *)malloc(s);
		m_vplMapStream->read(ptr, s);

		ref<MemoryStream> m_stream = new MemoryStream(ptr, s);
		m_vpl_map = new VLightMap(m_stream, m_vplMapInstanceManager);
		m_vplmap_id = sched->registerResource(m_vpl_map);


		free(ptr);
		
		//m_vpl_map = new VLightMap(m_vplMapStream, m_vplMapInstanceManager);

	} else {
		if (m_vpl_map.get())
			m_vpl_map->clear();

		m_vpl_map = new VLightMap();

		m_vplmap_id = sched->registerResource(m_vpl_map);

		m_vpl_map->raw_vpl.reserve(m_vpl_count);
		m_vpl_offset = 0;

		if (m_vpl_count <= 1024) {
			
			std::vector<VPL> m_vpls;
			m_vpl_offset = generateVPLs(scene, 0, m_vpl_count, m_vplMaxDepth, false, m_vpls);
			vpls_.insert(vpls_.end(), m_vpls.begin(), m_vpls.end());

			m_vpl_path_count = m_vpl_offset;

			for (std::vector<VPL>::iterator itr = m_vpls.begin(); itr != m_vpls.end(); itr++) {
				m_vpl_map->raw_vpl.push_back(*itr);
			}
		}
		else { // parallel sample vpl


			ref<ParallelProcess> proc = getVPLProcess(scene, queue, job, sceneResID, sensorResID, samplerResID, integratorResID);

			scene->bindUsedResources(proc);
			bindUsedResources(proc);


			m_process = proc;

			sched->schedule(proc);
			sched->wait(proc);
			m_process = NULL;

			if (proc->getReturnStatus() != ParallelProcess::ESuccess) {
				sched->unregisterResource(integratorResID);
				return false;
			}
		}
		m_clamping = calculateMinDistance(scene, vpls_, 0.1f);
		vpls_.clear();

		Log(EInfo, "Generated %i virtual point lights", m_vpl_map->raw_vpl.size());

		m_vpl_map->normalizeRawVPL(m_vpl_path_count);
		m_vpl_map->generateStandardVPL(m_random);


		if (m_serializeVplMap) {
			m_vpl_map->serialize(m_vplMapStream, m_vplMapInstanceManager);
			m_vplMapStream->flush();
		}
		
	}
	// sensor
	bool flag;

	if (m_serializeSensorMap && m_haveSensorMapBuffer) {
		m_sensor_map = new VSensorMap(m_sensorMapStream, m_sensorMapInstanceManager);

	} else {
		m_sensor_map = new VSensorMap();

		{
			const Film *film = scene->getFilm();
			Vector2i s = film->getSize();
			s[0] += film->getReconstructionFilter()->getBorderSize();
			s[1] += film->getReconstructionFilter()->getBorderSize();
			uint32_t n_reserve = s[0] * s[1] * scene->getSampler()->getSampleCount();
			m_sensor_map->m_sample_pathes.reserve(n_reserve);
			m_sensor_map->m_sensor_tree.m_leaves.reserve(n_reserve);
		}

		m_sensormap_id = sched->registerResource(m_sensor_map);


		/* Gather VSensor - parallelize */
		m_sensor_map->clear();

		cout << "Begin Sensing..." << endl;
		ref<ParallelProcess> proc = getSenseProcess(scene, queue, job, sceneResID, sensorResID, samplerResID, integratorResID);
	

		scene->bindUsedResources(proc);
		bindUsedResources(proc);


		sched->schedule(proc);


		m_process = proc;


		sched->wait(proc);
		m_process = NULL;


		if (proc->getReturnStatus() != ParallelProcess::ESuccess) {
			sched->unregisterResource(integratorResID);
			return false;
		}
	 

		/* Construct VSensor hierarchy */		
		cout << "Constructing Sensor Map..." << endl;
		fixSensor(scene);

		buildSensorMap(scene, queue, job, sceneResID, sensorResID, samplerResID, integratorResID);

		sched->unregisterResource(integratorResID);

		if (m_serializeSensorMap) {
			m_sensor_map->serialize(m_sensorMapStream, m_sensorMapInstanceManager);
			m_sensorMapStream->flush();
		}
		

		cout << endl;
		cout << "raw:" << m_vpl_map->raw_vpl.size() << " ori:" << m_vpl_map->m_oriented_tree.m_leaves.size() <<
		" omni:" << m_vpl_map->m_omni_tree.m_leaves.size() << " dir:" << m_vpl_map->m_directional_tree.m_leaves.size() << endl;
		if (!m_vpl_map->m_oriented_tree.m_leaves.empty()) {
			cout << "sum intensity : " << m_vpl_map->m_oriented_tree.m_root->intensity.toString() << endl;
		}
		cout << "sensor:" << m_sensor_map->m_sensor_tree.m_leaves.size() << endl;


		flag = proc->getReturnStatus() == ParallelProcess::ESuccess;
	}

	return flag;
}

void ManyLightIntegrator::wakeup(ConfigurableObject *parent, std::map<std::string, SerializableObject *> &params) {
	if (!m_vpl_map.get() && params.find("vplMap") != params.end())
		m_vpl_map = static_cast<VLightMap *>(params["vplMap"]);


	if (!m_sensor_map.get() && params.find("sensorMap") != params.end())
		m_sensor_map = static_cast<VSensorMap *>(params["sensorMap"]);
}


void ManyLightIntegrator::cancel() {
	if (m_process)
		Scheduler::getInstance()->cancel(m_process);
}

#if 0


Spectrum ManyLightIntegrator::Li2(const RayDifferential &r, RadianceQueryRecord &rRec) const
{
	/* Some aliases and local variables */
	const Scene *scene = rRec.scene;
	Intersection &its = rRec.its;
	RayDifferential ray(r);
	Spectrum Li(0.0f);
	bool scattered = false;

	/* Perform the first ray intersection (or ignore if the
	intersection has already been provided). */
	rRec.rayIntersect(ray);
	ray.mint = Epsilon;

	Spectrum throughput(1.0f);
	Float eta = 1.0f;

	if (!its.isValid()) {
		/* If no intersection could be found, potentially return
		radiance from a environment luminaire if it exists */
		if ((rRec.type & RadianceQueryRecord::EEmittedRadiance)
			&& (!m_hideEmitters || scattered))
			Li += throughput * scene->evalEnvironment(ray);

		return Li;
	}

	/* Possibly include emitted radiance if requested */
	if (its.isEmitter() && (rRec.type & RadianceQueryRecord::EEmittedRadiance)
		&& (!m_hideEmitters || scattered))
		Li += throughput * its.Le(-ray.d);
	const BSDF *bsdf = its.getBSDF(ray);


	/* Include radiance from a subsurface scattering model if requested */
	if (its.hasSubsurface() && (rRec.type & RadianceQueryRecord::ESubsurfaceRadiance))
		Li += throughput * its.LoSub(scene, rRec.sampler, -ray.d, rRec.depth);


	if ((m_strictNormals && dot(ray.d, its.geoFrame.n) * Frame::cosTheta(its.wi) >= 0))
		return Li;


	DirectSamplingRecord dRec(its);
	float minDistSqr = 0.01f * 0.01f;

	if (rRec.type & RadianceQueryRecord::EDirectSurfaceRadiance && (bsdf->getType() & BSDF::ESmooth)) {

		
		for (int i = 0; i < m_vpls.size(); i++)
		{
			const VPL &vpl = m_vpls[i];
			const Emitter *emitter = vpl.emitter;
			Assert(emitter != NULL);
			

			bool EMITTER_VPL = (vpl.type == EDirectionalEmitterVPL || vpl.type == EPointEmitterVPL);
			bool DIRECTIONAL_VPL = (vpl.type == EDirectionalEmitterVPL);
			bool VPL_ON_SURFACE = vpl.type == ESurfaceVPL || (vpl.type == EPointEmitterVPL &&
				vpl.emitter->getType() & Emitter::EOnSurface);

			Vector woWorld;
			float distSqr = 0;
			float dist = 0;
			if (DIRECTIONAL_VPL) {
				woWorld = -vpl.its.shFrame.n;
			}
			else {
				woWorld = (vpl.its.p - its.p);
				distSqr = dot(woWorld, woWorld);
				dist = sqrt(distSqr);
				woWorld /= dist;
			}

			Vector wo = its.shFrame.toLocal(woWorld);
			Vector vplWo = vpl.its.shFrame.toLocal(woWorld);

			Spectrum result = vpl.P;


			Spectrum test;

			if (EMITTER_VPL) {

				if (VPL_ON_SURFACE)
				{
					PositionSamplingRecord pRec(vpl.its, ESolidAngle);
					DirectionSamplingRecord dRec(vplWo, ESolidAngle);
					result *= emitter->evalDirection(dRec, pRec); // * cos

				}
				else
				{
					PositionSamplingRecord pRec(vpl.its, ESolidAngle);
					DirectionSamplingRecord dRec(vplWo, ESolidAngle);

					result *= emitter->evalDirection(dRec, pRec);
				}
				
			}
 

			// bsdf
			BSDFSamplingRecord bRec(its, wo);
			const Spectrum v = bsdf->eval(bRec);

			cout << " 3 " << v.toString() << endl;

			result *= v;
	
			if (!DIRECTIONAL_VPL)
			{
				result *= (1.0f / std::max(distSqr, minDistSqr));

				if (result.getLuminance() > 0.0001)
				{
					Ray ray(its.p, woWorld, Epsilon, dist * (1 - ShadowEpsilon), 0);
					if (scene->rayIntersect(ray))
					{
						result = Spectrum(0.0f);
					}
				}
			}
			else
			{
				
				if (result.getLuminance() > 0.0001)
				{
					Ray ray(its.p, woWorld, Epsilon, dist * (1 - ShadowEpsilon), 0);
					if (scene->rayIntersect(ray))
					{
						result = Spectrum(0.0f);
					}
				}
			}
		
			Li += result;
		}

		

		//Spectrum value = scene->sampleEmitterDirect(dRec, rRec.nextSample2D());
		//if (!value.isZero()) {
		//	const Emitter *emitter = static_cast<const Emitter *>(dRec.object);

		//	/* Allocate a record for querying the BSDF */
		//	BSDFSamplingRecord bRec(its, its.toLocal(dRec.d), ERadiance);

		//	/* Evaluate BSDF * cos(theta) */
		//	const Spectrum bsdfVal = bsdf->eval(bRec);

		//	/* Prevent light leaks due to the use of shading normals */
		//	if (!bsdfVal.isZero() && (!m_strictNormals
		//		|| dot(its.geoFrame.n, dRec.d) * Frame::cosTheta(bRec.wo) > 0)) {

		//		/* Calculate prob. of having generated that direction
		//		using BSDF sampling */
		//		Float bsdfPdf = (emitter->isOnSurface() && dRec.measure == ESolidAngle)
		//			? bsdf->pdf(bRec) : 0;

		//		/* Weight using the power heuristic */
		//		Float weight = miWeight(dRec.pdf, bsdfPdf);
		//		Li += throughput * value * bsdfVal * weight;
		//	}
		
	}

	



	return Li;
}

#endif // 0

Spectrum ManyLightIntegrator::nvLi(const RayDifferential &ray, RadianceQueryRecord &rRec, bool scattered, uint32_t &shadow_count) const
{

	/* Some aliases and local variables */
	const Scene *scene = rRec.scene;
	Intersection &its = rRec.its;

	Spectrum Li = Spectrum(0.0f);

	if (!its.isValid()) {
		/* If no intersection could be found, potentially return
		radiance from a environment luminaire if it exists */
		if ((rRec.type & RadianceQueryRecord::EEmittedRadiance)
			&& (!m_hideEmitters || scattered)) {
			Spectrum v = scene->evalEnvironment(ray);
			Float m = v.max();
			if (m > 1.0f) {
				v /= m;
			}
			Li += v;
		}
			
		
		return Li;
	}

	/* Possibly include emitted radiance if requested */
	if (its.isEmitter()/* && (rRec.type & RadianceQueryRecord::EEmittedRadiance)
		&& (!m_hideEmitters || scattered)*/) {
		Spectrum v = its.Le(-ray.d);
		Float m = v.max();
		if (m > 1.0f) {
			v /= m;
		}
		Li += v;
	}
		


	/* Include radiance from a subsurface scattering model if requested */
	/*if (its.hasSubsurface() && (rRec.type & RadianceQueryRecord::ESubsurfaceRadiance)) {
		Spectrum v = its.LoSub(scene, rRec.sampler, -ray.d, rRec.depth);
		Float m = v.max();
		if (m > 1.0f) {
			v /= m;
		}
		Li += v;
	}*/


	return Li;
}


void ManyLightIntegrator::collectVSensor(const RayDifferential &r, RadianceQueryRecord &rRec, SamplePath *samplePath,
		Sampler* sampler) const
{

	//if (//Point2i(samplePath->getPos()) == (Point2i(103, 604)) ||
	//	//TAABB<Point2i>(Point2i(200, 300), Point2i(300, 400)).contains(Point2i(samplePath->getPos())) ||
	//	TAABB<Point2i>(Point2i(577, 862), Point2i(644, 920)).contains(Point2i(samplePath->getPos())) ||
	//	//TAABB<Point2i>(Point2i(280, 360), Point2i(320, 380)).overlaps(sn->sampleBbox) ||
	//	0) {
	//	
	//	//	sample_ratio *= 5;
	//}
	//else {
	//	return;
	//}

	static Float ii = 0;
	Spectrum Li(0.0f);
	Spectrum throughput(1.0f);
	Float eta = 1.0f;

	uint32_t shadow_count = 0;

	/* Some aliases and local variables */
	const Scene *scene = rRec.scene;
	//Intersection &its = rRec.its;
	RayDifferential ray(r);
	bool scattered = false;
	

	/* Perform the first ray intersection (or ignore if the
	intersection has already been provided). */
	rRec.rayIntersect(ray);
	ray.mint = Epsilon;

	while (rRec.depth <= m_vpsMaxDepth || m_vpsMaxDepth < 0) {
		if (!rRec.its.isValid()) {
			samplePath->addLi(throughput * nvLi(ray, rRec, scattered, shadow_count));
			samplePath->hit_background = true;
			break;
		}

        if(rRec.its.isEmitter() || rRec.its.getBSDF()->getType() & BSDF::ESmooth){
        	if(rRec.its.isEmitter()){
        		samplePath->addLi(throughput * nvLi(ray, rRec, scattered, shadow_count));
        	}

        	samplePath->emplaceSensor(rRec.its, throughput);
        	break;
        }


		const BSDF *bsdf = rRec.its.getBSDF();
	
		BSDFSamplingRecord bsdf_sample_record(rRec.its, sampler);
        bsdf_sample_record.typeMask = rRec.its.getBSDF()->isDielectric() ? BSDF::EDeltaTransmission | BSDF::ENull : 
        	BSDF::EDeltaReflection;

        rRec.its.getBSDF()->sample(bsdf_sample_record, sampler->next2D());
        ray = Ray(rRec.its.p, bsdf_sample_record.its.toWorld(bsdf_sample_record.wo), ray.time);
        ray.mint = Epsilon;
        scene->rayIntersect(ray, rRec.its);

        rRec.depth++;
	}
	

	// reflection ?
	
}




bool ManyLightIntegrator::render(Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID)
{

	ref<Scheduler> sched = Scheduler::getInstance();
	int integratorResID = sched->registerResource(this);

	ref<Sensor> sensor = static_cast<Sensor *>(sched->getResource(sensorResID));
	ref<Film> film = sensor->getFilm();

	size_t nCores = sched->getCoreCount();
	Sampler *sampler = static_cast<Sampler *>(sched->getResource(samplerResID, 0));
	size_t sampleCount = sampler->getSampleCount();
	

	Log(EInfo, "Starting render job (%ix%i, " SIZE_T_FMT " %s, " SIZE_T_FMT
		" %s, " SSE_STR ") ..", film->getCropSize().x, film->getCropSize().y,
		sampleCount, sampleCount == 1 ? "sample" : "samples", nCores,
		nCores == 1 ? "core" : "cores");


	/* Render VSensor - parallelize */
	//ref<ParallelProcess> proc = getRenderProcess(scene, queue, job, sceneResID, sensorResID, samplerResID, integratorResID);
	ref<ParallelProcess> proc = getRenderProcess(scene, queue, job, sceneResID, sensorResID, m_sliceSamplerResID, integratorResID);
	
	scene->bindUsedResources(proc);
	bindUsedResources(proc);

	sched->schedule(proc);

	m_process = proc;

	ref<Timer> timer = new Timer();
	sched->wait(proc);
	m_process = NULL;
	cout << endl << "Core Time: " << timer->getSecondsSinceStart() << endl;


	sched->unregisterResource(integratorResID);

	return proc->getReturnStatus() == ParallelProcess::ESuccess;

}

void ManyLightIntegrator::bindUsedResources(ParallelProcess *proc) const
{
	if (m_vpl_map.get())
		proc->bindResource("vplMap", m_vplmap_id);

	if (m_sensor_map.get())
		proc->bindResource("sensorMap", m_sensormap_id);


}


void ManyLightIntegrator::senseBlock(const Scene *scene, const Sensor *sensor, Sampler *sampler, VSensorBlock *block, const bool &stop, const std::vector< TPoint2<uint8_t> > &points) 
{
	Properties props("independent");
    ref<Sampler> ind_sampler = static_cast<Sampler*>(PluginManager::getInstance()->createObject(MTS_CLASS(Sampler), props));
    sampler->configure();
    sampler->generate(Point2i(0));

	Float diffScaleFactor = 1.0f /
		std::sqrt((Float)sampler->getSampleCount());

	bool needsApertureSample = sensor->needsApertureSample();
	bool needsTimeSample = sensor->needsTimeSample();

	RadianceQueryRecord rRec(scene, sampler);
	Point2 apertureSample(0.5f);
	Float timeSample = 0.5f;
	RayDifferential sensorRay;

	block->clear();
	
	uint32_t queryType = RadianceQueryRecord::ESensorRay;


	if (!sensor->getFilm()->hasAlpha()) /* Don't compute an alpha channel if we don't have to */
		queryType &= ~RadianceQueryRecord::EOpacity;
	


	for (size_t i = 0; i < points.size(); ++i) {
		Point2i offset = Point2i(points[i]) + Vector2i(block->getOffset());

		if (stop)
			break;

		sampler->generate(offset);
	
		for (size_t j = 0; j < sampler->getSampleCount(); j++) {
	
			rRec.newQuery(queryType, sensor->getMedium());
			//ref<Random> random = new Random((uint64_t)0);

			Point2 samplePos(offset.x, offset.y);		
			if (sampler->getSampleCount() > 1) {
				Point2 off = rRec.nextSample2D();

				//off.x = random->nextFloat();
				//off.y = random->nextFloat();
			
				off.x = math::clamp<Float>(off.x, 1e-5f, 1.0f - 1e-5); // avoid zero sample
				off.y = math::clamp<Float>(off.y, 1e-5f, 1.0f - 1e-5);
				samplePos += off;
			}
			else {
				samplePos += Point2(0.5f); // move to the pixel center
			}



			if (needsApertureSample)
				apertureSample = rRec.nextSample2D();
			if (needsTimeSample)
				timeSample = rRec.nextSample1D();

			Spectrum spec = sensor->sampleRayDifferential(
				sensorRay, samplePos, apertureSample, timeSample);
		
			sensorRay.scaleDifferential(diffScaleFactor);

			SamplePath *sample_path = SamplePath::create();
			sample_path->setPos(samplePos);
			sample_path->setImportance(spec);

			collectVSensor(sensorRay, rRec, sample_path, ind_sampler);


			sample_path->setAlpha(rRec.alpha);
			

			block->put(sample_path);
			

			sampler->advance();
		}
	}
}



void ManyLightIntegrator::put(const VSensorBlock *sensor_block)
{
	
	const std::vector<pool_ref<SamplePath>> &sp = sensor_block->getSamplePath();
	//typedef std::list<VSensor, boost::fast_pool_allocator<VSensor>> IT;
	typedef std::vector<VSensor> IT;

	for (std::vector<pool_ref<SamplePath>>::const_iterator itr = sp.begin(); itr != sp.end(); itr++) {
		m_sensor_map->m_sample_pathes.push_back(*itr);
		//const IT &vs = (*itr)->getSensors();
		//for (IT::const_iterator i = vs.begin(); i != vs.end(); i++) {
		//	m_sensor_map->m_sensor_tree.m_leaves.push_back(*i);
		//}

	}

}

void ManyLightIntegrator::put(const VPLGenerateResult *vplResult)
{

	uint32_t ori_size = m_vpl_map->raw_vpl.size();
	m_vpl_map->raw_vpl.resize(m_vpl_map->raw_vpl.size() + vplResult->m_vpls.size(), vplResult->m_vpls[0]);
	std::copy(vplResult->m_vpls.begin(), vplResult->m_vpls.end(), m_vpl_map->raw_vpl.begin() + ori_size);
	m_vpl_offset = std::max(m_vpl_offset, vplResult->offset);
	m_vpl_path_count += vplResult->path_count;

	//m_vpl_map->raw_vpl.reserve(m_vpl_map->raw_vpl.size() + vplResult->m_vpls.size());
	//m_vpl_map->raw_vpl.insert(m_vpl_map->raw_vpl.end(), vplResult->m_vpls.begin(), vplResult->m_vpls.end());

}

 
void ManyLightIntegrator::postprocessSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *result) {


}


void ManyLightIntegrator::renderSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *result) 
{


	std::stack<const VSensorNode *> stack;
	stack.push(slice_root);
	while (!stack.empty()) {

		if (stop)
			break;

		const VSensorNode *n = stack.top();
		stack.pop();

		if (n->isLeaf()) {
			 
			const VSensor *sensor = n->rep_sensor;


//			if (//TAABB<Point2i>(Point2i(230-100, 216-20), Point2i(261+20, 240+20)).contains(sensor->getImagePos()) ||
//				TAABB<Point2i>(Point2i(189, 127), Point2i(216, 147)).contains(sensor->getImagePos()) ||
////  				TAABB<Point2i>(Point2i(341, 211), Point2i(369, 234)).contains(sensor->getImagePos()) ||
////  				TAABB<Point2i>(Point2i(559, 734), Point2i(584, 768)).contains(sensor->getImagePos()) ||
//// 			    TAABB<Point2i>(Point2i(118, 216), Point2i(207, 233)).contains(sensor->getImagePos()) ||
//// 			    TAABB<Point2i>(Point2i(270, 456), Point2i(280, 466)).contains(sensor->getImagePos()) ||
////  				TAABB<Point2i>(Point2i(489, 404), Point2i(497, 414)).contains(sensor->getImagePos()) ||
//				0)
//			{
//
//			}
//			else {
//				continue;
//			}
//

			Spectrum Li(0.0f);
			uint32_t shadow_count = 0;
			 
			{
				
				const std::vector<VOrientedLight> &ori_lights = m_vpl_map->m_oriented_tree.m_leaves; 
				for (std::vector<VOrientedLight>::const_iterator itr = ori_lights.begin(); itr != ori_lights.end(); itr++)
				{
					Li += sensor->strength * itr->intensity * evalMTDV(&(*itr), sensor, scene, shadow_count);
 				}
			}

			{
				const std::vector<VOmniLight> &omni_lights = m_vpl_map->m_omni_tree.m_leaves;
				for (std::vector<VOmniLight>::const_iterator itr = omni_lights.begin(); itr != omni_lights.end(); itr++)
				{
					Li += sensor->strength * itr->intensity * evalMTDV(&(*itr), sensor, scene, shadow_count);
				}
			}

			{
				const std::vector<VDirectionalLight> &dir_lights = m_vpl_map->m_directional_tree.m_leaves;
				for (std::vector<VDirectionalLight>::const_iterator itr = dir_lights.begin(); itr != dir_lights.end(); itr++)
				{
					Li += sensor->strength * itr->intensity * evalMTDV(&(*itr), sensor, scene, shadow_count);
				}
			}

			result->put(sensor, Li);
		}
		else {

			stack.push(n->right);
			stack.push(n->left);
		}
	

	}

}


void ManyLightIntegrator::postprocess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID)
{

	if (m_postprocess_slice) {

		ref<Scheduler> sched = Scheduler::getInstance();
		int integratorResID = sched->registerResource(this);

		ref<ParallelProcess> proc = getPostProcess(scene, queue, job, sceneResID, sensorResID, samplerResID, integratorResID);

		scene->bindUsedResources(proc);
		bindUsedResources(proc);

		sched->schedule(proc);

		m_process = proc;

		sched->wait(proc);
		m_process = NULL;
	}

	if (m_backGroundFile != "" && std::ifstream(m_backGroundFile)) {

		ref<Bitmap> bitmap = new Bitmap(fs::path(m_backGroundFile));

		uint32_t w = bitmap->getWidth();
		uint32_t h = bitmap->getHeight();

		Vector2i size = scene->getSensor()->getFilm()->getSize();

		std::vector<pool_ref<SamplePath>> &sample_pathes = m_sensor_map->m_sample_pathes;

		for (auto path : sample_pathes) {
			if (path->hit_background) {
				Point2 p = path->getPos();
				Point2i t(p.x * w / size.x, p.y * h / size.y);
				t.x = std::min(t.x, (int)w - 1);
				t.y = std::min(t.y, (int)h - 1);
				Spectrum bg = bitmap->getPixel(t);
				path->setLi(bg);
			}
		}
		
	}


	ref<Scheduler> sched = Scheduler::getInstance();
	ref<Sensor> sensor = static_cast<Sensor *>(sched->getResource(sensorResID));
	ref<Film> film = sensor->getFilm(); 

	Bitmap::EPixelFormat m_pixelFormat = Bitmap::ESpectrumAlphaWeight;
	int m_channelCount = -1;
	bool m_warnInvalid = true;
	Vector2i size = film->getSize();
	int borderSize = film->getReconstructionFilter()->getBorderSize();
	Point2i offset = Point2i(0, 0);

	if (film->hasHighQualityEdges()) {
		offset.x -= borderSize;
		offset.y -= borderSize;
		size.x += 2 * borderSize;
		size.y += 2 * borderSize;
	}
	
	ref<ImageBlock> block =  new ImageBlock(m_pixelFormat,
		size,
		film->getReconstructionFilter(),
		m_channelCount, m_warnInvalid);
 
	block->setOffset(offset);
	block->clear();

	 
	std::vector<pool_ref<SamplePath>> &sample_pathes = m_sensor_map->m_sample_pathes;
	for (std::vector<pool_ref<SamplePath>>::iterator itr = sample_pathes.begin(); itr != sample_pathes.end(); itr++) {
		//Point2i pi((*itr)->getPos());
		//Point2 p(0.5f);
		//p.x += pi.x; p.y += pi.y;

		//block->put(p, (*itr)->getLi() * (*itr)->getImportance(), (*itr)->getAlpha());

		block->put((*itr)->getPos(), (*itr)->getLi() * (*itr)->getImportance(), (*itr)->getAlpha());
	}

	
	film->put(block);
	
	queue->signalWorkEnd(job, block, m_cancel);

}



ref<ParallelProcess> ManyLightIntegrator::getSenseProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID)
{
	
	ref<ParallelProcess> proc = new BlockSenseProcess(job, queue, scene->getBlockSize());
	
	proc->bindResource("scene", sceneResID);
	proc->bindResource("sensor", sensorResID);
	proc->bindResource("sampler", samplerResID);
	proc->bindResource("integrator", integratorResID);

	return proc;
}

ref<ParallelProcess> ManyLightIntegrator::getVPLProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID) {

	ref<ParallelProcess> proc = new VPLGenerateProcess(job, queue, m_vpl_count, m_vplMaxDepth);

	proc->bindResource("scene", sceneResID);
	proc->bindResource("sensor", sensorResID);
	proc->bindResource("sampler", samplerResID);
	proc->bindResource("integrator", integratorResID);

	return proc;
}

ref<ParallelProcess> ManyLightIntegrator::getRenderProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID)
{
	ref<WorkProcessor> render = new SliceRender(m_sliceSize);
	ref<ParallelProcess> proc = new SliceProcess(job, queue, render, "Rendering Slice");

	proc->bindResource("scene", sceneResID);
	proc->bindResource("sensor", sensorResID);
	proc->bindResource("sampler", samplerResID);
	proc->bindResource("integrator", integratorResID);

	return proc;
}

ref<ParallelProcess> ManyLightIntegrator::getPostProcess(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID)
{
	ref<WorkProcessor> render = new SlicePostProcessor(m_sliceSize);
	ref<ParallelProcess> proc = new SliceProcess(job, queue, render, "Postprocessing Slice");

	proc->bindResource("scene", sceneResID);
	proc->bindResource("sensor", sensorResID);
	proc->bindResource("sampler", samplerResID);
	proc->bindResource("integrator", integratorResID);

	return proc;
}

ManyLightIntegrator::~ManyLightIntegrator()
{
	ref<Scheduler> sched = Scheduler::getInstance();

	if (m_sliceSamplerResID != -1) {
		//sched->unregisterResource(m_sliceSamplerResID);
		m_sliceSamplerResID = -1;
	}

	if (m_sensormap_id != -1) {
		sched->unregisterResource(m_sensormap_id);
		m_sensormap_id = -1;
	}

	if (m_vplmap_id != -1) {
		sched->unregisterResource(m_vplmap_id);
		m_vplmap_id = -1;
	}

	PathPool::release_memory();
}


void ManyLightIntegrator::fixSensor(const Scene *scene)
{
	if (scene->getSampler()->getSampleCount() > 1) return;

	const Film *film = scene->getFilm();
	Vector2i s = film->getSize();
	int width = s.x;
	int height = s.y;


	struct T {
		pool_ref<SamplePath> path;
		int normal_sign[3];
		T() {
			normal_sign[0] = normal_sign[1] = normal_sign[2] = 0;
		}
	};

	std::vector<T> image(width * height);

	for (std::vector<pool_ref<SamplePath>>::iterator itr = m_sensor_map->m_sample_pathes.begin(); itr != m_sensor_map->m_sample_pathes.end(); itr++) {
		const std::vector<VSensor> &sensors = (*itr)->getSensors();
		if (sensors.size() > 0) {
			Point2i p = sensors[0].getImagePos();
			if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) continue;

			int off = p.x + p.y * width;
			image[off].path = (*itr);
			Vector n = sensors[0].its.geoFrame.n;
			for (int i = 0; i < 3; i++)
				image[off].normal_sign[i] = n[i] < 0 ? -1 : std::signbit(n[i]);
		}
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {

			int off = x + y * width;
			if (image[off].path.get() == NULL) continue;

			int n_off[4];
			n_off[0] = x > 0 ? off - 1 : off; 
			n_off[1] = x < width - 1 ? off + 1 : off;
			n_off[2] = y > 0 ? off - width : off;
			n_off[3] = y < height - 1 ? off + width : off;

			for (int i = 0; i < 4; i++) {
				if (image[n_off[i]].path.get() == nullptr) {
					n_off[i] = off;
				}
			}

			if (n_off[0] == off && n_off[1] == off && n_off[2] == off && n_off[3] == off) {
				continue;
			}

			bool singular = false;			
			for (int i = 0; i < 3; i++) {
				if ((n_off[0] == off ? true : image[n_off[0]].normal_sign[i] != image[off].normal_sign[i]) &&
					(n_off[1] == off ? true : image[n_off[1]].normal_sign[i] != image[off].normal_sign[i]) &&
					(n_off[2] == off ? true : image[n_off[2]].normal_sign[i] != image[off].normal_sign[i]) &&
					(n_off[3] == off ? true : image[n_off[3]].normal_sign[i] != image[off].normal_sign[i])) {
					singular = true;
					break;
				}
			}

			if (singular) {
	
				int o = std::min((int)((float)std::rand() / RAND_MAX) * 4, (int)3);
				while (n_off[o] == off/* || image[n_off[o]].path.get() == nullptr*/) {
					o = (o + 1) % 4;
				}

				image[off].path->copySensors(*(image[n_off[o]].path.get()));

			}
		}
	}
	

}

void ManyLightIntegrator::initSensorLeaves()
{
	if (m_sensor_map->m_sensor_tree.m_leaves.size() == 0) {
		std::vector<pool_ref<SamplePath>> &sp = m_sensor_map->m_sample_pathes;
		typedef std::vector<VSensor> IT;

		for (std::vector<pool_ref<SamplePath>>::const_iterator itr = sp.begin(); itr != sp.end(); itr++) {

			const IT &vs = (*itr)->getSensors();
			for (IT::const_iterator i = vs.begin(); i != vs.end(); i++) {
				m_sensor_map->m_sensor_tree.m_leaves.push_back(*i);
			}

		}
	}
}

class SliceCluster : public SliceRender {

public:
	SliceCluster(int ms) : SliceRender(ms) { }


	void process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop) {

		const SliceWorkUnit *rect = static_cast<const SliceWorkUnit *>(workUnit);
		SliceWorkResult *result = static_cast<SliceWorkResult *>(workResult);

#ifdef MTS_DEBUG_FP
		enableFPExceptions();
#endif

		m_integrator->clusterSlice(m_scene, m_sensor, m_sampler, stop, rect->getSliceRoot(), result);

#ifdef MTS_DEBUG_FP
		disableFPExceptions();
#endif

	}

	virtual ref<WorkProcessor> clone() const {
		return new SliceCluster(m_sliceSize);
	}

	MTS_DECLARE_CLASS()

protected:
	virtual ~SliceCluster() { }
};

//
//
//void ManyLightIntegrator::clusterSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *slice_result)
//{
//	ref<Random> random = new Random();
//	const VSensorNode *oriSlice = slice_root;
//	auto gauss = [](Float x, Float u, Float g) {
//		auto sqr = [](Float x) {
//			return x * x;
//		};
//		return math::fastexp(-sqr(x - u) / (sqr(g) * 2)) / (math::safe_sqrt(2.0 * M_PI) * g);
//	};
//
//
//	AABB oriSliceBox = oriSlice->bbox;
//	std::vector < std::pair < const VSensorNode*, int > > sliceNodes;
//	m_sensor_map->calAdjSlices(oriSlice, sliceNodes, m_sliceSize);
//	std::queue <const VSensorNode*> sensorQueue;
//	sensorQueue.push(oriSlice);
//	while (!sensorQueue.empty()) {
//		const VSensorNode *curNode = sensorQueue.front();
//		sensorQueue.pop();
//		if (curNode->isLeaf()) {
//			VSensor* sensor = const_cast<VSensor*>(curNode->rep_sensor);
//
//			Float minDis = 1e30, oriDis;
//			int mi = -1, oi = -1;
//			for (int i = 0; i < sliceNodes.size(); ++i) {
//				if (!sliceNodes[i].first->bbox.contains(sensor->its.p)) {
//					Float curDis = sliceNodes[i].first->bbox.distanceTo(sensor->its.p) + 1e-4;
//					if (curDis < minDis) {
//						minDis = curDis;
//						mi = i;
//					}
//				}
//				else {
//					oriDis = distance(sliceNodes[i].first->bbox.getCenter(), sensor->its.p) + 1e-4;
//					oi = i;
//				}
//			}
//			int res = -1;
//			if (oriDis < minDis * 5)
//				res = oi;
//			else
//				res = random->nextFloat() < 0.5 ? oi : mi;
//			sensor->clusterCenter = sliceNodes[res].second;
//
//		}
//		else {
//			sensorQueue.push(curNode->left);
//			sensorQueue.push(curNode->right);
//		}
//	}
//
//}

void ManyLightIntegrator::clusterSlice(const Scene *scene, const Sensor *sensor, Sampler *sampler, const bool &stop, const VSensorNode *slice_root, SliceWorkResult *slice_result)
{
	ref<Random> random = new Random((uint64_t)0);
	const VSensorNode *oriSlice = slice_root;


	struct KNeighbor {
		 
#define K_NEIGHBOR 3
		int r_id[K_NEIGHBOR];
		Float distance[K_NEIGHBOR];

		void init() {
			for (int i = 0; i < K_NEIGHBOR; i++) {
				r_id[i] = -1; distance[i] = FLT_MAX; 
			}
		}

		Float gauss(Float x, Float g) {
			auto sqr = [](Float x) {
				return x * x;
			};
			return math::fastexp(-sqr(x) / ((g) * 2)) / (math::safe_sqrt(2.0 * M_PI) * g);
		};

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

			Float th = 2 * distance[0];
			Float nor = 1.0f / std::max(1e-6f, distance[0]);

			 
			if (0) {
				
				Float pdf[K_NEIGHBOR];
				Float pdf_sum = 0;
				for (int i = 0; i < K_NEIGHBOR; i++) {
					pdf[i] = (r_id[i] >= 0 && distance[i] <= th) ? gauss(nor * (distance[i] - distance[0]), 0.02f) : 0;
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
				while (n >= 0 && (r_id[n] < 0 || distance[n] > th)) {
					n--;
				}

				assert(n >= 0);
				
				int rep_r = r_id[random->nextUInt(n + 1)];
				assert(rep_r >= 0);
				return rep_r;
			}
		}

#undef K_NEIGHBOR
	};

	AABB oriSliceBox = oriSlice->bbox;
	std::vector < std::pair < const VSensorNode*, int > > sliceNodes;
	m_sensor_map->calAdjSlices(oriSlice, sliceNodes, m_sliceSize);


	std::vector<Float> dots;
	dots.reserve(sliceNodes.size());


	std::vector < std::pair < const VSensorNode*, int >> temp;
	temp.reserve(sliceNodes.size());

	std::queue <const VSensorNode*> sensorQueue;
	sensorQueue.push(oriSlice);

	KNeighbor neighbor;
	while (!sensorQueue.empty()) {

		const VSensorNode *curNode = sensorQueue.front();
		sensorQueue.pop();
		if (curNode->isLeaf()) {
			VSensor* sensor = const_cast<VSensor*>(curNode->rep_sensor);

			// filter out incoherent normal 
			dots.clear();
			Float max_d = -FLT_MAX;
			for (auto t : sliceNodes) {
				//Float d = dot(sensor->its.geoFrame.n, t.first->normal_cone.GetAxis());
				Float d = dot(sensor->its.geoFrame.n, t.first->rep_sensor->its.geoFrame.n);
				max_d = std::max(d, max_d);
				dots.push_back(d);
			}

			temp.clear();
			Float th = max_d - 0.2;
			for (int i = 0; i < sliceNodes.size(); i++) {
				if (dots[i] >= th) {
					temp.push_back(sliceNodes[i]);
				}
			}


			Float oriDis = -1;
			int oi = -1;

			neighbor.init();
			Float minDis = neighbor.getHeadDistance();
			Float maxDot = 0;

			for (int i = 0; i < temp.size(); ++i) {
				if (temp[i].first == oriSlice) {
					oriDis = distance(temp[i].first->bbox.getCenter(), sensor->its.p);
					oi = i;
				}
				
				//Float curDis = sliceNodes[i].first->bbox.distanceTo(sensor->its.p);
				Float curDis = distance(sensor->its.p, temp[i].first->bbox.getCenter());
				if (curDis < minDis) {
					neighbor.push(curDis, i);
					minDis = neighbor.getHeadDistance();						
				}				
			}	

			int res = neighbor.select(random);
			sensor->clusterCenter = temp[res].second;

		}
		else {
			sensorQueue.push(curNode->left);
			sensorQueue.push(curNode->right);
		}
	}

}


void ManyLightIntegrator::buildSensorMap(const Scene *scene, RenderQueue *queue, const RenderJob *job, int sceneResID, int sensorResID, int samplerResID, int integratorResID)
{
	initSensorLeaves();
	
	if (!m_kmeansFlag) 
	{
		//m_random->seed((uint64_t)0);
		m_sensor_map->build(m_random);
		
		divideSlices();
	
		if (m_randomFlag) {

			//initClustersByRandom();

			// parallel
			m_sensor_map->threshold = 4e-2;
			ref<Scheduler> sched = Scheduler::getInstance();

			ref<WorkProcessor> cluster = new SliceCluster(m_sliceSize);
			ref<ParallelProcess> proc = new SliceProcess(job, queue, cluster, "Clustering Slice");

			proc->bindResource("scene", sceneResID);
			proc->bindResource("sensor", sensorResID);
			proc->bindResource("sampler", samplerResID);
			proc->bindResource("integrator", integratorResID);

			m_process = proc;

			sched->schedule(proc);
			sched->wait(proc);
			m_process = NULL;

			m_sensor_map->buildByClusterCenters(m_random);
		}
		
	}
	else 
	{
		initClustersByKmeans(scene);
		for (auto& sensor : m_sensor_map->m_sensor_tree.m_leaves) {
			Float minDis = 1e30;
			int k = -1;
			for (int i = 0; i < m_clusterNum; ++i) {
				Float dis = (sensor.its.p - m_clusterCenters[i].p).lengthSquared();
				if (dis < minDis) {
					minDis = dis;
					k = i;
				}
			}
			sensor.clusterCenter = k;
		}
		m_sensor_map->buildByClusterCenters(m_random);
	}
}

void ManyLightIntegrator::initClustersByRandom()
{
	const VSensorNode *root = m_sensor_map->m_sensor_tree.m_root;
	std::vector<VSensor>& sensors = m_sensor_map->m_sensor_tree.m_leaves;
	ref<Random> random = new Random();

	auto gauss = [](Float x, Float u, Float g) {
		auto sqr = [](Float x) {
			return x * x;
		};
		return math::fastexp(-sqr(x - u) / (sqr(g) * 2)) / (math::safe_sqrt(2.0 * M_PI) * g);
	};

	m_sensor_map->threshold = 4e-2;

	std::queue <const VSensorNode*> sliceQueue;
	sliceQueue.push(root);
	while (!sliceQueue.empty()) {
		const VSensorNode *oriSlice = sliceQueue.front();
		sliceQueue.pop();
		if (oriSlice->isSlice) {
			AABB oriSliceBox = oriSlice->bbox;
			std::vector < std::pair < const VSensorNode*, int > > sliceNodes;
			m_sensor_map->calAdjSlices(oriSlice, sliceNodes, m_sliceSize);
			std::queue <const VSensorNode*> sensorQueue;
			sensorQueue.push(oriSlice);
			while (!sensorQueue.empty()) {
				const VSensorNode *curNode = sensorQueue.front();
				sensorQueue.pop();
				if (curNode->isLeaf()) {
					VSensor* sensor = const_cast<VSensor*>(curNode->rep_sensor);

					Float minDis = 1e30, oriDis;
					int mi = -1, oi = -1;
					for (int i = 0; i < sliceNodes.size(); ++i) {
						if (!sliceNodes[i].first->bbox.contains(sensor->its.p)) {
							Float curDis = sliceNodes[i].first->bbox.distanceTo(sensor->its.p) + 1e-4;
							if (curDis < minDis) {
								minDis = curDis;
								mi = i;
							}
						} else {
							oriDis = distance(sliceNodes[i].first->bbox.getCenter(), sensor->its.p) + 1e-4;
							oi = i;
						}
					}
					int res = -1;
					if (oriDis < minDis * 5)
						res = oi;
					else
						res = random->nextFloat() < 0.5 ? oi : mi;
					sensor->clusterCenter = sliceNodes[res].second;

				} else {
					sensorQueue.push(curNode->left);
					sensorQueue.push(curNode->right);
				}
			}

		} else {
			sliceQueue.push(oriSlice->left);
			sliceQueue.push(oriSlice->right);
		}
	}
	
}


void ManyLightIntegrator::initClustersByKmeans(const Scene *scene)
{
	const VPointTree<VSensor>& sensorTree = m_sensor_map->m_sensor_tree;
	const Film *film = scene->getFilm();
	Vector2i size = film->getSize();
	int width = size.x;
	int height = size.y;
	
	const std::vector<VSensor>& sensors = sensorTree.m_leaves;
	m_clusterNum = sensors.size() / (width + height) + 10;

	m_clusterCenters.reserve(m_clusterNum);
	for (int i = 0; i < m_clusterNum; ++i)
		m_clusterCenters.push_back(sensors[rand() % sensors.size()].its);

	std::vector < int > clusterSizes(m_clusterNum);
	std::vector < Intersection > newClusterCenters;
	for (int d = 0; d < m_iterationNum; ++d) {
		printf("iterate %d\n", d);
		newClusterCenters.clear();
		for (int i = 0; i < m_clusterNum; ++i)
			newClusterCenters.emplace_back();
		std::fill(clusterSizes.begin(), clusterSizes.end(), 0);

		for (auto& sensor : sensors) {
			Float minDis = 1e30;
			int k = -1;
			for (int i = 0; i < m_clusterNum; ++i) {
				Float dis = (sensor.its.p - m_clusterCenters[i].p).lengthSquared();
				if (dis < minDis) {
					minDis = dis;
					k = i;
				}
			}
			++clusterSizes[k];
			newClusterCenters[k].p += sensor.its.p;
		}
		for (int i = 0; i < m_clusterNum; ++i)
			if (clusterSizes[i])
				newClusterCenters[i].p /= clusterSizes[i];
			else
				newClusterCenters[i] = sensors[rand() % sensors.size()].its;
		m_clusterCenters.swap(newClusterCenters);
	}
}


void ManyLightIntegrator::divideSlices()
{
	VSensorNode *root = m_sensor_map->m_sensor_tree.m_root;
	std::vector<VSensor>& sensors = m_sensor_map->m_sensor_tree.m_leaves;
	Vector extend = root->bbox.getExtents();
	Float l = (extend.x + extend.y + extend.y) / 30;
	std::stack<VSensorNode *> stack;
	stack.push(root);

	int min_threshold = m_sliceSize / 2;

	while (!stack.empty()) {

		VSensorNode *n = stack.top();
		stack.pop();
		
		if (n->num_sensor <= m_sliceSize && ((n->num_sensor <= min_threshold) || (n->left->bbox.distanceTo(n->right->bbox) < l))) {
			//if (n->num_sensor <= m_sliceSize && (n->num_sensor <= m_sliceSize / 2 || (n->left->bbox.getSurfaceArea() + n->right->bbox.getSurfaceArea()) > 0.5 * n->bbox.getSurfaceArea())) {
			n->isSlice = true;
		} else {
			assert(!n->isLeaf());
			stack.push(n->right);
			stack.push(n->left);
		}
	}
}




//MTS_IMPLEMENT_CLASS(SlicePostProcess, false, SliceRenderProcess)
//MTS_IMPLEMENT_CLASS(SlicePostProcessor, false, SliceRender)

MTS_IMPLEMENT_CLASS(SliceCluster, false, SliceRender)
MTS_IMPLEMENT_CLASS(ManyLightIntegrator, false, MonteCarloIntegrator)
#if PLUGIN_EXPORT == PLUGIN_EXPORT_MANY_LIGHT
MTS_EXPORT_PLUGIN(ManyLightIntegrator, "Many-light integrator");
#endif
MTS_NAMESPACE_END