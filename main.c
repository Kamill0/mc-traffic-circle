#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/* Circle size */
#define CSIZE 16
/* Entrance size */
#define ESIZE 4
/* Simulation duration */
#define SIMSIZE 10000

/* Entrances parameters
   f - Mean time between vehicle arrivals	
   d - matrix of probabilities that car entering at i will exit at j  */

	     /* N W S E */
const int f[ESIZE] = {3,3,4,2};  
			   /* N    W    S    E */
const double d[ESIZE][ESIZE] = { {0.1, 0.2, 0.5, 0.2},  /*N*/
			   {0.2, 0.1, 0.3, 0.4},  /*W*/
			   {0.5, 0.1, 0.1, 0.3},  /*S*/
			   {0.3, 0.4, 0.2, 0.1}};  /*E*/	

/* Data stuctures representing the traffic circle:
   circle - current state of traffic circle
   new-circle - next state of traffic circle  */

int circle[CSIZE], new_circle[CSIZE];

/* Data structures representing the four entrances:
   offset - each entrance's location (index) in traffic circle
   arrival - value 1 if car arrived in this time step
   wait_cnt - number of cars that have had to wait
   arrival_cnt - total number of cars that have arrived
   queue - number of cars waiting to enter circle
   queue_accum - accumulated queue size over all time steps */

int offset[ESIZE] = {0,4,8,12}, arrival[ESIZE], wait_cnt[ESIZE], arrival_cnt[ESIZE], queue[ESIZE], queue_accum[ESIZE];

/*	function initialize_arrays - fills up arrays with starting values */
void initialize_arrays(){
	int i;
	for(i=0;i<CSIZE;++i){
		circle[i] = new_circle[i] = -1;
	}

	for(i=0;i<ESIZE;++i){
		arrival[i] = wait_cnt[i] = arrival_cnt[i] = queue[i] = queue_accum[i] = 0;
	}
}
 
/*	function choose_exit - determines on which of the four entrances (N/S/W/E) car will leave the traffic circle 
	@arg index - integer value of the index, representing the entrance that the car used to enter the traffic circle
	@return - integer value of the index, representing the entrance that the car will use to leave the traffic circle  */
int choose_exit(int index){

}


/*
	Main program function, performs the simulation process
*/
int main (int argc, char* argv[])
{
	int i, j, steps, numprocs, myid;
	double u;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);
  
	/* Main simulation loop */
	while (steps++ < SIMSIZE){
		/* 1 step - new cars arrive at entrances */
		for(i=0;i<ESIZE;++i){
			u =  (double)rand() / (double)RAND_MAX;
			if (u <= 1./f[i]){
				arrival[i] = 1;
				arrival_cnt[i] += 1;
			} else {
				arrival[i] = 0;
			}
		}

		/* 2 step - cars inside circle advance simulataneously */
		for(i=0;i<CSIZE;++i){
			j = (i+1) % 16;
			if (circle[i] == 1 || circle[i] == j) {
				new_circle[j] = -1;
			} else {
				new_circle[j] = circle[i];
			}
		}	
		circle = new_circle;
			
		/*3 step - cars enter circle */
		for(i=0;i<ESIZE;++i){
			if (circle[offset[i]] == -1) {		/* There is a space for a car to enter */
				if(queue[i] > 0){		/* Car waiting in the queue enters the circle */
					queue[i] -= 1;
					circle[offset[i]] = choose_exit(i); //TODO
				} else if (arrival[i] > 0) {	/* Newly arrived car enters the circle */
					arrival[i] = 0;
					circle[offset[i]] = choose_exit(i) //TODO
				}
			}

			if (arrival[i] > 0) {			/* Newlu arrived car queues up */
				wait_cnt[i] += 1;
				queue[i] += 1;
			}
		}
		
		for(i=0;i<CSIZE;++i){
			queue_accum[i] = queue_accum[i] + queue[i];		
		}
		
	}	/* End of iteration */ 

	MPI_Finalize();
	return 0; 
}	
