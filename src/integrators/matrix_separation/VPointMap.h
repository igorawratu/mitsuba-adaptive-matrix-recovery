#ifndef __VPointMAP_H_
#define __VPointMAP_H_

#include <mitsuba/core/serialization.h>
#include <mitsuba/core/kdtree.h>
#include <mitsuba/render/vpl.h>

#include "SamplePath.h"
#include "VLight.h"
#include "VSensor.h"

MTS_NAMESPACE_BEGIN

class MTS_EXPORT VLightMap : public SerializableObject
{

public:
	VLightMap();
	VLightMap(Stream *stream, InstanceManager *manager);

	virtual void serialize(Stream *stream, InstanceManager *manager) const;
	virtual void generateStandardVPL(Random *random);
	virtual void normalizeRawVPL(size_t offset);
	virtual void clear();

	MTS_DECLARE_CLASS();
public:
//protected:
	VPointTree<VOrientedLight> m_oriented_tree;
	VPointTree<VOmniLight> m_omni_tree;
	VPointTree<VDirectionalLight> m_directional_tree;

	int vplSize() {
		return m_oriented_tree.m_leaves.size() + m_omni_tree.m_leaves.size() + m_directional_tree.m_leaves.size();
	}

	int getIndex(const VLight *n) const {

		if (!m_oriented_tree.m_leaves.empty()) {
			int temp = m_oriented_tree.getIndex((VOrientedLight *)n);
			if (temp >= 0 && temp < m_oriented_tree.m_leaves.size()) {
				return temp;
			}
		}
		

		if (!m_omni_tree.m_leaves.empty()) {
			int temp = m_omni_tree.getIndex((VOmniLight *)n);
			if (temp >= 0 && temp < m_omni_tree.m_leaves.size()) {
				return temp + m_oriented_tree.m_leaves.size();
			}
		}

		if (!m_directional_tree.m_leaves.empty()) {
			int temp = m_directional_tree.getIndex((VDirectionalLight *)n);
			if (temp >= 0 && temp < m_directional_tree.m_leaves.size()) {
				return temp + m_oriented_tree.m_leaves.size() + m_omni_tree.m_leaves.size();
			}
		}

		return -1;
	}

	int getIndex(const VLightNode *n) const {

		if (!m_oriented_tree.m_nodes.empty()) {
			int temp = m_oriented_tree.getIndex((VOrientedLightNode *)n);
			if (temp >= 0 && temp < m_oriented_tree.m_nodes.size()) {
				return temp;
			}
		}


		if (!m_omni_tree.m_nodes.empty()) {
			int temp = m_omni_tree.getIndex((VOmniLightNode *)n);
			if (temp >= 0 && temp < m_omni_tree.m_nodes.size()) {
				return temp + m_oriented_tree.m_nodes.size();
			}
		}

		if (!m_directional_tree.m_nodes.empty()) {
			int temp = m_directional_tree.getIndex((VDirectionalLightNode *)n);
			if (temp >= 0 && temp < m_directional_tree.m_nodes.size()) {
				return temp + m_oriented_tree.m_nodes.size() + m_omni_tree.m_nodes.size();
			}
		}

		return -1;
	}


	std::vector<VPL> raw_vpl;
	Spectrum m_vpl_average;
protected:
	virtual ~VLightMap();
	
	//ref<std::deque<VPL>> m_vpls;


};

class MTS_EXPORT VSensorMap : public SerializableObject
{

public:
	VSensorMap();
	VSensorMap(Stream *stream, InstanceManager *manager);

	virtual void serialize(Stream *stream, InstanceManager *manager) const;
	virtual void build(Random *random);
	virtual void buildByClusterCenters(Random *random);
	virtual void buildByPixel(std::vector<VSensorNode *>& m_pixel_nodes, Random *random);
	virtual void clear();  

	void calAdjSlices(const VSensorNode *slice, std::vector < std::pair < const VSensorNode*, int > > &slices, int sliceSize) const;
	int getIndex(const VSensorNode *n) const {

		if (!m_sensor_tree.m_leaves.empty()) {
			int temp = m_sensor_tree.getIndex((VSensorNode *)n);			
			return temp;
		}


		return -1;
	}

	int getIndex(const VSensor *n) const {
		if (!m_sensor_tree.m_leaves.empty()) {
			int temp = m_sensor_tree.getIndex((VSensor *)n);
			return temp;
		}

		return -1;
	}

	MTS_DECLARE_CLASS();
public:
	//protected:
	VPointTree<VSensor> m_sensor_tree;

	std::vector<pool_ref<SamplePath>> m_sample_pathes;
	std::vector<const VSensorNode *> m_slice_root;

	Float threshold;

	//std::vector<VPL> m_other_vpls;
protected:
	virtual ~VSensorMap();

	//ref<std::deque<VPL>> m_vpls;


};



MTS_NAMESPACE_END
#endif