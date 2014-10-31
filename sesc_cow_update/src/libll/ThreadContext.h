#ifndef THREADCONTEXT_H
#define THREADCONTEXT_H

#include <stdint.h>
#include <vector>
#include <set>
#include <queue>
#include <list>
#include "Snippets.h"
#include "event.h"

#ifdef SESC_COW_UPDATE
#include "estl.h"
#endif

#if (defined MIPS_EMUL)
#include "AddressSpace.h"
#include "SignalHandling.h"
#include "FileSys.h"
#include "InstDesc.h"
#include "LinuxSys.h"

// Use this define to debug the simulated application
// It enables call stack tracking
//#define DEBUG_BENCH

#else
#include "HeapManager.h"
#include "icode.h"

#if (defined TLS)
namespace tls {
  class Epoch;
}
#endif

/* For future support of the exec() call. Information needed about
 * the object file goes here.
 */
typedef struct objfile {
  int32_t rdata_size;
  int32_t db_size;
  char *objname;
} objfile_t, *objfile_ptr;

/* The maximum number of file descriptors per thread. */
#define MAX_FDNUM 20

// Simulated machine has 32-bit registers
typedef int32_t IntRegValue;

enum IntRegName{
  RetValReg   =  2,
  RetValHiReg =  2,
  RetValLoReg =  3,
  IntArg1Reg  =  4,
  IntArg2Reg  =  5,
  IntArg3Reg  =  6,
  IntArg4Reg  =  7,
  JmpPtrReg   = 25,
  GlobPtrReg  = 28,
  StkPtrReg   = 29,
  RetAddrReg  = 31
};
#endif // Else of (defined MIPS_EMUL)
/*
#ifdef SESC_COW_UPDATE
enum RunState{
  Running,
  WaitRemoteData,
  ReadyCommit
};
#endif
*/
//BEGIN UPDATE
//special data structure
 #ifdef SESC_COW_UPDATE
 
  //BEGIN spec thread data structure 
struct Variable{
	int addr;
	std::list<ThreadContext *> threadVector;
};
typedef std::vector<struct Variable> VarVector;

struct  VersionTag{
public:
	uint32_t addr;//physical address refers to raddr in Dinst
	uint32_t size;//data size;
	Pid_t pid;//useless
	int version;//read version

//remote access
//	int getRemote;//may be useless
	int preVersion;//record the previous thread's final version
	int realAddr;//identify the data is a shared data
	int iteration;//request for the definit iteration thread

public:
	bool findEntry(uint32_t addr,int size){
		if(addr>=this->addr&&(addr+size)<=(this->addr+this->size))
			return true;
		else
			return false;
	}
 };
//   typedef HASH_MAP<RAddr, VersionTag*> VTable;
   typedef std::vector<VersionTag*> VTable;

   typedef std::vector<VAddr> DataCopy;
   
   //END spec thread data structure
   
//thread parameter
struct ThreadPara{
	uint32_t entry;
	uint32_t intArg1Reg;
	uint32_t intArg2Reg;
	uint32_t jmpPtrReg;
	uint32_t retAddrReg;
	Pid_t ppid;
};
  //remote access
  
	typedef std::vector<VersionTag*> WriteVersion;
	typedef std::vector<RAddr> ShareAddr;

 #endif
//END UPDATE
/*
#ifdef SESC_COW_VERSION
class versionTag{
public:
	uint32_t addr;
	uint32_t size;
	Pid_t pid;
	int version;
#ifdef SESC_COW_REMOTE
	int getRemote;
	int preVersion;
	int realAddr;
#endif
//	bool RorW;
public:
	bool findEntry(uint32_t addr,int size){
		if(addr>=this->addr&&(addr+size)<=(this->addr+this->size))
			return true;
		else
			return false;
	}
};
#endif
#ifdef SESC_COW_REPLAY
struct threadPrama{
  uint32_t entry;
  uint32_t intArg1Reg;
  uint32_t intArg2Reg;
  uint32_t jmpPtrReg;
  uint32_t retAddrReg;
  Pid_t ppid;
};
#endif

#ifdef SESC_REMOTE
struct variable{
	int addr;
	std::list<ThreadContext *> threadVector;
};
typedef std::vector<struct variable> VarVector;
#endif
#ifdef SESC_COW_REMOTE
typedef std::vector<RAddr> ShareAddr;
#endif
*/
#if (defined MIPS_EMUL)
class ThreadContext : public GCObject{
 public:
  typedef SmartPtr<ThreadContext> pointer;
 private:
  typedef std::vector<pointer> ContextVector;
#else
class ThreadContext {
  typedef std::vector<ThreadContext *> ContextVector;
#endif
  // Static variables
  static ContextVector pid2context;


#if !(defined MIPS_EMUL)
  static Pid_t baseClonedPid;
  static Pid_t nextClonedPid;
  static ContextVector clonedPool;

  static size_t nThreads;
  static int maxThreads;

  static Pid_t baseActualPid;
  static Pid_t nextActualPid;
  static ContextVector actualPool;
  static ThreadContext *mainThreadContext;


#endif //!(defined MIPS_EMUL)

  // Memory Mapping

#if !(defined MIPS_EMUL)
  // Lower and upper bound for valid data addresses
  VAddr dataVAddrLb;
  VAddr dataVAddrUb;
  // Lower and upper bound for stack addresses in all threads
  VAddr allStacksAddrLb;
  VAddr allStacksAddrUb;
#endif //!(defined MIPS_EMUL)
  // Lower and upper bound for stack addresses in this thread
  VAddr myStackAddrLb;
  VAddr myStackAddrUb;

#if !(defined MIPS_EMUL)
  // Real address is simply virtual address plus this offset
  RAddr virtToRealOffset;
#endif //!(defined MIPS_EMUL)

#ifdef SESC_COW_UPDATE
  bool specThread;
#endif

  // Local Variables
#if !(defined MIPS_EMUL)
 public:
 #ifdef SESC_COW_UPDATE
 //for global
	static int nIterations;
  //for local
	int iteration;

	VTable vt;
	DataCopy dc;
	struct ThreadPara threadPara; 
	WriteVersion  writeVersion;
	ShareAddr addrs;//remote request address(physical address)
 #endif

/*
#ifdef SESC_COW
  bool spec;
#endif 
#ifdef SESC_COW_VERSION
   typedef std::vector <versionTag *> VTable;
   VTable vt;
   typedef std::vector<VAddr> DataCopy;
   DataCopy dc;
#endif
#ifdef SESC_COW_REPLAY
struct threadPrama prama;
#endif
*/

//NO USE FOR UPDATE
/*
#if( SESC_COW_TIME_PENDING||SESC_COW_STLS)
 int nCommitPage;
 int nCmpEntry;
#endif
*/
/*
#ifdef SESC_COW_STLS
 int tlsCopy;
#endif

#ifdef SESC_REMOTE
VarVector varVector;
RunState runState;
#endif
#ifdef SESC_COW_REMOTE
	typedef std::vector <versionTag *> WriteVersion;
	WriteVersion  writeVersion;
	ShareAddr addrs;
	
	int iteration;
#endif
*/
  IntRegValue reg[33];	// The extra register is for writes to r0
  int32_t lo;
  int32_t hi;
#endif
 private:
#if !(defined MIPS_EMUL)
  uint32_t fcr0;	// floating point control register 0
  float fp[32];	        // floating point (and double) registers
  uint32_t fcr31;	// floating point control register 31
  icode_ptr picode;	// pointer to the next instruction (the "pc")
#endif
  int32_t pid;		// process id
#if !(defined MIPS_EMUL)
  VAddr vaddr;
  RAddr raddr;		// real address computed by an event function
  icode_ptr target;	// place to store branch target during delay slot
#endif


  HeapManager  *heapManager;  // Heap manager for this thread
  ThreadContext *parent;    // pointer to parent
  ThreadContext *youngest;  // pointer to youngest child
  ThreadContext *sibling;   // pointer to next older sibling
  int32_t *perrno;	            // pointer to the errno variable
  int32_t rerrno;		    // most recent errno for this thread

  char      *fd;	    // file descriptors; =1 means open, =0 means closed


private:


public:
//BEGIN UPDATE
#ifdef SESC_COW_UPDATE
//base section
	void setSpecThread(bool spec){specThread = spec;}
	bool isSpecThread(){return specThread;}
	void commitVersion();
//version section
	void clear();
	int getDataVersion(VersionTag* vTag);
	VersionTag* findVTEntry(uint32_t addr,int size);
	void updateMainVersion(uint32_t addr,int size);//not called

	//void addLVTEntry(uint32_t addr,int size);
	void addLVTEntry(uint32_t addr,int size);
	void incVersion(uint32_t addr,int size);
	bool dataInStack(uint32_t addr);
//remote access section
	int getPreVersion(VersionTag* vTag);
	void writeVersionUpdate(RAddr addr,int size);
	int getWriteVersion(RAddr addr,int size);
	void addVersionedEntry(RAddr addr,int size,int version);
	void setVersion(RAddr addr, int size,int version);
	bool isSharedData(RAddr addr, int size);
	Pid_t iter2Pid(int iter);
//replay section
	 bool isOrderCommit();
	 void setPid2Context(ThreadContext * thread);
//	 void oldActual(Pid_t pid);
	 void freePid();
	 void setThreadPara(struct ThreadPara tp){
		threadPara.intArg1Reg = tp.intArg1Reg;
		threadPara.intArg2Reg =  tp.intArg2Reg;
		threadPara.jmpPtrReg = tp.jmpPtrReg;
		threadPara.entry = tp.entry;
		threadPara.ppid = tp.ppid;
		threadPara.retAddrReg = tp.retAddrReg;
	}
	struct ThreadPara getThreadPrama(){
		return threadPara;
	}
#endif
//END UPDATE
/*	
#ifdef SESC_COW_VERSION
void versionClear();

int getVersion(int index);
#ifdef SESC_COW_REMOTE
int getPreVersion(int index);
#endif
int findEntry(uint32_t addr,int size);

//bool verCmp();

void commitEntry(uint32_t addr,int size);

void addEntry(uint32_t addr,int size);
void addEntry(uint32_t addr,int size,Pid_t pid);
void versionIns(uint32_t addr,int size);
bool dataInStack(uint32_t addr);
	
#endif
#ifdef SESC_COW_REPLAY
 bool isOrderCommit();
 void setPid2Context(ThreadContext * thread);
 void oldActual(Pid_t pid);
 void freePid();
#endif
#ifdef SESC_COW_STLS
 void firstWrite(uint32_t addr,int size);
#endif
#ifdef SESC_REMOTE
void  delRemoteMark();
Pid_t getRemotePid(RAddr addr);
#endif
#ifdef SESC_COW_REMOTE
	void writeVersionUpdate(RAddr addr,int size);
	int getWriteVersion(RAddr addr,int size);
	void addVersionEntry(RAddr addr,int size,int version);
	void writeVersionClear();
	void setVersion(RAddr addr, int size,int version);
	bool isSharedVar(RAddr addr, int size);
	Pid_t iter2Pid(int iter);
#endif
*/
  inline IntRegValue getIntReg(IntRegName name) const {
    return reg[name];
  }
  inline void setIntReg(IntRegName name, IntRegValue value) {
    reg[name]=value;
  }
  
  inline IntRegValue getIntArg1(void) const{
    return getIntReg(IntArg1Reg);
  }
  inline IntRegValue getIntArg2(void) const{
    return getIntReg(IntArg2Reg);
  }
  inline IntRegValue getIntArg3(void) const{
    return getIntReg(IntArg3Reg);
  }
  inline IntRegValue getIntArg4(void) const{
    return getIntReg(IntArg4Reg);
  }
  inline VAddr getGlobPtr(void) const{
    return getIntReg(GlobPtrReg);
  }
  inline void setGlobPtr(VAddr addr){
    setIntReg(GlobPtrReg,addr);
  }
  inline VAddr getStkPtr(void) const{
    return getIntReg(StkPtrReg);
  }
  inline void setStkPtr(int32_t val){
    I(sizeof(val)==4);
    setIntReg(StkPtrReg,val);
  }
  inline void setRetVal(int32_t val){
    I(sizeof(val)==4);
    setIntReg(RetValReg,val);
  }
  inline void setRetVal64(int64_t val){
    I(sizeof(val)==8);
    uint64_t valLo=val;
    valLo&=0xFFFFFFFFllu;
    uint64_t valHi=val;
    valHi>>=32;
    valHi&=0xFFFFFFFFllu;
    setIntReg(RetValLoReg,(IntRegValue)valLo);
    setIntReg(RetValHiReg,(IntRegValue)valHi);
  }
  

  inline icode_ptr getPCIcode(void) const{
    I((pid!=-1)||(picode==&invalidIcode));
    return picode;
  }
  /*
#ifdef SESC_COW_REPLAY
inline void setThreadPrama(struct threadPrama tp){
	prama.intArg1Reg = tp.intArg1Reg;
	prama.intArg2Reg =  tp.intArg2Reg;
	prama.jmpPtrReg = tp.jmpPtrReg;
	prama.entry = tp.entry;
	prama.ppid = tp.ppid;
	prama.retAddrReg = tp.retAddrReg;
}
inline struct threadPrama getThreadPrama(){
	return prama;
}
#endif
*/
  inline void setPCIcode(icode_ptr nextIcode){
    I((pid!=-1)||(nextIcode==&invalidIcode));
    picode=nextIcode;
  }
  
  inline icode_ptr getRetIcode(void) const{
    return addr2icode(getIntReg(RetAddrReg));
  }


  
  // Returns the pid of the context
  Pid_t getPid(void) const { return pid; }

#if !(defined MIPS_EMUL)
  // Sets the pid of the context
  void setPid(Pid_t newPid){
    pid=newPid;
  }
/*
#ifdef SESC_COW
 void setSpecFlag(bool flag)
{
	spec = flag;
}
bool getSpecFlag()
{
	return spec;
}
#endif 
*/
  int32_t getErrno(void){
    return *perrno;
  }

  void setErrno(int32_t newErrno){
    I(perrno);
    *perrno=newErrno;
  }
#endif



#if !(defined MIPS_EMUL)
  bool isCloned(void) const{ return (pid>=baseClonedPid); }
#endif // !(defined MIPS_EMUL)

  void copy(const ThreadContext *src);

#if !(defined MIPS_EMUL)
  uint32_t getFPUControl31() const { return fcr31; }
  void setFPUControl31(uint32_t v) {
    fcr31 = v;
  }

  uint32_t getFPUControl0() const { return fcr0; }
  void setFPUControl0(uint32_t v) {
    fcr0 = v;
  }

  int32_t getREG(icode_ptr pi, int32_t R) { return reg[pi->args[R]];}
  void setREG(icode_ptr pi, int32_t R, int32_t val) { 
	  reg[pi->args[R]] = val;
  }

  void setREGFromMem(icode_ptr pi, int32_t R, int32_t *addr) {
#ifdef LENDIAN
    int32_t val = SWAP_WORD(*addr);
#else
    int32_t val = *addr;
#endif
    setREG(pi, R, val);
  }

  float getFP( icode_ptr pi, int32_t R) { return fp[pi->args[R]]; }
  void  setFP( icode_ptr pi, int32_t R, float val) { 
    fp[pi->args[R]] = val; 
  }
  void  setFPFromMem( icode_ptr pi, int32_t R, float *addr) { 
    float *pos = &fp[pi->args[R]];
#ifdef LENDIAN
    uint32_t v1;
    v1 = *(uint32_t *)addr;
    v1 = SWAP_WORD(v1);
    *pos = *(float *)&v1;
#else
    *pos = *addr;
#endif
  }

  double getDP( icode_ptr pi, int32_t R) const { 
#ifdef SPARC 
  // MIPS supports 32 bit align double access
    uint32_t w1 = *(uint32_t *) &fp[pi->args[R]];
    uint32_t w2 = *(uint32_t *) &fp[pi->args[R]+1];
    static uint64_t ret = w2;
    ret = w2;
    ret = (ret<<32) | w1;
    return *(double *) (&ret);
#else 
    return *(double *) &fp[pi->args[R]];
#endif
  }

  void   setDP( icode_ptr pi, int32_t R, double val) { 
#ifdef SPARC 
    uint32_t *pos = (uint32_t*)&fp[pi->args[R]];
    uint32_t b1 = ((uint32_t *)&val)[0];
    uint32_t b2 = ((uint32_t *)&val)[1];
    pos[0] = b1;
    pos[1] = b2;	
#else
    *((double *)&fp[pi->args[R]]) = val; 
#endif
  }


  void   setDPFromMem( icode_ptr pi, int32_t R, double *addr) { 
#ifdef SPARC 
    uint32_t *pos = (uint32_t*) ((long)(fp) + pi->args[R]);
    pos[0] = (uint32_t) addr[0];
    pos[1] = (uint32_t) addr[1];
#else
    double *pos = (double *) &fp[pi->args[R]];
#ifdef LENDIAN
    uint64_t v1;
//	printf("error addr = %x\n",addr);
    v1 = *(uint64_t *)(addr);
    v1 = SWAP_LONG(v1);
    *pos = *(double *)&v1;
#else
    *pos = *addr;
#endif // LENDIAN
#endif // SPARC
  }

  int32_t getWFP(icode_ptr pi, int32_t R) { return *(int32_t   *)&fp[pi->args[R]]; }
  void setWFP(icode_ptr pi, int32_t R, int32_t val) { 
    *((int32_t   *)&fp[pi->args[R]]) = val; 
  }

  // Methods used by ops.m4 and coproc.m4 (mostly)
  int32_t getREGNUM(int32_t R) const { return reg[R]; }
  void setREGNUM(int32_t R, int32_t val) {
    reg[R] = val;
  }

  // FIXME: SPARC
  double getDPNUM(int32_t R) {return *((double *)&fp[R]); }
  void   setDPNUM(int32_t R, double val) {
    *((double *) &fp[R]) = val; 
  }

  // for constant (or unshifted) register indices
  float getFPNUM(int32_t i) const { return fp[i]; }
  int32_t getWFPNUM(int32_t i) const  { return *((int32_t *)&fp[i]); }

  RAddr getRAddr() const{
    return raddr;
  }
  void setRAddr(RAddr a){
    raddr = a;
  }
  VAddr getVAddr() const{
  	return vaddr;
  }
  void setVAddr(VAddr a){
  	vaddr = a;	
  }

  void dump();
  void dumpStack();

  static void staticConstructor(void);

  static size_t size();
#endif // !(defined MIPS_EMUL)

  static ThreadContext *getContext(Pid_t pid);


  static ThreadContext *getMainThreadContext(void);

  static ThreadContext *newActual(void);
  static ThreadContext *newActual(Pid_t pid);
  static ThreadContext *newCloned(void);
  void free(void);
  static uint64_t getMemValue(RAddr p, unsigned dsize); 


  // BEGIN Memory Mapping
  bool isValidDataVAddr(VAddr vaddr) const{
#if !(defined MIPS_EMUL)
    return (vaddr>=dataVAddrLb)&&(vaddr<dataVAddrUb);
#else
    return canRead(vaddr,1)||canWrite(vaddr,1);
#endif
  }

  void setHeapManager(HeapManager *newHeapManager){
    I(!heapManager);
    heapManager=newHeapManager;
    heapManager->addReference();
  }
  HeapManager *getHeapManager(void) const{
    I(heapManager);
    return heapManager;
  }

  inline void setStack(VAddr stackLb, VAddr stackUb){
    myStackAddrLb=stackLb;
    myStackAddrUb=stackUb;
  }
  inline VAddr getStackAddr(void) const{
    return myStackAddrLb;
  }
  inline VAddr getStackSize(void) const{
    return myStackAddrUb-myStackAddrLb;
  }
#if !(defined MIPS_EMUL)
  void initAddressing(VAddr dataVAddrLb, VAddr dataVAddrUb,
		      MINTAddrType rMap, MINTAddrType mMap, MINTAddrType sTop);
  RAddr virt2real(VAddr vaddr, short opflags=E_READ | E_BYTE) const{


    I(isValidDataVAddr(vaddr));
    return virtToRealOffset+vaddr;
  }
  VAddr real2virt(RAddr raddr) const{
    VAddr vaddr=raddr-virtToRealOffset;
    I(isValidDataVAddr(vaddr));
    return vaddr;
  }
#endif // !(defined MIPS_EMUL)

  bool isHeapData(VAddr addr) const{
    I(heapManager);
    return heapManager->isHeapAddr(addr);
  }


  bool isLocalStackData(VAddr addr) const {
    return (addr>=myStackAddrLb)&&(addr<myStackAddrUb);
  }

  VAddr getStackTop() const{
    return myStackAddrLb;
  }
  // END Memory Mapping

#if !(defined MIPS_EMUL)
  ThreadContext *getParent() const { return parent; }

  void useSameAddrSpace(ThreadContext *parent);
  void shareAddrSpace(ThreadContext *parent, int32_t share_all, int32_t copy_stack);
  void newChild(ThreadContext *child);
  void init();
#endif

#if (defined MIPS_EMUL)
  inline InstDesc *getIDesc(void) const {
    return iDesc;
  }
  inline void updIDesc(ssize_t ddiff){
    I((ddiff>=-1)&&(ddiff<4));
    iDesc+=ddiff;
  }
  inline VAddr getIAddr(void) const {
    return iAddr;
  }
  inline void setIAddr(VAddr addr){
    iAddr=addr;
    iDesc=iAddr?virt2inst(addr):0;
  }
  inline void updIAddr(ssize_t adiff, ssize_t ddiff){
    I((ddiff>=-1)&&(ddiff<4));
    I((adiff>=-4)&&(adiff<=8));
    iAddr+=adiff;
    iDesc+=ddiff;
  }
  inline VAddr getDAddr(void) const {
    return dAddr;
  }
  inline void setDAddr(VAddr addr){
    dAddr=addr;
  }
  inline void addDInst(void){
    nDInsts++;
  }
  inline void delDInst(void){
    nDInsts--;
  }
  inline size_t getNDInsts(void){
    return nDInsts;
  }
  static inline int32_t nextReady(int32_t startPid){
    int32_t foundPid=startPid;
    do{
      if(foundPid==(int)(pid2context.size()))
        foundPid=0;
      ThreadContext *context=pid2context[foundPid];
      if(context&&(!context->isSuspended())&&(!context->isExited()))
        return foundPid;
      foundPid++;
    }while(foundPid!=startPid);
    return -1;      
  }
  inline bool skipInst(void);
  static int64_t skipInsts(int64_t skipCount);
#if (defined HAS_MEM_STATE)
  inline const MemState &getState(VAddr addr) const{
    return addressSpace->getState(addr);
  }
  inline MemState &getState(VAddr addr){
    return addressSpace->getState(addr);
  }
#endif
  inline bool canRead(VAddr addr, size_t len) const{
    return addressSpace->canRead(addr,len);
  }
  inline bool canWrite(VAddr addr, size_t len) const{
    return addressSpace->canWrite(addr,len);
  }
  void    writeMemFromBuf(VAddr addr, size_t len, const void *buf);
//  ssize_t writeMemFromFile(VAddr addr, size_t len, int32_t fd, bool natFile, bool usePread=false, off_t offs=0);
  void    writeMemWithByte(VAddr addr, size_t len, uint8_t c);  
  void    readMemToBuf(VAddr addr, size_t len, void *buf);
//  ssize_t readMemToFile(VAddr addr, size_t len, int32_t fd, bool natFile);
  ssize_t readMemString(VAddr stringVAddr, size_t maxSize, char *dstStr);
  template<class T>
  inline T readMemRaw(VAddr addr){
    if(sizeof(T)>sizeof(MemAlignType)){
      fail("ThreadContext:writeMemRaw with a too-large type\n");
//      T tmp;
//      I(canRead(addr,sizeof(T)));
//      readMemToBuf(addr,sizeof(T),&tmp);
//      return tmp;
    }
//    for(size_t i=0;i<(sizeof(T)+MemState::Granularity-1)/MemState::Granularity;i++)
//      if(getState(addr+i*MemState::Granularity).st==0)
//        fail("Uninitialized read found\n");
    return addressSpace->read<T>(addr);
  }
  template<class T>
  inline void writeMemRaw(VAddr addr, const T &val){
    //   if((addr>=0x4d565c)&&(addr<0x4d565c+12)){
    //     I(0);
    //     I(iAddr!=0x004bb428);
    //     I(iAddr!=0x004c8604);
    //     const char *fname="Unknown";
    //     if(iAddr)
    //       fname=getAddressSpace()->getFuncName(getAddressSpace()->getFuncAddr(iAddr));
    //     printf("Write 0x%08x to 0x%08x at 0x%08x in %s\n",
    //       val,addr,iAddr,fname);
    //   }
    if(sizeof(T)>sizeof(MemAlignType)){
     fail("ThreadContext:writeMemRaw with a too-large type\n");
//      if(!canWrite(addr,sizeof(val)))
//	return false;
//      writeMemFromBuf(addr,sizeof(val),&val);
//      return true;
    }
//    for(size_t i=0;i<(sizeof(T)+MemState::Granularity-1)/MemState::Granularity;i++)
//      getState(addr+i*MemState::Granularity).st=1;
    addressSpace->write<T>(addr,val);
  }
#if (defined DEBUG_BENCH)
  VAddr readMemWord(VAddr addr);
#endif

  //
  // File system
  //
 private:
  FileSys::FileSys::pointer fileSys;
  FileSys::OpenFiles::pointer openFiles;
 public:
  FileSys::FileSys *getFileSys(void) const{
    return fileSys;
  }
  FileSys::OpenFiles *getOpenFiles(void) const{
    return openFiles;
  }

  //
  // Signal handling
  //
 private:
  SignalTable::pointer sigTable;
  SignalSet   sigMask;
  SignalQueue maskedSig;
  SignalQueue readySig;
  bool        suspSig;
 public:
  void setSignalTable(SignalTable *newSigTable){
    sigTable=newSigTable;
  }
  SignalTable *getSignalTable(void) const{
    return sigTable;
  }
  void suspend(void);
  void signal(SigInfo *sigInfo);
  void resume(void);
  const SignalSet &getSignalMask(void) const{
    return sigMask;
  }
  void setSignalMask(const SignalSet &newMask){
    sigMask=newMask;
    for(size_t i=0;i<maskedSig.size();i++){
      SignalID sig=maskedSig[i]->signo;
      if(!sigMask.test(sig)){
	readySig.push_back(maskedSig[i]);
	maskedSig[i]=maskedSig.back();
	maskedSig.pop_back();
      }
    }
    for(size_t i=0;i<readySig.size();i++){
      SignalID sig=readySig[i]->signo;
      if(sigMask.test(sig)){
	maskedSig.push_back(readySig[i]);
	readySig[i]=readySig.back();
	readySig.pop_back();
      }
    }
    if((!readySig.empty())&&suspSig)
      resume();
  }
  bool hasReadySignal(void) const{
    return !readySig.empty();
  }
  SigInfo *nextReadySignal(void){
    I(hasReadySignal());
    SigInfo *sigInfo=readySig.back();
    readySig.pop_back();
    return sigInfo;
  }

  // System state

  LinuxSys *mySystem;
  LinuxSys *getSystem(void) const{
    return mySystem;
  }

  // Parent/Child relationships
 private:
  typedef std::set<int> IntSet;
  // Thread id of this thread
  int32_t tid;
  // tid of the thread group leader
  int32_t tgid;
  // This set is empty for threads that are not thread group leader
  // In a thread group leader, this set contains the other members of the thread group
  IntSet tgtids;

  // Process group Id is the PId of the process group leader
  int32_t pgid;

  int parentID;
  IntSet childIDs;
  // Signal sent to parent when this thread dies/exits
  SignalID  exitSig;
  // Futex to clear when this thread dies/exits
  VAddr clear_child_tid;
  // Robust list head pointer
  VAddr robust_list;
 public:
  int32_t gettgid(void) const{
    return tgid;
  }
  size_t gettgtids(int tids[], size_t slots) const{
    IntSet::const_iterator it=tgtids.begin();
    for(size_t i=0;i<slots;i++,it++)
      tids[i]=*it;
    return tgtids.size();
  }
  int32_t gettid(void) const{
    return tid;
  }
  int32_t getpgid(void) const{
    return pgid;
  }
  int getppid(void) const{
    return parentID;
  }
  void setRobustList(VAddr headptr){
    robust_list=headptr;
  }
  void setTidAddress(VAddr tidptr){
    clear_child_tid=tidptr;
  }
  int32_t  getParentID(void) const{
    return parentID;
  }
  bool hasChildren(void) const{
    return !childIDs.empty();
  }
  bool isChildID(int32_t id) const{
    return (childIDs.find(id)!=childIDs.end());
  }
  int32_t findZombieChild(void) const;
  SignalID getExitSig(void){
    return exitSig;
  }
 private:
  bool     exited;
  int32_t      exitCode;
  SignalID killSignal;
 public:
  bool isSuspended(void) const{
    return suspSig;
  }
  bool isExited(void) const{
    return exited;
  }
  int32_t getExitCode(void) const{
    return exitCode;
  }
  bool isKilled(void) const{
    return (killSignal!=SigNone);
  }
  SignalID getKillSignal(void) const{
    return killSignal;
  }
  // Exit this process
  // Returns: true if exit complete, false if process is now zombie
  bool exit(int32_t code);
  // Reap an exited process
  void reap();
  void doKill(SignalID sig){
    I(!isExited());
    I(!isKilled());
    I(sig!=SigNone);
    killSignal=sig;
  }

  // Debugging

  class CallStackEntry{
  public:
    VAddr entry;
    VAddr ra;
    VAddr sp;
    bool  tailr;
    CallStackEntry(VAddr entry, VAddr  ra, VAddr sp, bool tailr)
      : entry(entry), ra(ra), sp(sp), tailr(tailr){
    }
  };
  typedef std::vector<CallStackEntry> CallStack;
  CallStack callStack;

  void execCall(VAddr entry, VAddr  ra, VAddr sp);
  void execRet(VAddr entry, VAddr ra, VAddr sp);
  void dumpCallStack(void);
  void clearCallStack(void);

#endif // For (defined MIPS_EMUL)
#if !(defined MIPS_EMUL)
  icode_ptr getPicode() const { return picode; }
  void setPicode(icode_ptr p) {
    picode = p;
  }

  icode_ptr getTarget() const { return target; }
  void setTarget(icode_ptr p) {
    target = p;
  }

  int32_t getperrno() const { return *perrno; }
  void setperrno(int32_t v) {
    I(perrno);
    *perrno = v;
  }

  int32_t getFD(int32_t id) const { return fd[id]; }
  void setFD(int32_t id, int32_t val) {
    fd[id] = val;
  }
  
  static void initMainThread();
#endif // For !(defined MIPS_EMUL)
};

#if !(defined MIPS_EMUL)
typedef ThreadContext mint_thread_t;

// This class helps extract function parameters for substitutions (e.g. in subs.cpp)
// First, prepare for parameter extraction by constructing an instance of MintFuncArgs.
// The constructor takes as a parameter the ThreadContext in which a function has just
// been called (i.e. right after the jal and the delay slot have been emulated
// Then get the parameters in order from first to last, using getInt32 or getInt64
// MintFuncArgs automatically takes care of getting the needed parameter from the register
// or from the stack, according to the MIPS III caling convention. In particular, it correctly
// implements extraction of 64-bit parameters, allowing lstat64 and similar functions to work
// The entire process of parameter extraction does not change the thread context in any way
class MintFuncArgs{
 private:
  const ThreadContext *myContext;
  const icode_t *myIcode;
  int32_t   curPos;
 public:
  MintFuncArgs(const ThreadContext *context, const icode_t *picode)
    : myContext(context), myIcode(picode), curPos(0)
    {
    }
  int32_t getInt32(void);
  int64_t getInt64(void);
  VAddr getVAddr(void){ return (VAddr)getInt32(); }
};

#define REGNUM(R) (*((int32_t *) &pthread->reg[R]))
#endif // For !(defined MIPS_EMUL)

#endif // THREADCONTEXT_H
