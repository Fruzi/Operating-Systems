#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  /* Assignment 4 */
  if (!(argc == 3 || argc == 4)) {
    printf(2, "Usage: ln [-s] old new\n");
    exit();
  }
  if (strcmp(argv[1], "-s") == 0) {
    // Symbolic link
    if (symlink(argv[2], argv[3]) < 0)
      printf(2, "symlink %s %s: failed\n", argv[2], argv[3]);
  } else {
    // Hard link
    if (link(argv[1], argv[2]) < 0)
      printf(2, "link %s %s: failed\n", argv[1], argv[2]);
  }
  exit();
}
