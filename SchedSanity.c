#include "types.h"
#include "stat.h"
#include "user.h"

#define P_TYPES 4
#define PROCS_PER_TYPE 4
#define TOTAL_SUB_PROCS (P_TYPES * PROCS_PER_TYPE)
#define L_LOOP 50000
#define M_LOOP 1000

typedef struct{
	int pid;
	int wtime;
	int rtime;
	int iotime;
}times;

void m_calc(){
	int i;
	for (i=0; i<M_LOOP ; i++);
}

void l_calc(){
	int i;
	for (i=0; i<L_LOOP ; i++);
}

void m_io(){
	int i;
	for (i=0; i<M_LOOP ; i++){
		printf(1, "a");
	}
}

void l_io(){
	int i;
	for (i=0; i<L_LOOP ; i++){
		printf(1, "b");
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
			else {
				#ifdef CFSD
				set_priority((i%3)+1);
				#endif
				switch (i % P_TYPES) {
					case 0:
						m_calc();
						exit();
					case 1:
						l_calc();
						exit();
					case 2:
						m_io();
						exit();
					case 3:
						l_io();
						exit();
					default:
						break;
				}
			}
		}
	}
	if(pid){
		times time_results_avgs[P_TYPES];
		int j;
		for (i=0; i<TOTAL_SUB_PROCS ; i++){
			int waitret = wait2(times_arr[i].pid, &times_arr[i].wtime,
				  &times_arr[i].rtime, &times_arr[i].iotime);
			if (waitret < 0) {
				printf(2, "wait2 error\n");
				exit();
			}
		}
		printf(1, "\n");
		for (i=0; i<TOTAL_SUB_PROCS ; i++){
			j = i % P_TYPES;
			time_results_avgs[j].wtime += times_arr[i].wtime;
			time_results_avgs[j].rtime += times_arr[i].rtime;
			time_results_avgs[j].iotime += times_arr[i].iotime;

			printf(1, "%d: w: %d r: %d io: %d\n",
			i, times_arr[i].wtime, times_arr[i].rtime, times_arr[i].iotime);
		}
		printf(1, "\n");
		printf(1, "medium calc:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[0].wtime / PROCS_PER_TYPE,
				 time_results_avgs[0].rtime / PROCS_PER_TYPE,
				 time_results_avgs[0].iotime / PROCS_PER_TYPE);
		printf(1, "large calc:\n w: %d\n r: %d\n io: %d\n",
			   time_results_avgs[1].wtime / PROCS_PER_TYPE,
				 time_results_avgs[1].rtime / PROCS_PER_TYPE,
				 time_results_avgs[1].iotime / PROCS_PER_TYPE);
		printf(1, "medium io:\n w: %d\n r: %d\n io: %d\n",
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
