

/**Phase 2 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * scheduler.c
 * 
 * This file contains code for the Scheduler. It implements a multilevel
 * feedback queue scheduler, with a time slice value of 5 milliseconds.
 * Also performs dead lock detection, and defines currentProc, readyQ,
 * procCnt, and softBlockCnt.
 * 
 * -If the readyQ is empty, and the procCnt is 0, then HALT().
 * -Deadlock is defined as when, procCnt is > 0, and softBlockCnt ==0,
 * and in response, PANIC().
 * -If procCnt and softBlockCnt are > 0, then WAIT().
 * 
 * */
 
#include "../h/types.h"
#include "../h/const.h"
#include "../e/pcb.e"
#include "../e/asl.e"
#include "../e/exceptions.e"
#include "../e/initial.e"





int debugS()
{
	int i = 0;
	++i;
}
/**-------------------------------------------------------------------*/
/**                              GLOBALS                              */
/**-------------------------------------------------------------------*/
								/* NONE*/

/** END GLOBALS ------------------------------------------------------*/

/**-------------------------------------------------------------------*/
/**                              HELPERS                              */
/**-------------------------------------------------------------------*/
/** PUT IN Q - Helper to Scheduler
 * Takes the pcb_PTR parameter and places it into the correct priority 
 * queue based on totalProcTime and IO activity
 * */
void putInQ(pcb_PTR p)
{
	/* Placement Criteria
	 * 
	 *  1 - if a job is on first-fourth Quantum, this goes into [0]
	 *  2 - if a job returned from waiting for IO, put into [0]
	 *  3 - if neither 1 or 2, and job is now on 4th Quantum
	 * 		place into [1]
	 *  4 - if a job is on a Quantum greater than 2, it is a long job,
	 * 		place into [2] 
	 */
	 
	 /* placing into proc [0]*/
	 if((p->totalProcTime < QUANTUM) || (p->waitedForIO))
	 {
		 p->waitedForIO = FALSE; /* the io has finished */
		 insertProcQ(&(MLPQ[0]), p);
	 }
	 
	 /* placing into proc [1] */
	 else if((p->totalProcTime) < 4*QUANTUM)
	 {
		 insertProcQ(&(MLPQ[1]), p);
	 }
	 
	 /* placing into proc [2] */
	 else
	 {
		 insertProcQ(&(MLPQ[2]), p);
	 }
}

/** NEXT PROC - Helper to Scheduler
 * This returns the next pcb_PTR that can be run based on which pcb has
 * the highest priority.
 * */
pcb_PTR nextProc()
{
	/* what to do here!
	 * 1 -> if process in [0], it has the highest priority 
	 * 2 -> [0] is empty, so if process in [1] use that
	 * 3 -> if both [0]and [1] are empty, use [2], the lowest priority*/
	 
	 pcb_PTR p = NULL;
	 
	 /* check highest priority first */
	 if(!(emptyProcQ(MLPQ[0]))) /* if not empty */
	 {
		 p = removeProcQ(&(MLPQ[0]));
	 }
	 /* check next priority */
	 else if( !(emptyProcQ(MLPQ[1]))) /* if not empty */
	 {
		 p = removeProcQ(&(MLPQ[1]));
	 }
	 /* if both of the above are empty, then we will check the lowest
	  * priority queue */
	  else if (!(emptyProcQ(MLPQ[2])))
	  {
		  p = removeProcQ(&(MLPQ[2]));
	  }
	  
	return p;
}

/** END HELPERS ------------------------------------------------------*/




/** SCHEDULER - EXTRA CREDIT INCLUDED 
 * This function is in charge of setting up the process, as well as 
 * HALTING, WAITING, and PANICKING the system*/
void scheduler()
{
	/* grabbing the next proc */
	pcb_PTR p = nextProc();
	if(p != NULL)
	{
		currentProc = p; /* new proc is now current*/
		
		if(currentProc->timeLeft < 1)
		{
			currentProc->timeLeft = QUANTUM;
			putInQ(currentProc);
			scheduler(); /* try again */
		}
		
		else
		{
			STCK(procTime);
			/* give it the time it has */
			setTIMER(currentProc->timeLeft); 
			/* switch to the current proc */
			LDST(&(currentProc->p_s)); 
		}
	}
	
	else
	{
		currentProc = NULL;
		/* if no process, then we've done all we need to do*/
		if(procCnt == 0)
		{
			HALT(); /* done! */
		}
		
		else if(softBlockCnt == 0)/* DEAD LOCK */
		{
			debugS(9,8,7,6);
			PANIC(); /* Panic at the disco */
		}		
		else /* both greater than zero*, so wait for device */
		{ 
			/* things 
			 * 1. turn on interrupts
			 * 2. annnnnd then we wait */ 
			 setSTATUS(getSTATUS() | IEC_ON | MASK_ALL); 
			 /* for the current status, turn on interrupts as 
			  * well as allow all device interrupts*/
			 WAIT(); /* wait for interrupt */
		}
			
	}
}
