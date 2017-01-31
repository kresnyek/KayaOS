 /**Phase 2 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * interrupt.c
 * 
 * This file contains code for I/O Trap Handler.
 * 
 * A device interrupt occurs when:
 * 	-a previously initiated I/O request completes
 * 	-a processor local timer, or the interval timer,
 * 		makes a 0x0000.0000 => 0xFFFF.FFFF transition
 * 
 * The interrupt exception handler:
 *  -acknowledges the outstanding interrupt
 *  -performs a "V" opertation on the nucleus maintained semaphore
 * 		associated with the interrupting (sub)device
 *  -if SYS8 was called before the interrupt was handled,
 * 		store the interrupting (sub)device's status word, in the
 * 		newly unblocked processes' v0.
 * 		-Otherwise, store the interrupting device's status word until
 * 			SYS8 is requested.
 * */
 
 #include "../h/types.h"
 #include "../h/const.h"
 #include "../e/asl.e"
 #include "../e/pcb.e"
 #include "../e/exceptions.e"
 #include "../e/initial.e"

/**--------------------------------------------------------------------*/
/**                              GLOBALS                               */
/**--------------------------------------------------------------------*/
								/* NONE */
/** END GLOBALS ------------------------------------------------------*/

/**-------------------------------------------------------------------*/
/**                              HELPERS                              */
/**-------------------------------------------------------------------*/
	
	
	 void debugI()
	 {
		 int i = 0;
		 ++i;
	 }							
/** GET LINE NUMBER - helper to intrhandler
 * PARAMETERS: 
 * interrupts - This is the cleaned up interrupts integer for 
 * comparison*/
int getLineNumber(unsigned int interrupts)
{
	int i,line = -1; 
	int bits[] = {IM0_ON, IM1_ON, IM2_ON, IM3_ON, IM4_ON, IM5_ON, 
														IM6_ON, IM7_ON};
	/* checks each bit to find the one on with the highest priority */
	for(i = 0; i< 8; ++i)
	{
		if((interrupts & (bits[i])) > 0)
		{
			/* line found, the ith line*/
			line = i;
			break;
		}
	}
	
	return line;
}

/** GET DEVICE NUMBER - helper to intrHandler
 *  returns the device number of the highest priority device in the 
 * bitMap
 */
int getDevNum(unsigned int bitMap)
{
	int devNum = -1, i;
	unsigned int devices[] = {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, 
																SEVEN};
	/* determines the highest prioritized device that has an interrupt 
	 * pending
	 * */
	for(i = 0; i < 8; ++i)
	{
		if(bitMap == devices[i])
		{
			devNum = i;
			break;
		}
	}
	
	return devNum;
}

/** PROCESS TIMER INTERRUPT - helper for intrHandler
 * This function deals with the interrupts caused by the process clock
 * */
void procTimerIntr()
{
	/* if a proc is running, it's the end of their time */
	if(currentProc != NULL)
	{
		/* give it more time */
		currentProc->totalProcTime += QUANTUM;
		putInQ(currentProc);
		currentProc = NULL;
	}
	/* if none, then just call the scheduler */
	scheduler();
}

/**INTERNAL TIMER INTERRUPT - helper for intrHandler
 * This function takes a pointer to a semaphore and the start time.
 * deals with interrupts from the interval timer */
void intervalTimerIntr(int_PTR semAdd, int startTime)
{
	/*locals*/
	pcb_PTR p;
	int endTime;
	/* shhhh the interval timer*/
	LDIT(ITAMOUNT);
	/* grab the end time to charge the proc(s)*/
	STCK(endTime);
				
	if((++(*semAdd)) <= 0) /* if a proc is waiting */
	{
		p = removeBlocked(semAdd);
		/* for each proc, charge it and wake it up*/
		while(p != NULL)
		{
			--softBlockCnt;
			p->p_semAdd = NULL;
			p->timeLeft -=  endTime-startTime; /* charge em*/
			p->waitedForIO = TRUE;
			putInQ(p);
			p = removeBlocked(semAdd);
		}
	}	
	
	(*semAdd) = 0; /* set interval timer to have no one waiting */
	
	if(currentProc != NULL)
	{
		if(currentProc->timeLeft > 0)
		{
			setTIMER(currentProc->timeLeft);	
			switchContext(&(currentProc->p_s));
		}
		putInQ(currentProc);
	}
    scheduler();
}

/** TERMINAL INTERRUPT - helper to intrHandler
 * This function takes in a pointer to a device address, and a pointer
 * to a process. These are then used to deal with the interrupt. 
 * */
void terminalIntr(device_PTR addr, pcb_PTR p)
{
	/* if transmitted character to terminal */
	if(addr->t_transm_command > 1)
	{
		p->p_s.s_v0 = addr->t_transm_status;
		addr->t_transm_command = ACK;
	}
			
	else /* if character received from terminal*/
	{
		p->p_s.s_v0 = addr->t_recv_status;
		addr->t_recv_command = ACK;
	}
}
/** END HELPERS ------------------------------------------------------*/

/**I/O Trap Handler
 * This function handles the interrupts
 * Steps:
 * 1. determine line number
 * 2. determine instance number
 * 3. locate device register
 * 4. get the status
 * 		return val of proc that created the interrupt, in v0.
 * 5. acknowledge the interrupt
 * 6. calculate, or determine the sema4 for this device
 * 7. "V" the sema4
 * 8. LDST(oldInt) other option: scheduler();*/
void ioTrapHandler()
{
	/*locals*/
	int line=0, devNum=0, startTime, endTime;
	int_PTR semAdd;
	pcb_PTR p;
	device_PTR addr;
	state_PTR state = (state_PTR) INTROLD;
	unsigned int interrupts = (state->s_cause) & MASK_ALL;
	dev_PTR d = (dev_PTR) RAMBASEADDR;
	STCK(startTime);
	
	/* if not null, grab the most current state of that proc*/
	if(currentProc != NULL) 
	{
		moveState(&(currentProc->p_s), (state_PTR) INTROLD);
	}
	
	line = getLineNumber(interrupts);
	
	/* Now, we need to check which device in that array */
	if(line < 3)/* if line is a clock*/
	{
		semAdd = &(deviceLookup[0][line]);
		switch(line)
		{
			case 1: /*processer timer*/
				procTimerIntr();
			break;
			
			case 2: /* psuedo clock tick from the interval timer */
				intervalTimerIntr(semAdd, startTime);
			break;
	
			default:
				PANIC();
			break;
		}		
	}
	/* getting the device number */
	devNum = getDevNum(d->interrupt_dev[line-3]);
		
	/* the device we find at line and instance */
	semAdd = &(deviceLookup[line-2][devNum]);
	
	/* V semadd*/
	++(*semAdd);
	p = removeBlocked(semAdd);
	  
	  /*overlaying the device structure over the memory location*/
	addr = (device_PTR) (DEVREGS + ((line-3) * 128) + (devNum *16));
	
	/*if interrupt from terminal*/
	if(line == 7)
	{
			terminalIntr(addr, p);
	}
	
	else
	{
		debugI(13,p,13,13);
		/* other devices*/
		/*if(p!=NULL)
		{*/
			p->p_s.s_v0 = addr->d_status;
		/*}
		/*acknowledge the interrupt*/
		addr->d_command = ACK;
	}
		
	if(p != NULL)
	{
		p->p_semAdd = NULL;
		STCK(endTime);
		p->timeLeft -= (endTime - startTime);
		p->waitedForIO = TRUE; /* just finished waiting */
		putInQ(p);/* put back in ready q*/
		--softBlockCnt;
	}
			
	if(currentProc != NULL)
	{
		moveState(&(currentProc->p_s), (state_PTR) INTROLD);
		
		if(currentProc->timeLeft > 0)
		{
			setTIMER(currentProc->timeLeft);	
			switchContext(&(currentProc->p_s));
		}
		putInQ(currentProc);
	}
	scheduler();
}
