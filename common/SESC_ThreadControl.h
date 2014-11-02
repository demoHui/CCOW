#include "sescapi.h"

#define NUMCORE 20
/*control variables*/
typedef struct{
	int numCores;
//	int availableThread;
	int hasWork;
	int workloadSize;
	int allocatedWork;
	int* args;
	int numPara;
//	int* dependence;
}ControlStruct;

void ThreadDispatcher(void(*func)(void*),ControlStruct* cs,int* index);
void ThreadCollector(void(*func)(void*),ControlStruct* cs,int* index);
void ThreadsControl(void(*func)(void*),int * arg,int n,int workloadSize,int* index);
