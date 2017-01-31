/**Phase 3 OS Project Fall 2016
 * Justine Templin, Cassie Kresnye, Hana Chemaly
 * types.h
 * 
 * In addition to Phase 1 and Phase 2 constants, this file contains
 * constants for Phase3.
 * */
#ifndef CONSTS
#define CONSTS

/*********************************************************************** 
 *
 * This header file contains utility constants & macro definitions.
 * 
 **********************************************************************/

/* Hardware & software constants */
#define PAGESIZE		4096	/* page size in bytes */
#define WORDLEN			4		/* word size in bytes */
#define PTEMAGICNO		0x2A

#define ROMPAGESTART	0x20000000	 /* ROM Reserved Page */
#define OSFRAMES        30
/*bottom of last page of os */
#define ENDOS 	(ROMPAGESTART + (64 * PAGESIZE))
#define MAXUPROC 1


#define TAPEPOOLSTART (ROMPAGESTART + ((OSFRAMES-DEVPERINT) * PAGESIZE))
#define DISKPOOLSTART (TAPEPOOLSTART - (DEVPERINT * PAGESIZE))
#define STACKPOOLSTART DISKPOOLSTART

/* Mem areas for new processor states*/
#define SYSNEW  0x200003D4
#define PRGMNEW 0x200002BC
#define TLBNEW  0x200001A4
#define INTRNEW 0x2000008C

/* mem areas for old processor states */
#define SYSOLD  0x20000348
#define PRGMOLD 0x20000230
#define TLBOLD  0x20000118
#define INTROLD 0x20000000


/* timer, timescale, TOD-LO and other bus regs */
#define RAMBASEADDR	    0x10000000
#define TODLOADDR	    0x1000001C
#define INTERVALTMR	    0x10000020	
#define TIMESCALEADDR	0x10000024


/* utility constants */
#define	TRUE		1
#define	FALSE		0
#define ON              1
#define OFF             0
#define HIDDEN		static
#define EOS		'\0'

#define NULL ((void *)0xFFFFFFFF)


/* vectors number and type */
#define VECTSNUM	4

#define TLBTRAP		0
#define PROGTRAP	1
#define SYSTRAP		2

#define TRAPTYPES	3


/* device interrupts */
#define DISKINT		3
#define TAPEINT 	4
#define NETWINT 	5
#define PRNTINT 	6
#define TERMINT		7

#define DEVREGLEN	4 /* device register field length in bytes 
													& regs per dev */
#define DEVREGSIZE	16 	/* device register size in bytes */

/* device register field number for non-terminal devices */
#define STATUS		0
#define COMMAND		1
#define DATA0		2
#define DATA1		3

/* device register field number for terminal devices */
#define RECVSTATUS      0
#define RECVCOMMAND     1
#define TRANSTATUS      2
#define TRANCOMMAND     3


/* device common STATUS codes */
#define UNINSTALLED	0
#define READY		1
#define BUSY		3

/* device common COMMAND codes */
#define RESET		0
#define ACK		1

/* operations */
#define	MIN(A,B)	((A) < (B) ? A : B)
#define MAX(A,B)	((A) < (B) ? B : A)
#define	ALIGNED(A)	(((unsigned)A & 0x3) == 0)

/* Useful operations */
#define STCK(T) ((T) = ((* ((cpu_t *) TODLOADDR)) / (* ((cpu_t *) TIMESCALEADDR))))
#define LDIT(T)	((* ((cpu_t *) INTERVALTMR)) = (T) * (* ((cpu_t *) TIMESCALEADDR))) 

#define MAXPROC 20

/* switches */

#define ALL_OFF 0x00000000
#define ALL_ON 0x11111111
/* 1111 1111 1111 1111 1111 1111 1000 0011*/
#define EXC_OFF 0xFFFFFF83


#define IEC_ON 0x00000001 /* interrupts enabled current*/
#define KUC_ON 0x00000002 /* Usermode Enabled current */
#define IEP_ON 0x00000004 /* interrupts enabled previous*/
#define KUP_ON 0x00000008 /* Usermode Enabled previous */
#define IEO_ON 0x00000010 /* interrupts enabled old*/
#define KUO_ON 0x00000020 /* Usermode enabled old*/

#define IM0_ON 0x00000100 /* Interrupt Mask allows the 0 line*/
#define IM1_ON 0x00000200 /* Interrupt Mask allows the 1 line*/
#define IM2_ON 0x00000400 /* Interrupt Mask allows the 2 line*/
#define IM3_ON 0x00000800 /* Interrupt Mask allows the 3 line*/
#define IM4_ON 0x00001000 /* Interrupt Mask allows the 4 line*/
#define IM5_ON 0x00002000 /* Interrupt Mask allows the 5 line*/
#define IM6_ON 0x00004000 /* Interrupt Mask allows the 6 line*/
#define IM7_ON 0x00008000 /* Interrupt Mask allows the 7 line*/

#define MASK_ALL 0x0000FF00 /* and INTR_ONLY*/

#define BEV_ON 0x00400000 /* Bootstrap Exception Vector enabled */
#define VMC_ON 0x01000000 /* Virtual Memory enabled current */
#define VMP_ON 0x02000000 /* Virtual Memory enabled previous */
#define VM0_ON 0x04000000 /* Virtual Memory enabled old */
#define TE_ON  0x08000000 /* Local Timer Enabled */

#define CU0_ON 0x10000000 /* coprocessesor 0 usability enabled */
#define CU1_ON 0x20000000 /* coprocessesor 1 usability enabled */
#define CU2_ON 0x40000000 /* coprocessesor 2 usability enabled */
#define CU3_ON 0x80000000 /* coprocessesor 3 usability enabled */

#define EXC_ONLY 0x0000003C
/* 0000 0000 0000 0000 1111 1111 0000 0000 */

/* #define MAXINT 0x7FFFFFF*/
#define MAXINT 2147483647 

#define CREATEPROCESS    1
#define TERMINATEPROCESS 2
#define VERHOGEN         3
#define PASSEREN         4
#define SESV             5 /* Specify Exception State Vector */
#define GETCPUTIME       6
#define WAITFORCLOCK     7
#define WAITFORIO        8 /* waiting for an device */

#define GETTIME          6


/* types of devices */
#define INTPROCINTR 0
#define PROCLT      1
#define INTTIMER    2
#define DISKDEV     3
#define TAPEDEV     4
#define NETDEV      5
#define PRINTDEV    6
#define TERMDEV     7

/* for sys5, the different types of exceptions */
#define TLBTYPE  0
#define PRGMTYPE 1
#define SYSTYPE  2

#define ZERO  0x00000001
#define ONE   0x00000002
#define TWO   0x00000004
#define THREE 0x00000008
#define FOUR  0x00000010
#define FIVE  0x00000020
#define SIX   0x00000040
#define SEVEN 0x00000080

#define DEVREGS 0x10000050
#define RIEXC    0x00000028 /* reserved instruction exception*/



/* define BOOTTOP */
#define BOOTTOP 0x20000000
#define ROMTOP 0x10000000
#define DEVTOP 0x1FC00000
#define QUANTUM 5000
#define ITAMOUNT 100000 /* amount to set interval timer to */

#define SEEKCYL 2
#define READBLK 3
#define WRITEBLK 4

/* phase 3 stuff*/
#define KUSEGPTESIZE 32
#define KSEGOSPTESIZE 64
#define TRAPTYPES 3

#define SEGTABLENUM 64
#define PAGETABLENUM 8
#define SEGTABLEBASE 0x20000500
#define NUMOFSWAMP   10

#define EOB  2
#define EOT  0
#define SEGOS 0
#define SEG2  2
#define SEG3  3

#define PRINTCHAR 2

/* stacks */

#define SYSSTACK DISKPOOLSTART
#define TLBSTACK (SYSSTACK - PAGESIZE)

#define ALL_INVALID 0xFFFFFCFF
#define GLOBAL 0x100
#define VALID  0x200
#define DIRTY  0x400
/* 0011 1111 1111 1111 1111 0000 0000 0000*/
#define VPN_ONLY 0x3FFFF000

#endif
