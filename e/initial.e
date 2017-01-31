#ifndef INITIAL
#define INITIAL
/*initial.e*/

/* globals */
extern pcb_PTR MLPQ[3];
extern int procCnt; /* number of active processes */
extern int softBlockCnt; /* number of devices waiting on device */
extern pcb_PTR currentProc; /* current running process */
extern int deviceLookup[7][8]; /* array of the device semaphores*/
extern cpu_t procTime; /*the time it takes a process to complete*/
/*end globals */

extern int main();

#endif
