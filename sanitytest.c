#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

int main() {
  int fd;
  int i, n;
  int total = 0;
  char buf[BSIZE];
  memset(buf, 'a', BSIZE);

  if ((fd = open("textfile", O_CREATE|O_RDWR)) < 0) {
    printf(1, "error: open\n");
    exit();
  }
  for (i = 0; i < (1 << 20) / BSIZE; i++) {
    if ((n = write(fd, buf, BSIZE)) != BSIZE) {
      printf(1, "error: write\n");
      exit();
    }
    total += n;
    if (total == BSIZE * NDIRECT) {
      printf(1, "Finished writing %dKB (direct)\n", total >> 10);
    } else if (total == BSIZE * (NDIRECT + NINDIRECT)) {
      printf(1, "Finished writing %dKB (single indirect)\n", total >> 10);
    }
  }
  printf(1, "Finished writing %dMB\n", total >> 20);
  close(fd);
  exit();
}
