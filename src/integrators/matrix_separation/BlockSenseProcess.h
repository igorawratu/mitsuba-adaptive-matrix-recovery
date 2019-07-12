#ifndef __MANY_LIGHT_BLOCK_SENSE_PROCESS_H_
#define __MANY_LIGHT_BLOCK_SENSE_PROCESS_H_

#include <mitsuba/core/sched.h>
#include <mitsuba/render/imageproc.h>
#include <mitsuba/render/renderproc.h>

#include "SamplePath.h"
#include "VSensor.h"
#include "ManyLightIntegrator.h"

MTS_NAMESPACE_BEGIN

class ManyLightIntegrator;


class VSensorBlock : public WorkResult {
public:
	VSensorBlock(const Vector2i &size,
		const ReconstructionFilter *filter = NULL, int channels = -1, bool warn = true) :
		m_offset(0), m_size(size), m_filter(filter), m_weightsX(NULL), m_weightsY(NULL), m_warn(warn)
		{ m_borderSize = filter ? filter->getBorderSize() : 0; }

	//////////////////////////////////////////////////////////////////////////



	/// Set the current block offset
	inline void setOffset(const Point2i &offset) { m_offset = offset; }

	/// Return the current block offset
	inline const Point2i &getOffset() const { return m_offset; }

	/// Set the current block size
	inline void setSize(const Vector2i &size) { m_size = size; }

	/// Return the current block size
	inline const Vector2i &getSize() const { return m_size; }

	/// Return the bitmap's width in pixels
	inline int getWidth() const { return m_size.x; }

	/// Return the bitmap's height in pixels
	inline int getHeight() const { return m_size.y; }

	/// Warn when writing bad sample values?
	inline bool getWarn() const { return m_warn; }

	/// Warn when writing bad sample values?
	inline void setWarn(bool warn) { m_warn = warn; }

	/// Return the border region used by the reconstruction filter
	inline int getBorderSize() const { return m_borderSize; }



	//////////////////////////////////////////////////////////////////////////

	inline void clear() { m_sample_pathes.clear(); }

	inline bool put(SamplePath *samplePath) {

		m_sample_pathes.push_back(pool_ref<SamplePath>(samplePath));

		return true;
	}

	inline const std::vector<pool_ref<SamplePath>> & getSamplePath() const { return m_sample_pathes;  }

	

	// ======================================================================
	//! @{ \name Implementation of the WorkResult interface
	// ======================================================================

	void load(Stream *stream);
	void save(Stream *stream) const;
	std::string toString() const;

	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~VSensorBlock() { }
protected:
	std::vector<pool_ref<SamplePath>> m_sample_pathes;

	//ref<Bitmap> m_bitmap;
	Point2i m_offset;
	Vector2i m_size;
	int m_borderSize;
	const ReconstructionFilter *m_filter;
	Float *m_weightsX, *m_weightsY;
	bool m_warn;
};



class BlockSenseProcess : public BlockedImageProcess {
public:
	BlockSenseProcess(const RenderJob *parent, RenderQueue *queue,
		int blockSize);

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

	ref<WorkProcessor> createWorkProcessor() const;
	void processResult(const WorkResult *result, bool cancelled);
	void bindResource(const std::string &name, int id);
	EStatus generateWork(WorkUnit *unit, int worker);



	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~BlockSenseProcess();
protected:
	ref<RenderQueue> m_queue;
	ref<ManyLightIntegrator> m_integrator;
	//ref<Scene> m_scene;
	ref<Film> m_film;
	const RenderJob *m_parent;
	int m_resultCount;
	ref<Mutex> m_resultMutex;
	ProgressReporter *m_progress;
	int m_borderSize;
	Bitmap::EPixelFormat m_pixelFormat;
	int m_channelCount;
	bool m_warnInvalid;
};


MTS_NAMESPACE_END

#endif // !__MANY_LIGHT_CLUSTERED_PROCESS_H_
