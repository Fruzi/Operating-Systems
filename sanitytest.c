#include "types.h"
#include "stat.h"
#include "user.h"
#define PAGESIZE 4096

int main() {
	uint num_pages=40;
  int i, j;
  printf(1, "Test: create a 2D array of size(PAGESIZE/2) * %d, then assign values\n", num_pages);
  printf(1, "into it in column-first order.\n");
  printf(1, "Before malloc((PAGESIZE/2) * %d)\n", num_pages);
  char **b = (char**)malloc((PAGESIZE/2) * num_pages);
  printf(1, "After malloc\n");
  printf(1, "Before assignments\n");
  for (j = 0; j < num_pages; j++) {
    for (i = 0; i < PAGESIZE/2; i++) {
      b[i][j] = 5;
    }
  }
  printf(1, "After assignments\n");
  printf(1, "Done\n");
  exit();	
}
