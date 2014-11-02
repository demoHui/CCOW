#ifndef __TLS_COMMON_H__
#define __TLS_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/times.h>
#ifdef SESC_COW
//#include "/root/sesc_cow/src/libapp/sescapi.h"
#include "/home/sesc_project/HVD-TLS/common/sescapi.h"
#include "SESC_ThreadControl.h"
#endif

#define EXCEPTION 0x00000001
#define REPLY 0x00000002
#define FINISH 0x00000004
#define START 0x00000008
#define CONFIRM 0x00000010
#define REROLL 0x00000020
#define KILL 0x00000040
#define ENDING 0X00000080
#define NUM_THREAD 3
#define NUM_CHILD_ARGS 6
#define VT_LEN 10
#define DIM 500
#define CVAR_NUM 50
#define INIT_GRAN 1

#define CT_NORMAL	0
#define CT_DARRAY	1
#define CT_RPLUS	2
#define CT_IARRAY	3
#define CT_ICOUNTER	4

#define MU_CHAR 0
#define MU_SHORT 1
#define MU_INT 2
#define MU_FLOAT 3
#define MU_DOUBLE 4
#define MU_NORMAL 5

#define MT_RDMAP 0
#define MT_WRMAP 1

#ifndef MAX_MMAP_REC_NUM
#define MAX_MMAP_REC_NUM 1000000
#endif

#ifdef SESC_COW
#define pthread_mutex_t slock_t
#define pthread_mutex_lock(a) sesc_lock(a)
#define pthread_mutex_unlock(a) sesc_unlock(a)
#endif


/*compiler options*/
#define TLS_DEBUG
//#define SESC_COW
#define TLS_PROF
#define TLS_DPART
#define TLS_VAL_CHK
#define TLS_MMAP
#define TLS_DGRAN
#define TLS_WHILE
//#define TLS_PRED
#define TLS_OVERHEAD
//#define TLS_GETACROSS
//#define TLS_ONTHEFLY
//#define TLS_IARRAY //this is a very bad solution of task confirm.

#endif
