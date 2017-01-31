/**Phase 2 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * exceptions.c
 * 
 * This file enumerates 8 syscalls as well as the prgTrapHandler, 
 * TlbTrapHandler, and sysCallHandler.
 * 
 * This file also includes moveState and passUpOrDie functions.
 * 
 * -Syscalls 1-8 can only be executed when in Kernel Mode.
 * -If SYS5 is called more than once
 * per exception type by any process, this should be treated as an
 * error (SYS2).
 * -The time variables are in milliseconds.
 */
 #include "../h/types.h"
 #include "../h/const.h"
 #include "../e/asl.e"
 #include "../e/pcb.e"
 #include "../e/scheduler.e"
 #include "../e/initial.e"
 #include "../e/initProc.e"
 
/**-------------------------------------------------------------------*/
/**                              GLOBALS                              */
/**-------------------------------------------------------------------*/
/** END GLOBALS ------------------------------------------------------*/

/**-------------------------------------------------------------------*/
/**                              HELPERS                              */
/**-------------------------------------------------------------------*/
		
		
void debugE()
{
	int i = 0;
	++i;
}
						
/** PASS UP OR DIE HELPER - a helper for all pass-up-or-die 
 * 													functionalities
 * With the given parameters, this function deals with the exception
 * that occured
 * */								
void passUpOrDieHelper(state_PTR oldState, int (*f)(pcb_PTR),
													state_PTR newState)
{
	if((*f)(currentProc))
	{
		 /* move current into old*/
		moveState(oldState, &(currentProc->p_s));
		switchContext(newState);
	}
	else
	{
		sys2(currentProc);
		scheduler();
	}
}
/** END HELPERS ------------------------------------------------------*/    

/** SWITCH CONTEXT
 * switch to state given in parameter */
void switchContext(state_PTR s)
{
	LDST(s);
}

/** MOVE STATE
 * Moves the state of oldState into state newState
 * */
void moveState(state_PTR newState, state_PTR oldState)
{
	int i;
	/*copying old register data into new registers*/
	newState->s_asid = oldState->s_asid;
	newState->s_cause = oldState->s_cause;
	newState->s_status = oldState->s_status;
	newState->s_pc = oldState->s_pc;
	
	for(i = 0; i < 34; ++i)/* the 35 regs*/
	{
		newState->s_reg[i] = oldState->s_reg[i];
	}
}

/**THE SYSCALL HANDLER
 * This function handles syscalls 1-8, for process in user and kernel 
 * mode. For syscalls 9 and up, a pass-up-or-die policy is used.
 * */
void sysCallHandler() {
	/* Wake up */
	int totalTime = 0;
	state_PTR oldState = (state_PTR) SYSOLD;
	
	/* updating currentProc with SYSOLD */
	unsigned int excCode = oldState->s_a0;
	int mode = (oldState->s_status & KUP_ON); /* get the old mode */
	/* Don't want to repeat this call a million times */
	oldState->s_pc = oldState->s_pc + 4;
	moveState(&(currentProc->p_s), (state_PTR) SYSOLD);
	
	if(mode)
	{
		if(excCode < 9) /* program trap */
		{
			currentProc->p_s.s_cause = currentProc->p_s.s_cause 
										& EXC_OFF;
			currentProc->p_s.s_cause = 40;
			moveState((state_PTR) PRGMOLD, &(currentProc->p_s));
			/* continue to charge time for this */
			switchContext((state_PTR) PRGMNEW);
		}
		else
		{
			debugE(4,4,4,4);
			/* continue charging time for this */
			passUpOrDie();
		}			
	}
	
	else
	{
		/* In kernel mode switch statements */
		switch(excCode)
		{
			case CREATEPROCESS:
					sys1();
			break;
				
			case TERMINATEPROCESS:
					sys2(currentProc);
					currentProc == NULL;
					scheduler();
			break;
					
			case VERHOGEN:
				sys3();
			break;
				
			case PASSEREN:
				sys4();
			break;
				
			case SESV:
				sys5();
			break;
			
			case GETCPUTIME:
				sys6();
			break;
				
			case WAITFORCLOCK:
				sys7();
			break;
			
			case WAITFORIO:
				sys8();
			break;
				
			default: /* 9 or up */
				passUpOrDie();
			break;
		}
		/* should not get here! */
	}
}

/**-----------------------SYSCALLS------------------------------------*/
/** CREATEPROCESS SYSCALL (SYS1)  *Must be in kernel mode*
 *  creates a process to be a child of the running process, and places
 *  the new process into the MLPQ
 */
int sys1()
{
	state_PTR oldState = (state_PTR) SYSOLD;
	pcb_PTR newProc;
	 
	 newProc = allocPcb();
	 if(newProc == NULL)
	 {
		 /* no pcb's to allocate, so cannot get on for new proc*/
		 oldState->s_v0 = -1;
	 }
	 else
	 { 
		 /* load the state arg into new proc*/
		moveState(&(newProc->p_s), (state_PTR)(oldState->s_a1));
		/* default to no semAdd */
		newProc->p_semAdd = NULL;
		/* give it some time to do things in*/
		newProc->timeLeft = QUANTUM;
		newProc->totalProcTime = 0;
		
		/* make a child of running proc*/
		insertChild(currentProc, newProc);
		/* pop in ready queue*/
		putInQ(newProc); 
		++procCnt;
		oldState->s_v0 = 0;
	}
	/* keep running on process's time*/
	/*moveState(&(currentProc->p_s), (state_PTR) oldState);*/
	LDST(&(currentProc->p_s));
}

/** TERMINATE PROCESS (SYS2) *Must be in Kernel Mode*
 * This causes the executing process (p) to cease to exist, and all
 * progenies of this process are termiated as well. This instruction
 * isn't completed until all progenies are deleted.
 */
void sys2(pcb_PTR p)
{	
	  /* let the kids go on vacation */
	  while(!emptyChild(p)) 
	  {
		/*... and let their kids kids go on vacation*/
	  	sys2(removeChild(p)); 
	  }
	  
	  /* if current proc, let it get away from it's parent for a little
	  * vacation on free pcb */
	  if(p == currentProc) 
	  {
		   outChild(p);
	  }
	  
	  /* else, get it out of its queue */
	  else if(p->p_semAdd == NULL)
	  {
		  if((outProcQ(&MLPQ[0],p)) == NULL)
		  {
			if((outProcQ(&MLPQ[1],p)) == NULL)
			{
				outProcQ(&MLPQ[2],p);
			}
		  }
	  }
	  
	  /* else currently blocked, but still wants to go on vacation */
	  else
	  {
		  outBlocked(p);
		  /* if blocked on a device */
		  if((p->p_semAdd >= &(deviceLookup[0][0])) && (p->p_semAdd <= &(deviceLookup[7][8])))
			{
				/* no longer waiting */
				--softBlockCnt;
			}
			else /* not on a device */
			{
				++(*(p->p_semAdd));
			}  
	  }
	   --procCnt;
	  
	   freePcb(p);
}

/** VERHOGEN "V" (SYS3) *Must be in Kernel Mode*
 * This is interprested by the nucleus to perform a V operation
 * on the given semaphore for the running process
 */
void sys3()
{
	  pcb_PTR p;
	  state_PTR oldState = (state_PTR) SYSOLD;
	  int_PTR semAdd = (int_PTR) oldState->s_a1;
	  
	  /*this is incrementing the semAdd */
	  if((++(*semAdd)) <= 0)
	  {
		  p = removeBlocked(semAdd);
		  p->p_semAdd = NULL;
		  /* place into correct queue */
		  putInQ(p); 
	  }
	  
	  /* keep running on that processors time */
   	  switchContext(&(currentProc->p_s)); /*load the old state*/
}

/** PASSEREN "P" (SYS4) *Must be in Kernel Mode*
 * This syscall is similar to SYS3, except instead of performing V
 * on the given semaphore, the P operation is perfomed on the sempahore.
 * Then a SYSCALL instruction is executed.
 */
void sys4()
{
	state_PTR oldState = (state_PTR) SYSOLD;
	int_PTR semAdd = (int_PTR) oldState->s_a1;
	int endTime;
	
	if ((--(*semAdd)) < 0) 
	{
		STCK(endTime);
		/* charge the current proc*/
		currentProc->timeLeft -= (endTime - procTime);
	
		if(insertBlocked(semAdd,currentProc))
		{
			PANIC();
		}
		
		currentProc = NULL;
		scheduler();
	}
	/* keep running on that processors time */
	switchContext(&(currentProc->p_s));
}

/** Specify_Exception_State_Vectore (SYS5) *Must be in Kernel Mode*
 * When this SYSCALL is called, three pieces of information are sent
 * to the nucleus:
 * 	1.type of excpetion for wich the vector is being established(in a1)
 *  2.The address into which the old processor state is to be stored
 * when an excpetion occurs while running this process (in a2)
 *  3. The processor state area that is to be taken as the new
 * processor state. (in a3)
 * The nuclues perfoms a passing up. If SYS5 is called more than once
 * per exception type by any process, this should be treated as an error 
 * (SYS2)
 */
void sys5()
{
	int type = currentProc->p_s.s_a1;
	state_PTR oldState = (state_PTR) currentProc->p_s.s_a2;
	state_PTR newState = (state_PTR) currentProc->p_s.s_a3;

	switch(type)
	{
		case TLBTYPE:
			/* check if already set up */
			if(isTLBFlag(currentProc)) 
			{
				sys2(currentProc); /* sorry pal...*/
				currentProc = NULL;
			}
			
			else
			{
				setTLBFlag(currentProc, oldState,  newState);
			}
			
			break;
			
		case PRGMTYPE:
			/* check if already set up */
			if(isPGMTrapFlag(currentProc))
			{
				sys2(currentProc); /* take em to a nice farm upstate*/
				currentProc = NULL;
			}
			
			else
			{
				setPGMTrapFlag(currentProc, oldState, newState);
			}
			
			break;
			
		case SYSTYPE:
			/* check if already set up */
			if(isSYSBP(currentProc))
			{
				sys2(currentProc); /* don't eat the ice cream */
				currentProc = NULL;
			}
			
			else
			{
				setSYSBP(currentProc, oldState, newState);
			}
			
			break;
	}
	
	if(currentProc != NULL)
	{
		/* keep charging that process */
		switchContext(&(currentProc->p_s));
	}
	/* nothing to charge because a sys2 was called */
	scheduler();
}

/** GET_CPU_TIME (SYS6) *Must be in Kernel Mode*
 * returns the time of day (microseconds) to the running process
 */
int sys6()
{	
	dev_PTR dev = (dev_PTR)RAMBASEADDR;
	currentProc->p_s.s_v0 = dev->todlo;
	/*keep charging that proc */
	switchContext(&(currentProc->p_s));
}

/** WAITFORCLOCK (SYS7) *Must be in Kernel Mode*
 * This syscall perfoms a P operation in the psuedo-clock timer
 * semaphore maintained by the nucleus. This semaphore is automatically
 * V'ed every 10000 milliseconds by the nucleus.
 */
void sys7()
{	
	int_PTR semAdd = &(deviceLookup[0][2]);
	int endTime;
	
	if((--(*semAdd)) < 0)
	{
		STCK(endTime);
		/*charge the current proc*/
		currentProc->totalProcTime += (endTime - procTime);
		currentProc->timeLeft -= (endTime - procTime);
		
		if(insertBlocked(semAdd, currentProc))
		{
			PANIC();
		}
		/*else the current block is NULL, 
		 * 				increment SBC and call scheduler*/
		currentProc = NULL;
		++softBlockCnt;
		scheduler();
	}
	
	switchContext(&(currentProc->p_s));
}

/** WAIT_FOR_IO (SYS8) *Must be in Kernel Mode*
 * This syscall perfoms a P operation in the semaphore that the nucleus
 * maintains for the I/O device in a1, a2, and a3.
 */
void sys8()
{
	
	state_PTR oldState = (state_PTR) SYSOLD;
	int type = oldState->s_a1;
	int deviceNum = oldState->s_a2;
	int isReadTerm = oldState->s_a3;
	int endTime;
	
	/* if greater, that is the device, if less, its on the first line*/
	deviceNum = (type > 2) ? deviceNum : type; 
	
	/* if less than 3, on line 0, else online 1-6*/
	type = (type < 3) ? 0 : (type-2 +isReadTerm);

	int_PTR mutex = &(deviceLookup[type][deviceNum]);
	/* p the semaphore */
	if ((--(*mutex)) < 0) 
	{

		STCK(endTime);
		/* charge the current proc*/
		currentProc->timeLeft -= (endTime - procTime);
		
		if(insertBlocked(mutex,currentProc))
		{
			PANIC();
		}
		
		/*else increment SBC, current proc to NULL, 
		 * 							head home and don't look back*/
		++softBlockCnt;
		currentProc = NULL;
		scheduler();
	}

	switchContext(oldState);
}

/**--------------------------EXCEPTION HANDLERS-----------------------*/

/** TLB HANDLER
 * This is where the OS wakes up if a TLB exception occurs.
 * */
void tlbHandler() {
	/* move into current proc */
	
	moveState(&(currentProc->p_s), (state_PTR) TLBOLD); 
	int asid = (currentProc->p_s.s_asid >> 6) & 0x00003F;
	moveState(&(procMeta[asid-1].uold_trap[TLBTYPE]), &(currentProc->p_s));
	passUpOrDieHelper(currentProc->sysCallFlag[0], 
							isTLBFlag, currentProc->sysCallFlag[3]);
}

/** PROGRAM TRAP HANDLER
 * This is where the OS wakes up if a program exception occurs.
 * */
void prgTrapHandler() {

	passUpOrDieHelper(currentProc->sysCallFlag[1], isPGMTrapFlag,
										currentProc->sysCallFlag[4]);
}

/** SYS/BP HANDLER
 * This is where the OS wakes up if a syscall/bp exception occurs.
 * */
void passUpOrDie()
{	int asid = (currentProc->p_s.s_asid >> 6) & 0x00003F;
	moveState(&(procMeta[asid-1].uold_trap[SYSTYPE]), &(currentProc->p_s));
	passUpOrDieHelper(currentProc->sysCallFlag[2], 
								isSYSBP,currentProc->sysCallFlag[5]);
}

/**-------------------------------------------------------------------*/



