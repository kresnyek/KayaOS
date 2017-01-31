/**Phase 3 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * types.h
 * 
 * In addition to Phase 1 and Phase 2 data structure, this file contains
 * data structures for Phase3, including: Page Table, Segment Table, 
 * and the Swamp Pool (We renamed it Swamp from Swap)
 * 
 * */
#ifndef TYPES
#define TYPES
#include "const.h"

/********************************************************************** 
 *
 * This header file contains utility types definitions.
 * 
 **********************************************************************/

typedef int* int_PTR;
typedef signed int cpu_t;
typedef unsigned int memaddr;

typedef struct {
	unsigned int d_status;
	unsigned int d_command;
	unsigned int d_data0;
	unsigned int d_data1;
} device_t, *device_PTR;

#define t_recv_status		d_status
#define t_recv_command		d_command
#define t_transm_status		d_data0
#define t_transm_command	d_data1

#define DEVINTNUM 5
#define DEVPERINT 8
typedef struct {
	unsigned int rambase;
	unsigned int ramsize;
	unsigned int execbase;
	unsigned int execsize;
	unsigned int bootbase;
	unsigned int bootsize;
	unsigned int todhi;
	unsigned int todlo;
	unsigned int intervaltimer;
	unsigned int timescale;
	unsigned int inst_dev[DEVINTNUM];
	unsigned int interrupt_dev[DEVINTNUM];
	device_t   devreg[DEVINTNUM * DEVPERINT];
} devregarea_t, *dev_PTR;

#define STATEREGNUM	31
typedef struct state_t {
	unsigned int	s_asid;
	unsigned int	s_cause;
	unsigned int	s_status;
	unsigned int 	s_pc;
	int	 			s_reg[STATEREGNUM];

} state_t, *state_PTR;

#define	s_at	s_reg[0]
#define	s_v0	s_reg[1]
#define s_v1	s_reg[2]
#define s_a0	s_reg[3]
#define s_a1	s_reg[4]
#define s_a2	s_reg[5]
#define s_a3	s_reg[6]
#define s_t0	s_reg[7]
#define s_t1	s_reg[8]
#define s_t2	s_reg[9]
#define s_t3	s_reg[10]
#define s_t4	s_reg[11]
#define s_t5	s_reg[12]
#define s_t6	s_reg[13]
#define s_t7	s_reg[14]
#define s_s0	s_reg[15]
#define s_s1	s_reg[16]
#define s_s2	s_reg[17]
#define s_s3	s_reg[18]
#define s_s4	s_reg[19]
#define s_s5	s_reg[20]
#define s_s6	s_reg[21]
#define s_s7	s_reg[22]
#define s_t8	s_reg[23]
#define s_t9	s_reg[24]
#define s_gp	s_reg[25]
#define s_sp	s_reg[26]
#define s_fp	s_reg[27]
#define s_ra	s_reg[28]
#define s_HI	s_reg[29]
#define s_LO	s_reg[30]

typedef struct pcb_t{
	/* Process queue fields*/
	struct pcb_t	*p_next, /* pointer to next entity */
					*p_prev, /* pointer to previous entity */

					*p_prnt, /* pointer to parent */
					*p_child, /* pointer to 1st child */
					/* these are divided to allow the child queue to 
					 * be doubly linked */
					*p_rightSib, /* pointer to next sibling */
					*p_leftSib; /* pointer to previous sibling */
	state_PTR 		sysCallFlag[6];/* for use in syscall5, flags for 
														repeated use*/
	/* 0 - TLB old
	 * 1 - PRGMTRAP old
	 * 2 - SYS/BP old
	 * 3 - TLB new
	 * 4 - PRGMTRP new
	 * 5 - SYS/BP new */
	
	state_t			p_s;  /*processor state*/
	int 			*p_semAdd; /* pointer to sema4 on which process 
															block */
	
	/* for the scheduler*/
	cpu_t           totalProcTime; /* total time of the process */
	cpu_t           timeLeft; /* time left in the quantum*/  
	int             waitedForIO; /* this is set to true if a proc was 
								just waiting for IO */
} pcb_t, *pcb_PTR;


/* Phase 3------------------------------------------------------------*/
/****************************our stuff*********************************/

/*Phase 3 Structure, KsegOS page table
 * 64 enteries*/
 
typedef struct pteEntry_t {
     unsigned int    pte_entryHI;
     unsigned int    pte_entryLO;
} pteEntry_t, *pteEntry_PTR;


typedef struct pte_t {
     int         header;
     pteEntry_t    pteTable[KUSEGPTESIZE];
} pte_t, *pte_PTR;


typedef struct pteOS_t {
     int         header;
     pteEntry_t    pteTable[KSEGOSPTESIZE];
} pteOS_t, *pteOS_PTR;


typedef struct segTbl_t {
     pteOS_PTR         ksegOS;
     pte_PTR           kUseg2;
     pte_PTR           kUseg3;
} segTbl_t, *segTbl_PTR;

typedef struct segTbl
{
	segTbl_t entries[SEGTABLENUM];
} segTbl;


/* Uprocess information structure type */
typedef struct uProc_t {
     int            up_sem;             /* private semaphore */   
     pte_t          up_pte;             /* page table (kUseg2)*/   

     int            up_bckStoreAddr;  /* sector # for seg2 drum area*/

     state_t        uold_trap[TRAPTYPES];/*save areas for old states*/

     state_t        unew_trap[TRAPTYPES];/*new state for trap handling*/

} uProc_t, *uProc_PTR;


/* Page swap pool information structure type */
typedef struct swap_t {
     int            sw_asid;        /* ASID number*/
     int            sw_segNo;        /* page's virt seg. no*/
     int            sw_pageNo;        /* page's virt page no.*/
     pteEntry_PTR   sw_pte;        /* page's PTE entry.*/
} swap_t;

#endif
