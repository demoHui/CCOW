/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela

This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <string.h>
#include <set>

#include "SescConf.h"

#include "GMemorySystem.h"

#include "MemRequest.h"
#include "MemObj.h"
#include "Pipeline.h"
#include "Resource.h"
#include "Cluster.h"


ID(int32_t MemRequest::numMemReqs = 0;);

#ifdef SESC_SMP_DEBUG
pool<ReqPathEntry> ReqPathEntry::pPool(4096, "ReqPathEntry");
#endif

/************************************************
 *        MemRequest 
 ************************************************/
MemRequest::MemRequest()
  :accessCB(this)
  ,returnAccessCB(this)
{
  IS(dinst = 0);
  IS(gproc = 0);
  IS(currentMemObj = 0);
  IS(reqId = numMemReqs++);
  wToRLevel = -1;
}
//void MemRequest::setFields(DInst *d, MemOperation mop, MemObj *mo)


MemRequest::~MemRequest() 
{
  // to avoid warnings
}

void MemRequest::access()
{
  currentMemObj->access(this);
}

void MemRequest::returnAccess()
{
  mutateReadToWrite();
  currentMemObj->returnAccess(this);
}

/************************************************
 *        DMemRequest 
 ************************************************/

pool<DMemRequest, true>  DMemRequest::actPool(32, "DMemRequest");

void DMemRequest::destroy() 
{
  I(dinst == 0);
  IS(currentMemObj = 0 );
  I(acknowledged);

#ifdef SESC_SMP_DEBUG
  clearPath();
#endif

  actPool.in(this);
}

void DMemRequest::dinstAck(DInst *dinst, MemOperation memOp, TimeDelta_t lat)
{
  I(dinst);
  
  I(!dinst->isLoadForwarded());
  if (memOp == MemWrite) {
	    if(dinst->getResource()){
		    Cluster* c = dinst->getResource()->getCluster();
		    FUStore* r = (FUStore*) c->getResource(iStore);
		    r->storeCompleted();
	    	}else {printf("DMemRequest:dinstAck  dinst = %x error!\n",dinst);exit(0);};
	    I(dinst->isExecuted());
	    dinst->destroy();
  }else{
    I(memOp == MemRead);
    I(dinst->getResource());

    dinst->doAtExecutedCB.schedule(lat);
  }
}

void DMemRequest::create(DInst *dinst, GMemorySystem *gmem, MemOperation mop)
{
  // turn off address translation
  int32_t old_addr = dinst->getVaddr();

#if !((defined TRACE_DRIVEN)||(defined QEMU_DRIVEN))
#if (defined MIPS_EMUL)
  ThreadContext *context=dinst->context;
#else
  ThreadContext *context=ThreadContext::getMainThreadContext();
#endif
//#ifndef SESC_COW_STLS
  if(!context->isValidDataVAddr(old_addr)){
    dinstAck(dinst, mop, 0);
    dinst = 0;
    return;
  }
//#endif
#endif

  DMemRequest *r = actPool.out();

  I(dinst != 0);

  IS(r->acknowledged = false);
  I(r->memStack.empty());
  r->currentClockStamp = (Time_t) -1;

  r->setFields(dinst, mop, gmem->getDataSource());
  r->dataReq = true;
  r->prefetch= false;
  r->priority = 0;
#ifdef TLS
  r->clearStall();
#endif

/*
 #ifdef SESC_COW_STLS
 int32_t ph_addr;
 if(dinst->getSpecFlag())
  ph_addr= dinst->raddr;
 else{
	   ph_addr = gmem->getMemoryOS()->TLBTranslate(old_addr);
	  if (ph_addr == -1) {
	    gmem->getMemoryOS()->solveRequest(r);
	    return;
	  }
 }
 #else
 */
  int32_t ph_addr = gmem->getMemoryOS()->TLBTranslate(old_addr);
  if (ph_addr == -1) {
    gmem->getMemoryOS()->solveRequest(r);
    return;
  }
//#endif

  r->setPAddr(ph_addr);
  r->access();
}

void DMemRequest::ack(TimeDelta_t lat)
{
  if (dinst==0)
    return; // avoid double ack

  I(!acknowledged);           // no double ack
  IS(acknowledged = true);

  dinstAck(dinst, memOp, lat);

  	dinst = 0;
}

VAddr DMemRequest::getVaddr() const 
{
  I(dinst);
  return dinst->getVaddr();
}


/************************************************
 *        IMemRequest 
 ************************************************/

pool<IMemRequest, true>  IMemRequest::actPool(32, "IMemRequest");

void IMemRequest::destroy() 
{
  I(dinst == 0);
  IS(currentMemObj = 0 );
  I(acknowledged);

#ifdef SESC_SMP_DEBUG
  clearPath();
#endif

  actPool.in(this);
}

void IMemRequest::create(DInst *dinst, GMemorySystem *gmem, IBucket *bb)
{
  IMemRequest *r = actPool.out();
 
  IS(r->acknowledged = false);
  I(r->memStack.empty());
  r->currentClockStamp = (Time_t) -1;

  r->setFields(dinst, MemRead, gmem->getInstrSource());
  r->buffer  = bb;
  r->dataReq = false;
  r->prefetch= false;
  r->priority= 0;

  int32_t old_addr = dinst->getInst()->getAddr();
  int32_t ph_addr = gmem->getMemoryOS()->ITLBTranslate(old_addr);
  if (ph_addr == -1) {
    gmem->getMemoryOS()->solveRequest(r);
    return;
  }

  r->setPAddr(ph_addr); 
  r->access();
}

void IMemRequest::ack(TimeDelta_t lat)
{
  if (dinst==0)
    return; // avoid double ack

  I(!acknowledged);           // no double ack
  IS(acknowledged = true);

  buffer->markFetchedCB.schedule(lat);

  dinst = 0;
}

VAddr IMemRequest::getVaddr() const 
{
  I(dinst);
  return dinst->getInst()->getAddr();
}

/************************************************
 *        CBMemRequest 
 ************************************************/

pool<CBMemRequest, true>  CBMemRequest::actPool(32, "CBMemRequest");

void CBMemRequest::destroy() 
{
  I(dinst == 0);
  I(cb == 0);
  I(acknowledged);

#ifdef SESC_SMP_DEBUG
  clearPath();
#endif

  actPool.in(this);
}

CBMemRequest *CBMemRequest::create(TimeDelta_t lat, MemObj *m
				   ,MemOperation mop, PAddr addr
				   ,CallbackBase *cb)
{
  CBMemRequest *r = actPool.out();

  IS(r->acknowledged = false);
  I(r->memStack.empty());
  r->currentClockStamp = (Time_t) -1;

  r->setFields(0, mop, m);
  r->setPAddr(addr);
  r->cb = cb;
  r->dataReq = true;
  r->prefetch= false;

  r->accessCB.schedule(lat);
  #ifdef TLS
  r->clearStall();
  #endif
  return r;
}

void CBMemRequest::ack(TimeDelta_t lat)
{
  IS(acknowledged = true);

  if (cb==0)
    return; // avoid double ack

  CallbackBase *ncb=cb;
  cb = 0;
  if (lat==0)
    ncb->call();
  else
    ncb->schedule(lat, ncb);
}


VAddr CBMemRequest::getVaddr() const 
{
  //  I(0);
  return 0; // not mapping for vaddr (only paddr)
}

/************************************************
 *        StaticCBMemRequest 
 ************************************************/

void StaticCBMemRequest::destroy() 
{
  I(dinst == 0);
  I(acknowledged);

  I(ackDone);

#ifdef SESC_SMP_DEBUG
  clearPath();
#endif

  // Do nothing
}

StaticCBMemRequest::StaticCBMemRequest(StaticCallbackBase *c)
{
  cb = c;

  ackDone = false;
}


void StaticCBMemRequest::launch(TimeDelta_t lat, MemObj *m
				, MemOperation mop, PAddr addr)
{
  setFields(0,mop,m);
  setPAddr(addr);

  ackDone=false;

  accessCB.schedule(lat);
}

void StaticCBMemRequest::ack(TimeDelta_t lat)
{
  IS(acknowledged = true);

  if (ackDone || cb == 0)
    return; // avoid double ack

  ackDone=true;

  if (lat==0)
    cb->call();
  else
    cb->schedule(lat,cb);
}

VAddr StaticCBMemRequest::getVaddr() const 
{
  I(0);
  return 0; // not mapping for vaddr (only paddr)
}


/***********************************************
*	SMemRequest
************************************************/

pool<SMemRequest, true> SMemRequest::actPool(32, "SMemRequest");
void SMemRequest::create(DInst *dinst, GMemorySystem*gmem, MemOperation mop)
{
	SMemRequest*r = actPool.out();
	r->currentClockStamp = (Time_t) -1;
	r->setFields(dinst,mop,gmem->getSpecSource());
	r->dataReq = true;
	r->prefetch = false;
	r->priority = 0;
	r->exePath = dinst->getExecPath();
	r->ackRemote = false;//to  assure remote access
//	printf("INFO: SMemRequest dinst %x, re-pid %d, re-version %d\n",\
		dinst, dinst->getRemotePid(), dinst->getReqVersion());
	r->access();
}
void SMemRequest::destroy()
{
	actPool.in(this);
}
void SMemRequest::dinstAck(DInst * dinst, MemOperation memOp, TimeDelta_t lat)
{
	if(memOp == MemSpecWrite)
		{
			Cluster* c = dinst->getResource()->getCluster();
			FUStore* r =(FUStore*) c->getResource(iStore);
			r->storeCompleted();
			dinst->destroy();
		}//end of if memop == memspec write
	else if(memOp == MemSpecRead || memOp == MemSpecReadRemote || memOp == MemSpecCommit)
		{
			dinst->doAtExecutedCB.schedule(lat);
		}//end of if memop == memspecread
	else 
		{
			printf("Error :  wrong dinst execution path , dinst = %x\n",dinst);
			exit(0);
		}//end of else
}
void SMemRequest::ack(TimeDelta_t lat)
{
	dinstAck(dinst, memOp, lat);
	dinst = 0;
}
/************************************************
*	end SMemRequest
*************************************************/
/************************************************
 *        EMemRequest 
 ************************************************/
 /*
#ifdef SESC_COW
pool<EMemRequest, true>  EMemRequest::actPool(32, "EMemRequest");

void EMemRequest::create(DInst *dinst, GMemorySystem *gmem, MemOperation mop)
{

  EMemRequest *r = actPool.out();

  I(dinst != 0);
  IS(r->acknowledged = false);
  I(r->memStack.empty());
  r->currentClockStamp = (Time_t) -1;
#ifdef SESC_COW
  r->setFields(dinst, mop, gmem->getSpecSource());
#endif
  r->dataReq = true;
  r->prefetch= false;
  r->priority = 0;
#ifdef SESC_COW_TIME
  r->exePath = dinst->exePath;
#endif
#ifdef SESC_COW_STLS
  dinst->cmpFlag = 1;
#endif
  r->access();
}
void EMemRequest::destroy() 
{
	 actPool.in(this);
}
void EMemRequest::dinstAck(DInst *dinst, MemOperation memOp, TimeDelta_t lat)
{
	  if (memOp == MemWrite) {
		Cluster* c = dinst->getResource()->getCluster();
		FUStore* r = (FUStore*) c->getResource(iStore);
		r->storeCompleted();
		dinst->destroy();
	  }
#if( SESC_COW_TIME_PENDING||SESC_COW_STLS)
	  else if(memOp == MemCommit){
		dinst->commitDinstCB.schedule(lat);
		dinst = 0;
	  }
#endif
	  else{
		dinst->doAtExecutedCB.schedule(lat);
	  }
}


#endif
*/
