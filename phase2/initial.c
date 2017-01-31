/**Phase 2 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * initial.c
 * 
 * This file contains code for the nucleus initialization.
 * -Populates the 4 new areas in ROM: (PC, $SP, STATUS).
 * -Initializes three things:
 * 		1-level two data structures
 * 		2-all nucleus maintained variables
 * 		3-all nucleus maintained semaphores
 * -Instantiate a single process (test) and puts it on a multi-level 
 * 		feedback queue.
 * -Lastly calls the scheduler.
 * */

#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/p2test.e"
#include "../e/interrupts.e"
#include "../e/exceptions.e"
#include "../e/scheduler.e"

/**-------------------------------------------------------------------*/
/**                              GLOBALS                              */
/**-------------------------------------------------------------------*/

/* this is the Multilevel processing queue - only used in the scheduler
 * We break it down into 3 subqueues
 * [0] - HIGHEST PRIORITY
 * 	    - All jobs begin here
 * 	    - To stay on this queue, we prioritize input and 
 * 		  output processes
 * 		- This implies keeping track of when a process asks for io
 * 
 * [1] - MEDIUM PRIORITY
 * 		- This queue is entered if a process runs through its quantum 
 * 		  and is not asking for io
 * 		- can be moved back to first queue once io is asked for
 * 
 * [2] - LOW PRIORITY 
 * 		- uses full quantum and does not request any io, so a user is 
 * 			most likely not using it*/
pcb_PTR MLPQ[3];

/* number of active processes */
int procCnt;

/* number of devices waiting on device */
int softBlockCnt; 

/* current running process */
pcb_PTR currentProc;

/* array of the device semaphores*/
int deviceLookup[7][8];

/*the time it takes a process to complete*/
cpu_t procTime; 

/** END GLOBALS ------------------------------------------------------*/

/**-------------------------------------------------------------------*/
/**                              HELPERS                              */
/**-------------------------------------------------------------------*/
								/* None*/
/** END HELPERS ------------------------------------------------------*/


/** The main performs the nucleus initialization, including:
 * 1. set PC tp nucleus function
 * 2. set $sp to RAMTOP
 * 3. set status reg to make interrupts, turn virtual memory off, enable
 *    local timer, and start in kernel mode.
 * 4. initPcbs()
 * 5. initSemd()
 * 6. initialize nuclues variables (Process Count, Soft-block Count,
 * 	  Ready Queue, and Current Process
 * 7. initialize the nucleus semaphores.
 * 8. instantiate a single process and place in ready queue.  
 * 9. and finally call the scheduler.
 * */
int main()
{
	/* states for os to load */
	state_PTR sysbr, pgrm, tlb, intr;
	
	/* the starting process, which is p2test*/
	pcb_PTR startProc = NULL;
	
	/* math to get RAMTOP*/
	dev_PTR d = (dev_PTR) RAMBASEADDR;
	
	/*last page of physical memory*/
	memaddr RAMTOP = (d->rambase + d->ramsize);
	
	 /* locals for looping */
	int i, j;
	
	/* BOOTSTRAPPING */
	/* set up SYSCALL/BREAK New Area */
	sysbr = (state_PTR) SYSNEW;
	sysbr->s_status = ALL_OFF;
	sysbr->s_sp = RAMTOP;
	/* to function that handles syscalls and breaks*/
	sysbr->s_pc = sysbr->s_t9 = (memaddr) sysCallHandler; 
	
	/* set up PROGRAM TRAP New Area */
	pgrm = (state_PTR) PRGMNEW;
	pgrm->s_status = ALL_OFF;
	pgrm->s_sp = RAMTOP;
	/* to the function that handles program traps */
	pgrm->s_pc = pgrm->s_t9 = (memaddr) prgTrapHandler; 
	
	/* set up TLB New Area */
	tlb = (state_PTR) TLBNEW;
	tlb->s_status = ALL_OFF;
	tlb->s_sp = RAMTOP;
	/* to the function that handles tlb */
	tlb->s_pc = tlb->s_t9 = (memaddr) tlbHandler; 
	
	/* set up INTERRUPT New Area */
	intr = (state_PTR) INTRNEW;
	intr->s_status = ALL_OFF;
	intr->s_sp = RAMTOP;
	/* to the function that handles interrupts */
	intr->s_pc = intr->s_t9 = (memaddr) ioTrapHandler; 
	
	/* create all PCBs and ASL*/
	initPcbs();
	initASL();
	
	/* initialize the globals*/
	procCnt = 0; /* no processes yet */
	softBlockCnt = 0; /* can't block a process that isn't there */
	currentProc = NULL;/* again, we got no processes man...*/
	
	
	/* fill in the MLPQ array with the different process queues */
	for(i = 0; i < 3; ++i)
	{
		MLPQ[i] = mkEmptyProcQ();
	}
	
	/* semaphores for the devices */
	for(i = 0; i < 7; ++i)/* default all semaphores to 0*/
	{
		for(j = 0; j < 8; ++j)
		{
			deviceLookup[i][j] = 0;
		}
	}
	
	/*setting up the first process */
	startProc = allocPcb();
	startProc->p_s.s_status = ALL_OFF |TE_ON | IEP_ON | MASK_ALL;
	startProc->p_s.s_sp = RAMTOP - PAGESIZE;
	startProc->p_s.s_pc = startProc->p_s.s_t9 = (memaddr)test;
	startProc->timeLeft = QUANTUM;
	
	/* we now have a new proc, so up the proc count*/
	procCnt++;
	
	/*place in the queue */
	putInQ(startProc);
	LDIT(ITAMOUNT); /* set interval timer */
	scheduler();
}
