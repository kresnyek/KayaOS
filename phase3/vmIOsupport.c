/**Phase 3 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * vmlOsupport.c
 * 
 * This file contains code for readOut, writeOut, userSyscallHandler, 
 * and userTLBhandler. 
 * 
 * */

#include "../h/const.h"
#include "../h/types.h"
#include "../e/initProc.e"

/*----------------------------Globals---------------------------------*/
int swampIndex = 0;
/*--------------------------End Globals-------------------------------*/

/*-------------------------HELPERS------------------------------------*/
/*helper function for the TLB Handler */
int pickAVictim(){
	static int frame = 0;
	frame = (frame+1) % NUMOFSWAMP;
	return frame;
}
/*------------------------END HELPERS---------------------------------*/

/*This function takes in asid, sector, and addr and reads it to disk.*/
void readOut(int asid, int sector, memaddr addr)
{
	device_PTR addrDisk = (device_PTR) (DEVREGS + ((DISKINT-3) * 128));
	
	SYSCALL(PASSEREN, &(deviceMutex[1][0]),0,0);
	toggleInterrupts(TRUE);
	addrDisk->d_command = ( (sector) << 8) | SEEKCYL;
	int statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);
	toggleInterrupts(FALSE);

	if(statusDisk == READY)
	{
		toggleInterrupts(TRUE);
		addrDisk->d_data0 = addr; 
		addrDisk->d_command = ALL_OFF |((asid-1) << 8) | READBLK;
		statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);	
		toggleInterrupts(FALSE);
	}
	else
	{
		PANIC();
	}
	SYSCALL(VERHOGEN, &(deviceMutex[1][0]),0,0);
}

/*This function takes in asid, sector, and addr and writes it to disk.*/
void writeOut(int asid, int sector, memaddr addr)
{
	device_PTR addrDisk = (device_PTR) (DEVREGS + ((DISKINT-3) * 128));
	SYSCALL(PASSEREN, &(deviceMutex[1][0]),0,0);
	
	toggleInterrupts(TRUE);
	addrDisk->d_command = ( (sector) << 8) | SEEKCYL;
	int statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);
	toggleInterrupts(FALSE);

	if(statusDisk == READY)
	{
		toggleInterrupts(TRUE);
		addrDisk->d_data0 = addr; 
		addrDisk->d_command = ALL_OFF |((asid-1) << 8) | WRITEBLK;
		statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);	
		toggleInterrupts(FALSE);
	}
	else
	{
		PANIC();
	}
	SYSCALL(VERHOGEN, &(deviceMutex[1][0]),0,0);
}

/*This function is the User TLB handler. The steps are:
	 * who?
	 * why?
	 * what page & seg?
	 * p(swamp)
	 * if 3 do other stuff
	 * else, grab a frame
	 * if occupied -> turn valid off in current page table
	 * back it up
	 * change pte
	 * nuke
	 * update swamp
	 * v(swamp)
	 * pray
	 * */
	 
 void userTLBHandler()
{
	
	 int asid = ((getENTRYHI() & 0x00000FC0) >> 6);
	 state_PTR oldState = &(procMeta[asid-1].uold_trap[TLBTYPE]);
	 memaddr address;
	 dev_PTR d = (dev_PTR) RAMBASEADDR;
	
	/*last page of physical memory*/
	 memaddr RAMTOP = (d->rambase + d->ramsize);
	 /* -2 pages for init and test, then for rest of pages in swamp */
	 memaddr swapPoolAddr =  RAMTOP - (2 * PAGESIZE) - 
												(NUMOFSWAMP * PAGESIZE);
	 /* grab old state */
	 int seg =(oldState->s_asid >> 30);
	 int pageNum = (oldState->s_asid << 2) >> 14;
	 int cause = (oldState->s_cause & EXC_ONLY) >> 2;
	 
	 
	 if(pageNum > 31)/* for stack */
	 {
		 pageNum = 31;
	 }
	 
	 if((cause != 2) && (cause != 3))
	 {
		 SYSCALL(18, 0, 0, 0);
	 }
	 /* else, continue */
	 
	 /*P the swamp sem*/
	 SYSCALL(PASSEREN,&swampMutex,0,0);
	 
	 int vic = pickAVictim();
	 address = (memaddr) swapPoolAddr + (vic * PAGESIZE);
	 /*if vic is occupied, go to its page table and make it invalid*/
	 /* if greater than 0, then the page table is currently occuied and 
	  * needs to be kicked out*/
	if (swamp[vic].sw_asid != -1) 
	{
		 toggleInterrupts(TRUE);
		 swamp[vic].sw_pte->pte_entryLO = 
						(swamp[vic].sw_pte->pte_entryLO) & ALL_INVALID;
		 TLBCLR();
		 toggleInterrupts(FALSE);
		 int oldAsid = swamp[vic].sw_asid;
		 int oldPageNum = swamp[vic].sw_pageNo;
		 /* write back to storage */
		 writeOut(oldAsid, oldPageNum, address);
	 }
	 
	 toggleInterrupts(TRUE);
	 readOut(asid, pageNum, address);
	 swamp[vic].sw_asid = asid;
 	 swamp[vic].sw_segNo = seg;
 	 swamp[vic].sw_pageNo = pageNum;
 	 swamp[vic].sw_pte = &(procMeta[asid-1].up_pte.pteTable[pageNum]);
 	 swamp[vic].sw_pte->pte_entryLO = address | DIRTY| VALID;
 	 
 	 TLBCLR();
	 toggleInterrupts(FALSE);
	 
	 SYSCALL(VERHOGEN, &swampMutex, 0, 0);
	 
	 LDST(oldState);
}



/*This function is the User syscall Handler. If a syscall gets called 
 * while in usermode, wake up here*/ 
void userSyscallHandler()
{
	/* routing everytihng to be a print call*/
	/*state_PTR oldState = (state_PTR) SYSOLD;*/
	int asid = ((getENTRYHI() & 0x00000FC0) >> 6);
	state_PTR oldState = &(procMeta[asid-1].uold_trap[SYSTYPE]);
	int totalTransmit = 0, i = 0, command, status;
	int excCode = oldState->s_a0;

	device_PTR  base;
	if(excCode == 18)
	{
		SYSCALL(VERHOGEN, (int) &masterSem, 0, 0);
		SYSCALL(TERMINATEPROCESS, 0,0,0);
	}
	else if(excCode == 17)
	{
		 oldState->s_v0 = SYSCALL(GETCPUTIME);
	}
	else if(excCode == 16)
	{
		int len = oldState->s_a2;
		char * s = oldState->s_a1;
		int ch;
		
		if((oldState->s_a1 <= (char*) ENDOS))
		{
			SYSCALL(TERMINATEPROCESS,0,0,0);
		}
		else if((len < 0 )|| (len > 128))
		{
			SYSCALL(TERMINATEPROCESS,0,0,0);
		}
		
		base = (device_PTR)(0x10000050 + 
							((PRNTINT-3) * DEVREGSIZE * DEVPERINT));
		
		for(i = 0; i < len; ++i) 
		{
			ch = s[i];
			command = PRINTCHAR;
			
			/* turn off interrupts */
			toggleInterrupts(TRUE);
			base->d_data0 = ch;
			base->d_command = command;
			/* make call to waitIO after issuing IO request*/
			status = SYSCALL(WAITFORIO, PRNTINT, (asid-1), FALSE);
			/* turn back on interrupts*/
			toggleInterrupts(FALSE);
			
			if(base->d_status != READY) 
			{
				
				/* error in printing */
				/* negate i for placing in callers v0 */
				i = i * -1;
				/* exit print loop */
				break;
			}	
			else
			{
				++totalTransmit;
			}
	   }
		procMeta[asid-1].uold_trap[SYSTYPE].s_v0 = totalTransmit;
    }
    else if(excCode == 13)
    {
		if(oldState->s_a1 < 0)
		{
			SYSCALL(18,0,0,0);
		}
		for(i=0; i < (oldState->s_a1)*10; ++i)
		{
			SYSCALL(WAITFORCLOCK,0,0,0);
		}
	}
	
	else if(excCode == 10)
	{
		base = (device_PTR)(0x10000050 + 
							(((TERMINT-3) * DEVREGSIZE * DEVPERINT)) + 
												(DEVREGSIZE*(asid-1)));
		int len = oldState->s_a2;
		char * s = oldState->s_a1;
		
		for(i = 0; i < len; ++i) 
		{
			command = s[i];
			command = (command << 8) | 2;
			
			/* turn off interrupts */
			toggleInterrupts(TRUE);
			base -> t_transm_command = command;
			/* make call to waitIO after issuing IO request*/
			status = SYSCALL(WAITFORIO, TERMINT, (asid-1), FALSE);
			/* turn back on interrupts*/
			toggleInterrupts(FALSE);
			
			if((status & 0xFF) != 5) 
			{
				/* error in printing */
				/* negate i for placing in callers v0 */
				i = i * -1;
				/* exit print loop */
				break;
			}	
			else
			{
				++totalTransmit;
			}
	   }
		procMeta[asid-1].uold_trap[SYSTYPE].s_v0 = totalTransmit;
    }
    
	LDST(&(procMeta[asid-1].uold_trap[SYSTYPE]));
}

void userPrgmHandler()
{
	SYSCALL(18,0,0,0);
}
