#include "types.h"
#include "stat.h"
#include "user.h"

int success = 0;

void my_signal_handler(int signum) {
  printf(1, "in handler\n");
  switch (signum) {
  case 10:
    success = 1;
    break;
  }
  //sigret();
}

int main() {
  signal(10, my_signal_handler);
  int pid;
  switch ((pid = fork())) {
  case -1:
    return -1;
  case 0:
    while (!success);
    break;
  default:
    kill(pid, 10);
    break;
  }
  exit();
}
