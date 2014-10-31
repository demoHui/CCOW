#include "ElmCtrl.h"

/*uint32_t elmCtrl::specRead(VAddr addr)
{
	uint32_t pageIndex = pt->findEntry(addr);
	if(pageIndex != addr)
		return pages[pageIndex].read(addr);
	else
		return addr;
}


void elmCtrl::specWrite(VAddr addr, uint32_t val)
{
	uint32_t pageIndex = pt->findEntry(addr);
	bool success = false;
	if(pageIndex != addr)
		pages[pageIndex].write(addr,val);
	else
		{
			success = allocatePage(addr);
			if(success){
				pageIndex = pt->findEntry(addr);
				pages[pageIndex].write(addr,val);
			}
				
			else{printf("no free pages for allocate\n");exit(0);}
		}		
}
*/
bool elmCtrl::allocatePage(RAddr addr)
{
	uint32_t index = pt->allocateEntry(addr);
	if(index != addr)
		return true;
	else
		return false;
}

uint32_t elmCtrl:: getRSpecAddr(RAddr addr)
{
	uint32_t pageIndex = pt->findEntry(addr);
	if(pageIndex != addr )
		return pages[pageIndex].readAddr(addr);
	else
		return ACCESSERR;
}
uint32_t elmCtrl::getWSpecAddr(RAddr addr,unsigned int typeSize)
{
	uint32_t pageIndex = pt->findEntry(addr);
	bool success = false;
	if(pageIndex != addr)
		pages[pageIndex].writeAddr(addr,typeSize);
	else
		{
			success = allocatePage(addr);
			if(success){
				pageIndex = pt->findEntry(addr);
				pages[pageIndex].writeAddr(addr,typeSize);
			}
			else{printf("no free pages for allocate\n");exit(0);}
		}
}