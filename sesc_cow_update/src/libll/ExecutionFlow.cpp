/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Milos Prvulovic
                  James Tuck
                  Luis Ceze

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

#include "OSSim.h"
#include "ExecutionFlow.h"
#include "DInst.h"
#include "Events.h"
#include "GMemoryOS.h"
#include "TraceGen.h"
#include "GMemorySystem.h"
#include "MemRequest.h"


#include "mintapi.h"
#if !(defined MIPS_EMUL)
#include "opcodes.h"
#endif // !(defined MIPS_EMUL)

ExecutionFlow::ExecutionFlow(int32_t cId, int32_t i, GMemorySystem *gmem)
  : GFlow(i, cId, gmem)
{
  picodePC = 0;
  ev = NoEvent;

  goingRabbit = false;

  // Copy the address space from initial context
  memcpy(&thread,ThreadContext::getMainThreadContext(),sizeof(ThreadContext));

  thread.setPid(-1);
  thread.setPicode(&invalidIcode);

  pendingDInst = 0;
}

int32_t ExecutionFlow::exeInst(void)
{
#ifdef SESC_COW_UPDATE
  picodePC->specInst = thread.isSpecThread();
#endif
  // Instruction address
  int32_t iAddr = picodePC->addr;
//  printf("INFO:pid %d  iAddr %x\n",thread.getPid(),iAddr);
  // Instruction flags
  short opflags=picodePC->opflags;
  // For load/store instructions, will contain the virtual address of data
  // For other instuctions, set to non-zero to return success at the end
  VAddr dAddrV=1;
  // For load/store instructions, the Real address of data
  // The Real address is the actual address in the simulator
  //   where the data is found. With versioning, the Real address
  //   actually points to the correct version of the data item
  RAddr dAddrR=0;
  
  // For load/store instructions, need to translate the data address
  //MARK TO MODIFY
  if(opflags&E_MEM_REF) {
    // Get the Virtual address
         dAddrV = (*((int32_t *)&thread.reg[picodePC->args[RS]])) + picodePC->immed;
    thread.setVAddr(dAddrR);
    // Get the Real address
    dAddrR = thread.virt2real(dAddrV, opflags);
	
#ifdef SESC_COW_UPDATE
//	printf("pid %d Raddr %x Vaddr %x\n",thread.getPid(),dAddrR,dAddrV);
	RAddr addr = *((int32_t *)&thread.reg[picodePC->args[RS]]);
	
	ShareAddr::iterator it;
    	for(it = thread.addrs.begin();it != thread.addrs.end();it++)
    	{
		if(*it == dAddrV){
			 VersionTag* vTag = thread.findVTEntry(dAddrV,1);
			 if(vTag){
				vTag->addr = dAddrR;
			 }
		}
		ThreadContext::getContext(thread.getPid())->copy(&thread);
  	}
#endif
    // Put the Real data address in the thread structure
    thread.setRAddr(dAddrR);
  }
  do{
#ifdef SESC_COW_UPDATE
   oldPicodePC = picodePC;
   picodePC->raddr = dAddrR;
   picodePC->exitInst = 0;
   picodePC->specInst = thread.isSpecThread();
   picodePC=(picodePC->func)(picodePC, &thread);//picode PC ? how to modified
#else
    picodePC=(picodePC->func)(picodePC, &thread);//picode PC ? how to modified
#endif
    I(picodePC);
#ifdef SESC_COW_UPDATE
//	printf("pid %d return iAddr %x\n",thread.getPid(),picodePC->addr);
   if(picodePC->specInst&&picodePC->addr == iAddr){
		return 0xffffffff;
   }
#endif
 }while(picodePC->addr==iAddr);
return dAddrV;
}

void ExecutionFlow::exeInstFast()
{
  I(goingRabbit);
#if (defined MIPS_EMUL)
  I(!trainCache); // Not supported yet
  ThreadContext *thread=context;
  InstDesc *iDesc=thread->getIDesc();
  (*iDesc)(thread);
  VAddr vaddr=thread->getDAddr();
  thread->setDAddr(0);
#else // For (defined MIPS_EMUL)
  // This exeInstFast can be called when for sure there are no speculative
  // threads (TLS, or TASKSCALAR)
  int32_t iAddr   =picodePC->addr;
  short iFlags=picodePC->opflags;


  if (trainCache) {
    // 10 advance clock. IPC of 0.1 is supported now
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
    EventScheduler::advanceClock();
  }

  if(iFlags&E_MEM_REF) {
    // Get the Logical address
         VAddr vaddr = (*((int32_t *)&thread.reg[picodePC->args[RS]])) + picodePC->immed;
    // Get the Real address
    thread.setRAddr(thread.virt2real(vaddr, iFlags));
    if (trainCache)
      CBMemRequest::create(0, trainCache, MemRead, vaddr, 0);

#ifdef TS_PROFILING
    if (osSim->enoughMarks1()) {
      if(iFlags&E_WRITE) {
          int32_t value = *((int32_t *)&thread.reg[picodePC->args[RT]]);
          if (value == SWAP_WORD(*(int32_t *)thread.getRAddr())) {
          //silent store
          //LOG("silent store@0x%lx, %ld", picodePC->addr, value);
          osSim->getProfiler()->recWrite(vaddr, picodePC, true);
        } else {
          osSim->getProfiler()->recWrite(vaddr, picodePC, false);
        }
      }
      if(iFlags&E_READ) {
        osSim->getProfiler()->recRead(vaddr, picodePC);
      }
    }
#endif    
  }

  do{
    picodePC=(picodePC->func)(picodePC, &thread);
  }while(picodePC->addr==iAddr);
#endif // For else of (defined MIPS_EMUL)
}

void ExecutionFlow::switchIn(int32_t i)
{
  I(thread.getPid() == -1);
  I(thread.getPicode()==&invalidIcode);
  loadThreadContext(i);
  I(thread.getPid() == i);

  LOG("ExecutionFlow[%d] switchIn pid(%d) 0x%x @%lld", fid, i, picodePC->addr, globalClock);


  //I(!pendingDInst);
  if( pendingDInst ) {
    pendingDInst->scrap();
    pendingDInst = 0;
  }
}

void ExecutionFlow::switchOut(int32_t i)
{
#ifdef TS_TIMELINE
  TraceGen::add(verID,"out=%lld",globalClock);
  verID = 0;
#endif

#if (defined MIPS_EMUL)
  I(context);
  I(context->getPid()==i);
#if (defined DEBUG_SIGNALS)
  MSG("ExecutionFlow[%d] switchOut pid(%d) 0x%x @%lld", fid, i, context->getNextInstDesc()->addr, globalClock);
#endif
  context=0;
#else // (defined MIPS_EMUL)
  LOG("ExecutionFlow[%d] switchOut pid(%d) 0x%x @%lld", fid, i, picodePC->addr, globalClock);
  // Must be running thread i
  I(thread.getPid() == i);
  // Save the context of the thread
  saveThreadContext(i);
  // Now running no thread at all
  thread.setPid(-1);
  thread.setPicode(&invalidIcode);
#endif // Else of (defined MIPS_EMUL)


 if( pendingDInst ) {
#if (defined TLS)
    I(thread.getEpoch());
    I(thread.getEpoch()==tls::Epoch::getEpoch(i));
    thread.getEpoch()->execInstr();
#endif
    pendingDInst->scrap();
    pendingDInst = 0;
  }
}

void ExecutionFlow::dump(const char *str) const
{
#if !(defined MIPS_EMUL)
  if(picodePC == 0) {
    MSG("Flow(%d): context not ready", fid);
    return;
  }

  LOG("Flow(%d):pc=0x%x:%s", fid, (int)getPCInst()    // (int) because fake warning in gcc
      , str);

  LOG("Flowd:id=%3d:addr=0x%x:%s", fid, static_cast < unsigned >(picodePC->addr),
      str);
#endif
}

DInst *ExecutionFlow::executePC()
{
  if(pendingDInst) {
    DInst *dinst = pendingDInst;
    pendingDInst = 0;
    return dinst;
  }

  DInst *dinst=0;
  // We will need the original picodePC later
  icode_ptr origPIcode=picodePC;
  I(origPIcode);
  // Need to get the pid here because exeInst may switch to another thread
  Pid_t     origPid=thread.getPid();
  // Determine whether the instruction has a delay slot
  bool hasDelay= ((picodePC->opflags) == E_UFUNC);
  bool isEvent=false;

  // Library calls always have a delay slot and the target precalculated
  if(hasDelay && picodePC->target) {
    // If it is an event, set the flag
    isEvent=Instruction::getInst(picodePC->target->instID)->isEvent();
  }
  // Remember the ID of the instruction
  InstID cPC = getPCInst();
 
  int32_t vaddr = exeInst();
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
 if(vaddr == 0xffffffff) {
   return 0;
 }
#endif
	dinst=DInst::createInst(cPC
                          ,vaddr
                          ,fid
                         );  
//#ifdef SESC_COW_TIME
#ifdef SESC_COW_UPDATE
if(oldPicodePC->specInst){
    dinst->setSpecInst(oldPicodePC->specInst);
//   dinst->elmMiss = oldPicodePC->elmMiss;
    dinst->setExecPath(oldPicodePC->exePath);
dinst->setDataVersion(oldPicodePC->dataVersion);
dinst->setExitInst(oldPicodePC->exitInst);
dinst->setRaddr(oldPicodePC->raddr);
	if(oldPicodePC->exePath = REMOTEACCESS)
		{
			dinst->setReqVersion(oldPicodePC->reqVersion);
			dinst->setRemotePid( oldPicodePC->remotePid);
		}
}
#endif

#ifdef SESC_COW_UPDATE
  if(dinst->isExitInst())
	 return dinst;
#endif
  // If no delay slot, just return the instruction, not iBJ command
  if(!hasDelay) {
    return dinst;
  }


  // The delay slot must execute
  I(thread.getPid()==origPid);
  I(getPCInst()==cPC+1);
  cPC = getPCInst();
  vaddr = exeInst();
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
 if(vaddr == 0xffffffff)
 	return 0;
#endif

  if(vaddr==0) {
    dinst->scrap();
    return 0;
  }


  I(vaddr);
  I(pendingDInst==0);
  // If a non-event branch with a delay slot, we reverse the order of the branch
  // and its delay slot, and first return the delay slot. The branch is still in
  // "pendingDInst".
  pendingDInst=dinst;
  
  I(pendingDInst->getInst()->getAddr());

  dinst = DInst::createInst(cPC
                            ,vaddr
                            ,fid
                           );
//#ifdef SESC_COW_TIME
#ifdef SESC_COW_UPDATE
if(oldPicodePC->specInst){
    	dinst->setSpecInst(oldPicodePC->specInst);
//	dinst->elmMiss = oldPicodePC->elmMiss;
	dinst->setExecPath(oldPicodePC->exePath);
	dinst->setExitInst(oldPicodePC->exitInst);
	dinst->setDataVersion(oldPicodePC->dataVersion);
	dinst->setRaddr(oldPicodePC->raddr);
	if(oldPicodePC->exePath = REMOTEACCESS)
		{
			dinst->setReqVersion(oldPicodePC->reqVersion);
			dinst->setRemotePid( oldPicodePC->remotePid);
		}
}
#endif

#ifdef SESC_COW_UPDATE
	if(dinst->isExitInst())
	   return dinst;
#endif

  if(!isEvent) {
    // Reverse the order between the delay slot and the iBJ
    return dinst;
  }

  // Execute the actual event (but do not time it)
  I(thread.getPid()==origPid);
  vaddr = exeInst();
// #ifdef SESC_COW
#ifdef SESC_COW_UPDATE
  if(vaddr == 0xffffffff){
   return 0;
 }
#endif
  I(vaddr);
//#ifdef SESC_COW_TIME
#ifdef SESC_COW_UPDATE
if(oldPicodePC->specInst){
    	dinst->setSpecInst(oldPicodePC->specInst);

	dinst->setExecPath(oldPicodePC->exePath);
	dinst->setExitInst(oldPicodePC->exitInst);
	dinst->setDataVersion(oldPicodePC->dataVersion);
	dinst->setRaddr(oldPicodePC->raddr);
	if(oldPicodePC->exePath = REMOTEACCESS)
		{
			dinst->setReqVersion(oldPicodePC->reqVersion);
			dinst->setRemotePid( oldPicodePC->remotePid);
		}
}
#endif

#ifdef SESC_COW_UPDATE
	if(dinst->isExitInst())
	   return dinst;
#endif


  if( ev == NoEvent ) {
    // This was a PreEvent (not notified see Event.cpp)
    // Only the delay slot instruction is "visible"
    return dinst;
  }
  I(ev != PreEvent);
  if(ev == FastSimBeginEvent ) {
    goRabbitMode();
    ev = NoEvent;
    return dinst;
  }
  I(ev != FastSimBeginEvent);
  I(ev != FastSimEndEvent);
  I(pendingDInst);
  // In the case of the event, the original iBJ dissapears
  pendingDInst->scrap();
  // Create a fake dynamic instruction for the event
  pendingDInst = DInst::createInst(Instruction::getEventID(ev)
                                   ,evAddr
                                   ,fid
                                  );

//#ifdef SESC_COW_TIME
#ifdef SESC_COW_UPDATE
	if(oldPicodePC->specInst)
	{
		pendingDInst->setSpecInst(oldPicodePC->specInst);
	    	pendingDInst->setExecPath(oldPicodePC->exePath);
		pendingDInst->setExitInst(oldPicodePC->exitInst);
		pendingDInst->setDataVersion(oldPicodePC->dataVersion);
		pendingDInst->setRaddr(oldPicodePC->raddr);
		if(oldPicodePC->exePath = REMOTEACCESS)
		{
			pendingDInst->setReqVersion(oldPicodePC->reqVersion);
			pendingDInst->setRemotePid( oldPicodePC->remotePid);
		}
	}
#endif
	if(oldPicodePC->exitInst)
	{
		printf("INFO: lose exit inst\n");
		exit(0);
	}
if( evCB ) {
    pendingDInst->addEvent(evCB);
    evCB = NULL;
  }
  ev=NoEvent;
  // Return the delay slot instruction. The fake event instruction will come next.
  return dinst;
}

void ExecutionFlow::goRabbitMode(long long n2skip)
{
  int32_t nFastSims = 0;
  if( ev == FastSimBeginEvent ) {
    // Can't train cache in those cases. Cache only be train if the
    // processor did not even started to execute instructions
    trainCache = 0;
    nFastSims++;
  }else{
    I(globalClock==0);
  }

  if (n2skip) {
    I(!goingRabbit);
    goingRabbit = true;
  }

  nExec=0;
  
  do {
    ev=NoEvent;
    if( n2skip > 0 )
      n2skip--;

    nExec++;

#if (defined MIPS_EMUL)
    I(goingRabbit);
    exeInstFast();
#else
    if (goingRabbit)
      exeInstFast();
    else {
      exeInst();
#ifdef TASKSCALAR
      propagateDepsIfNeeded();
#endif
    }
#endif // For else of (defined MIPS_EMUL)

#ifdef SESC_SIMPOINT
    const Instruction *inst = Instruction::getInst(picodePC->instID);
    if (inst->isBranch())
      bb[picodePC->instID]++;
    static int32_t conta = 10000000; // 10M
    conta--;
    if (conta<=0) { // Every 2**26 instructions
      fprintf(stderr,"\nT");
      for( HASH_MAP<InstID,int>::iterator it=bb.begin();
	   it != bb.end();
	   it++) {
	fprintf(stderr, ":%d:%d ", it->first, it->second);
      }
      fprintf(stderr, "\n");
      bb.clear();
      conta = 10000000; // 10M
    }
#endif

#if (defined MIPS_EMUL)
    if(!context) {
#else
    if(thread.getPid() == -1) {
#endif // For else of (defined MIPS_EMUL)
      ev=NoEvent;
      goingRabbit = false;
      LOG("1.goRabbitMode::Skipped %lld instructions",nExec);
      return;
    }
    
    if( ev == FastSimEndEvent ) {
      nFastSims--;
    }else if( ev == FastSimBeginEvent ) {
      nFastSims++;
    }else if( ev ) {
      if( evCB )
        evCB->call();
      else{
        // Those kind of events have no callbacks because they
        // go through the memory backend. Since we are in
        // FastMode and everything happens atomically, we
        // don't need to notify the backend.
        I(ev == ReleaseEvent ||
          ev == AcquireEvent ||
          ev == MemFenceEvent||
          ev == FetchOpEvent );
      }
    }

#if (defined MIPS_EMUL)
    if( osSim->enoughMTMarks1(context->getPid(),true) )
#else
    if( osSim->enoughMTMarks1(thread.getPid(),true) )
#endif // For else of (defined MIPS_EMUL)
      break;
    if( n2skip == 0 && goingRabbit && osSim->enoughMarks1() && nFastSims == 0 )
      break;

  }while(nFastSims || n2skip || goingRabbit);

  ev=NoEvent;
  LOG("2.goRabbitMode::Skipped %lld instructions",nExec);

  if (trainCache) {
    // Finish all the outstading requests
    while(!EventScheduler::empty())
      EventScheduler::advanceClock();

    //    EventScheduler::reset();
#ifdef TASKSCALAR
    TaskContext::collectCB.schedule(63023);
#endif
  }

  goingRabbit = false;
}

#if !(defined MIPS_EMUL)
icode_ptr ExecutionFlow::getInstructionPointer(void)
{
#if (defined TLS)
  I(0);
  return thread.getPCIcode();
#else
  return picodePC; 
#endif
}

void ExecutionFlow::setInstructionPointer(icode_ptr picode) 
{ 
#if (defined TLS)
  I(0);
#endif
  thread.setPicode(picode);
  picodePC=picode;
}
#endif // For !(defined MIPS_EMUL)
