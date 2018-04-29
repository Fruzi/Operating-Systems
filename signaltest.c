#include "types.h"
#include "stat.h"
#include "user.h"

#define SIG_DFL -1
#define SIGINT 1
#define SIG_IGN 1
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

int success = 0;
int ret_arr[2] = { 0 };
int cpid[5];
int j;

void sigCatcher(int signum) {
  printf(1, "PID %d caught one\n", getpid());
  if (j > -1){
    kill(cpid[j], SIGINT);      
  }
  kill(getpid(), SIGKILL);
}




void my_signal_handler(int signum) {
  switch (signum) {
  case SIGSTOP:
    ret_arr[0] = signum;
    break;
  case SIGCONT:
    ret_arr[1] = signum;
    break;
  case 10:
    success = 1;
    break;
  }
  //sigret();
}

void signaltest(){
  int pid;
  signal(10, my_signal_handler);
  signal(SIGSTOP, my_signal_handler);
  signal(SIGCONT, my_signal_handler);
  switch ((pid = fork())) {
    case -1:
      exit();
    case 0:
      while (!success);
      printf(1, "0: %d, 1: %d\n", ret_arr[0], ret_arr[1]);  // ret_arr should be [17, 19]
      break;
    default:
      kill(pid, SIGSTOP);
      kill(pid, SIGCONT);
      kill(pid, 10);
      wait();
      break;
  }
  exit();
}

void synctest(){
  int i, pid, zombie;
  signal(SIGINT, sigCatcher);
  for (i = 0; i < 5; i++) {
    if ((pid = fork()) == 0) {
      printf(1, "PID %d ready\n", getpid());
      j = i - 1;
      while(1);
    } else
      cpid[i] = pid;
  }
  sleep(250);
  kill(cpid[4], SIGINT);
  for (i = 0; i < 5; i++) {
    zombie = wait();
    printf(1, "%d is dead\n", zombie);
  }
  exit();
}

int main(int argc, char** argv) {
  switch(argc){
    case 1:
      signaltest();
      break;
    case 2:
      synctest();
  } 
}


