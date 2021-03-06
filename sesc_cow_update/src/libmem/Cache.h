/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Luis Ceze
                  Karin Strauss
		  Jose Renau

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

#ifndef CACHE_H
#define CACHE_H

#include <queue>
#include <vector>

#include "estl.h"
#include "CacheCore.h"
#include "GStats.h"
#include "Port.h"
#include "MemObj.h"
#include "MemorySystem.h"
#include "MemRequest.h"
#include "MSHR.h"
#include "Snippets.h"
#include "ThreadContext.h"

#ifdef TASKSCALAR
#include "HVersion.h"
#include "VMemReq.h"
#endif




class CState : public StateGeneric<> {
private:
  bool valid;
  bool dirty;
  bool locked;
  bool spec;
  uint32_t ckpId;
  int32_t nReadMisses; // number of pending read ops when the line was brought to the cache
  int32_t nReadAccesses;
public:
  CState() {
    valid = false;
    dirty = false;
    locked = false;
    spec = false;
    ckpId = 0;
    nReadMisses = 0;
    nReadAccesses = 0;
    clearTag();
  }
  bool isLocked() const {
    return (locked == true);
  }
  void lock() {
    I(valid);
    locked = true;
  }
  bool isDirty() const {
    return dirty;
  }
  void makeDirty() {
    dirty = true;
  }
  void makeClean() {
    dirty = false;
  }
  bool isValid() const {
    return valid;
  }
  void validate() {
    I(getTag());
    valid = true;
    locked = false;
    nReadMisses = 0;
    nReadAccesses = 0;
  }
  void invalidate() {
    valid = false;
    dirty = false;
    locked = false;
    spec = false;
    ckpId = 0;
    clearTag();
  }
  void setSpec(bool s) {
    spec = s;
  }
  bool isSpec() {
    return spec;
  }

  void incReadAccesses() {
    nReadAccesses++;
  }
  int32_t getReadAccesses() {
    return nReadAccesses;
  }

  void setReadMisses(int32_t n) {
    nReadMisses = n;
  }
  int32_t getReadMisses() {
    return nReadMisses;
  }

  void setCkpId(unsigned ci) {
    ckpId = ci;
  }
  uint32_t getCkpId() {
    return ckpId;
  }
};

class Cache: public MemObj
{
protected:
  typedef CacheGeneric<CState,PAddr> CacheType;
  typedef CacheGeneric<CState,PAddr>::CacheLine Line;

  const bool inclusiveCache;
  int32_t nBanks;


  CacheType **cacheBanks;
  MSHR<PAddr,Cache> **bankMSHRs;
  
  typedef HASH_MAP<PAddr, int> WBuff; 

  WBuff wbuff;  // write buffer
  int32_t maxPendingWrites;
  int32_t pendingWrites;

  class Entry {
  public:
    int32_t outsResps;        // outstanding responses: number of caches 
                          // that still need to acknowledge invalidates
    CallbackBase *cb;
    Entry() {
      outsResps = 0;
      cb = 0;
    }
  };

  typedef HASH_MAP<PAddr, Entry> PendInvTable;

  PendInvTable pendInvTable; // pending invalidate table

  PortGeneric *cachePort;
  PortGeneric **bankPorts;
  PortGeneric **mshrPorts;

  int32_t defaultMask;
  TimeDelta_t missDelay;
  TimeDelta_t hitDelay;
  TimeDelta_t fwdDelay;
  bool doWBFwd;

  // BEGIN Statistics
  GStatsCntr readHalfMiss;
  GStatsCntr writeHalfMiss;
  GStatsCntr extraReadMiss;
  GStatsCntr extraReadHit;  
  GStatsCntr writeMiss;
  GStatsCntr readMiss;
  GStatsCntr readHit;
  GStatsCntr writeHit;
  GStatsCntr writeBack;
  GStatsCntr lineFill;
  GStatsCntr linePush;
  GStatsCntr nForwarded;
  GStatsCntr nWBFull;
  GStatsTimingAvg avgPendingWrites;
  GStatsAvg  avgMissLat;
  GStatsCntr rejected;
  GStatsCntr rejectedHits;
  GStatsCntr **nAccesses;
  // END Statistics

#ifdef MSHR_BWSTATS
  GStatsHist secondaryMissHist;
  GStatsHist accessesHist;
  GStatsPeriodicHist mshrBWHist;
  bool parallelMSHR;
#endif

  int32_t getBankId(PAddr addr) const {
    // FIXME: perhaps we should make this more efficient
    // by allowing only power of 2 nBanks
    return ((calcTag(addr)) % nBanks);
  }

  CacheType *getCacheBank(PAddr addr) const {
    return cacheBanks[getBankId(addr)];
  }

  PortGeneric *getBankPort(PAddr addr) const {
    return bankPorts[getBankId(addr)];
  }

  MSHR<PAddr,Cache> *getBankMSHR(PAddr addr) {
    return bankMSHRs[getBankId(addr)];
  }

  Time_t nextCacheSlot() {
    return cachePort->nextSlot();
  }

  Time_t nextBankSlot(PAddr addr) {
    return bankPorts[getBankId(addr)]->nextSlot();
  }

  Time_t nextMSHRSlot(PAddr addr) {
    return mshrPorts[getBankId(addr)]->nextSlot();
  }

  virtual void sendMiss(MemRequest *mreq) = 0;

  void doReadBank(MemRequest *mreq);
  void doRead(MemRequest *mreq);
  void doReadQueued(MemRequest *mreq);
  void doWriteBank(MemRequest *mreq);
  virtual void doWrite(MemRequest *mreq);
  void doWriteQueued(MemRequest *mreq);
  void activateOverflow(MemRequest *mreq);

  void readMissHandler(MemRequest *mreq);
  void writeMissHandler(MemRequest *mreq);

  void wbuffAdd(PAddr addr);
  void wbuffRemove(PAddr addr);
  bool isInWBuff(PAddr addr);

  Line *allocateLine(PAddr addr, CallbackBase *cb);
  void doAllocateLine(PAddr addr, PAddr rpl_addr, CallbackBase *cb);
  void doAllocateLineRetry(PAddr addr, CallbackBase *cb);

  virtual void doReturnAccess(MemRequest *mreq);
  virtual void preReturnAccess(MemRequest *mreq);

  virtual void doWriteBack(PAddr addr) = 0;
  virtual void inclusionCheck(PAddr addr) { }

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doReadBank> 
    doReadBankCB;

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doRead> 
    doReadCB;

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doWriteBank> 
    doWriteBankCB;

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doWrite> 
    doWriteCB;

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doReadQueued> 
    doReadQueuedCB;  

  typedef CallbackMember1<Cache, MemRequest *, &Cache::doWriteQueued> 
    doWriteQueuedCB;

  typedef CallbackMember1<Cache, MemRequest *, &Cache::activateOverflow> 
    activateOverflowCB;

  typedef CallbackMember1<Cache, MemRequest *,
                         &Cache::doReturnAccess> doReturnAccessCB;

  typedef CallbackMember1<Cache, MemRequest *,
                         &Cache::preReturnAccess> preReturnAccessCB;

  typedef CallbackMember3<Cache, PAddr, PAddr, CallbackBase *,
                         &Cache::doAllocateLine> doAllocateLineCB;

  typedef CallbackMember2<Cache, PAddr, CallbackBase *,
                         &Cache::doAllocateLineRetry> doAllocateLineRetryCB;

public:
  Cache(MemorySystem *gms, const char *descr_section, 
	const char *name = NULL);
  virtual ~Cache();

  void access(MemRequest *mreq);
  virtual void read(MemRequest *mreq);
  virtual void write(MemRequest *mreq);
  virtual void pushLine(MemRequest *mreq) =0;
  virtual void specialOp(MemRequest *mreq);
  virtual void returnAccess(MemRequest *mreq);

#ifdef SESC_COW_UPDATE
/*api for Ecache*/
  virtual void pushBack(MemRequest *mreq){}
  virtual void readRemote(MemRequest *mreq){}
  virtual bool dataIn(RAddr addr){}
//  virtual void specRead(MemRequest* mreq){}
//  virtual void specWrite(MemRequest* mreq){}
 // virtual void ackRequest()=0;
#endif

  virtual bool canAcceptStore(PAddr addr);
  virtual bool canAcceptLoad(PAddr addr);
  
  bool isInCache(PAddr addr) const;

  // same as above plus schedule callback to doInvalidate
  void invalidate(PAddr addr, ushort size, MemObj *oc);
  void doInvalidate(PAddr addr, ushort size);

  virtual const bool isCache() const { return true; }

  void dump() const;

  PAddr calcTag(PAddr addr) const { return cacheBanks[0]->calcTag(addr); }

  Time_t getNextFreeCycle() const;


  //used by SVCache
  virtual void ckpRestart(uint32_t ckpId) {}
  virtual void ckpCommit(uint32_t ckpId) {}
};

class WBCache : public Cache {
protected:
  void sendMiss(MemRequest *mreq);
  void doWriteBack(PAddr addr); 
  void doReturnAccess(MemRequest *mreq);

  typedef CallbackMember1<WBCache, MemRequest *, &WBCache::doReturnAccess>
    doReturnAccessCB;

public:
  WBCache(MemorySystem *gms, const char *descr_section, 
	  const char *name = NULL);
  ~WBCache();

  void pushLine(MemRequest *mreq);
};

class WTCache : public Cache {
protected:
  void doWrite(MemRequest *mreq);
  void sendMiss(MemRequest *mreq);
  void doWriteBack(PAddr addr);
  void writePropagateHandler(MemRequest *mreq);
  void propagateDown(MemRequest *mreq);
  void reexecuteDoWrite(MemRequest *mreq);
  void doReturnAccess(MemRequest *mreq);  

  typedef CallbackMember1<WTCache, MemRequest *, &WTCache::reexecuteDoWrite> 
    reexecuteDoWriteCB;

  typedef CallbackMember1<WTCache, MemRequest *, &WTCache::doReturnAccess>
    doReturnAccessCB;

  void inclusionCheck(PAddr addr);

public:
  WTCache(MemorySystem *gms, const char *descr_section, 
	  const char *name = NULL);
  ~WTCache();

  void pushLine(MemRequest *mreq);
};

class SVCache : public WBCache {
 protected:
  GStatsCntr nInvLines;
  GStatsCntr nCommitedLines;
  GStatsCntr nSpecOverflow;

  bool doMSHRopt;
  
 public:

  SVCache(MemorySystem *gms, const char *descr_section,
	  const char *name = NULL);
  ~SVCache();

  virtual void doWrite(MemRequest *mreq);
  virtual void preReturnAccess(MemRequest *mreq);
  virtual void doReturnAccess(MemRequest *mreq);
  virtual void ckpRestart(uint32_t ckpId);
  virtual void ckpCommit(uint32_t ckpId);
};

class NICECache : public Cache
{
// a 100% hit cache, used for debugging or as main memory
protected:
  void sendMiss(MemRequest *mreq);
  void doWriteBack(PAddr addr);

public:
  NICECache(MemorySystem *gms, const char *section, const char *name = NULL);
  ~NICECache();

  void read(MemRequest *mreq);
  void write(MemRequest *mreq);
  void pushLine(MemRequest *mreq);
  void returnAccess(MemRequest *mreq);
  void specialOp(MemRequest *mreq);
};
/**************begin new sesc cow ***************************/
/*************memobj :: ECache******************************/
#ifdef SESC_COW_UPDATE
class ECache : public Cache
{
private:
	/*need to record remote request*/
	class RReq{
	public:
		RReq(){}
		~RReq(){}
		RAddr raddr;
		int version;
		MemRequest *mreq;
	};
	typedef std::vector<RReq*> RReqSet;
	RReqSet rReqs;

	typedef struct{
		RAddr raddr;
		int version;
	}DataVersion;
	typedef std::vector<DataVersion*> SpecData;
	SpecData sData;
protected:
	////////////////////realize virtual function in Cache////////////////
	void sendMiss(MemRequest *mreq);
	void doWriteBack(PAddr  addr);
	void ackRequest( MemRequest *mreq);
	void clear()
	{
		if(!rReqs.empty())
		{
			printf("INFO: error rreqs should be empty here\n");
			RReqSet::iterator it = rReqs.begin();
			for(;it!=rReqs.end();it++)
				ackRequest((*it)->mreq);
//			exit(0);
		}
		rReqs.clear();
		sData.clear();
	}
public:
	ECache(MemorySystem *gms, const char *sesction, const char *name=NULL);
	~ECache();

	/*API in Cache class*/
	void read(MemRequest *mreq);
	void write(MemRequest *mreq);
	void returnAccess(MemRequest * mreq);
	void specialOp(MemRequest * mreq);
	void pushLine(MemRequest * mreq);

	/*API for ECache Commit*/
	void pushBack(MemRequest *mreq);

	/*API for ECache Remote Access*/
	void readRemote(MemRequest *mreq);

	void addRemoteReq(int version, MemRequest *mreq);
	bool dataIn(RAddr addr)
	{
		SpecData::iterator it = sData.begin();
		for(;it!=sData.end();it++)
			if((*it)->raddr == addr&&(*it)->version != -1)
				{
//					printf("INFO:  add remote req version match addr : %x\n",addr);
					return true;
				}
		return false;
	}
};
#endif
/***************end  new sesc cow*************************/
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE///USE IT TEMPARORYLY
/*****************ELMCTRL.H******************/
#define PAGESIZE 4*1024
#define pageOffsetMask 0x0fff
#define tagMask  0xfffff000
#define byteMask 0x0ff
#define indexMask 0x0ff000
#define wordOffsetMask 0x01
#define ACCESSERR 0xffffffff
#define INDEXERR 0xffffffff
#define PAGENUM 1024


typedef uint32_t  VAddr;
typedef unsigned char   BYTE;

class Page
{
public:
	BYTE data[PAGESIZE];
	uint32_t validBits[PAGESIZE/32];
	int modified;
public:
	Page(){	}
	~Page()
	{
		//free(data);
	}
	void init()
	{
//		memset(data,0,sizeof(data));
//		memset(validBits,0,sizeof(validBits));
		int i,j;
		for(i = 0;i< PAGESIZE;i++)
			data[i] = 0;
		for(j = 0 ;j < PAGESIZE/32;j++)
			validBits[j] = 0;
		modified = 0;
	}
	uint32_t readAddr(RAddr addr)
	{
		int index = addr & pageOffsetMask;
		if(getValidBit(addr))
			return (uint32_t)&data[index];	
		else return ACCESSERR;
	}
	/*
#ifdef SESC_COW_STLS
	uint32_t readPreAddr(RAddr addr)
	{
		int index = addr & pageOffsetMask;
		return (uint32_t)&data[index];
	}
#endif
*/
	uint32_t writeAddr(RAddr addr,unsigned int typeSize)
	{
		int index = addr & pageOffsetMask;
		int i ;
		for (i = 0;i<typeSize;i++)
			 setValidBit(addr+i);
		return (uint32_t)&data[index];
	}
	void setValidBit(uint32_t addr)
	{
		int pageOffset = addr & pageOffsetMask;
		int wordIndex = pageOffset /32;
		int wordOffset = pageOffset%32;
		uint32_t val = 0x01 << wordOffset;
		validBits[wordIndex] |= val; 
	}
	bool getValidBit(uint32_t addr)
	{
		int pageOffset = addr & pageOffsetMask;
		int wordIndex = pageOffset /32;
		int wordOffset = pageOffset%32;
		uint32_t val = 0x01 << wordOffset;
		return validBits[wordIndex] & val;
	}
	void commitPage(uint32_t pageAddr,ThreadContext* pthread)
	{
		int index ;
		uint32_t raddr,offset = 1;
		for(index = 0; index < PAGESIZE; index ++){
			raddr = pageAddr+index;
			if(validBits[index/32]&offset){
				*(BYTE *)raddr = data[index];
			}
			offset = offset << 1;
			if(!offset) offset = 1;
		}
	}
	void clear()
	{
//		memset(data,0,PAGESIZE);
//		memset(validBits,0,PAGESIZE/32);
		int i,j;
		for(i = 0;i< PAGESIZE;i++)
			data[i] = 0;
		for(j = 0 ;j < PAGESIZE/32;j++)
			validBits[j] = 0;
		modified = 0;
	}
};

class pageTable{
public:
	bool validBits[PAGENUM];
	uint32_t ptTags[PAGENUM];
	uint32_t elmPageAddr[PAGENUM];
public:
	pageTable(){}
	~pageTable()
		{
			
		}
	void init(void)
		{
			int i;
			for(i =0;i<PAGENUM;i++){
				validBits[i] = 0;
				ptTags[i]  = 0;
				elmPageAddr[i] = 0;
			}
		}
	uint32_t findEntry(RAddr addr)
		{
			uint32_t elmPageI = getPtIndex( addr);
			uint32_t tag = addr & tagMask;
			int cnt = 1;
			while(validBits[elmPageI]){
				if(cnt == PAGENUM)
					return INDEXERR;
				if(ptTags[elmPageI] == tag)
					return elmPageI;
				else{
					elmPageI += cnt;
					cnt ++;
					elmPageI = elmPageI%PAGENUM;
				}
			}
			return INDEXERR;
		}
	uint32_t allocateEntry(RAddr addr)
		{
			uint32_t elmPageI = getPtIndex( addr);
			int cnt = 1;
			while(validBits[elmPageI]){
				if(cnt != PAGENUM){
					elmPageI += cnt;
					cnt ++;
					elmPageI = elmPageI%PAGENUM;
				}
				else break;
			}
			if(cnt != PAGENUM){
				setValid(elmPageI);
				setPtTag(addr,elmPageI);
				return elmPageI;
			}
			return INDEXERR;
		}
	uint32_t getPtIndex(RAddr addr)
		{
			uint32_t tmp = addr & indexMask;
			return tmp >> 12;
		}
	uint32_t getPtTag(RAddr addr)
		{
			
			return addr & tagMask;
		}
	void setValid(uint32_t index)
		{
			validBits[index] = 1;
		}
	void setPtTag(RAddr addr,uint32_t index)
		{
			ptTags[index] = addr & tagMask;
		}
	void clear()
		{
			int i;
			for(i =0;i<PAGENUM;i++){
				validBits[i] = 0;
				ptTags[i]  = 0;
				elmPageAddr[i] = 0;
			}
		}
};

class elmCtrl{
protected:
	Page pages[PAGENUM];
	pageTable pt;
public:
	elmCtrl()
		{
			
		}
	~elmCtrl()
		{	
			
		}
	void init()
	{
		pt.init();
		int i ;
		for(i=0;i<PAGENUM;i++)
			pages[i].init();
	}
	bool allocatePage(RAddr addr);
	uint32_t getRSpecAddr(RAddr addr);
	uint32_t getWSpecAddr(RAddr addr,unsigned int typeSize,icode_ptr picode);
	void elmCommit(ThreadContext* pthread);
	/*
#ifdef SESC_COW_STLS
	uint32_t getPreAddr(RAddr addr);
#endif
*/

/*	uint32_t specRead(VAddr addr);
	void specWrite(VAddr addr,uint32_t val);
*/};
#endif//endof ifdef sesc_cow_update
/*****************ELM.H******************/
#ifdef SESC_COW
class ELM: public MemObj{
public:
	MemObj* dataCache;
	
	TimeDelta_t missDelay;
  	TimeDelta_t hitDelay;
	TimeDelta_t commitDelay;
	TimeDelta_t cmpDelay;

	int cmpBlk;
	
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
	bool canAcceptStore(PAddr addr)
		{
			return true;
		}
	bool canAcceptLoad(PAddr addr){return true;}
	Time_t getNextFreeCycle() const {
		return elmPort->calcNextSlot();
		}


  	void returnAccess(MemRequest *mreq) {
		return ;
  		}

 	void invalidate(PAddr addr, ushort size, MemObj *oc) {
		return ;
 		}
/*
	#if( SESC_COW_TIME_PENDING||SESC_COW_STLS)
	void commit(MemRequest * mreq);
	#endif
*/
};

#endif//endof ifdef sesc_cow
#endif//end of ifndef cache_h
