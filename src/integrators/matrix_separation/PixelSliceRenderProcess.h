#ifndef __PIXEL_SLICE_RENDER_PROCESS_H_
#define __PIXEL_SLICE_RENDER_PROCESS_H_
#include "SliceProcess.h"

MTS_NAMESPACE_BEGIN

class MultiLCIntegrator;

class PixelSliceRenderProcess : public SliceProcess
{
public:
	PixelSliceRenderProcess(const RenderJob *parent, RenderQueue *queue, ref<WorkProcessor> processor, std::string progress_title = "Processing Pixel Slice", const std::vector<VSensorNode *>& pixelNodes = std::vector<VSensorNode *>());

protected:
	virtual ~PixelSliceRenderProcess();
	virtual void init();

private:
	std::vector<VSensorNode *> m_pixel_nodes;

};

MTS_NAMESPACE_END
#endif
