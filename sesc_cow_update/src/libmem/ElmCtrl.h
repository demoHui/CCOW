#ifndef ELMCTRL_H
#define ELMCTRL_H

#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Addressing.h"

#define pageSize 4*1024
#define pageOffsetMask 0x0fff
#define tagMask  0xffe00000
#define byteMask 0x0ff
#define indexMask 0x03ff000
#define wordOffsetMask 0x01
#define ACCESSERR 0xffffffff

typedef uint32_t  VAddr;
typedef unsigned char   BYTE;

class Page
{
protected:
	//BYTE *data;
	uint32_t *validBits;
public:
	Page(){	}
	~Page()
	{
		//free(data);
		
	}
	void destroy()
		{
			free(validBits);
		}
	void init()
	{
		//data = (BYTE *)malloc(sizeof(BYTE)*pageSize);
		validBits = (uint32_t *)malloc(sizeof(uint32_t)*(pageSize/32));
		//memset(data,0,pageSize);
		memset(validBits,0,pageSize/32);
	}
	uint32_t readAddr(RAddr addr)
	{
		int index = addr & pageOffsetMask;
		if(getValidBit(addr))
			return (uint32_t)&data[index];	
		else return ACCESSERR;
	}
	uint32_t writeAddr(RAddr addr,unsigned int typeSize)
	{
		int index = addr & pageOffsetMask;
		int i ;
		for (i = 0;i<typeSize;i++,index++)
			 setValidBit(addr+i);
		return (uint32_t)&data[index];
	}
/*	uint32_t read(uint32_t addr)
	{
		int index = addr & pageOffsetMask;
		uint32_t byte4 = 0;
		int i;
		if(getValidBit(addr)){
				for(i = 0; i<4; i ++,index++)  
					byte4 += (uint32_t)data[index] << (8 * i);
				return byte4;
			}
		else return ACCESSERR;
	}
	void write(uint32_t addr,uint32_t val)
	{
		int index = addr & pageOffsetMask;
		int i ;
		for (i = 0;i<4;i++,index++){
			 BYTE tmp = val & byteMask;
			 data[index] = tmp;
			 setValidBit(addr+i);
			 val = val >> 8;
		}
	}
*/	void setValidBit(uint32_t addr)
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
	void clear()
	{
		memset(validBits,0,pageSize/32);
	}

};

class pageTable{
protected:
	bool* validBits;
	uint32_t* ptTags;
	uint32_t* elmPageAddr;
public:
	pageTable(){}
	~pageTable()
		{
			
		}
	void destroy()
		{
			free(validBits);
			free(ptTags);
			free(elmPageAddr);
		}
	void init(int num)
		{
				validBits = (bool *)malloc(sizeof(bool)*num);
				ptTags = (uint32_t *)malloc(sizeof(uint32_t)*num);
				elmPageAddr = (uint32_t*)malloc(sizeof(uint32_t)*num);
				memset(validBits,0,sizeof(bool));
				memset(ptTags,0,sizeof(uint32_t));
				memset(elmPageAddr,0,sizeof(uint32_t));
		}
	uint32_t findEntry(RAddr addr)
		{
			uint32_t elmPageI = getPtIndex( addr);
			uint32_t tag = addr & tagMask;
			if(validBits[elmPageI]&&(ptTags[elmPageI] == tag))
				return elmPageI;
			else
				return addr;
		}
	uint32_t allocateEntry(RAddr addr)
		{
			uint32_t elmPageI = getPtIndex( addr);
			if(validBits[elmPageI])
				return addr;
			else {
				setValid(elmPageI);
				setPtTag(addr,elmPageI);
				return elmPageI;
			}
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
};

class elmCtrl{
protected:
	Page *pages;
	int pageNum;
	pageTable *pt;
public:
	elmCtrl()
		{
			
		}
	~elmCtrl()
		{	
			pt->destroy();
			int i;
			for(i=0;i<pageNum;i++)
				pages[i]->destroy();
			free(pages);
			free(pt);
		}
	void init(int Num)
	{
		pages = (Page *)malloc(sizeof(Page)*pageNum);
		pageNum = Num;
		pt = (pageTable *)malloc(sizeof(pageTable));
		pt->init(pageNum);
		int i ;
		for(i=0;i<pageNum;i++)
			pages[i].init();
	}
	bool allocatePage(RAddr addr);
	uint32_t getRSpecAddr(RAddr addr);
	uint32_t getWSpecAddr(RAddr addr,unsigned int typeSize);
/*	uint32_t specRead(VAddr addr);
	void specWrite(VAddr addr,uint32_t val);
*/};

#endif
