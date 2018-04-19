#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  int i;
  for (;;) {
      for (i = 0; i < 1000000; i++);
      printf(1, "looped\n");
  }
  return 0;
}
