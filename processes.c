/* processes.c -- returns a list of N 'processes', where each of the N
   lines consists of (randomly generated):
       	arrival time, in range [0..MAXARRIVAL)
        size (amt. of memory required) in range [1..MAXSIZE]
        service time (amt. of time required) in range [1..MAXSERVICE]

   Solaris compilation: gcc processes.c -o processes
   Solaris run:         processes > pfile
	this will capture the output in a file called 'pfile', for
	use as input for the 'fit' program.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define N 50			/* number of processes */
#define MAXARRIVAL 80		/* latest arrival time */
#define MAXSIZE 30		/* maximum memory size of a process */
#define MAXSERVICE 100		/* maximum amount of service time required */
#define MAXINT 32767

int main() {
  int i, arrival, size, service;
  srand(getpid());
  for (i = 0; i < N; i++) {
    arrival = rand() % MAXARRIVAL;
    size = rand() % MAXSIZE + 1;
    service = rand() % MAXSERVICE + 1;
    printf("%d %d %d\n", arrival, size, service);
  }
}
