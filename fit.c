/* fit.c -- simulates a memory in which a memory allocation algorithm is
   employed (either first-fit, next-fit, best-fit, or worst-fit).  Takes
   as input a list of 'processes' (see the companion program, processes.c).

   Solaris compilation:   gcc fit.c -o fit
   Solaris use (typical): fit f < pfile > resultsf
	this will use the contents of a file called 'pfile' as input, and
	apply the first-fit algorithm to the processes -- output will be
	captured in the file 'resultsf'.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*****************************************************************************/

#define N 50			/* number of processes */
#define MEMSIZE 100		/* memory size */
#define TOTALTIME 100		/* total time to run */
#define HOLE -1			/* an empty area of memory */
#define NOTARRIVED 0		/* process status, before it's arrived */
#define WAITING 1		/* process status, arrived but waiting */
#define INMEMORY 2		/* process status, running in memory */
#define FINISHED 3		/* process status, finished running */
#define FALSE 0
#define TRUE 1

/*****************************************************************************/

typedef struct {		/* Process data structure (array) */
  int pid;				/* process identifier */
  int arrival;				/* time of arrival */
  int size;				/* amount of memory required */
  int service;				/* amount of service time required */
  int status;				/* process' current status */
} Process;

typedef struct memnode {	/* Memory Node data structure (linked list) */
  int pid;				/* process/hole identifier */
  int start;				/* starting address of process/hole */
  int size;				/* size of process/hole */
  struct memnode *next, *previous;	/* pointers to next/previous nodes */
} MemNode;

/*****************************************************************************/

MemNode *memory;			/* pointer to first node of memory */
MemNode *start;				/* where to start searching memory */
MemNode *last_location;			/* (next-fit) where we left off */
Process process[N];			/* array of processes */
int current_time = 0;			/* current time */
int unserviced = 0;			/* total of unserviced time */
int required_service = 0;		/* total of required service time */
char fit_type;				/* f = first fit
					   n = next fit
					   b = best fit
					   w = worst fit */
					/* algorithm to use */
MemNode *(*fit_algorithm)(Process *, MemNode *);

/*****************************************************************************/

int main(int argc, char *argv[]) {

  int i;
  MemNode *m;
  MemNode *first_fit(Process *, MemNode *);
  MemNode *next_fit(Process *, MemNode *);
  MemNode *best_fit(Process *, MemNode *);
  MemNode *worst_fit(Process *, MemNode *);
  MemNode *insert(Process *, MemNode *);
  void report(int, char *, int);
  MemNode *release(int);
  void instructions(char *);

  if (argc < 2) {
    instructions(argv[0]);
    exit(0);
  }
  fit_type = argv[1][0];
  if (! (fit_type == 'f' || 
	 fit_type == 'n' || 
	 fit_type == 'b' ||
	 fit_type == 'w'    )) {
    instructions(argv[0]);
    exit(0);
  }
  switch(fit_type) {
  case 'f': fit_algorithm = first_fit; break;
  case 'n': fit_algorithm = next_fit; break;
  case 'b': fit_algorithm = best_fit; break;
  case 'w': fit_algorithm = worst_fit; break;
  }

  memory = (MemNode *) malloc(sizeof(MemNode));		/* initialize       */
  memory->pid = HOLE;					/*  memory with one */
  memory->start = 0;					/*  big hole        */
  memory->size = MEMSIZE;
  memory->next = memory->previous = NULL;
  last_location = memory;			/* initialize for next-fit */

  for (i = 0; i < N; i++) {		/* get the N processes from stdin */
    process[i].pid = i;				/* assign a process id */
    scanf("%d %d %d", 				/* get process info */
	  &process[i].arrival, &process[i].size, &process[i].service);
    process[i].status = NOTARRIVED;		/* set process status */
    required_service += process[i].service;	/* total up service time */
  }
  printf("Finished reading\n");
  fflush(stdout);

  report(current_time, "START", -1);
						/* in each time unit */
  for (current_time = 0; current_time < TOTALTIME; current_time++) {	
    for (i = 0; i < N; i++) {
      if (process[i].arrival <= current_time 	/* check for new arrivals */
	  && process[i].status == NOTARRIVED) {
	process[i].status = WAITING;
	report(current_time, "ARRIVED", i);
      }						/* check for done processes */
      if (process[i].service == 0 && process[i].status != FINISHED) {
	process[i].status = FINISHED;
	last_location = release (i);		/* release memory */
	report(current_time, "FINISHED", i);
      }
    }
    for (i = 0; i < N; i++)			/* if waiting, try to fit */
      if (process[i].status == WAITING)	{	/*  process in memory     */
	if (fit_type == 'n')
	  start = last_location;
	else
	  start = memory;
	if ((m = fit_algorithm(&process[i], start)) != NULL) {
	  process[i].status = INMEMORY;
	  last_location = insert(&process[i], m);	/* grab memory */
	  report(current_time, "INMEMORY", i);
	}		
      }
    for (i = 0; i < N; i++)			/* give service to each */
      if (process[i].status == INMEMORY)	/*  process in memory   */
	process[i].service = process[i].service - 1;
  }

  for (i = 0; i < N; i++)			/* total up the unserviced   */
    unserviced += process[i].service;		/*  time for comparison with */
						/*  total time required      */

  report(current_time, "END", -1);
  printf("Using the ");
  switch (fit_type) {
  case 'f': printf("first fit "); break;
  case 'n': printf("next fit "); break;
  case 'b': printf("best fit "); break;
  case 'w': printf("worst fit "); break;
  }
  printf("algorithm\n");
  printf("\tTotal unserviced time is %d\n", unserviced);
  printf("\tTotal service required is %d\n", required_service);
  printf("\tPercent unserviced is %d%%\n", unserviced * 100 / required_service);
}

/*****************************************************************************/

MemNode *first_fit(Process *proc, MemNode *m) {
                                                /* find first available    */
  while (m) {					/*  hole that's big enough */
    if (m->pid == HOLE && m->size >= proc->size)
      return m;					/* successful */
    else
      m = m->next;
  }
  return NULL;					/* unsuccessful */
}

/*****************************************************************************/

MemNode *next_fit(Process *proc, MemNode *m) {		

  MemNode *current = m;
						/* find next available    */
  while (m) {					/*  hole that's big enough */
    if (m->pid == HOLE && m->size >= proc->size) {
      last_location = m;
      return m;					/* successful */
    }
    else
      m = m->next;
  }
  m = memory;					/* start over at beginning */
  while (m != current) {
    if (m->pid == HOLE && m->size >= proc->size) {
      last_location = m;
      return m;					/* successful */
    }
    else
      m = m->next;
  }
  return NULL;					/* unsuccessful */
}

/*****************************************************************************/

MemNode *best_fit(Process *proc, MemNode *m) {		

  MemNode *best = NULL;
  int least_diff = 100, diff;

  while (m) {
    if (m->pid == HOLE && m->size >= proc->size) {
      diff = m->size - proc->size;
      if (diff < least_diff) {
	least_diff = diff;
	best = m;
      }
    }
    m = m->next;
  }
  return best;
}

/*****************************************************************************/

MemNode *worst_fit(Process *proc, MemNode *m) {		

  /* here's one for you to do! */

}

/*****************************************************************************/

MemNode *insert(Process *proc, MemNode *hole) { /* put process in memory */

  MemNode *m;
						/* create a new node */
  m = (MemNode *) malloc(sizeof (MemNode));
  m->pid = proc->pid;				/* assign it the particulars */
  m->start = hole->start;			/*  of the process           */
  m->size = proc->size;				
  hole->start += proc->size;			/* adjust the hole node */
  hole->size -= proc->size;
  m->next = hole;				/* hook it up to the */
  m->previous = hole->previous;			/*  linked list      */
  hole->previous = m;				
  if (m->previous == NULL)			/* if first node, special */
    memory = m;					/*  case                  */
  else
    (m->previous)->next = m;
  return m;
}

/*****************************************************************************/

MemNode *release(int pid) {

  MemNode *m = memory, *n, *p;

  while (m->pid != pid)			/* find entry in list */
    m = m->next;

  n = m->next;				/* n is next node */
  p = m->previous;			/* p is previous node */

  if (p == NULL) {			/* if at beginning of list */
    if (n->pid == HOLE) {		/* if next entry is a hole */
      n->size += m->size;
      n->start = m->start;
      n->previous = NULL;
      memory = n;
      free(m);
      return memory;
    }
    else {				/* if next entry isn't a hole */
      m->pid = HOLE;
      return m;
    }
  }
  else if (n == NULL) {			/* if at end of list */
    if (p->pid == HOLE) {		/* if previous entry is a hole */
      p->size += m->size;
      p->next = NULL;
      free(m);
      return p;
    }
    else {				/* if previous entry isn't a hole */
      m->pid = HOLE;
      return m;
    }
  }
  else {				/* in middle somewhere */
    if (n->pid == HOLE) {
      if (p->pid == HOLE) {		/* between two holes */
	p->size = p->size + m->size + n->size;
	if (n->next != NULL)
	  (n->next)->previous = p;
	p->next = n->next;
	free(m);
	free(n);
	return p;
      }
      else {				/* next entry is a hole */
	n->size += m->size;
	n->start = m->start;
	p->next = n;
	n->previous = p;
	free(m);
	return n;
      }
    }
    else {
      if (p->pid == HOLE) {		/* previous entry is a hole */
	p->size += m->size;
	n->previous = p;
	p->next = n;
	free(m);
	return p;
      }
      else {
	m->pid = HOLE;			/* no hole on either side */
	return m;
      }
    }
  }
}

/*****************************************************************************/

void report(int time, char *message, int pid) {

  MemNode *m = memory;
  int i;

  printf("Time: %d %s", time, message);
  if (pid != -1)
    printf(":P%d", pid);
  printf("\n  Memory [PID,start,size]: ");
  while (m) {
    printf("->[");
    if (m->pid == HOLE)
      printf("H,");
    else
      printf("P%d,", m->pid);
    printf("%d,%d]", m->start, m->size);
    m = m->next;
  }
  printf("\n");
  printf("  Waiting (PID,arrival,size,t): ");
  for (i = 0; i < N; i++) 
    if (process[i].status == WAITING) 
      printf("(P%d,%d,%d,%d) ", i, process[i].arrival, process[i].size,
	     process[i].service);
  printf("\n\n");
}

/*****************************************************************************/

void instructions(char *command) {

  printf("Usage: %s fit-type\n", command);
  printf("  where fit-type is\n");
  printf("     f   for first fit\n");
  printf("     n   for next fit\n");
  printf("     b   for best fit\n");
  printf("     w   for worst fit\n");
}

