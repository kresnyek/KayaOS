/*ASL.c Module
 *  This file implements the ASL and a stack of free semd_t.
 *  The ASL is a linear, singly linked, head pointed (ASL_h) sorted list.
 *  The freelist is a singly linked, head pointed ( semdFree_h), linear 
 *  (stack) list.
 *  
 *   BY: Hana Chemaly, Justine Templin, Cassie Kresnye
 */
#include "../h/const.h"
#include "../h/types.h"
#include "../e/pcb.e"

/* semaphore descriptor type */
typedef struct semd_t {
	struct semd_t *s_next; /* next element on the ASL */
	int 		  *s_semAdd; /* pointer to the semaphore */
	pcb_t 		  *s_procQ; /* tail pointer to a process queue */
} semd_t, *semd_PTR;


/*semaphore free list*/
semd_PTR semdFree_h;
/* active semaphore list*/
semd_PTR ASL_h;
/* this is a debug function. Send in up to 4 parameters to see in
 * regs[a0-3].
 * */
void debug2()
{
	int i = 0;
	++i;
}

/* This method pops the semdFree list (a stack).
* Returns NULL if the semdFree list is empty,
* otherwise it returns a pointer to the new semAdd.
*/
semd_PTR popFree(int *semAdd)
{
	/* if the stack is empty*/
	if(semdFree_h == NULL)
	{
		return NULL;
	}

	/* else grab one*/
	semd_PTR retVal = semdFree_h;
	semdFree_h = semdFree_h->s_next;
	retVal->s_next = NULL;
	retVal->s_procQ = mkEmptyProcQ();
	retVal->s_semAdd = semAdd;

	return retVal;
}

/* This method pushes to the semdFree list (a stack).
*/
void pushFree(semd_PTR s)
{
	s->s_next = semdFree_h;
	semdFree_h = s;
}
/* ****************************************************************** */
/* ASL MAINTENANCE */
/* ****************************************************************** */
/* Helpers*/
/*This method finds the parent of the semAdd pointer given.
* Returns a pointer to the parent if found, otherwise, the
* parent doesn't exist and the method returns the last semd (which 
* will have a dummy as it's next, so can be tested for legitimacy).
*/
semd_PTR findParent(int_PTR semAdd)
{
	/*start search at head of ASL*/
	semd_PTR current = ASL_h; 

	/* while not at end of ASL and the next semAdd is less than the target*/
	while((current->s_next->s_next != NULL) && ((current->s_next->s_semAdd)< semAdd))
	{
		/*else, keep looking*/
		current = current->s_next;
	}

	return current;
}

pcb_PTR roHelper(pcb_PTR (*f)(pcb_PTR*, pcb_PTR), int* semAdd, pcb_PTR p)
{
	/*local variables*/
	semd_PTR prnt, child;
	pcb_PTR retVal;
	
	prnt = findParent(semAdd);
	child = prnt->s_next;
	

	/* if found, get pcb*/
	debug2(0, semAdd, child->s_semAdd, prnt);
	if(child->s_semAdd == semAdd)
	{
		retVal = (*f)(&(child->s_procQ), p);
		if(emptyProcQ(child->s_procQ))
		{
			/*point around child*/
			prnt->s_next = child->s_next;
			pushFree(child);
		}
		return retVal;
	}
	debug2(0,0,0,0);
	return NULL;

}
/* ****************************************************************** */



/* Insert the ProcBlk pointed to by p at the tail of the process queue
* associated with the semaphore whose physical address is semAdd
* and set the semaphore address of p to semAdd. If the semaphore is
* currently not active, allocates a new descriptor from the semdFree list,
* insert it in the ASL, initialize all of the fields and proceed as
* above.
* If a new semaphore is needed, and there is no more on the free list, 
* return TRUE, else returns FALSE.
*/
int insertBlocked(int_PTR semAdd, pcb_PTR p)
{
	/*local variables*/
	semd_PTR prnt;
	semd_PTR child;

	prnt = findParent(semAdd);

	/*If the child is found*/
	if(prnt->s_next->s_semAdd == semAdd)
	{
		p->p_semAdd = semAdd;
		insertProcQ(&(prnt->s_next->s_procQ), p);
		return FALSE;
	}

	/*If spot is found*/
	child = popFree(semAdd);

	/* if no semaphores are free to be used.*/
	if(child == NULL)
	{
		return TRUE;
	}

	/* else insert as usual*/
	child->s_next = prnt->s_next;
	prnt->s_next = child;
	p->p_semAdd = semAdd;
	insertProcQ(&(child->s_procQ), p);
	return FALSE;
}

/*Search the ASL for a descriptor of this semaphore. If none is
* found, return NULL; otherwise, remove the first (i.e. head) ProcBlk
* from the process queue of the found semaphore descriptor and return
* a pointer to it. If the process queue for this semaphore becomes
* empty (emptyProcQ(s procq) is TRUE), remove the semaphore
* descriptor from the ASL and return it to the semdFree list. */
pcb_PTR removeBlocked(int *semAdd)
{
	return roHelper(removeProcQ, semAdd, NULL);
}

/* Remove the ProcBlk pointed to by p from the process queue associated
* with p’s semaphore (p→ p semAdd) on the ASL. If ProcBlk
* pointed to by p does not appear in the process queue associated with
* p’s semaphore, which is an error condition, return NULL; otherwise,
* return p.
*/
pcb_PTR outBlocked(pcb_PTR p)
{
	return roHelper(outProcQ, p->p_semAdd, p);
} 

/* Return a pointer to the ProcBlk that is at the head of the process
*queue associated with the semaphore semAdd. Return NULL
*if semAdd is not found on the ASL or if the process queue associated
*with semAdd is empty.
*/
pcb_PTR headBlocked(int *semAdd)
{
	/*local variable*/
	semd_PTR prnt;

	prnt = findParent(semAdd);

	/* if the child's semAdd == semAdd*/
	if(prnt->s_next->s_semAdd == semAdd)
	{
		return headProcQ(prnt->s_next->s_procQ);
	}

	return NULL;
}

/* Initialize the semdFree list to contain all the elements of the array
*static semd t semdTable[MAXPROC]
*This method will be only called once during data structure initialization.
*/
void initASL()
{
	int i;
	/*initializing the ASL (queue)*/
	static semd_t ASLTable[MAXPROC+2];

	/*setting 2 dummy nodes*/
	semd_PTR front = &(ASLTable[MAXPROC]);
	semd_PTR end = &(ASLTable[MAXPROC+1]);

	/*tying dummy nodes together*/
	/*setting up the head dummy node's properties*/
	front->s_semAdd = 0;
	front->s_procQ = mkEmptyProcQ();
	front->s_next = end;

	/*setting up the tail dummy node's properties*/
	end->s_semAdd = MAXINT;
	end->s_procQ = mkEmptyProcQ();
	end->s_next = NULL;

	ASL_h = front;
	semdFree_h = NULL;

	for(i = 0; i < MAXPROC; i++)
	{
		pushFree(&(ASLTable[i]));
	}
}
