#ifndef PCB
#define PCB

/************************* PROCQ.E *****************************
*
*  The externals declaration file for the Process Control Block
*    Module.
*
*  Written by Mikeyg EDIT: PPG
*/

#include "../h/types.h"

extern void freePcb (pcb_PTR p);
extern pcb_PTR allocPcb ();
extern void initPcbs ();

extern int isTLBFlag(pcb_PTR p);
extern void setTLBFlag(pcb_PTR p, state_PTR old, state_PTR ne);
extern int isPGMTrapFlag(pcb_PTR p);
extern void setPGMTrapFlag(pcb_PTR p, state_PTR old, state_PTR ne);
extern int isSYSBP(pcb_PTR p);
extern void setSYSBP(pcb_PTR p, state_PTR old, state_PTR ne);

extern pcb_PTR mkEmptyProcQ ();
extern int emptyProcQ (pcb_PTR tp);
extern void insertProcQ (pcb_PTR *tp, pcb_PTR p);
extern pcb_PTR removeProcQ (pcb_PTR *tp);
extern pcb_PTR outProcQ (pcb_PTR *tp, pcb_PTR p);
extern pcb_PTR headProcQ (pcb_PTR tp);

extern int emptyChild (pcb_PTR p);
extern void insertChild (pcb_PTR prnt, pcb_PTR p);
extern pcb_PTR removeChild (pcb_PTR p);
extern pcb_PTR outChild (pcb_PTR p);

/***************************************************************/

#endif
