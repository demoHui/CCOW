/*
 * Routines for implementing the MIPS instruction set.
 *
 * Copyright (C) 1993 by Jack E. Veenstra (veenstra@cs.rochester.edu)
 * 
 * This file is part of MINT, a MIPS code interpreter and event generator
 * for parallel programs.
 * 
 * MINT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * MINT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MINT; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include "icode.h"
#include "ThreadContext.h"
#include "globals.h"
#include "opcodes.h"
#include "mendian.h"



//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
#include "OSSim.h"
#include "GProcessor.h"
#include "Cache.h"
#include <stdint.h>
#endif

#if ( defined(TASKSCALAR))
int rsesc_exception(int pid);
int rsesc_is_safe(int pid);
int rsesc_become_safe(int pid);
#endif

/* opcode functions in alphabetical order (more or less) */

M4_IN(add_op,
{
    int val;

    val = pthread->getREG(picode, RS) + pthread->getREG(picode, RT);
#ifdef OVERFLOW_CHK
    /* need to check for overflow here */
#endif

    pthread->setREG(picode, RD, val);
})

M4_IN(addi_op,
{
    int val;

    val = pthread->getREG(picode, RS) + picode->immed;
#ifdef OVERFLOW_CHK
    /* need to check for overflow here */
#endif

    pthread->setREG(picode, RT, val);
})

M4_IN(addiu_op,
{
	 pthread->setREG(picode, RT, pthread->getREG(picode, RS) + picode->immed);
})

M4_IN(sp_over_op,
{
  pthread->setREGNUM(29, pthread->getREG(picode, RS) + picode->immed);
  if(pthread->getStkPtr()<pthread->getStackTop()){
#ifdef TASKSCALAR
    if(!rsesc_is_safe(pthread->getPid())) {
      rsesc_exception(pthread->getPid());
#ifdef TS_CAVA
      rsesc_become_safe(pthread->getPid());
#endif
    } else {
#endif
      fprintf(stderr, "stack overflow at instruction 0x%x sp v=0x%x p=0x%x stack top=0x%x\n", picode->addr
	      ,pthread->virt2real(pthread->getStkPtr())
                      ,pthread->getREGNUM(29)
                      ,pthread->getStackTop());
      fprintf(stderr, "Increase the stack size with the `-k' option.\n");
      exit(1);
#ifdef TASKSCALAR
    }
#endif
  }
})

M4_IN(li_op,
{
  pthread->setREG(picode, RT, picode->immed);
})

M4_IN(addiu_xx_op,
{
  pthread->setREG(picode, RT, pthread->getREG(picode, RT) + picode->immed);
})

M4_IN(addu_op,
{
	pthread->setREG(picode, RD, pthread->getREG(picode, RS) + pthread->getREG(picode, RT));
})

M4_IN(move_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS));
})

M4_IN(move0_op,
{
  pthread->setREG(picode, RD, 0);
})

M4_IN(and_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) & pthread->getREG(picode, RT));
})

M4_IN(andi_op,
{
  pthread->setREG(picode, RT, pthread->getREG(picode, RS) & (unsigned short) picode->immed);
})

M4_IN(beq_op,
{
  if (pthread->getREG(picode, RS) == pthread->getREG(picode, RT))
    pthread->setTarget(picode->target);
  else
    pthread->setTarget(picode->not_taken);
})

M4_IN(b_op,
{
    pthread->setTarget(picode->target);
})

M4_IN(beq0_op,
{
    if (pthread->getREG(picode, RS) == 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(beql_op,
{
    /* R4000 instruction */
    if (pthread->getREG(picode, RS) == pthread->getREG(picode, RT))
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(bgez_op,
{
    if (pthread->getREG(picode, RS) >= 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(bgezal_op,
{
    /* The return location is unconditionally copied to r31 */
  pthread->setREGNUM(31, icode2addr(picode->not_taken));
  if (pthread->getREG(picode, RS) >= 0)
    pthread->setTarget(picode->target);
  else
    pthread->setTarget(picode->not_taken);
})

M4_IN(bgezall_op,
{
  /* The return location is unconditionally copied to r31 */
  pthread->setREGNUM(31, icode2addr(picode->not_taken));
  if (pthread->getREG(picode, RS) >= 0)
    pthread->setTarget(picode->target);
  else {
    /* The conditional branch is not taken, so nullify the branch delay
     * slot instruction, but still count one cycle for the cost.
     */
    return picode->not_taken;
  }
})

M4_IN(bgezl_op,
{
    if (pthread->getREG(picode, RS) >= 0)
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(bgtz_op,
{
    if (pthread->getREG(picode, RS) > 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(bgtzl_op,
{
    if (pthread->getREG(picode, RS) > 0)
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(blez_op,
{
    if (pthread->getREG(picode, RS) <= 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(blezl_op,
{
    if (pthread->getREG(picode, RS) <= 0)
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(bltz_op,
{
    if (pthread->getREG(picode, RS) < 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(bltzal_op,
{
  /* The return location is unconditionally copied to r31 */
  pthread->setREGNUM(31, icode2addr(picode->not_taken));
  if (pthread->getREG(picode, RS) < 0)
    pthread->setTarget(picode->target);
  else
    pthread->setTarget(picode->not_taken);
})

M4_IN(bltzall_op,
{
  /* The return location is unconditionally copied to r31 */
  pthread->setREGNUM(31, icode2addr(picode->not_taken));
  if (pthread->getREG(picode, RS) < 0)
    pthread->setTarget(picode->target);
  else {
    /* The conditional branch is not taken, so nullify the branch delay
     * slot instruction, but still count one cycle for the cost.
     */
    return picode->not_taken;
  }
})

M4_IN(bltzl_op,
{
    if (pthread->getREG(picode, RS) < 0)
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(bne_op,
{
    if (pthread->getREG(picode, RS) != pthread->getREG(picode, RT))
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(bne0_op,
{
    if (pthread->getREG(picode, RS) != 0)
        pthread->setTarget(picode->target);
    else
        pthread->setTarget(picode->not_taken);
})

M4_IN(bnel_op,
{
    /* R4000 instruction */
    if (pthread->getREG(picode, RS) != pthread->getREG(picode, RT))
        pthread->setTarget(picode->target);
    else {
        /* The conditional branch is not taken, so nullify the branch delay
         * slot instruction, but still count one cycle for the cost.
         */
        return picode->not_taken;
    }
})

M4_IN(break_op,
{

#ifdef TASKSCALAR
    if(!rsesc_is_safe(pthread->getPid())) {
        rsesc_exception(pthread->getPid());
    } else
#endif
    fatal("break instruction at 0x%x (division by zero?)\n", picode->addr);

})

M4_IN(cache_op,
{
    fatal("cache: not yet implemented\n");
})

M4_IN(div_op,
{
#ifdef TASKSCALAR
  if(pthread->getREG(picode, RT) == 0) {
      rsesc_exception(pthread->getPid());
      pthread->setREG(picode, RT, 0);
  } else 
#endif
  mips_div(pthread->getREG(picode, RS), pthread->getREG(picode, RT), &(pthread->lo), &(pthread->hi));
})

M4_IN(divu_op,
{
#ifdef TASKSCALAR
        if(((unsigned int) pthread->getREG(picode, RT)) == 0) {
      rsesc_exception(pthread->getPid());
      pthread->setREG(picode, RT, 0);
   } else 
#endif
         mips_divu((unsigned int) pthread->getREG(picode, RS),
                (unsigned int) pthread->getREG(picode, RT),
      &(pthread->lo), &(pthread->hi));
})

M4_IN(j_op,
{
  pthread->setTarget(picode->target);
})

M4_IN(jal_op,
{
  pthread->setREGNUM(31, picode->addr + 8);
          
  pthread->setTarget(picode->target);
})

M4_IN(jalr_op,
{
    icode_ptr iaddr;

    pthread->setREG(picode, RD, picode->addr + 8);

#if (defined(TASKSCALAR))
    if (pthread->getREG(picode, RS) & 0x3
        ||
         pthread->getREG(picode, RS) < Text_start 
        || pthread->getREG(picode, RS) >= (Text_end + 4 * EXTRA_ICODES)) {
#ifdef DEBUG
      fprintf(stderr,"Jump to hell 0x%x target 0x%x\n",picode->addr, pthread->getREG(picode, RS));
#endif
      rsesc_exception(pthread->getPid());
#ifdef TS_CAVA
      rsesc_become_safe(pthread->getPid());
#endif
      iaddr = picode;
    }else{
      iaddr = addr2icode(pthread->getREG(picode, RS));
    }
#else
#ifdef ADDRESS_CHK
    if (pthread->getREG(picode, RS) & 0x3)
        address_exception_op(picode, pthread);
#endif
#ifdef DEBUG
         if ((signed int) pthread->getREG(picode, RS) >= (Text_end + 4 * EXTRA_ICODES)) {
        fatal("target address (0x%x) of jalr instruction at addr 0x%x is past end of text.\n",
              pthread->getREG(picode, RS), picode->addr);
    }
#endif

    iaddr = addr2icode(pthread->getREG(picode, RS));
#endif

    pthread->setTarget(iaddr);
})


M4_IN(jr_op,
{
    icode_ptr iaddr;

#if (defined(TASKSCALAR))
    if (pthread->getREG(picode, RS) & 0x3
        ||
         pthread->getREG(picode, RS) < Text_start 
        || pthread->getREG(picode, RS) >= (Text_end + 4 * EXTRA_ICODES)) {
#ifdef DEBUG
      fprintf(stderr,"Jump to hell 0x%x target 0x%x\n",picode->addr, pthread->getREG(picode, RS));
#endif
      rsesc_exception(pthread->getPid());
#ifdef TS_CAVA
      rsesc_become_safe(pthread->getPid());
#endif
      iaddr = picode;
    }else{
      iaddr = addr2icode(pthread->getREG(picode, RS));
    }
#else
#ifdef ADDRESS_CHK
    if (pthread->getREG(picode, RS) & 0x3)
        address_exception_op(picode, pthread);
#endif
#ifdef DEBUG
         if ((signed int) pthread->getREG(picode, RS) >= (Text_end + 4 * EXTRA_ICODES)) {
        fatal("target address (0x%x) of jr instruction at addr 0x%x is past end of text.\n",
              pthread->getREG(picode, RS), picode->addr);
    }
#endif
    iaddr = addr2icode(pthread->getREG(picode, RS));
#endif

    pthread->setTarget(iaddr);
})

M4_BREAD(lb_op, pthread->getREG(picode, RT),
{
    /* read value from memory */
#ifdef SESC_COW_UPDATE
  if(!pthread->isSpecThread())
  	pthread->setREG(picode, RT, (int) *(signed char *) raddr);
  else{
  	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
	uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
	picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
	if(laddr == ACCESSERR){
		VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(signed char));
		if(vTag){
			int pid = pthread->iter2Pid(vTag->iteration);
			if(pthread->isSharedData(raddr,sizeof(signed char)) && pid != -1){
				GProcessor* pro = osSim->pid2GProcessor(pid);
				if(pro){
					ThreadContext* thread = pro->getThreadContext(pid);
					int version =thread->getWriteVersion(raddr,sizeof(signed char));
					if(version == -1){
//							picode-> exePath = REMOTEACCESS;
							return picode;
				}
				else{
					laddr = pro->elm.getRSpecAddr((RAddr)raddr);
					picode->exePath = REMOTEACCESS;
					pthread->setVersion(raddr,sizeof(signed char),version);
					pthread->setREG(picode, RT, (int) *(signed char *) laddr);
					picode->remotePid = pid;
					picode->reqVersion = version;
					if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(signed char)) == NULL)
						pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(signed char),pthread->getPreVersion(vTag));
				}
			}
			else{
				pthread->setREG(picode, RT, (int) *(signed char *) raddr);
				VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
				if(v &&pthread->findVTEntry(raddr,0))
					pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
				else{
					pthread->setVersion(raddr,0,0);
					pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
				}
			}
		}
	}
	else{
		pthread->setREG(picode, RT, (int) *(signed char *) raddr);
		pthread->addLVTEntry(raddr,sizeof(signed char));
	}
	}
	else{
			pthread->setREG(picode, RT, (int) *(signed char *) laddr);
		}
  }
#else
  pthread->setREG(picode, RT, (int) *(signed char *) raddr);
#endif
})

M4_BREAD(lbu_op, pthread->getREG(picode, RT),
{
    /* read value from memory */
#ifdef SESC_COW_UPDATE
	if(!pthread->isSpecThread())
  		pthread->setREG(picode, RT, (int) *(unsigned char *) raddr);
 	else{
	  	RunningProcs proc = osSim->cpus;
	  	GProcessor* cpu = proc.getCurrentCPU();
		uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		
		if(laddr == ACCESSERR){
			VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(unsigned char));
			if(vTag){
				int pid = pthread->iter2Pid(vTag->iteration);
				
				if(vTag && pthread->isSharedData(raddr,sizeof(unsigned char)) && pid != -1){
					GProcessor* pro = osSim->pid2GProcessor(pid);
					if(pro){
						ThreadContext* thread = pro->getThreadContext(pid);
						int version = thread->getWriteVersion(raddr,sizeof(unsigned char));
						
						if(version == -1){
//							picode->exePath = REMOTEACCESS;
							return picode;
						}
						else{
							laddr = pro->elm.getRSpecAddr((RAddr)raddr);
							picode->exePath = REMOTEACCESS;
							pthread->setVersion(raddr,sizeof(unsigned char),version);
							pthread->setREG(picode, RT, (int) *(unsigned char *) laddr);
							picode->remotePid = pid;
							picode->reqVersion = version;
							if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(unsigned char)) == NULL)
								pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(unsigned char),pthread->getPreVersion(vTag));
						}
					}
				}
				else{
					pthread->setREG(picode, RT, (int) *(unsigned char *) raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
				
			}
			else{
				pthread->setREG(picode, RT, (int) *(unsigned char *) raddr);
				pthread->addLVTEntry(raddr,sizeof(unsigned char));
			}
		}
		else{
			pthread->setREG(picode, RT, (int) *(unsigned char *) laddr);
		}
	}
#else
   pthread->setREG(picode, RT, (int) *(unsigned char *) raddr);
#endif

})

M4_DREAD(ldc1_op, pthread->getDP(picode, ICODEFT),
{
#ifdef ADDRESS_CHK
    if (raddr & 7)
        address_exception_op(picode, pthread);
#endif
    /* read value from memory */
#ifdef SESC_COW_UPDATE
	  if(!pthread->isSpecThread())
		pthread->setDPFromMem(picode, ICODEFT, (double *) raddr);
	  else{
	  	RunningProcs proc = osSim->cpus;
  		GProcessor* cpu = proc.getCurrentCPU();
		uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		if(laddr == ACCESSERR){
			  	VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(double));
				if(vTag){
					int pid = pthread->iter2Pid(vTag->iteration);
/*					
					if(pthread->isSharedData(raddr,sizeof(double)))
						printf("pid %d shared data addr = %x, remote pid = %d, thread iteration  = %d\n",pthread->getPid(),raddr,pid,vTag->iteration);
*/
					if(pthread->isSharedData(raddr,sizeof(double)) && pid != -1){
						GProcessor* pro = osSim->pid2GProcessor(pid);
						if(pro){
							  ThreadContext* thread = pro->getThreadContext(pid);
							  int version = thread->getWriteVersion(raddr,sizeof(double));
//							  printf("version = %d, preversion = %d\n",version,pthread->getPreVersion(vTag));
								if(version == -1){
//										picode->exePath = REMOTEACCESS;
										return picode;
								}
								else{
										laddr = pro->elm.getRSpecAddr((RAddr)raddr);
										picode->exePath = REMOTEACCESS;
										pthread->setVersion(raddr,sizeof(double),version);
										pthread->setDPFromMem(picode, ICODEFT, (double *) laddr);
										picode->remotePid = pid;
								  		picode->reqVersion = version;
										if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(double)) == NULL)
											pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(double),pthread->getPreVersion(vTag));
								}
					}
				}
				else{
					pthread->setDPFromMem(picode, ICODEFT, (double *) raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(double));
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
			}
			else{
				pthread->setDPFromMem(picode, ICODEFT, (double *) raddr);
				pthread->addLVTEntry(raddr,sizeof(double));
			}
		}
		else{
				pthread->setDPFromMem(picode, ICODEFT, (double *) laddr);
		}
	  }
#else
    pthread->setDPFromMem(picode, ICODEFT, (double *) raddr);
#endif
})

M4_READ(ldc2_op, pthread->getREG(picode, RT),
{
    fatal("ldc2: not yet implemented\n");
})

M4_READ(ldc3_op, pthread->getREG(picode, RT),
{
    fatal("ldc3: not yet implemented\n");
})

M4_SREAD(lh_op, pthread->getREG(picode, RT),
{
#ifdef ADDRESS_CHK
  if (raddr & 1)
    address_exception_op(picode, pthread);
#endif
#ifdef LENDIAN
{
	unsigned short val ;

//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
   	 val = *(unsigned short *) raddr;
   else{
   		 RunningProcs proc = osSim->cpus;
  		 GProcessor* cpu = proc.getCurrentCPU();
		 uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		 picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		 if(laddr == ACCESSERR){
		  VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(unsigned short));
		  if(vTag){
		  	  int pid = pthread->iter2Pid(vTag->iteration);
			  if( pthread->isSharedData(raddr,sizeof(unsigned short)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
					  ThreadContext* thread = pro->getThreadContext(pid);
					  int version = thread->getWriteVersion(raddr,sizeof(unsigned short));
						  if(version == -1){
							  return picode;
						  }
						  else{
							  laddr = pro->elm.getRSpecAddr((RAddr)raddr);
							  picode->exePath = REMOTEACCESS;
							  val = *(unsigned short *) laddr;
							  pthread->setVersion(raddr,sizeof(unsigned short),version);
							  picode->remotePid = pid;
							  picode->reqVersion = version;
							  if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(unsigned short)) == NULL)
								  pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(unsigned short),pthread->getPreVersion(vTag));
						  }
					  }
			  }
			  else{
					val = *(unsigned short*)raddr;
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
			}
		 else{
			val = *(unsigned short *) raddr;
			  pthread->addLVTEntry(raddr,sizeof(unsigned short));
		 }
		 }
		 else{
			   val = *(unsigned short *) laddr;
		 	}
   	}
#else
   val = *(unsigned short *) raddr;
#endif

  val = SWAP_SHORT(val);
  pthread->setREG(picode, RT, *(signed short *)&val);

}
#else
#ifdef SESC_COW_UPDATE
	   if(!pthread->isSpecThread())
		  pthread->setREG(picode, RT, (int ) *(signed short *) raddr);
	   else{
		 RunningProcs proc = osSim->cpus;
  		 GProcessor* cpu = proc.getCurrentCPU();
		 uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		 picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		 if(laddr == ACCESSERR){
			VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(signed short));
		  if(vTag){
		  	  int pid = pthread->iter2Pid(vTag->iteration);
			  if(pthread->isSharedData(raddr,sizeof(signed short)) && pid != -1){
					  GProcessor* pro = osSim->pid2GProcessor(pid);
					  if(pro){
							  ThreadContext* thread = pro->getThreadContext(pid);
							  int version = thread->getWriteVersion(raddr,sizeof(signed short));
								if(version == -1){
										return picode;
									}
								else{
										laddr = pro->elm.getRSpecAddr((RAddr)raddr);
										picode->exePath = REMOTEACCESS;
										pthread->setREG(picode, RT, (int ) *(signed short *) laddr);
										pthread->setVersion(raddr,sizeof(signed short),version);
										picode->remotePid = pid;
									  	picode->reqVersion = version;
										if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(signed short)) == NULL)
											pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(signed short),pthread->getPreVersion(vTag));
								}
						}
				}
			else{
					pthread->setREG(picode, RT, (int) *(signed short *) raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
			}
		}
		else{
			  pthread->setREG(picode, RT, (int ) *(signed short *) raddr);
				  pthread->addLVTEntry(raddr,sizeof(signed short));
		  }
		 }
		 else{
			  pthread->setREG(picode, RT, (int ) *(signed short *) laddr);

		 	}
	   }
#else
 pthread->setREG(picode, RT, (int ) *(signed short *) raddr);
#endif 
#endif
})

M4_SREAD(lhu_op, pthread->getREG(picode, RT),
{
#ifdef ADDRESS_CHK
  if (raddr & 1)
    address_exception_op(picode, pthread);
#endif

#ifdef SESC_COW_UPDATE
		 if(!pthread->isSpecThread())
			pthread->setREG(picode, RT, (int ) *(unsigned short *) raddr);
		 else{
		     	RunningProcs proc = osSim->cpus;
  			GProcessor* cpu = proc.getCurrentCPU();
		   uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		   picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		   if(laddr == ACCESSERR){
	VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(unsigned short));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
		  if(pthread->isSharedData(raddr,sizeof(unsigned short)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(unsigned short));
							if(version == -1){
									return picode;
								}
							else{
									laddr = pro->elm.getRSpecAddr((RAddr)raddr);
									picode->exePath = REMOTEACCESS;
									pthread->setREG(picode, RT, (int ) *(unsigned short *) laddr);
									pthread->setVersion(raddr,sizeof(unsigned short),version);
									picode->remotePid = pid;
								   	picode->reqVersion = version;
									if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(unsigned short)) == NULL)
										pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(unsigned short),pthread->getPreVersion(vTag));
							}
						}
			}
			else{
					pthread->setREG(picode, RT, (int) *(unsigned short *) raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
			}
		}
		else{
				pthread->setREG(picode, RT, (int ) *(unsigned short *) raddr);
					pthread->addLVTEntry(raddr,sizeof(unsigned short));
		       }
		   }
		   else{
				pthread->setREG(picode, RT, (int ) *(unsigned short *) laddr);
		   	}
		 }
#else
  pthread->setREG(picode, RT, (int ) *(unsigned short *) raddr);
#endif 

#ifdef LENDIAN
  pthread->setREG(picode, RT, SWAP_SHORT(pthread->getREG(picode, RT)));
#endif
})

M4_READ(ll_op, pthread->getREG(picode, RT),
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
  /* read value from memory */
#ifdef SESC_COW_UPDATE
		   if(!pthread->isSpecThread())
			  pthread->setREGFromMem(picode, RT, (int *) raddr);
		   else{
			   	RunningProcs proc = osSim->cpus;
  			GProcessor* cpu = proc.getCurrentCPU();
			 uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
			 picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
			 if(laddr == ACCESSERR){
			VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(int));
			 if(vTag){
				  int pid = pthread->iter2Pid(vTag->iteration);
/*
				  if(pthread->isSharedData(raddr,sizeof(int)))
						printf("pid %d shared data addr = %x, remote pid = %d, pthread iteration  = %d\n",pthread->getPid(),raddr,pid,vTag->iteration);
*/
				  if(pthread->isSharedData(raddr,sizeof(int)) && pid != -1){
					  GProcessor* pro = osSim->pid2GProcessor(pid);
					  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(int));
							  if(version == -1){
								  return picode;
							  }
							  else{
								  laddr = pro->elm.getRSpecAddr((RAddr)raddr);
								  picode->exePath = REMOTEACCESS;
								  pthread->setREGFromMem(picode, RT, (int *) laddr);
								  pthread->setVersion(raddr,sizeof(int),version);
								  picode->remotePid = pid;
								  picode->reqVersion = version;
								  if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(int)) == NULL)
									  pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(int),pthread->getPreVersion(vTag));
							  }
					}
				}
				else{
					pthread->setREGFromMem(picode, RT, (int*)raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v&&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
			}
		  else{
				  pthread->setREGFromMem(picode, RT, (int *) raddr);
					  pthread->addLVTEntry(raddr,sizeof(int));
		   }
			 }
			 else{
				  pthread->setREGFromMem(picode, RT, (int *) laddr);
			 	}
		   }
#else
  pthread->setREGFromMem(picode, RT, (int *) raddr);
#endif 
})

M4_IN(lui_op,
{
	pthread->setREG(picode,RT,((IntRegValue)picode->immed)<<16);
})

M4_READ(lw_op, pthread->getREG(picode, RT),
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
  /* read value from memory */
#ifdef SESC_COW_UPDATE
	 if(!pthread->isSpecThread())
		pthread->setREGFromMem(picode, RT, (int *) raddr);
	 else{
	     	RunningProcs proc = osSim->cpus;
		GProcessor* cpu = proc.getCurrentCPU();
	   	uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
	   	picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
	  	 if(laddr == ACCESSERR){
	  VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(int));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
/*
		  if(pthread->isSharedData(raddr,sizeof(int)))
		  printf("pid %d shared data addr = %x, remote pid = %d, pthread iteration  = %d\n",pthread->getPid(),raddr,pid,vTag->iteration);
*/		  if(pthread->isSharedData(raddr,sizeof(int)) && pid != -1){
			  GProcessor* pro = osSim->pid2GProcessor(pid);
			  if(pro){
				  ThreadContext* thread = pro->getThreadContext(pid);
				  int version = thread->getWriteVersion(raddr,sizeof(int));
				  if(version == -1){
						  return picode;
					  }
				  else{
						  laddr = pro->elm.getRSpecAddr((RAddr)raddr);
						  picode->exePath = REMOTEACCESS;
						  pthread->setREGFromMem(picode, RT, (int *) laddr);
						  pthread->setVersion(raddr,sizeof(int),version);
						  picode->remotePid = pid;
						  picode->reqVersion = version;
						  if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(int)) == NULL)
							  pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(int),pthread->getPreVersion(vTag));
				  }
	  		}
		 }
		  else{
			pthread->setREGFromMem(picode, RT, (int*)raddr);
			VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
			
			if(v&&pthread->findVTEntry(raddr,0))
			{
				pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
			}
			else{
				pthread->setVersion(raddr,0,0);
				pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
			}
		}
	  }
	  else{
		pthread->setREGFromMem(picode, RT, (int *) raddr);
		pthread->addLVTEntry(raddr,sizeof(int));
	}
   }
   else{
		pthread->setREGFromMem(picode, RT, (int *) laddr);
   	}
 }
#else
  pthread->setREGFromMem(picode, RT, (int *) raddr);
#endif 
})

M4_FREAD(lwc1_op, pthread->getFP(picode, ICODEFT),
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
  /* read value from memory */
#ifdef SESC_COW_UPDATE
	   if(!pthread->isSpecThread())
		  pthread->setFPFromMem(picode, ICODEFT, (float *) raddr);
	   else{
		   	RunningProcs proc = osSim->cpus;
  		GProcessor* cpu = proc.getCurrentCPU();
		 uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		 picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
		 if(laddr == ACCESSERR){
	VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(float));
	if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
/*
		  if(pthread->isSharedData(raddr,sizeof(float)))
				printf("pid %d shared data addr = %x, remote pid = %d, pthread iteration  = %d\n",pthread->getPid(),raddr,pid,vTag->iteration);
*/		  if(pthread->isSharedData(raddr,sizeof(float)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(float));
							if(version == -1){
									return picode;
								}
							else{
									laddr = pro->elm.getRSpecAddr((RAddr)raddr);
									picode->exePath = REMOTEACCESS;
//									printf("INFO: pid %d REMOTE ACCESS raddr = %x\n",pthread->getPid(),raddr);
									pthread->setFPFromMem(picode, ICODEFT, (float *) laddr);
									pthread->setVersion(raddr,sizeof(float),version);
									picode->remotePid = pid;
								  	picode->reqVersion = version;
									if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(float)) ==NULL)
										pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(float),pthread->getPreVersion(vTag));
							}
					}
				}
				else{
					pthread->setFPFromMem(picode, ICODEFT, (float*) raddr);
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v&&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
		}
		else{
			  pthread->setFPFromMem(picode, ICODEFT, (float *) raddr);
//			  printf("pid %d addr %x add vt entry\n",pthread->getPid(),raddr);
			  pthread->addLVTEntry(raddr,sizeof(float));
		  }
		 }
		 else{
			  pthread->setFPFromMem(picode, ICODEFT, (float *) laddr);
		 }
	   }
#else
  pthread->setFPFromMem(picode, ICODEFT, (float *) raddr);
#endif 
})

M4_READ(lwc2_op, pthread->getWFP(picode, ICODEFT),
{
    fatal("lwc2: not yet implemented\n");
})

M4_READ(lwc3_op, pthread->getWFP(picode, ICODEFT),
{
    fatal("lwc3: not yet implemented\n");
})

M4_BREAD(lwl_op, pthread->getREG(picode, RT),
{
  /* read value from memory */
#ifdef LENDIAN
#ifdef SESC_COW_UPDATE
  if(!pthread->isSpecThread())
  	pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)raddr));
  else{
	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
	      uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		  picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
	   if(laddr == ACCESSERR){
	  VersionTag* vTag= pthread->findVTEntry(raddr,sizeof(char));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
		  if(pthread->isSharedData(raddr,sizeof(char)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(char)) ;
						if(version == -1){
								//picode->exePath = REMOTEACCESS;
								return picode;
							}
						else{
								laddr = pro->elm.getRSpecAddr((RAddr)raddr);
								picode->exePath = REMOTEACCESS;
								pthread->setVersion(raddr,sizeof(char),version);
								pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)laddr));
								picode->remotePid = pid;
								picode->reqVersion = version;
								if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(char)) == NULL)
									pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(char),pthread->getPreVersion(vTag));
						}
					}
			}
			else{
					pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)raddr));
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
				}
	}
	else{
			pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)raddr));
				pthread->addLVTEntry(raddr,sizeof(char));
		}
	   }
	   else{
			pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)laddr));
	   	}
  	}
#else
  pthread->setREG(picode, RT, mips_lwlLE(pthread->getREG(picode, RT), (char *)raddr));
#endif
#else
#ifdef SESC_COW_UPDATE
	 if(!pthread->isSpecThread())
		pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)raddr));
	 else{
	     	RunningProcs proc = osSim->cpus;
  		GProcessor* cpu = proc.getCurrentCPU();
	       uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
		   picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
	   if(laddr == ACCESSERR){
	  VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(char));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
		  if(pthread->isSharedData(raddr,sizeof(char)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(char));
						if(version == -1){
								//picode->exePath = REMOTEACCESS;
								return picode;
							}
						else{
								laddr = pro->elm.getRSpecAddr((RAddr)raddr);
								picode->exePath = REMOTEACCESS;
								pthread->setVersion(raddr,sizeof(char),version);
								pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)laddr));
								picode->remotePid = pid;
								picode->reqVersion = version;
								if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(char)) == NULL)
									pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(char),pthread->getPreVersion(vTag));
						}
					}
			}
			else{
					pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)raddr));
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
			}
	}
	else{
			pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)raddr));
				pthread->addLVTEntry(raddr,sizeof(char));
		 }
	   }
	   else{
			pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)laddr));
	   	}
	 }
#else
  pthread->setREG(picode, RT, mips_lwlBE(pthread->getREG(picode, RT), (char *)raddr));
#endif 
#endif
})

M4_BREAD(lwr_op, pthread->getREG(picode, RT),
{
  /* read value from memory */
#ifdef LENDIAN
#ifdef SESC_COW_UPDATE
 if(!pthread->isSpecThread())
	pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)raddr));
 else{
	  RunningProcs proc = osSim->cpus;
  GProcessor* cpu = proc.getCurrentCPU();
   uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
   picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
   if(laddr == ACCESSERR){
	  VersionTag* vTag  = pthread->findVTEntry(raddr,sizeof(char));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
		  if(pthread->isSharedData(raddr,sizeof(char)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(char));
						if(version == -1){
								return picode;
							}
						else{
								laddr = pro->elm.getRSpecAddr((RAddr)raddr);
								picode->exePath = REMOTEACCESS;
								pthread->setVersion(raddr,sizeof(char),version);
								pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)laddr));
								picode->remotePid = pid;
								picode->reqVersion = version;
								if(pthread->getMainThreadContext()->findVTEntry(raddr,sizeof(char)) == NULL)
									pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(char),pthread->getPreVersion(vTag));
						}
					}
			}
			else{
					pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)raddr));
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
			}
	}
	else{
		pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)raddr));
			pthread->addLVTEntry(raddr,sizeof(char));
		 }
   }
   else{
		pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)laddr));
   	}
 }
#else
  pthread->setREG(picode, RT, mips_lwrLE(pthread->getREG(picode, RT), (char *)raddr));
#endif
#else
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
	  pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)raddr));
   else{
	   	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
	 uint32_t laddr = cpu->elm.getRSpecAddr((RAddr)raddr);
	 picode->exePath = (laddr == ACCESSERR)?CACHE:ECACHE;
	 if(laddr == ACCESSERR){
	  VersionTag* vTag = pthread->findVTEntry(raddr,sizeof(char));
	  if(vTag){
		  int pid = pthread->iter2Pid(vTag->iteration);
		  if(pthread->isSharedData(raddr,sizeof(char)) && pid != -1){
				  GProcessor* pro = osSim->pid2GProcessor(pid);
				  if(pro){
						  ThreadContext* thread = pro->getThreadContext(pid);
						  int version = thread->getWriteVersion(raddr,sizeof(char));
						  if(version == -1){
								  return picode;
							  }
						  else{
								  laddr = pro->elm.getRSpecAddr((RAddr)raddr);
								  picode->exePath = REMOTEACCESS;
								  pthread->setVersion(raddr,sizeof(char),version);
								  pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)laddr));
								  picode->remotePid = pid;
								  picode->reqVersion = version;
								  if(pthread->getMainThreadContext(void)->findVTEntry(raddr,sizeof(char)) == NULL)
									  pthread->getMainThreadContext()->addVersionedEntry(raddr,sizeof(char),pthread->getPreVersion(vTag));
						  }
					  }
			  }
			  else{
					pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)raddr));
					VersionTag* v = pthread->getMainThreadContext()->findVTEntry(raddr,0);
					if(v &&pthread->findVTEntry(raddr,0))
						pthread->setVersion(raddr,0,pthread->getMainThreadContext()->getDataVersion(v));
					else{
						pthread->setVersion(raddr,0,0);
						pthread->getMainThreadContext()->addVersionedEntry(raddr,0,0);
					}
			}
	}
	  else{
		  pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)raddr));
			  pthread->addLVTEntry(raddr,sizeof(char));
		}
	 }
	 else{
		  pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)laddr));
	 }
   }
#else
  pthread->setREG(picode, RT, mips_lwrBE(pthread->getREG(picode, RT), (char *)raddr));
#endif 
#endif
})

M4_IN(mfhi_op,
{
  pthread->setREG(picode, RD, pthread->hi);
})

M4_IN(mflo_op,
{
  pthread->setREG(picode, RD, pthread->lo);
})

M4_IN(mthi_op,
{
    pthread->hi = pthread->getREG(picode, RS);
})

M4_IN(mtlo_op,
{
    pthread->lo = pthread->getREG(picode, RS);
})

M4_IN(mult_op,
{
    mips_mult(pthread->getREG(picode, RS), pthread->getREG(picode, RT), &(pthread->lo), &(pthread->hi));
})

M4_IN(multu_op,
{
         mips_multu((unsigned int) pthread->getREG(picode, RS), (unsigned int) pthread->getREG(picode, RT),
                &(pthread->lo), &(pthread->hi));
})

M4_IN(nop_op,
{
})

M4_IN(nor_op,
{
  pthread->setREG(picode, RD, ~(pthread->getREG(picode, RS) | pthread->getREG(picode, RT)));
})

M4_IN(or_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) | pthread->getREG(picode, RT));
})

M4_IN(ori_op,
{
  pthread->setREG(picode, RT, pthread->getREG(picode, RS) | (unsigned short) picode->immed);
})

M4_WRITE(sb_op,
{
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
	  *(char *) raddr = pthread->getREG(picode, RT);
   else{
	   	RunningProcs proc = osSim->cpus;
	  	GProcessor* cpu = proc.getCurrentCPU();
		 uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(char),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(char));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);
	 *(char *) laddr = pthread->getREG(picode, RT);
   }
#else
  *(char *) raddr = pthread->getREG(picode, RT);
#endif 
})

/* this needs to check if any other processor has written this address */
M4_WRITE(sc_op,
{
#ifdef ADDRESS_CHK
    if (raddr & 3)
        address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
if(!pthread->isSpecThread())
   	*(unsigned int *) raddr = SWAP_WORD(pthread->getREG(picode, RT));
else{
    	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
  	uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(unsigned int),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(unsigned int));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  	*(unsigned int *) laddr = SWAP_WORD(pthread->getREG(picode, RT));
}
#else
         *(unsigned int *) raddr = SWAP_WORD(pthread->getREG(picode, RT));
#endif
#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
if(!pthread->isSpecThread())
   *(int *) raddr = pthread->getREG(picode, RT);
else{
    	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
  	uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(int),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(int));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  *(int *) laddr = pthread->getREG(picode, RT);
}
#else
         *(int *) raddr = pthread->getREG(picode, RT);
#endif 
#endif
})

M4_WRITE(sdc1_op,
{
#ifdef ADDRESS_CHK
    if (raddr & 7)
        address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
{
  I(sizeof(double)==sizeof(unsigned long long));
  unsigned long long v1;
  *((double *)&v1)=pthread->getDP(picode,ICODEFT);
  v1 = SWAP_LONG(v1);
//  #ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
	  *((double *)raddr)=*((double *)&v1);
 else{
     	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
   	uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(double),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(double));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

   *((double *)laddr)=*((double *)&v1);
 }
  #else
  *((double *)&v1)=pthread->getDP(picode,ICODEFT);
  v1 = SWAP_LONG(v1);
  *((double *)raddr)=*((double *)&v1);
  #endif
} 
#else
#ifdef SESC_COW_UPDATE
 if(!pthread->isSpecThread())
	*((double *)raddr)=pthread->getDP(picode, ICODEFT);
 else{
     	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
   uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(double),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(double));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

   *((double *)laddr)=pthread->getDP(picode, ICODEFT);
 }
#else
 *((double *)raddr)=pthread->getDP(picode, ICODEFT);
#endif 
#endif
})

M4_WRITE(sdc2_op,
{
    fatal("sdc2: not yet implemented\n");
})

M4_WRITE(sdc3_op,
{
  /* fatal("sdc3: not yet implemented\n"); DO NOTHING*/
})

M4_WRITE(sh_op,
{
#ifdef ADDRESS_CHK
    if (raddr & 1)
        address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
  if(!pthread->isSpecThread())
	*(unsigned short *) raddr = SWAP_SHORT(pthread->getREG(picode, RT));
 else{
     	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
   uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(unsigned short),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(unsigned short));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

   *(unsigned short *) laddr = SWAP_SHORT(pthread->getREG(picode, RT));
 }
#else
    *(unsigned short *) raddr = SWAP_SHORT(pthread->getREG(picode, RT));
#endif
#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
 if(!pthread->isSpecThread())
	*(short *) raddr = pthread->getREG(picode, RT);
 else{
     	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
   uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(short),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(short));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

   *(short *) laddr = pthread->getREG(picode, RT);
 }
#else
    *(short *) raddr = pthread->getREG(picode, RT);
#endif
#endif
})

M4_IN(sll_op,
{
	pthread->setREG(picode, RD, pthread->getREG(picode, RT) << picode->args[SA]);
})

M4_IN(sllv_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RT) << (pthread->getREG(picode, RS) & 0x1f));
})

M4_IN(slt_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) < pthread->getREG(picode, RT));
})

M4_IN(slti_op,
{
  pthread->setREG(picode, RT, pthread->getREG(picode, RS) < picode->immed);
})

M4_IN(sltiu_op,
{
  pthread->setREG(picode, RT, (unsigned) pthread->getREG(picode, RS) < (unsigned) ((int) picode->immed));
})

M4_IN(sltu_op,
{
  pthread->setREG(picode, RD, (unsigned) pthread->getREG(picode, RS) < (unsigned) pthread->getREG(picode, RT));
})

M4_IN(sra_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RT) >> picode->args[SA]);
})

M4_IN(srav_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RT) >> (pthread->getREG(picode, RS) & 0x1f));
})

M4_IN(srl_op,
{
  pthread->setREG(picode, RD, (unsigned) pthread->getREG(picode, RT) >> picode->args[SA]);
})

M4_IN(srlv_op,
{
  pthread->setREG(picode, RD, (unsigned) pthread->getREG(picode, RT) >> (pthread->getREG(picode, RS) & 0x1f));
})

M4_IN(sub_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) - pthread->getREG(picode, RT));

#ifdef OVERFLOW_CHK
  /* need to check for overflow here */
#endif
})

M4_IN(subu_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) - pthread->getREG(picode, RT));
})

M4_WRITE(sw_op,
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
if(!pthread->isSpecThread())
   *(unsigned int *) raddr = SWAP_WORD(pthread->getREG(picode, RT));
else{
	  RunningProcs proc = osSim->cpus;
  GProcessor *cpu = proc.getCurrentCPU();
  uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(unsigned int),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(unsigned int));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  *(unsigned int *)laddr = SWAP_WORD(pthread->getREG(picode, RT));
}

#else
  *(unsigned int *) raddr = SWAP_WORD(pthread->getREG(picode, RT));
#endif
#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
  if(!pthread->isSpecThread())
	 *(int *) raddr = pthread->getREG(picode, RT);
  else{
	  	RunningProcs proc = osSim->cpus;
  	GProcessor *cpu = proc.getCurrentCPU();
	uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(int),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(int));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

	*(int *) laddr = pthread->getREG(picode, RT);
  }
#else
  *(int *) raddr = pthread->getREG(picode, RT);
#endif
#endif
})

M4_WRITE(swc1_op,
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
{
unsigned int v1;
*((float *)&v1)=pthread->getFP(picode,ICODEFT);
v1 = SWAP_WORD(v1);

//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
if(!pthread->isSpecThread())
   *((float *)raddr)=*((float *)&v1);
else{
	 RunningProcs proc = osSim->cpus;
	 GProcessor* cpu = proc.getCurrentCPU();
	  uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(float),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(float));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  *((float *)laddr)=*((float *)&v1);
}

#else
  I(sizeof(float)==sizeof(unsigned int));

  *((float *)raddr)=*((float *)&v1);
  #endif
 } 

#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
	  *(float *) raddr = pthread->getFP(picode, ICODEFT);
   else{
	   	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
	 uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(float),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(float));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

	 *(float *) laddr = pthread->getFP(picode, ICODEFT);
   }
#else
 	*(float *) raddr = pthread->getFP(picode, ICODEFT);
#endif
#endif
})

M4_WRITE(swc2_op,
{
  fatal("swc2: not yet implemented\n");
})

M4_WRITE(swc3_op,
{
  fatal("swc3: not yet implemented\n");
})

M4_WRITE(swl_op,
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
  /* write value to memory */
#ifdef LENDIAN
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
if(!pthread->isSpecThread())
   mips_swlLE(pthread->getREG(picode, RT), (char *)raddr);
else{
	   RunningProcs proc = osSim->cpus;
   GProcessor* cpu = proc.getCurrentCPU();
  uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(char),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(char));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  mips_swlLE(pthread->getREG(picode, RT), (char *)laddr);
}
#else
  mips_swlLE(pthread->getREG(picode, RT), (char *)raddr);
#endif
#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
 if(!pthread->isSpecThread())
	mips_swlBE(pthread->getREG(picode, RT), (char *)raddr);
 else{
     	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
   uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(char),picode);

	pthread->writeVersionUpdate((RAddr)raddr,sizeof(char));
	picode->dataVersion = pthread->getWriteVersion(raddr,0);

   mips_swlBE(pthread->getREG(picode, RT), (char *)laddr);
 }
#else
  mips_swlBE(pthread->getREG(picode, RT), (char *)raddr);
#endif 
#endif
})

M4_WRITE(swr_op,
{
#ifdef ADDRESS_CHK
  if (raddr & 3)
    address_exception_op(picode, pthread);
#endif
    /* write value to memory */
#ifdef LENDIAN
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
   mips_swrLE(pthread->getREG(picode, RT), (char *)raddr);
else{
	 RunningProcs proc = osSim->cpus;
 GProcessor* cpu = proc.getCurrentCPU();
  uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(char),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(char));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

  mips_swrLE(pthread->getREG(picode, RT), (char *)laddr);
}
#else
  mips_swrLE(pthread->getREG(picode, RT), (char *)raddr);
#endif
#else
//#ifdef SESC_COW
#ifdef SESC_COW_UPDATE
   if(!pthread->isSpecThread())
	  mips_swrBE(pthread->getREG(picode, RT), (char *)raddr);
   else{
	   	RunningProcs proc = osSim->cpus;
  	GProcessor* cpu = proc.getCurrentCPU();
	 uint32_t laddr = cpu->elm.getWSpecAddr((RAddr)raddr,sizeof(char),picode);

		pthread->writeVersionUpdate((RAddr)raddr,sizeof(char));
		picode->dataVersion = pthread->getWriteVersion(raddr,0);

	 mips_swrBE(pthread->getREG(picode, RT), (char *)laddr);
   }
#else
  mips_swrBE(pthread->getREG(picode, RT), (char *)raddr);
#endif
#endif
})

M4_IN(sync_op,
{
    /* fatal("sync: not yet implemented\n"); */
})

M4_IN1(syscall_op,
{
  int sysnum, addr;
  
  sysnum = pthread->getREGNUM(2);
  addr = pthread->getREGNUM(31) - 8;
  
#ifdef TASKSCALAR
  fprintf(stderr,"syscall to hell 0x%x (sysnum %d) from 0x%x\n",picode->addr, sysnum, addr);
  rsesc_exception(pthread->getPid());
#else
  if (sysnum=0x4001) /* syscall_exit */
    mint_exit(picode, pthread);
  else
    fatal("syscall %d at 0x%x, called from 0x%x, not supported yet.\n", sysnum, picode->addr, addr);
#endif
})

M4_IN(teq_op,
{
  if (pthread->getREG(picode, RS) == pthread->getREG(picode, RT)) {
#ifdef TASKSCALAR
    if(!rsesc_is_safe(pthread->getPid())) {
        rsesc_exception(pthread->getPid());
    } else
#endif
    fatal("teq: TRAP. trap handling not implemented (division by zero problem) at 0x%x\n", picode->addr);
  }
})

M4_IN(teqi_op,
{
  fatal("teqi: not yet implemented\n");
})

M4_IN(tge_op,
{
  fatal("tge: not yet implemented\n");
})

M4_IN(tgei_op,
{
  fatal("tgei: not yet implemented\n");
})

M4_IN(tgeiu_op,
{
  fatal("tgeiu: not yet implemented\n");
})

M4_IN(tgeu_op,
{
  fatal("tgeu: not yet implemented\n");
})

M4_IN(tlt_op,
{
  fatal("tlt: not yet implemented\n");
})

M4_IN(tlti_op,
{
  fatal("tlti: not yet implemented\n");
})

M4_IN(tltiu_op,
{
  fatal("tltiu: not yet implemented\n");
})

M4_IN(tltu_op,
{
  fatal("tltu: not yet implemented\n");
})

M4_IN(tne_op,
{
  fatal("tne: not yet implemented\n");
})

M4_IN(tnei_op,
{
  fatal("tnei: not yet implemented\n");
})

M4_IN(xor_op,
{
  pthread->setREG(picode, RD, pthread->getREG(picode, RS) ^ pthread->getREG(picode, RT));
})

M4_IN(xori_op,
{
  pthread->setREG(picode, RT, pthread->getREG(picode, RS) ^ (unsigned short) picode->immed);
})

M4_IN(swallow_op,
{   
  int i;
  icode_ptr ud_icode;
  printf("swallow_op: \n");
  ud_icode=picode+1;
  for(i=0; i<picode->args[RD]; i++,ud_icode++) {
    printf("parameter[%d]=%d",i,ud_icode->instr);
  }
})

M4_IN(call_op,
{
  int i;
  icode_ptr ud_icode;

  ud_icode=picode+2;
  for(i=1; i<picode->args[RD]; i++,ud_icode++) {
    //FIXME: This check against 200 is hack to catch addresses
    //that are passed in instead of registers.  So, we need 
    //some way to mark each parameter with a type, either
    //register or immediate or stack. Using a counter that increments
    //for each type, we know exactly where each item should be copied.
    
    if(ud_icode->instr > 200)
      pthread->setREGNUM(i+3, ud_icode->instr);
    else
      pthread->setREGNUM(i+3, pthread->getREGNUM(ud_icode->instr));
  }
  
  //Call ops have an extra nop that is not counted amoung the 
  //arguments that insures the delay slot has nothing harmful
  //in it, like restoration of the return register from the stack.
  pthread->setREGNUM(31, picode->addr + 4*(picode->args[RD]+1));
  
  pthread->setTarget(picode->target);
})

void rsesc_spawn_opcode(int, const int *, int);

M4_IN(spawn_op,
{
  icode_ptr ud_icode;
  int params[100];
  int i;

  printf("spawn op\n");

  ud_icode=picode+2;
  
  for(i=0; i<picode->args[RD]; i++,ud_icode++) {
    //FIXME: This check against 200 is hack to catch addresses
    //that are passed in instead of registers.  So, we need 
    //some way to mark each parameter with a type, either
    //register or immediate or stack. Using a counter that increments
    //for each type, we know exactly where each item should be copied.
    
    if(ud_icode->instr > 200)
      params[i] = ud_icode->instr;
    else
      params[i] = pthread->getREGNUM(ud_icode->instr);
  }
  
  rsesc_spawn_opcode(pthread->getPid(), params, i);
  
  pthread->setREGNUM(31, picode->addr + 8);
  
  pthread->setTarget(picode->target);
})

/* Local Variables: */
/* mode: c */
/* End: */
