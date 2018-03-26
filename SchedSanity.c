#include "types.h"
#include "stat.h"
#include "user.h"

#define NUM_SUB_P 4
#define L_LOOP 100
#define S_LOOP 4

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
		printf(1, "%d", i);
	}
	printf(1, "\n");
}

void l_io(){
	int i;
	for (i=0; i<L_LOOP ; i++){
		printf(1, "%d", i);
	}
	printf(1, "\n");
}

int main(void){
	int i;
	int pid=1;
	times times_arr[NUM_SUB_P];
	for (i=0; i<NUM_SUB_P ; i++){
		if(pid){
			pid=fork();
			if(pid){
				times_arr[i].pid=pid;
			}
			else if(i==0 && pid==0){
				s_calc();
				exit();
			}
			else if(i==1 && pid==0){
				l_calc();
				exit();
			}
			else if(i==2 && pid==0){
				s_io();
				exit();
			}
			else if(i==3 && pid==0){
				l_io();
				exit();
			}
		}
	}
	if(pid==times_arr[NUM_SUB_P].pid){
		for (i=0; i<NUM_SUB_P ; i++){
			wait2(times_arr[i].pid, &times_arr[i].wtime, 
				  &times_arr[i].rtime, &times_arr[i].iotime);
		}
		printf(1, "small calc:\n w: %d\n r: %d\n io: %d\n",
			   times_arr[0].wtime, times_arr[0].rtime, times_arr[0].iotime);
		printf(1, "large calc:\n w: %d\n r: %d\n io: %d\n",
			   times_arr[1].wtime, times_arr[1].rtime, times_arr[1].iotime);
		printf(1, "small io:\n w: %d\n r: %d\n io: %d\n",
			   times_arr[2].wtime, times_arr[2].rtime, times_arr[2].iotime);
		printf(1, "large io:\n w: %d\n r: %d\n io: %d\n",
			   times_arr[3].wtime, times_arr[3].rtime, times_arr[3].iotime);
	}
}		