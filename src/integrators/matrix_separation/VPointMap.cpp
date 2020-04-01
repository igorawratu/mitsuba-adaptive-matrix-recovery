#include "VPointMap.h"

#include <queue>
#include <limits>
#include <iostream>

MTS_NAMESPACE_BEGIN


VLightMap::VLightMap()
{
	m_vpl_average = Spectrum(0.0f);
}

VLightMap::VLightMap(Stream *stream, InstanceManager *manager)
	: SerializableObject(stream, manager)//,
	  //m_kdtree(0, PhotonTree::ESlidingMidpoint)
{
	int flag;

	flag = stream->readInt();
	if (-1 != flag)
		m_oriented_tree.unserialize(stream, manager);


	flag = stream->readInt();
	if (-1 != flag)
		m_omni_tree.unserialize(stream, manager);

	flag = stream->readInt();
	if (-1 != flag)
		m_directional_tree.unserialize(stream, manager);

	m_vpl_average = Spectrum(stream);
	//Assert(Photon::m_precompTableReady);
	//m_scale = (Float)stream->readFloat();
	//m_kdtree.resize(stream->readSize());
	//m_kdtree.setDepth(stream->readSize());
	//m_kdtree.setAABB(AABB(stream));
	//for (size_t i = 0; i < m_kdtree.size(); ++i)
	//	m_kdtree[i] = Photon(stream);
}

VLightMap::~VLightMap()
{

}

void VLightMap::serialize(Stream *stream, InstanceManager *manager) const
{
	Log(EDebug, "Serializing a vlight map (%s)");
	
	if (NULL == m_oriented_tree.m_root)
		stream->writeInt(-1);
	else {
		stream->writeInt(1);
		m_oriented_tree.serialize(stream, manager);
	}
	
	if (NULL == m_omni_tree.m_root)
		stream->writeInt(-1);
	else {
		stream->writeInt(1);
		m_omni_tree.serialize(stream, manager);
	}
	
	if (NULL == m_directional_tree.m_root)
		stream->writeInt(-1);
	else {
		stream->writeInt(1);
		m_directional_tree.serialize(stream, manager);
	}
	
	m_vpl_average.serialize(stream);
	//TODO
	//Log(EError, "Go away! Unnfinished yet.");
	//stream->writeSingle()
	//stream->writeFloat(m_scale);
	//stream->writeSize(m_kdtree.size());
	//stream->writeSize(m_kdtree.getDepth());
	//m_kdtree.getAABB().serialize(stream);
	//for (size_t i = 0; i < m_kdtree.size(); ++i)
	//	m_kdtree[i].serialize(stream);
}

void VLightMap::generateStandardVPL(Random *random)
{


	int c = 0; int q = 0;
	for (std::vector<VPL>::iterator itr = raw_vpl.begin(); itr != raw_vpl.end(); itr++)
	{			

		switch (itr->type)
		{
		case EPointEmitterVPL: {
								 
				if (itr->emitter->getType() & Emitter::EOnSurface) { // oriented
					 		
					// hack : always make it toward normal with only diffuse component
					PositionSamplingRecord pRec(itr->its, ESolidAngle);
					DirectionSamplingRecord dRec(pRec.n, ESolidAngle);

					Spectrum r = itr->emitter->evalDirection(dRec, pRec);
			
					r *= M_PI;  // standard VPL don't do this, but we use it to normalize D term : cos(theta) / PI
					m_oriented_tree.m_leaves.emplace_back(itr->P * r, itr->its.p, itr->its.shFrame.n); 
				} 
				else if (itr->emitter->getType() & Emitter::EDeltaPosition) { // emitters outside the structure
					m_omni_tree.m_leaves.emplace_back(itr->P, itr->its.p, itr->emitter); 
				}
				else {
					SLog(EError, "Unknown emitter!");
				}
			  
			}
			break;
		case EDirectionalEmitterVPL: {
										 
				// the power has been processed when it's generated				
				m_directional_tree.m_leaves.emplace_back(itr->P, itr->its.shFrame.n);
			}
			break;
		case ESurfaceVPL: {

				/*const BSDF *bsdf = itr->its.getBSDF();

				// hack : always make it toward normal with only diffuse component
				
				Spectrum r;
				if (itr->its.wi.z > 0) {
					BSDFSamplingRecord bRec(itr->its, Vector(0.0f, 0.0f, 1.0f));
					bRec.typeMask = BSDF::EDiffuseReflection;
					r = bsdf->eval(bRec);

				}
				else { // twosided bsdf

					BSDFSamplingRecord bRec(itr->its, Vector(0.0f, 0.0f, -1.0f));
					bRec.typeMask = BSDF::EDiffuseReflection;
					r = bsdf->eval(bRec);
				}
			
				if (!r.isZero()) {
					r *= M_PI;   */         // standard VPL don't do this, but we use it to normalize D term : cos(theta) / PI
					m_oriented_tree.m_leaves.emplace_back(itr->P, itr->its.p, itr->its.shFrame.n);
				//}
				
			}
			break;
		default:
			SLog(EError, "Unknown VPL!");
			break;
		}

	}

	// build tree

	m_oriented_tree.build(random);
	m_omni_tree.build(random);
	m_directional_tree.build(random);

	int vpl_num = m_oriented_tree.m_leaves.size() + m_omni_tree.m_leaves.size() + m_directional_tree.m_leaves.size();
	//printf("vpl num %d\n", vpl_num);
	if (vpl_num != 0) {
		if (m_oriented_tree.m_root)
			m_vpl_average += m_oriented_tree.m_root->intensity;
		if (m_omni_tree.m_root)
			m_vpl_average += m_omni_tree.m_root->intensity;
		if (m_directional_tree.m_root)
			m_vpl_average += m_directional_tree.m_root->intensity;
		m_vpl_average /= vpl_num;
	}
}

void VLightMap::clear()
{
	m_oriented_tree.clear();
	m_omni_tree.clear();
	m_directional_tree.clear();
}

void VLightMap::normalizeRawVPL(size_t offset)
{
	
	Float normalization = (Float)1 / offset;

	for (size_t i = 0; i < raw_vpl.size(); ++i) {
		(raw_vpl)[i].P *= normalization;
		(raw_vpl)[i].emitterScale *= normalization;
	}

}


MTS_IMPLEMENT_CLASS_S(VLightMap, false, SerializableObject)



// =================================================================================


VSensorMap::VSensorMap()
{
	threshold = 1e-2;
}

VSensorMap::VSensorMap(Stream *stream, InstanceManager *manager)
: SerializableObject(stream, manager)//,
//m_kdtree(0, PhotonTree::ESlidingMidpoint)
{

	int flag = stream->readInt();
	if (-1 != flag)
		m_sensor_tree.unserialize(stream, manager);

	unsigned sample_path_num = stream->readUInt();
	m_sample_pathes.reserve(sample_path_num);
	for (unsigned i = 0; i < sample_path_num; ++i)
		m_sample_pathes.push_back(static_cast<SamplePath *>(manager->getInstance(stream)));

	threshold = stream->readFloat();
	//threshold = 1e-2;
	//Assert(Photon::m_precompTableReady);
	//m_scale = (Float)stream->readFloat();
	//m_kdtree.resize(stream->readSize());
	//m_kdtree.setDepth(stream->readSize());
	//m_kdtree.setAABB(AABB(stream));
	//for (size_t i = 0; i < m_kdtree.size(); ++i)
	//	m_kdtree[i] = Photon(stream);
}

VSensorMap::~VSensorMap()
{

}

void VSensorMap::serialize(Stream *stream, InstanceManager *manager) const
{
	Log(EDebug, "Serializing a vlight map (%s)");

	if (0 == m_sensor_tree.m_leaves.size())
		stream->writeInt(-1);
	else {
		stream->writeInt(1);
		m_sensor_tree.serialize(stream, manager);
	}

	stream->writeUInt(m_sample_pathes.size());
	for (auto& sample_path : m_sample_pathes)
		manager->serialize(stream, sample_path.get());
	
	stream->writeFloat(threshold);

	//TODO
	//Log(EError, "Go away! Unnfinished yet.");
	//stream->writeSingle()
	//stream->writeFloat(m_scale);
	//stream->writeSize(m_kdtree.size());
	//stream->writeSize(m_kdtree.getDepth());
	//m_kdtree.getAABB().serialize(stream);
	//for (size_t i = 0; i < m_kdtree.size(); ++i)
	//	m_kdtree[i].serialize(stream);
}


void VSensorMap::build(Random *random)
{ 	
	m_sensor_tree.build(random);
}

void VSensorMap::buildByClusterCenters(Random *random)
{
	m_sensor_tree.buildByClusterCenters(random);
}

void VSensorMap::buildByPixel(std::vector<VSensorNode *>& m_pixel_nodes, Random *random)
{
	m_sensor_tree.buildByPixel(m_pixel_nodes, random);
}

void VSensorMap::clear()
{
	m_sensor_tree.clear();
	m_slice_root.clear();
}


//void VSensorMap::calAdjSlices(const VSensorNode *slice, std::vector < std::pair < const VSensorNode*, int > > &slices, int sliceSize) const
//{
//	const AABB& box = slice->bbox;
//	Vector extend = m_sensor_tree.m_root->bbox.getExtents();
//	Float l = (extend.x + extend.y + extend.y) / 30;
//	std::queue < std::pair < const VSensorNode*, int > > q;
//	q.push({ m_sensor_tree.m_root, 1 });
//	while (!q.empty()) {
//		const VSensorNode *cur = q.front().first;
//		int curOrd = q.front().second;
//		q.pop();
//		//if (cur->num_sensor <= sliceSize && (cur->num_sensor <= sliceSize / 2 || (cur->left->bbox.distanceTo(cur->right->bbox) < l))) {
//		if (cur->isSlice) {
//			slices.push_back({ cur, curOrd });
//		} else {
//			if (box.distanceTo(cur->left->bbox) < threshold)
//				q.push({ cur->left, curOrd * 2 });
//			if (box.distanceTo(cur->right->bbox) < threshold)
//				q.push({ cur->right, curOrd * 2 + 1 });
//		}
//	}
//}

void VSensorMap::calAdjSlices(const VSensorNode *slice, std::vector < std::pair < const VSensorNode*, int > > &slices, int sliceSize) const
{
	slices.clear();

	const AABB& box = slice->bbox;
	Vector extend = slice->bbox.getExtents();
	Float l = (extend.x + extend.y + extend.z) / 6;


	std::queue < std::pair < const VSensorNode*, int > > q;
	q.push({ m_sensor_tree.m_root, 1 });
	while (!q.empty()) {
		const VSensorNode *cur = q.front().first;
		int curOrd = q.front().second;
		q.pop();
		//if (cur->num_sensor <= sliceSize && (cur->num_sensor <= sliceSize / 2 || (cur->left->bbox.distanceTo(cur->right->bbox) < l))) {
		if (cur->isSlice) {	
			slices.push_back({ cur, curOrd });
		}
		else {
			std::cout << box.distanceTo(cur->left->bbox) << std::endl;
			if (box.distanceTo(cur->left->bbox) <= l + std::numeric_limits<float>::epsilon())
				q.push({ cur->left, curOrd * 2 });
			if (box.distanceTo(cur->right->bbox) <= l + std::numeric_limits<float>::epsilon())
				q.push({ cur->right, curOrd * 2 + 1 });
		}
	}

	assert(!slices.empty());

}

MTS_IMPLEMENT_CLASS_S(VSensorMap, false, SerializableObject)
MTS_NAMESPACE_END