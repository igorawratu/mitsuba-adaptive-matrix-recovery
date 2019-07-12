#include "VPLGenerateProcess.h"
#include <mitsuba/core/statistics.h>
#include <mitsuba/core/sfcurve.h>
#include <mitsuba/render/renderproc.h>
#include <mitsuba/render/rectwu.h>

#include "ManyLightIntegrator.h"


MTS_NAMESPACE_BEGIN

class VPLGenerator : public WorkProcessor {
public:
	VPLGenerator(bool warnInvalid) : m_warnInvalid(warnInvalid) { }

	VPLGenerator(Stream *stream, InstanceManager *manager) {
		//TODO
		AssertEx(0, "unfinished");
	}

	ref<WorkUnit> createWorkUnit() const {
		return new RectangularWorkUnit();
	}

	ref<WorkResult> createWorkResult() const {
		return new VPLGenerateResult();
	}

	void prepare() {

		Scene *scene = static_cast<Scene *>(getResource("scene"));
		m_scene = new Scene(scene);
		m_sampler = static_cast<Sampler *>(getResource("sampler"));
		m_sensor = static_cast<Sensor *>(getResource("sensor"));
		m_integrator = static_cast<ManyLightIntegrator *>(getResource("integrator"));
		m_scene->removeSensor(scene->getSensor());
		m_scene->addSensor(m_sensor);
		m_scene->setSensor(m_sensor);
		m_scene->setSampler(m_sampler);
		m_scene->setIntegrator(m_integrator);
		m_integrator->wakeup(m_scene, m_resources);
		m_scene->wakeup(m_scene, m_resources);
		m_scene->initializeBidirectional();
	}

	void process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop) {

		

		const VPLGenerateWork *vWork = static_cast<const VPLGenerateWork *>(workUnit);
		VPLGenerateResult *vResult = static_cast<VPLGenerateResult *>(workResult);

#ifdef MTS_DEBUG_FP
		enableFPExceptions();
#endif
	
		ref<Random> random = new Random(vWork->m_offset);
		m_integrator->generateVPL(m_scene, m_sensor, m_sampler, random, vWork, vResult);

#ifdef MTS_DEBUG_FP
		disableFPExceptions();
#endif
	}

	void serialize(Stream *stream, InstanceManager *manager) const {
		AssertEx(0, "unfinished");
		//stream->writeInt(m_pixelFormat);
		//stream->writeInt(m_channelCount);
		//stream->writeInt(m_blockSize);
		//stream->writeInt(m_borderSize);
		//stream->writeBool(m_warnInvalid);
	}

	ref<WorkProcessor> clone() const {
		return new VPLGenerator(m_warnInvalid);
	}

	MTS_DECLARE_CLASS()
protected:
	virtual ~VPLGenerator() { }
private:
	ref<Scene> m_scene;
	ref<Sensor> m_sensor;
	ref<Sampler> m_sampler;
	ref<ManyLightIntegrator> m_integrator;


	bool m_warnInvalid;
};

//////////////////////////////////////////////////////////////////////////

VPLGenerateProcess::VPLGenerateProcess(const RenderJob *parent, RenderQueue *queue, uint32_t numVPL, uint32_t maxDepth) : m_queue(queue), m_parent(parent), m_resultCount(0), m_progress(NULL) {
	m_resultMutex = new Mutex();
	m_warnInvalid = true;

	ref<Scheduler> sched = Scheduler::getInstance();

	m_numVPL = numVPL;
	//m_batchSize = 1024*16;
	//m_workTotal = std::round(m_numVPL / m_batchSize);
	m_workTotal = sched->getWorkerCount();
	m_batchSize = std::round(m_numVPL / m_workTotal);

	if (m_batchSize > 8192) {
		m_batchSize = 8192;
		m_workTotal = std::round(m_numVPL / m_batchSize);
	}

	m_generatedWork = 0;
	
	m_progress = new ProgressReporter("Generating VPL", m_workTotal, m_parent);
	
	m_maxDepth = maxDepth;
	
}

VPLGenerateProcess::~VPLGenerateProcess() {
	if (m_progress) {
		delete m_progress;
	}
		
}



ref<WorkProcessor> VPLGenerateProcess::createWorkProcessor() const {
	return new VPLGenerator(m_warnInvalid);
}

void VPLGenerateProcess::processResult(const WorkResult *result, bool cancelled) {

	const VPLGenerateResult *vplResult = static_cast<const VPLGenerateResult *>(result);	
	
	UniqueLock lock(m_resultMutex);

	m_integrator->put(vplResult);

	m_progress->update(++m_resultCount);
	lock.unlock();
}

ParallelProcess::EStatus VPLGenerateProcess::generateWork(WorkUnit *unit, int worker) {

	if (m_generatedWork == m_workTotal) {
		return EFailure;
	}

	VPLGenerateWork &vplWork = *static_cast<VPLGenerateWork *>(unit);
	vplWork.m_numVPL = m_batchSize;
	vplWork.m_offset = 2 * m_generatedWork * m_batchSize;
	vplWork.m_maxDepth = m_maxDepth;

	m_generatedWork++;

	return ESuccess;

}

void VPLGenerateProcess::bindResource(const std::string &name, int id) {

	if (name == "integrator") {
		m_integrator = static_cast<ManyLightIntegrator *>(Scheduler::getInstance()->getResource(id));
	}

	ParallelProcess::bindResource(name, id);
}


//////////////////////////////////////////////////////////////////////////

void VPLGenerateResult::load(Stream *stream)
{
	AssertEx(0, "unfinished");
	// TODO
}


void VPLGenerateResult::save(Stream *stream) const
{
	AssertEx(0, "unfinished");
	// TODO
}

std::string VPLGenerateResult::toString() const
{
	// TODO

	return std::string();
}

void VPLGenerateWork::load(Stream *stream)
{
	AssertEx(0, "unfinished");
	// TODO
}


void VPLGenerateWork::save(Stream *stream) const
{
	AssertEx(0, "unfinished");
	// TODO
}

std::string VPLGenerateWork::toString() const
{
	// TODO

	return std::string();
}

MTS_IMPLEMENT_CLASS(VPLGenerateWork, false, WorkUnit)
MTS_IMPLEMENT_CLASS(VPLGenerateResult, false, WorkResult)
MTS_IMPLEMENT_CLASS(VPLGenerateProcess, false, ParallelProcess)
MTS_IMPLEMENT_CLASS_S(VPLGenerator, false, WorkProcessor)
MTS_NAMESPACE_END

