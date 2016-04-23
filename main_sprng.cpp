#if __GNUC__ > 3
 #include <string.h>
 #include <iostream>
#else
 #include <iostream.h>
#endif
#include <stdio.h>
#include <mpi.h>		/* MPI header file */

#include "sprng_cpp.h"


/* Compile with:
     mpicxx -DSPRNG_MPI -DUSE_MPI -o main_sprng \
     main_sprng.cpp -I/opt/nfs/sprng5/include -L/opt/nfs/sprng5/lib -lsprng */

/* Circle size */
#define CSIZE 16
/* Entrance size */
#define ESIZE 4
/* Simulation duration */
#define SIMSIZE 100000

/* Entrances parameters
   f - Mean time between vehicle arrivals	
   d - matrix of probabilities that car entering at i will exit at j  */

	     	   /* N W S E */
const int f[ESIZE] = {3,3,4,2};  
			   	/* N    W    S    E */
const double d[ESIZE][ESIZE] = {{0.1, 0.2, 0.5, 0.2}, /*N*/
			   	{0.2, 0.1, 0.3, 0.4},  /*W*/
			   	{0.5, 0.1, 0.1, 0.3},  /*S*/
			   	{0.3, 0.4, 0.2, 0.1}}; /*E*/

typedef enum {N,W,S,E} ExitRoute;
static const char *directions[] = {"N", "W", "S", "E"};	

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
	@arg stream - pointer to the sprng generator initialized in main function 
	@arg index - integer value of the index, representing the entrance that the car used to enter the traffic circle
	@return - integer value of the index, representing the entrance that the car will use to leave the traffic circle  */

int choose_exit(Sprng *stream, int index){
	ExitRoute er;
	double u =  stream->sprng();
	if (u < d[index][0]){
		er = N;
	} else if (u < (d[index][0] + d[index][1])) {
		er = W;
	} else if (u < (d[index][0] + d[index][1] + d[index][2])) {
		er = S;
	} else {
		er = E;
	}
	return offset[er];
} 


/*
	Main program function, performs the simulation process
*/
int main (int argc, char* argv[])
{
	int i, j, steps, numprocs, myid, streamnum, nstreams, gtype;
	double u;
	Sprng *stream;

	/* Arrays for accumulating results from all of the nodes*/
	int sum_wait_cnt[ESIZE], sum_arrival_cnt[ESIZE], sum_queue_accum[ESIZE];

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
	MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  	streamnum = myid;	
  	nstreams = numprocs;

	/* Fixed as 2 type generator */
	gtype = 2;	
	MPI_Bcast(&gtype,1,MPI_INT,0,MPI_COMM_WORLD);
 	stream = SelectType(gtype);

	initialize_arrays();
  
	stream->init_sprng(streamnum,nstreams,make_sprng_seed(),SPRNG_DEFAULT);

	/* Main simulation loop */
	while (steps++ < SIMSIZE){
		/* 1 step - new cars arrive at entrances */
		for(i=0;i<ESIZE;++i){
			u =  stream->sprng();
			if (u <= 1./(double)f[i]){
				arrival[i] = 1;
				arrival_cnt[i]++;
			} else {
				arrival[i] = 0;
			}
		}

		/* 2 step - cars inside circle advance simulataneously */
		for(i=0;i<CSIZE;++i){
			j = (i+1) % 16;
			if (circle[i] == -1 || circle[i] == j) {
				new_circle[j] = -1;
			} else {
				new_circle[j] = circle[i];
			}
		}	

		/* Replace content of circle with new_circle */
		for(i=0;i<CSIZE;++i){
			circle[i] = new_circle[i];
		}
			
		/*3 step - cars enter circle */
		for(i=0;i<ESIZE;++i){
			if (circle[offset[i]] == -1) {		/* There is a space for a car to enter */
				if(queue[i] > 0){		/* Car waiting in the queue enters the circle */
					queue[i]--;
					circle[offset[i]] = choose_exit(stream, i); 
				} else if (arrival[i] > 0) {	/* Newly arrived car enters the circle */
					arrival[i] = 0;
					circle[offset[i]] = choose_exit(stream, i); 
				}
			}

			if (arrival[i] > 0) {			/* Newlu arrived car queues up */
				wait_cnt[i]++;
				queue[i]++;
			}
		}
		
		for(i=0;i<ESIZE;++i){
			queue_accum[i] += queue[i];		
		}

	}	/* End of main loop */ 

	/* Getting accumulated results from all nodes [16] */
	MPI_Reduce(wait_cnt, sum_wait_cnt, ESIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(arrival_cnt, sum_arrival_cnt, ESIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	MPI_Reduce(queue_accum, sum_queue_accum, ESIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	/* Generate results, printed only by master (myid: 0) process */
	if(!myid) {
		double probability, queue_avg_length;
		printf("Algorithm results [executed on %d nodes]: \n", numprocs);
		printf("----------------------------------------------------- \n");	
		for(i=0;i<ESIZE;++i){
			probability = 100. * (double)sum_wait_cnt[i]/(double)sum_arrival_cnt[i];
			queue_avg_length = (double)sum_queue_accum[i]/(SIMSIZE*numprocs);		
			printf("For entrance: %s \n", directions[i]);
			printf("Total number of arrived cars: %d \n", sum_arrival_cnt[i]);
			printf("Total number of cars that had to wait: %d \n", sum_wait_cnt[i]);
			printf("Probability of waiting: %.2lf %%\n", probability);
			printf("Average queue length: %lf \n", queue_avg_length);
			printf("----------------------------------------------------- \n");
		}
	}

	stream->free_sprng();
	MPI_Finalize();
	return 0; 
}	
