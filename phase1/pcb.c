/*pcb.c
 * 
 * pcb Q's are stored in a doubly linked, tail pointed (stored in the
 *  semaphore), circular queue. 
 * 
 * The free list is a linear, singly linked head pointed (pcbFree_h)
 *  stack.
 * 
 * a process tree is made of parents and child stacks (double linked, 
 *   null terminated, head pointed, linear list)
 * 
 *  PPG: Hana Chemaly, Justine Templin, Cassie Kresnye
 * */
#include "../h/const.h"
#include "../h/types.h"
#include "../e/exceptions.e"

/*headpointer to pcb Free list*/
pcb_PTR pcbFree_h;

/* debug statement. does some useless math. look at up to 4 parameters */
void debug1()
{
	int i = 0;
	++i;
}

/* ****************************************************************** */
/* PCB FREE LIST */
/* ****************************************************************** */

/*Insert the element pointed to by p into the pcbFree list.
 * This is the only function that can change pcbFree_h
 * pcbFree is a singly linked, null terminated, linear list. a stack. 
 * update: 9.21.16 ck*/
void freePcb(pcb_PTR p)
{
	
		p->p_next = pcbFree_h; /*point p to current head*/
		pcbFree_h = p; /* make p the new head */

}

/*Allocates ProcBlks. Return NULL if the pcbFree list is empty. Otherwise, remove
an element from the pcbFree list, provide initial values for ALL
of the ProcBlks fields (i.e. NULL and/or 0) and then return a
pointer to the removed element.*/
pcb_PTR allocPcb()
{
	int i =0;
	
	if(pcbFree_h == NULL) /*if the list is empty,cannot allocate any more pcb*/
	{
		return NULL;
	}
	else
	{
		pcb_PTR retVal = pcbFree_h; /* grab head */
		pcbFree_h = retVal->p_next; /*Move head to the next position*/
		
		/*Clean out the pcb*/
		retVal->p_next = NULL;
		retVal->p_prev = NULL;
		retVal->p_prnt = NULL;
		retVal->p_child = NULL;
		retVal->p_rightSib = NULL;
		retVal->p_leftSib = NULL;
		retVal->timeLeft = QUANTUM;
		retVal->totalProcTime = 0;
		retVal->waitedForIO = FALSE;
		
		for(i = 0; i < 6; ++i)
		{
			retVal->sysCallFlag[i] = NULL;
		}
		retVal->p_semAdd = NULL;

		return retVal;
	}
}
/* Initialize the pcbFree list to contain all the elements of the
static array of MAXPROC ProcBlks. This method will be called
only once during data structure initialization.*/

void initPcbs()
{
	pcbFree_h = NULL;
	static pcb_t staticArray[MAXPROC]; /* grab that static memory */
	int i;
	
	for(i = 0; i < MAXPROC; i++) /* put them into the free pcb stack */
	{
		freePcb(&staticArray[i]);
	}
}

/* SYSCALL FLAG METHODS*/

/*Getters for the flags*/

int isTLBFlag(pcb_PTR p)
{
	return(p->sysCallFlag[0] != NULL);
}

void setTLBFlag(pcb_PTR p, state_PTR old, state_PTR ne)
{
	p->sysCallFlag[0] = old;
	p->sysCallFlag[3] = ne;
}


int isPGMTrapFlag(pcb_PTR p)
{
	return(p->sysCallFlag[1] != NULL);
}

void setPGMTrapFlag(pcb_PTR p, state_PTR old, state_PTR ne)
{
	p->sysCallFlag[1] = old;
	p->sysCallFlag[4] = ne;
}

int isSYSBP(pcb_PTR p)
{
	return(p->sysCallFlag[2] != NULL);
}

void setSYSBP(pcb_PTR p, state_PTR old, state_PTR ne)
{
	p->sysCallFlag[2] = old;
	p->sysCallFlag[5] = ne;
}


/* ****************************************************************** */
/* PROC QUEUE MANAGEMENT */
/* ****************************************************************** */
/* Helpers */

pcb_PTR cleanPcb(pcb_PTR p)
{
	p->p_next = NULL;
	p->p_prev = NULL;
	return p;
}


/*  This function returns true if the pcb pointed to by p is inside of
 * the pcbQ pointed to by the tail tp.
 * else, it will return false
 * */
int isInQ(pcb_PTR tp, pcb_PTR p)
{
	pcb_PTR current = tp;
	if(tp == p)
	{
		return(TRUE);
	}
	current = current->p_next;
	
	while(current != NULL && (current != current->p_next))
	{
		
		if(current == p){
			return(TRUE);
		}	
		current = current->p_next;
	}
	return(FALSE);
}

/* ****************************************************************** */
/* This method is used to initialize a variable to be tail pointer to a
process queue.
Return a pointer to the tail of an empty process queue; i.e. NULL.
edit 89.7.16 ck */
pcb_PTR mkEmptyProcQ()
{
	return NULL;
}

/*PROC QUEUE---------------------------------------------------------------------*/

/* Return TRUE if the queue whose tail is pointed to by tp is empty.
Return FALSE otherwise. 
edit 8.30.16 ppg */

int emptyProcQ(pcb_PTR tp)
{
	/*updated 9.21.16 ck*/
	return (tp == NULL);
}


/* Insert the ProcBlk pointed to by p into the process queue whose
tail-pointer is pointed to by tp. Note the double indirection through
tp to allow for the possible updating of the tail pointer as well. */

void insertProcQ(pcb_PTR *tp, pcb_PTR p)
{
	if(emptyProcQ(*tp))/*if first element*/
	{
		/*point to self*/
		p->p_next = p;
		p->p_prev = p;
		/*move tp*/
		*tp = p;
	}

	else /* else, queue is not empty */
	{
		p->p_next = (*tp)->p_next;
		p->p_prev = *tp;
		(*tp)->p_next->p_prev = p;
		(*tp)->p_next = p;
		(*tp) = p; /* move tp to new tail */
	}
}
/*-----------------------------------------------------------------*/
/* Remove the first element from the process queue whose
tail-pointer is pointed to by tp. Return NULL if the process queue
was initially empty; otherwise return the pointer to the removed element.
Update the process queues tail pointer if necessary. */
pcb_PTR removeProcQ(pcb_PTR *tp)
{
	pcb_PTR retVal;
	
	if(emptyProcQ(*tp))/*if the process is empty */
	{
		return NULL;
	}

	retVal = (*tp)->p_next; /*grab the head*/
		
	if(retVal->p_next == retVal) /* if last pcb */
	{
		*tp = NULL;
		return cleanPcb(retVal);
	}
		
	retVal->p_next->p_prev = *tp;
	(*tp)->p_next = retVal->p_next;
	return cleanPcb(retVal);
}


/* Remove the ProBlk pointed to by p from the process queue whose tail-pointer is pointed to by tp. 
Update the porcess queue's tail pinter if necessary. If the desired entry is not in the indicated queue, 
return Null; otherwise, return p. Note that p can point to any element of the process queue.*/
pcb_PTR outProcQ(pcb_PTR *tp, pcb_PTR p)
{
	pcb_PTR tail = *tp;
	pcb_PTR retVal;
	debug1(1,1, isInQ(tail,p), 0); /* this guy is the issue!! */
	if(!emptyProcQ(tail) && (isInQ((*tp),p)))
	{
		debug1(2,0,0,0);
	
		retVal = tail->p_next;
		debug1(3,0,0,0);
		if(retVal->p_next == retVal) /* if last pcb */
		{
			debug1(4,0,0,0);
			return removeProcQ(tp);
		}
	
		if(p == tail)
		{
			debug1(5,0,0,0);
			tail = p->p_next;
		}
	
		p->p_next->p_prev = p->p_prev;
		p->p_prev->p_next = p->p_next;
		debug1(6,0,0,0);
	
		return cleanPcb(p);
	}
	return NULL;


}

/* returns a pointer to the pcb that is at the head of the procQ pointed
 *  to by tp. if tp is empter, will return null. 
 * 
 */
pcb_PTR headProcQ(pcb_PTR tp)
{
		return ((emptyProcQ(tp)) ? NULL : tp->p_next);
}

/* ****************************************************************** */
/*TREE MANAGEMENT*/
/*The portion using trees. This includes functions emptyChild, 
 * emptyParent, insertChild,
 removeChild, outChildHelper, and OutChild.
 */
 /* ****************************************************************** */
 /*Helpers*/
 
 /* A helper function for outChild. It removes the last child, and 
  * returns a ptr to it.*/
pcb_PTR cleanUpTheChild(pcb_PTR child)
{
	/*cleans up the child by setting rightSib, leftSib, and prnt
	 *  to NULL*/
	child->p_rightSib = NULL;
	child->p_leftSib = NULL;
	child->p_prnt = NULL;

	return child;
}
/* ****************************************************************** */


/* Checks to see if the pcb pointer p has a child. Return TRUE if the 
 * ProcBlk pointed to by p has no children. Return FALSE otherwise. */
int emptyChild(pcb_PTR p)
{				
	return (p->p_child == NULL);/* else return FALSE*/
}


/* Checks to see if pcb pointer p has a parent. Return TRUE if the ProcBlk pointed to 
by p has no parent. Return FALSE otherwise. This is a helper function for, outChild(bottom)*/
int emptyParent(pcb_PTR p)
{
	return (p->p_prnt == NULL); /*else return FALSE*/
}

/* returns true if this is the last subling*/
int lastSib(pcb_PTR p)
{
	return(p->p_rightSib == NULL);
}


/* Make the ProcBlk pointed to by p a child of the ProcBlk pointed
to by prnt. */
void insertChild(pcb_PTR prnt, pcb_PTR p)
{
	if((prnt == NULL) || (p == NULL)) /*if p or prnt is NULL, return*/
	{
		return;
	}
	
	p->p_prnt = prnt;
	p->p_leftSib = NULL;

	if(emptyChild(prnt))	/* if prnt has no children*/
	{
		/* initialize p*/
		p->p_rightSib = NULL; /* set p's rightSib to NULL*/
	}
	
	else /*if prnt has children already*/
	{
		/*connect p to others*/
		p->p_rightSib = prnt->p_child; /*set p's rightSib as a child of prnt*/
		/*connect others to p*/
		p->p_rightSib->p_leftSib = p;
	}
	
	prnt->p_child = p;
}


/* Make the first child of the ProcBlk pointed to by p no longer a
child of p. Return NULL if initially there were no children of p.
Otherwise, return a pointer to this removed first child ProcBlk. */
pcb_PTR removeChild(pcb_PTR p)
{
	if(emptyChild(p)) /* i have no kids :( */
	{
		return NULL;
	}
	
	pcb_PTR child = p->p_child; /* set p's child to a new pcb PTR child*/

	if(child->p_rightSib == NULL) /* if only child */
	{
		p->p_child = NULL;
	}

	else
	{
		/*cut out child*/
		child->p_rightSib->p_leftSib = NULL; /*now is first*/
		p->p_child = child->p_rightSib;
	}

	/*clean up child by setting pointers to rightSib, leftSib, and prnt NULL*/
	return cleanUpTheChild(child);  /*return the pointer to the child*/

}



/* Make the ProcBlk pointed to by p no longer the child of its parent.
If the ProcBlk pointed to by p has no parent, return NULL; otherwise,
return p. Note that the element pointed to by p need not be the first
child of its parent. */
pcb_PTR outChild(pcb_PTR p)
{
	if( (p == NULL) || emptyParent(p))/* if null or no parent*/
	{
		return NULL; 
	}
	
	if(p->p_leftSib == NULL)/*if first child*/
	{
		return removeChild(p->p_prnt);
	}

	if(p->p_rightSib == NULL) /* if last child*/
	{
		p->p_leftSib->p_rightSib = NULL; /*set p's leftSib's rightSib to NULL */
		return cleanUpTheChild(p); /* return the pointer to the child removed */
	}

	/* else somewhere in the middle */
	
	p->p_rightSib->p_leftSib = p->p_leftSib; /* set p's rightSib's leftSib equal to p's leftSib */
	p->p_leftSib->p_rightSib = p->p_rightSib; /*set p's leftSib's rightSib equal to p's rightSib*/
	
	return (cleanUpTheChild(p)); /*returns pointer p to the last child, which was removed*/
}

