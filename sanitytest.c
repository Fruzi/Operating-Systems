#include "types.h"
#include "stat.h"
#include "user.h"
#define PAGESIZE 4096

int main() {
	uint num_pages=17;
  printf(1, "AAA\n");
  char *b = (char*)malloc(PAGESIZE*num_pages);
  printf(1, "BBB\n");
  memset(b, 5, PAGESIZE*num_pages-1);
  printf(1, "CCC\n");
  printf(1, "%d\n", b[2*PAGESIZE]);
  printf(1, "DDD\n");
  exit();	
}