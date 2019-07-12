#ifndef __MANY_LIGHT_SLICE_RENDER_PROCESS_H_
#define __MANY_LIGHT_SLICE_RENDER_PROCESS_H_

#include <mitsuba/core/sched.h>
#include <mitsuba/render/imageproc.h>
#include <mitsuba/render/renderproc.h>

#include "VSensor.h"


MTS_NAMESPACE_BEGIN

class ManyLightIntegrator;


class SliceWorkResult : public WorkResult {
public:
	virtual void put(const VSensor *sensor, const Spectrum &_spectrum) {
		sensor->addToPath(_spectrum);
	}

	/// Fill the work result with content acquired from a binary data stream
	virtual void load(Stream *stream)
	{
		// process slice in local, just do nothing
		AssertEx(0, "unfinished");
	}

	/// Serialize a work result to a binary data stream
	virtual void save(Stream *stream) const
	{
		// process slice in local, just do nothing
		AssertEx(0, "unfinished");
	}

	/// Return a string representation
	virtual std::string toString() const
	{
		// TODO
		return std::string();
	}

	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~SliceWorkResult() { }
};

class SliceWorkUnit : public WorkUnit {
public:
	inline SliceWorkUnit() { }

	/* WorkUnit implementation */
	void set(const WorkUnit *wu) {
		const SliceWorkUnit *w = static_cast<const SliceWorkUnit *>(wu);
		m_slice_root = w->m_slice_root;
	}

	void load(Stream *stream) {
		AssertEx(0, "unfinished");
		// TODO
	}
	void save(Stream *stream) const {
		AssertEx(0, "unfinished");
		// TODO
	}

	std::string toString() const {
		AssertEx(0, "unfinished");
		return std::string();
	}


	inline const VSensorNode *getSliceRoot() const { return m_slice_root; }
	inline void setSliceRoot(const VSensorNode *slice_root) { m_slice_root = slice_root; }

	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~SliceWorkUnit() { }
private:
	const VSensorNode *m_slice_root;
};

//////////////////////////////////////////////////////////////////////////

class SliceRender : public WorkProcessor {
public:
	SliceRender(int sliceSize) { }

	SliceRender(Stream *stream, InstanceManager *manager) {
		AssertEx(0, "unfinished");
	}

	virtual ref<WorkUnit> createWorkUnit() const {
		return new SliceWorkUnit();
	}

	virtual ref<WorkResult> createWorkResult() const {
		return new SliceWorkResult();
	}

	virtual void prepare();

	virtual void process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop);

	virtual void serialize(Stream *stream, InstanceManager *manager) const {
		AssertEx(0, "unfinished");
	}

	virtual ref<WorkProcessor> clone() const {
		return new SliceRender(m_sliceSize);
	}

	MTS_DECLARE_CLASS()
protected:
	virtual ~SliceRender() { }
//private:
	ref<Scene> m_scene;
	ref<Sensor> m_sensor;
	ref<Sampler> m_sampler;
	ref<ManyLightIntegrator> m_integrator;

	int m_sliceSize;

};

class SlicePostProcessor : public SliceRender {

public:
	SlicePostProcessor(int ms) : SliceRender(ms) { }


	void process(const WorkUnit *workUnit, WorkResult *workResult, const bool &stop);

	virtual ref<WorkProcessor> clone() const {
		return new SlicePostProcessor(m_sliceSize);
	}

	MTS_DECLARE_CLASS()

};

//////////////////////////////////////////////////////////////////////////

class SliceProcess : public ParallelProcess {


	//////////////////////////////////////////////////////////////////////////

public:
	SliceProcess(const RenderJob *parent, RenderQueue *queue,
		ref<WorkProcessor> processor, std::string progress_title = "Processing Slice");

	/**
	* \brief Set the pixel format associated with the rendering process
	*
	* By default, this class works with image blocks that store a set
	* of spectral samples along with an alpha value and an accumulated
	* reconstruction filter weight for each pixel. This method can be
	* used to change this, which is useful when additional information
	* should be returned (e.g. normals or depth values).
	*
	* \param pixelFormat
	*    Desired pixel format (e.g. \ref Bitmap::EMultiChannel)
	* \param channelCount
	*    Number of image channels. Only needs to be specified when
	*    setting <tt>pixelFormat=Bitmap::EMultiChannel</tt>
	* \param warnInvalid
	*    By default, the rendering process issues a warning when writing
	*    negative, infinite or NaN-valued pixels. This flag can be used
	*    to turn off the warnings.
	*/
	void setPixelFormat(Bitmap::EPixelFormat pixelFormat,
		int channelCount = -1, bool warnInvalid = false);

	// ======================================================================
	//! @{ \name Implementation of the ParallelProcess interface
	// ======================================================================

	virtual ref<WorkProcessor> createWorkProcessor() const;
	virtual void processResult(const WorkResult *result, bool cancelled);
	virtual void bindResource(const std::string &name, int id);
	virtual EStatus generateWork(WorkUnit *unit, int worker);

	std::vector<const VSensorNode *> m_sliceRoot;
	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~SliceProcess();
	virtual void init();
protected:
	ref<WorkProcessor> m_processor;
	ref<RenderQueue> m_queue;
	ref<ManyLightIntegrator> m_integrator;

	//ref<Scene> m_scene;
	const RenderJob *m_parent;
	int m_resultCount;
	ref<Mutex> m_resultMutex;
	ProgressReporter *m_progress;


	//Point2i m_offset;
	//Vector2i m_size;
	int m_sliceSize;
	int m_numSliceGenerated;

	std::string m_progress_title;
};


MTS_NAMESPACE_END

#endif // !__MANY_LIGHT_CLUSTERED_PROCESS_H_
