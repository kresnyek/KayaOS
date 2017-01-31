#ifndef SCHEDULER
#define SCHEDULER

/*scheduler.e*/
extern void scheduler();
extern void chargeTime(int start, pcb_PTR p);
/*extern void theRedPhone();*/
extern void putInQ(pcb_PTR p);

#endif
