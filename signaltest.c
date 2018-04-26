#include "types.h"
#include "stat.h"
#include "user.h"

#define SIG_DFL -1
#define SIG_IGN 1
#define SIGKILL 9
#define SIGSTOP 17
#define SIGCONT 19

int success = 0;
int ret_arr[3] = { 0 };

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

int main() {
  signal(10, my_signal_handler);
  signal(SIGSTOP, my_signal_handler);
  signal(SIGCONT, my_signal_handler);
  int pid;
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
