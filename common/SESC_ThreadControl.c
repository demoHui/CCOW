#include "SESC_ThreadControl.h"

/******************default setting********************
arg[0] records the iteration which is the loop indices. 
before entering ThreadDispatcher, arg[0] are equal. we add
it with allocatedWork to gain current index.

arg[1] records availableThread's address, when allocating a thread
we decrease it. when a thread died, we increase it. Because the 
latter is done in sub-thread, we should ensure its modification is 
atomic.

arg[2] records the remote thread that produces the data it wants.-1 means 
it is not dependent on the previous threads that are on the run
*****************************************************/

int getDependence(int* indexs,int iteration,ControlStruct* cs)
{
	int i ;
	if(!indexs)
		return -1;
//	int max = (iteration < NUMCORE-1)?iteration:NUMCORE-1;
	if(iteration <= NUMCORE-1)
		for(i = 1;i<=iteration;i++)
		{
			int index = (iteration-i)*cs->numPara;
			if(indexs[cs->args[index]] == indexs[iteration])
			{
				return cs->args[index];
			}
		}
	else
		for(i = 1; i <= NUMCORE-1; i++)
		{
			int index = ((iteration-i)%NUMCORE)*cs->numPara;
			if(indexs[cs->args[index]] == indexs[iteration])
			{				
				return cs->args[index];
			}
		}
	return -1;
}
void ThreadDispatcher(void (*func)(void *),ControlStruct* cs,int* indexs)
{
	int i ;
	for(i=0;i<NUMCORE;i++)
	{
		if(cs->hasWork)
		{
			int index = i*cs->numPara;
			cs->args[index] = cs->allocatedWork;//loop index
			//cs->args[index+1] = (int)&cs->availableThread;
			//cs->args[index+1] = getDependence(indexs,cs->allocatedWork,cs);
			cs->args[index+1] = 0;
			sesc_spawn(func,(void*)&cs->args[index],1);
			sesc_dec_threads();
			cs->allocatedWork++;
		}
		if(cs->allocatedWork == cs->workloadSize)
		{
			cs->hasWork = 0;
			break;
		}
	}
}

void ThreadCollector(void (*func)(void*),ControlStruct* cs,int* indexs)
{
	int readyNext = 0;
	int dependenceNext = -1;
	int availableThread=0;
	while(cs->hasWork)
	{
		sesc_get_num_threads((uint32_t)&availableThread);
		if(!readyNext)
		{
			//dependenceNext = getDependence(indexs,cs->allocatedWork,cs);
			dependenceNext = 0;
			readyNext = 1;
		}
		if(availableThread>0)
		{
			int index = (cs->allocatedWork%NUMCORE)*cs->numPara;
			cs->args[index] = cs->allocatedWork;//loop index
			//cs->args[index+1] = (int)&cs->availableThread; 
			cs->args[index+1] = dependenceNext;
			sesc_spawn(func,(void*)&cs->args[index],1);
			sesc_dec_threads();
			cs->allocatedWork++;
			readyNext = 0;
		}
		if(cs->allocatedWork == cs->workloadSize)
		{
			cs->hasWork = 0;
		}
	}
	sesc_wait();
}

void ThreadsControl(void(*func)(void*),int* arg,int n,int workloadSize,int* index)
{
/***********init**************************/
	ControlStruct* cs = (ControlStruct*)malloc(sizeof(ControlStruct));
//	int* dependence = (int*)malloc(sizeof(int)*NUMCORE);
	cs->hasWork = (int)(workloadSize!=0);
	cs->workloadSize = workloadSize;
//	cs->numCores = NUMCORE;
//	cs->availableThread = NUMCORE;
	cs->allocatedWork = 0;
	cs->args =  arg;
	cs->numPara = n;
	//cs->dependence = dependence;
	
/***********thread dispatch****************/	
	ThreadDispatcher(func,cs,index);
/***********thread collection**************/
	ThreadCollector(func,cs,index);
	sesc_main_clear();
	free(cs);
//	free(dependence);
}
