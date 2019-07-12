
#include "PixelSliceRenderProcess.h"

#include "ManyLightIntegrator.h"

MTS_NAMESPACE_BEGIN


PixelSliceRenderProcess::PixelSliceRenderProcess(const RenderJob *parent, RenderQueue *queue, ref<WorkProcessor> processor, std::string progress_title, const std::vector<VSensorNode *>& pixelNodes) : 
	SliceProcess(parent, queue, processor, progress_title)
{
	m_pixel_nodes.clear();
	m_pixel_nodes.reserve(pixelNodes.size());
	for (auto& pixelNode : pixelNodes)
		m_pixel_nodes.push_back(pixelNode);
}

PixelSliceRenderProcess::~PixelSliceRenderProcess()
{
}

void PixelSliceRenderProcess::init()
{
	//const VPointTree<VSensor> &sensor_tree = m_integrator->getSensor();
	m_sliceRoot.clear();
	if (m_pixel_nodes.size() == 0) {
		const VPointTree<VSensor> &sensor_tree = m_integrator->getSensor();
		int nodes_num = sensor_tree.m_nodes.size();
		m_sliceRoot.reserve(nodes_num);
		std::stack<const VSensorNode *> stack;
		stack.push(sensor_tree.m_root);
		while (!stack.empty()) {
			const VSensorNode *cur = stack.top();
			stack.pop();
			if (cur->isPixel()) {
				m_sliceRoot.push_back(cur);
			} else {
				stack.push(cur->right);
				stack.push(cur->left);
			}
		}
	} else {
		puts("copy pixel slice");
		m_sliceRoot.reserve(m_pixel_nodes.size());
		for (auto& node : m_pixel_nodes)
			m_sliceRoot.push_back(node);
	}
	
	/*
	m_sliceRoot.clear();
	const VSensorNode *root = sensor_tree.m_root;
	std::stack<const VSensorNode *> stack;
	stack.push(root);

	while (!stack.empty()) {
		const VSensorNode *cur = stack.top();
		stack.pop();

		if (cur->isPixel()) {
			m_sliceRoot.push_back(cur);
		} else {
			stack.push(cur->right);
			stack.push(cur->left);
		}
	}
	*/
	m_numSliceGenerated = 0;
}

MTS_NAMESPACE_END
