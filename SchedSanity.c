#include "types.h"
#include "stat.h"
#include "user.h"

#define P_TYPES 4
#define PROCS_PER_TYPE 5
#define TOTAL_SUB_PROCS (P_TYPES * PROCS_PER_TYPE)
#define L_LOOP 10000
#define S_LOOP 100

typedef struct{
	int pid;
	int wtime;
	int rtime;
	int iotime;
}times;

void s_calc(){
	int i;
	int j;
	for (i=0; i<S_LOOP ; i++){
		j=i*5;
	}
}

void l_calc(){
	int i;
	int j;
	for (i=0; i<L_LOOP ; i++){
		j=i*5;
	}
}

void s_io(){
	int i;
	for (i=0; i<S_LOOP ; i++){
		printf(1, "");
	}
}

void l_io(){
	int i;
	for (i=0; i<L_LOOP ; i++){
		printf(1, "");
	}
}

int main(void){
	int i;
	int pid=1;
	times times_arr[TOTAL_SUB_PROCS];
	for (i=0; i<TOTAL_SUB_PROCS ; i++){
		if(pid){
			pid=fork();
			if(pid){
				times_arr[i].pid=pid;
			}
			else if(i%PROCS_PER_TYPE==0 && pid==0){
				s_calc();
				exit();
			}
			else if(i%PROCS_PER_TYPE==1 && pid==0){
				l_calc();
				exit();
			}
			else if(i%PROCS_PER_TYPE==2 && pid==0){
				s_io();
				exit();
			}
			else if(i%PROCS_PER_TYPE==3 && pid==0){
				l_io();
				exit();
			}
		}
	}
	if(pid>0){
		times time_results_avgs[P_TYPES];
		int j;
		for (i=0; i<TOTAL_SUB_PROCS ; i++){
			wait2(times_arr[i].pid, &times_arr[i].wtime, 
				  &times_arr[i].rtime, &times_arr[i].iotime);
			j = i%PROCS_PER_TYPE;
			time_results_avgs[j].wtime += times_arr[i].wtime;
			time_results_avgs[j].rtime += times_arr[i].rtime;
			time_results_avgs[j].iotime += times_arr[i].iotime;
		}
		
		printf(1, "small calc:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[0].wtime / PROCS_PER_TYPE,
				 time_results_avgs[0].rtime / PROCS_PER_TYPE,
				 time_results_avgs[0].iotime / PROCS_PER_TYPE);
		printf(1, "large calc:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[1].wtime / PROCS_PER_TYPE,
				 time_results_avgs[1].rtime / PROCS_PER_TYPE,
				 time_results_avgs[1].iotime / PROCS_PER_TYPE);
		printf(1, "small io:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[2].wtime / PROCS_PER_TYPE,
				 time_results_avgs[2].rtime / PROCS_PER_TYPE,
				 time_results_avgs[2].iotime / PROCS_PER_TYPE);
		printf(1, "large io:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[3].wtime / PROCS_PER_TYPE,
				 time_results_avgs[3].rtime / PROCS_PER_TYPE,
				 time_results_avgs[3].iotime / PROCS_PER_TYPE);
	}
	exit();
}		