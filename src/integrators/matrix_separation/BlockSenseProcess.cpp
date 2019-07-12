#include "BlockSenseProcess.h"
#include <mitsuba/core/statistics.h>
#include <mitsuba/core/sfcurve.h>
#include <mitsuba/render/renderproc.h>
#include <mitsuba/render/rectwu.h>

#include "ManyLightIntegrator.h"


MTS_NAMESPACE_BEGIN

class BlockSensor : public WorkProcessor {
public:
	BlockSensor(Bitmap::EPixelFormat pixelFormat, int channelCount, int blockSize,
		int borderSize, bool warnInvalid) : m_pixelFormat(pixelFormat),
		m_channelCount(channelCount), m_blockSize(blockSize),
		m_borderSize(borderSize), m_warnInvalid(warnInvalid) { }

	BlockSensor(Stream *stream, InstanceManager *manager) {
		m_pixelFormat = (Bitmap::EPixelFormat) stream->readInt();
		m_channelCount = stream->readInt();
		m_blockSize = stream->readInt();
		m_borderSize = stream->readInt();
		m_warnInvalid = stream->readBool();
	}

	ref<WorkUnit> createWorkUnit() const {
		return new RectangularWorkUnit();
	}

	ref<WorkResult> createWorkResult() const {
		return new VSensorBlock(
			Vector2i(m_blockSize),
			m_sensor->getFilm()->getReconstructionFilter(),
			m_channelCount, m_warnInvalid);
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

	void process(const WorkUnit *workUnit, WorkResult *workResult,
		const bool &stop) {

		const RectangularWorkUnit *rect = static_cast<const RectangularWorkUnit *>(workUnit);
		VSensorBlock *block = static_cast<VSensorBlock *>(workResult);

#ifdef MTS_DEBUG_FP
		enableFPExceptions();
#endif

		block->setOffset(rect->getOffset());
		block->setSize(rect->getSize());
	
		m_hilbertCurve.initialize(TVector2<uint8_t>(rect->getSize()));

		m_integrator->senseBlock(m_scene, m_sensor, m_sampler,
			block, stop, m_hilbertCurve.getPoints());

#ifdef MTS_DEBUG_FP
		disableFPExceptions();
#endif
	}

	void serialize(Stream *stream, InstanceManager *manager) const {
		stream->writeInt(m_pixelFormat);
		stream->writeInt(m_channelCount);
		stream->writeInt(m_blockSize);
		stream->writeInt(m_borderSize);
		stream->writeBool(m_warnInvalid);
	}

	ref<WorkProcessor> clone() const {
		return new BlockSensor(m_pixelFormat, m_channelCount,
			m_blockSize, m_borderSize, m_warnInvalid);
	}

	MTS_DECLARE_CLASS()
protected:
	virtual ~BlockSensor() { }
private:
	ref<Scene> m_scene;
	ref<Sensor> m_sensor;
	ref<Sampler> m_sampler;
	ref<ManyLightIntegrator> m_integrator;
	Bitmap::EPixelFormat m_pixelFormat;
	int m_channelCount;
	int m_blockSize;
	int m_borderSize;
	bool m_warnInvalid;
	HilbertCurve2D<uint8_t> m_hilbertCurve;
};

//////////////////////////////////////////////////////////////////////////

BlockSenseProcess::BlockSenseProcess(const RenderJob *parent, RenderQueue *queue,
	int blockSize) : m_queue(queue), m_parent(parent), m_resultCount(0), m_progress(NULL) {
	m_blockSize = blockSize;
	m_resultMutex = new Mutex();
	m_pixelFormat = Bitmap::ESpectrumAlphaWeight;
	m_channelCount = -1;
	m_warnInvalid = true;
}

BlockSenseProcess::~BlockSenseProcess() {
	if (m_progress)
		delete m_progress;
}

void BlockSenseProcess::setPixelFormat(Bitmap::EPixelFormat pixelFormat, int channelCount, bool warnInvalid) {
	m_pixelFormat = pixelFormat;
	m_channelCount = channelCount;
	m_warnInvalid = warnInvalid;
}

ref<WorkProcessor> BlockSenseProcess::createWorkProcessor() const {
	return new BlockSensor(m_pixelFormat, m_channelCount,
		m_blockSize, m_borderSize, m_warnInvalid);
}

void BlockSenseProcess::processResult(const WorkResult *result, bool cancelled) {
	const VSensorBlock *block = static_cast<const VSensorBlock *>(result);
	UniqueLock lock(m_resultMutex);
	//m_film->put(block);
	m_integrator->put(block);
	m_progress->update(++m_resultCount);
	lock.unlock(); 
	//_queue->signalWorkEnd(m_parent, block, cancelled);
}

ParallelProcess::EStatus BlockSenseProcess::generateWork(WorkUnit *unit, int worker) {
	EStatus status = BlockedImageProcess::generateWork(unit, worker);


	RectangularWorkUnit &rect = *static_cast<RectangularWorkUnit *>(unit);

	//rect.setOffset(Point2i(50, 400));
	//rect.setOffset(Point2i(69, 400));
	//rect.setSize(Vector2i(32, 32));

	//rect.setOffset(Point2i(800, 600));
	//rect.setSize(Vector2i(32, 32));


	//rect.setOffset(Point2i(1005, 911));
	//rect.setSize(Vector2i(32, 32));

	//rect.setOffset(Point2i(120, 956));
	//rect.setSize(Vector2i(32, 32));

	//rect.setOffset(Point2i(850, 345));
	//rect.setSize(Point2i(860, 355) - rect.getOffset());

	//m_numBlocksGenerated = m_numBlocksTotal;



	if (status == ESuccess)
		m_queue->signalWorkBegin(m_parent, static_cast<RectangularWorkUnit *>(unit), worker);
	return status;
}

void BlockSenseProcess::bindResource(const std::string &name, int id) {
	if (name == "sensor") {
		m_film = static_cast<Sensor *>(Scheduler::getInstance()->getResource(id))->getFilm();
		m_borderSize = m_film->getReconstructionFilter()->getBorderSize();

		Point2i offset = Point2i(0, 0);
		Vector2i size = m_film->getCropSize();

		if (m_film->hasHighQualityEdges()) {
			offset.x -= m_borderSize;
			offset.y -= m_borderSize;
			size.x += 2 * m_borderSize;
			size.y += 2 * m_borderSize;
		}

		if (m_blockSize < m_borderSize)
			Log(EError, "The block size must be larger than the image reconstruction filter radius!");

	
		BlockedImageProcess::init(offset, size, m_blockSize);
		if (m_progress)
			delete m_progress;
		m_progress = new ProgressReporter("Sensing", m_numBlocksTotal, m_parent);
	}

	if (name == "integrator") {
		m_integrator = static_cast<ManyLightIntegrator *>(Scheduler::getInstance()->getResource(id));
	}

	BlockedImageProcess::bindResource(name, id);
}


void VSensorBlock::load(Stream *stream)
{
	AssertEx(0, "unfinished");
	// TODO
}


void VSensorBlock::save(Stream *stream) const
{
	AssertEx(0, "unfinished");
	// TODO
}

std::string VSensorBlock::toString() const
{
	// TODO

	return std::string();
}

MTS_IMPLEMENT_CLASS(VSensorBlock, false, WorkResult)
MTS_IMPLEMENT_CLASS(BlockSenseProcess, false, BlockedImageProcess)
MTS_IMPLEMENT_CLASS_S(BlockSensor, false, WorkProcessor)
MTS_NAMESPACE_END

