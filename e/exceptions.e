#ifndef EXCEPTIONS
#define EXCEPTIONS
/*exceptions.e*/

extern void moveState(state_PTR, state_PTR);
extern void sysCallHandler();

extern void prgTrapHandler();

extern void tlbHandler();


#endif
