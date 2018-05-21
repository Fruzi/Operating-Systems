#include "types.h"
#include "stat.h"
#include "user.h"

int main() {
  char *b = (char*)malloc(1000000);
  printf(1, "%d\n", b[0]);
  exit();
}