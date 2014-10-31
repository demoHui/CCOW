#ifndef ELM_H
#define ELM_H

#include <math.h>

#include "estl.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MemRequest.h"

#include "GStats.h"

#include "Snippets.h"
#include "ThreadContext.h"


class ELM: public MemObj{
public:
//	int  nPage;
//	PAddr baseAddr;
	MemObj* dataCache;
	
	TimeDelta_t missDelay;
  	TimeDelta_t hitDelay;
	
       GStatsCntr writeMiss;
       GStatsCntr readMiss;
       GStatsCntr readHit;
       GStatsCntr writeHit;
	GStatsAvg  avgMissLat;

	PortGeneric *elmPort;

	void doRead(MemRequest *mreq);
	void doWrite(MemRequest *mreq);
	//void doReturnAccess(MemRequest *mreq);
  	//void preReturnAccess(MemRequest *mreq);
	void readMissHandler(MemRequest *mreq);
	void sendMiss(MemRequest *mreq);
	typedef CallbackMember1<ELM, MemRequest *, &ELM::doRead>     doReadCB;
	typedef CallbackMember1<ELM, MemRequest *, &ELM::doWrite>    doWriteCB;
	//typedef CallbackMember1<ELM, MemRequest *,&ELM::doReturnAccess> doReturnAccessCB;
    //typedef CallbackMember1<ELM, MemRequest *,&ELM::preReturnAccess> preReturnAccessCB;

	Time_t nextElmSlot() {    return elmPort->nextSlot();  }

public:
	ELM(MemorySystem *gms, const char *descr_section,const char *name = NULL);
	~ELM();
  	void access(MemRequest *mreq);
	void read(MemRequest *mreq);
	void write(MemRequest *mreq);

	//need to decide whether can allocate space for the variables
	bool canAcceptStore(PAddr addr);
	bool canAcceptLoad(PAddr addr);
};
#endif
