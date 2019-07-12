#ifndef __MANY_LIGHT_VPL_GENERATE_PROCESS_H_
#define __MANY_LIGHT_VPL_GENERATE_PROCESS_H_

#include <mitsuba/core/sched.h>
#include <mitsuba/render/imageproc.h>
#include <mitsuba/render/renderproc.h>

#include "SamplePath.h"
#include "VSensor.h"
#include "ManyLightIntegrator.h"

MTS_NAMESPACE_BEGIN

class ManyLightIntegrator;

class VPLGenerateWork : public WorkUnit {
public:
	VPLGenerateWork() { }

	uint32_t m_numVPL;
	uint32_t m_offset;
	uint32_t m_maxDepth;


	void load(Stream *stream);
	void save(Stream *stream) const;
	std::string toString() const;

	MTS_DECLARE_CLASS();
};


class VPLGenerateResult : public WorkResult {
public:
	VPLGenerateResult()
	{ 
		
	}
	

	// ======================================================================
	//! @{ \name Implementation of the WorkResult interface
	// ======================================================================

	void load(Stream *stream);
	void save(Stream *stream) const;
	std::string toString() const;

	std::vector<VPL> m_vpls;
	size_t offset;
	size_t path_count;

	MTS_DECLARE_CLASS()
protected:
	/// Virtual destructor
	virtual ~VPLGenerateResult() { 
	}
protected:
	

};



class VPLGenerateProcess : public ParallelProcess {
public:
	VPLGenerateProcess(const RenderJob *parent, RenderQueue *queue, uint32_t numVPL, uint32_t maxDepth);


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
	virtual ~VPLGenerateProcess();
protected:
	ref<RenderQueue> m_queue;
	ref<ManyLightIntegrator> m_integrator;
	//ref<Scene> m_scene;
	const RenderJob *m_parent;
	int m_resultCount;
	ref<Mutex> m_resultMutex;
	ProgressReporter *m_progress;
	
	uint32_t m_numVPL;
	uint32_t m_batchSize;
	
	uint32_t m_workTotal;
	uint32_t m_generatedWork;

	uint32_t m_maxDepth;


	bool m_warnInvalid;
};


MTS_NAMESPACE_END

#endif // !__MANY_LIGHT_CLUSTERED_PROCESS_H_
