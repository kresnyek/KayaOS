/**Phase 3 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * vmlOsupport.c
 * 
 * This file contains 
 * */
/* initProc.c */
#include "../e/pcb.e"
#include "../h/types.h"
#include "../h/const.h"
#include "../e/vmIOsupport.e"

#include "/usr/local/include/umps2/umps/libumps.e"


/*------------------------Globals-------------------------------------*/
/*This is the global segment table, indexed by the procid */
pteOS_t pageTblOS;
/* 8 for kU2*/
pte_t pagetblu3;
/* This is the global structure holing info about the processes*/
uProc_t  procMeta[MAXUPROC];
int masterSem;
/* swap pool */
swap_t swamp[NUMOFSWAMP];
int swampMutex;
int currentStatus;

int deviceMutex[7][8];

/*---------------------- End Globals----------------------------------*/


void toggleInterrupts(int flag)
{
	/* grab current*/
	if(flag) /* turn off*/
	{
		currentStatus = getSTATUS();
		setSTATUS(ALL_OFF);
	}
	else /* turn on*/
	{
		setSTATUS(currentStatus);
	}
	
}

/*This function takes in parametes sp, pc, asid, and a state_PTR to
 * build a new state*/
void makeState(unsigned int sp, memaddr pc, int asid, state_PTR state)
{
		state->s_asid = (asid << 6);
		state->s_sp = sp;
		state->s_pc = state->s_t9 = pc;
}

/*This function takes a device number, which is the tape that we are 
 * reading from. It gets placed into disk 0*/
void readAndStore(int deviceNum)
{
	int moreToRead = TRUE, statusTape = READY;
	int currentPage = 0;
	int statusDisk;
	/*set up addresses*/
	device_PTR addrTape = (device_PTR) (DEVREGS + ((TAPEINT-3) * 128) + 
													((deviceNum-1)*16));
	/* this is device reg of disk 0*/
	device_PTR addrDisk = (device_PTR) (DEVREGS + ((DISKINT-3) * 128));
	
	/*while there are still blocks left to read/write*/		
	while (moreToRead  && (statusTape == READY))
	 {
		 /*safety check, make sure reading one block works, then 
		  * 												continue*/
		 toggleInterrupts(TRUE);
		 addrTape->d_data0 = (memaddr) TAPEPOOLSTART + 
											(PAGESIZE*(deviceNum-1));
		 addrTape->d_command = READBLK;
		 statusTape= SYSCALL(WAITFORIO, TAPEDEV, (deviceNum-1), 0);
		 toggleInterrupts(FALSE);
		
		 SYSCALL(PASSEREN, &(deviceMutex[1][0]),0,0);
		 toggleInterrupts(TRUE);
		 addrDisk->d_command = ((currentPage)  << 8) | SEEKCYL;
		 statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);
	     toggleInterrupts(FALSE);
	     SYSCALL(VERHOGEN, &(deviceMutex[1][0]),0,0);
	     
	     if(statusDisk == READY)
	     {
			 SYSCALL(PASSEREN, &(deviceMutex[1][0]),0,0);
			 toggleInterrupts(TRUE);
			 addrDisk->d_data0 = (memaddr) TAPEPOOLSTART + 
											(PAGESIZE*(deviceNum-1));
			 addrDisk->d_command = ((deviceNum-1) << 8)| WRITEBLK;
			 statusDisk = SYSCALL(WAITFORIO , DISKDEV, 0, 0);
			 toggleInterrupts(FALSE);
			 SYSCALL(VERHOGEN, &(deviceMutex[1][0]),0,0);
		 }
		 
		 if(addrTape->d_data1 != EOB)
		 {
			 moreToRead = FALSE;
		 }
		 ++currentPage;
	}
}

/*The professor is our PPG version of midwife*/
void professor()
{
	/* sets up an individual proc right before running */
	/* read in tape for it*/
	/* new process to set up */
	state_PTR tlbOld, tlbNew, prgmOld, prgmNew, sysOld, sysNew;
	state_t newProc;
	
	/* Figuring out who this is */
	int asid = ((getENTRYHI() & 0x00000FC0) >> 6);
	
	
	/*debug4(2,2,2,2);
	/*procMeta[asid-1].up_bckStoreAddr = 0 /* actually set this up */;
	tlbNew = &(procMeta[asid-1].unew_trap[TLBTYPE]);
	prgmNew = &(procMeta[asid-1].unew_trap[PRGMTYPE]);
	sysNew = &(procMeta[asid-1].unew_trap[SYSTYPE]);
	
	tlbNew->s_asid = asid << 6;
	tlbNew->s_sp = (SYSSTACK)-(PAGESIZE*((asid-1)*2));
	tlbNew->s_pc = tlbNew->s_t9 = (memaddr) userTLBHandler;

	prgmNew->s_asid = asid << 6;
	prgmNew->s_sp = (TLBSTACK)+(PAGESIZE*((asid-1)*2));
	prgmNew->s_pc = prgmNew->s_t9 = (memaddr) userPrgmHandler;
		
	sysNew->s_asid = asid << 6;
	sysNew->s_sp = (SYSSTACK)+(PAGESIZE*((asid-1)*2));
	sysNew->s_pc = sysNew->s_t9 = (memaddr) userSyscallHandler;
	
	tlbNew->s_status = prgmNew->s_status = sysNew->s_status = 
							ALL_OFF |TE_ON | IEP_ON | MASK_ALL | VMP_ON;
	/* Setting up old areas */
	SYSCALL(SESV, TLBTYPE, 
						&(procMeta[asid-1].uold_trap[TLBTYPE]), tlbNew);
	SYSCALL(SESV, PRGMTYPE, 
					&(procMeta[asid-1].uold_trap[PRGMTYPE]), prgmNew);
	SYSCALL(SESV, SYSTYPE, 
						&(procMeta[asid-1].uold_trap[SYSTYPE]), sysNew);
	
	readAndStore(asid);
	
	makeState(0xC0000000, 0x800000B0, asid, &newProc);
	newProc.s_status = ALL_OFF | TE_ON | VMP_ON |IEP_ON | MASK_ALL | 
																KUP_ON;
	LDST(&newProc);
}

/* This function does the following:
 * 1. sets up swamp
 * 2. sets up kSegOS pagetables (without correct values)
 * 3. sets up kU3/2 pagetables (without correct values)
 * 4. calls professor (aka midwife)
 * 5. P and then calls Sys2
 * */
void test()
{
	int i,j; /* for looping */
	state_t state;
	/* lay down segment table at well known location */
	segTbl_PTR segTable;
	/* semAdd to terminate test*/
	masterSem = 0;
	swampMutex = 1;
	
	/* setting up the swamp */
	for(i = 0; i < NUMOFSWAMP; ++i)
	{
		swamp[i].sw_asid = -1;
		swamp[i].sw_pte = NULL; 
	}
	
	/* semaphores for the devices */
	for(i = 0; i < 7; ++i)/* default all semaphores to 0*/
	{
		for(j = 0; j < 8; ++j)
		{
			deviceMutex[i][j] = 1;
		}
	}
	
	pageTblOS.header = (PTEMAGICNO << 24) | KSEGOSPTESIZE;
	for(i = 0; i< KSEGOSPTESIZE; ++i)
	{
		/* The page to frame relation is 1:1*/
		pageTblOS.pteTable[i].pte_entryHI = 
		(SEGOS << 30) |  ((0x20000 + i) << 12);
		pageTblOS.pteTable[i].pte_entryLO = 
		ALL_OFF | ((0x20000 + i) << 12)| DIRTY | VALID | GLOBAL;
	}
	
	pagetblu3.header = (PTEMAGICNO << 24) | KUSEGPTESIZE;
	for(i = 0; i < KUSEGPTESIZE; ++i)
	{
		pagetblu3.pteTable[i].pte_entryHI = 
					(SEG3 << 30) |  ((0xC0000 + i) << 12) | ((-1) << 6);
		pagetblu3.pteTable[i].pte_entryLO = ALL_OFF | DIRTY | GLOBAL;
	}
	
	for(i = 0;i < MAXUPROC; ++i)
	{
		procMeta[i].up_pte.header = (PTEMAGICNO << 24) | KUSEGPTESIZE;
		for(j = 0; j < KUSEGPTESIZE-1; ++j)
		{
			procMeta[i].up_pte.pteTable[j].pte_entryHI = 
				(SEG2 << 30) |  ((0x80000 + j) << 12) | ((i+1) << 6);
			procMeta[i].up_pte.pteTable[j].pte_entryLO= ALL_OFF | DIRTY;
			/* not valid because not setup, and not global because only
			 * certain procs are allowed to access it */
		}
		/* stack page */
		procMeta[i].up_pte.pteTable[KUSEGPTESIZE-1].pte_entryHI = 
					(SEG2 << 30) |  ((0xBFFFF << 12) | (i+1) << 6);
		
		segTable = (segTbl_PTR) (SEGTABLEBASE + ((i+1) * 0x0000000C));
		
		segTable->ksegOS = &(pageTblOS);
		segTable->kUseg2 = &(procMeta[i].up_pte);
		segTable->kUseg3 = &(pagetblu3);	
		
		procMeta[i-1].up_sem = 0;
		
		makeState((SYSSTACK - (PAGESIZE*(i*2))),(memaddr)professor,i+1, 
															&state);
		state.s_status = ALL_OFF |TE_ON | IEP_ON | MASK_ALL;
		SYSCALL(CREATEPROCESS, &state, 0,0);
	}

	for(i = 0; i<MAXUPROC; ++i)
	{
		/* wait for the other procs to finish */
		SYSCALL(PASSEREN, &masterSem,0,0);
		
	}
	
	SYSCALL(TERMINATEPROCESS,0,0,0);
}
