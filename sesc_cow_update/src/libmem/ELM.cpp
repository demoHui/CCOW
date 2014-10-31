#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <iostream>

#include "nanassert.h"

#include "ELM.h"
#include "SescConf.h"
#include "MemorySystem.h"
#include "OSSim.h"
#include "GProcessor.h"


ELM::ELM(MemorySystem *gms, const char *section, const char *name)
  : MemObj(section, name)
  ,writeMiss("%s:writeMiss", name)
  ,readMiss("%s:readMiss", name)
  ,readHit("%s:readHit", name)
  ,writeHit("%s:writeHit", name)
  ,avgMissLat("%s_avgMissLat", name)
{
  MemObj *lower_level = NULL;
  

  lower_level = gms->declareMemoryObj(section, k_lowerLevel);


  int32_t elmNumPorts = SescConf->getInt(section, k_numPorts);
  int32_t elmPortOccp = SescConf->getInt(section, k_portOccp);

  elmPort = PortGeneric::create(name, elmNumPorts, elmPortOccp);

  SescConf->isInt(section, k_hitDelay);
  hitDelay = SescConf->getInt(section, k_hitDelay);

  SescConf->isInt(section, k_missDelay);
  missDelay = SescConf->getInt(section, k_missDelay);

  

  if (lower_level != NULL)
      addLowerLevel(lower_level);
//  char* elmPtr = (char*)malloc(K(nPage)*sizeof(char));
//  memset(elmPtr,0,sizeof(char));
//  baseAddr = (PAddr)elmPtr;
}

ELM::~ELM()
{
	//delete []baseAddr;
}

void ELM::read(MemRequest *mreq)
{ 
  doReadCB::scheduleAbs(nextElmSlot(), this, mreq);
}


void ELM::doRead(MemRequest *mreq)
{
  readHit.inc();
  mreq->EMemRequest::goUp(hitDelay);
}


/*void ELM::readMissHandler(MemRequest *mreq){

  mreq->setClockStamp(globalClock);//need subtract one cycle???

  readMiss.inc();
  sendMiss(mreq);
}

void ELM::sendMiss(MemRequest *mreq)
{
  mreq->goCache(0,dataCache);
}
*/
void ELM::write(MemRequest *mreq)
{ 
  doWriteCB::scheduleAbs(nextElmSlot(), this, mreq);
}

void ELM::doWrite(MemRequest *mreq)
{
//if hashFunc doesn't find a free space...
  Time_t lat = hitDelay;
  if(mreq->exePath == ALLOCPAGE){
  	lat += missDelay;
  	writeMiss.inc();
  }
  else
  	writeHit.inc();
  mreq->EMemRequest::goUp(hitDelay);
}

void ELM::access(MemRequest *mreq) 
{
  mreq->setClockStamp((Time_t) - 1);

  switch(mreq->getMemOperation()){
  case MemRead:    read(mreq);       break;
  case MemWrite:   write(mreq);      break;
  default: break;
  }
}

