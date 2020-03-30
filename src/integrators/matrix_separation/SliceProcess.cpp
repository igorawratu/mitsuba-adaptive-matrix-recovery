#include "SliceProcess.h"
#include <mitsuba/core/statistics.h>
#include <mitsuba/core/sfcurve.h>
#include <mitsuba/render/renderproc.h>
#include <mitsuba/render/rectwu.h>

#include "ManyLightIntegrator.h"


MTS_NAMESPACE_BEGIN


void SliceRender::prepare()
{

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

void SliceRender::process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop)
{

	const SliceWorkUnit *rect = static_cast<const SliceWorkUnit *>(workUnit);
	SliceWorkResult *result = static_cast<SliceWorkResult *>(workResult);


#ifdef MTS_DEBUG_FP
	enableFPExceptions();
#endif

	m_sampler->generate(rect->getSliceRoot()->rep_sensor->getImagePos());
	m_integrator->renderSlice(m_scene, m_sensor, m_sampler, stop, rect->getSliceRoot(), result);

#ifdef MTS_DEBUG_FP
	disableFPExceptions();
#endif
}


SliceProcess::SliceProcess(const RenderJob *parent, RenderQueue *queue,
	ref<WorkProcessor> processor, std::string p_title) : m_queue(queue), m_parent(parent), m_resultCount(0), m_progress(NULL), m_processor{ processor }, m_progress_title{ p_title } {
	m_resultMutex = new Mutex();
}

SliceProcess::~SliceProcess() {
	if (m_progress)
		delete m_progress;
}


ref<WorkProcessor> SliceProcess::createWorkProcessor() const {
	return m_processor->clone();
}

void SliceProcess::processResult(const WorkResult *result, bool cancelled) {
	//const ImageBlock *block = static_cast<const ImageBlock *>(result);
	UniqueLock lock(m_resultMutex);
	

	m_progress->update(++m_resultCount);
	lock.unlock();
	//m_queue->signalWorkEnd(m_parent, block, cancelled);
}

ParallelProcess::EStatus SliceProcess::generateWork(WorkUnit *unit, int worker) {

	SliceWorkUnit &su = *static_cast<SliceWorkUnit *>(unit);

	if (m_numSliceGenerated == m_sliceRoot.size())
		return EFailure;

	const VSensorNode *slice_root = m_sliceRoot[m_numSliceGenerated];

	Point2i pos(std::floor(slice_root->sampleBbox.min[0]), std::floor(slice_root->sampleBbox.min[1]));
	Point2i max_pos(std::ceil(slice_root->sampleBbox.max[0]), std::ceil(slice_root->sampleBbox.max[1]));
	

	su.setSliceRoot(m_sliceRoot[m_numSliceGenerated]);

	m_numSliceGenerated++;
	
	return ESuccess;
}



void SliceProcess::init()
{
	//if (!m_integrator.get() || !m_film.get()) {
	//	return;
	//}
	if (m_integrator->getSliceRoot().size() > 0) {
		m_sliceRoot = m_integrator->getSliceRoot();
	}
	else {
		m_integrator->divideSlices();

		const VPointTree<VSensor> &sensor_tree = m_integrator->getSensor();
		if (sensor_tree.m_leaves.size() == 0) {
			return;
		}

		m_sliceRoot.clear();

		const VSensorNode *root = sensor_tree.m_root;
		Vector extend = root->bbox.getExtents();
		Float l = (extend.x + extend.y + extend.y) / 30;
		std::stack<const VSensorNode *> stack;
		stack.push(root);

		while (!stack.empty()) {

			const VSensorNode *n = stack.top();
			stack.pop();

			if (n->isSlice) {
			//if (n->num_sensor <= m_sliceSize && (n->num_sensor <= m_sliceSize / 2 || (n->left->bbox.distanceTo(n->right->bbox) < l))) {
				//if (n->num_sensor <= m_sliceSize && (n->num_sensor <= m_sliceSize / 2 || (n->left->bbox.getSurfaceArea() + n->right->bbox.getSurfaceArea()) > 0.5 * n->bbox.getSurfaceArea())) {
				m_sliceRoot.push_back(n);
			}
			else {

				stack.push(n->right);
				stack.push(n->left);
			}
		}
	}

	m_numSliceGenerated = 0;
}


void SliceProcess::bindResource(const std::string &name, int id) {
	if (name == "sensor") {
		//m_film = static_cast<Sensor *>(Scheduler::getInstance()->getResource(id))->getFilm();
		//m_borderSize = m_film->getReconstructionFilter()->getBorderSize();

		//m_offset = Point2i(0, 0);
		//m_size = m_film->getCropSize();

		//if (m_film->hasHighQualityEdges()) {
			//Log(EError, "need test");
			//m_offset.x -= m_borderSize;
			//m_offset.y -= m_borderSize;
			//m_size.x += 2 * m_borderSize;
			//m_size.y += 2 * m_borderSize;
		//}

		//if (m_blockSize < m_borderSize)
		//	Log(EError, "The block size must be larger than the image reconstruction filter radius!");

		//BlockedImageProcess::init(offset, size, m_blockSize);
		//init();
	}

	if (name == "integrator") {

		m_integrator = static_cast<ManyLightIntegrator *>(Scheduler::getInstance()->getResource(id));
		
		init();

		if (m_progress)
			delete m_progress;
		m_progress = new ProgressReporter(m_progress_title, m_sliceRoot.size(), m_parent);
	}

	ParallelProcess::bindResource(name, id);
}


void SlicePostProcessor::process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop)
{

	const SliceWorkUnit *rect = static_cast<const SliceWorkUnit *>(workUnit);
	SliceWorkResult *result = static_cast<SliceWorkResult *>(workResult);

#ifdef MTS_DEBUG_FP
	enableFPExceptions();
#endif

	m_integrator->postprocessSlice(m_scene, m_sensor, m_sampler, stop, rect->getSliceRoot(), result);

#ifdef MTS_DEBUG_FP
	disableFPExceptions();
#endif
}



MTS_IMPLEMENT_CLASS(SliceWorkUnit, false, RectangularWorkUnit)
MTS_IMPLEMENT_CLASS(SliceWorkResult, false, WorkResult)
MTS_IMPLEMENT_CLASS(SliceProcess, false, ParallelProcess)

MTS_IMPLEMENT_CLASS(SliceRender, false, WorkProcessor)
MTS_IMPLEMENT_CLASS(SlicePostProcessor, false, SliceRender)


MTS_NAMESPACE_END
