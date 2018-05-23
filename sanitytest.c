#include "types.h"
#include "stat.h"
#include "user.h"
#define PAGESIZE 4096

int main() {
  printf(1, "AAA\n");
  char *b = (char*)malloc(PAGESIZE*5);
  printf(1, "BBB\n");
  memset(b, 5, PAGESIZE*5-1);
  printf(1, "CCC\n");
  printf(1, "%d\n", b[2*PAGESIZE]);
  printf(1, "DDD\n");
  exit();	
}