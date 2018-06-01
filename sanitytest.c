#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

int main(int argc, char **argv) {
  int fd;
  int i, n, total;
  char buf[BSIZE] = {0};

  if ((fd = open("textfile", O_CREATE|O_RDWR)) < 0) {
    printf(1, "error: open\n");
    exit();
  }
  total = 0;
  for(i = 0; i < 1954; i++){
    if((n = write(fd, buf, BSIZE)) != BSIZE){
      printf(1, "error: write\n");
      exit();
    }
    total += n;
    if (total == BSIZE * NDIRECT) {
      printf(1, "Finished writing %dKB (direct)\n", total / 1000);
    } else if (total == BSIZE * (NDIRECT + NINDIRECT)) {
      printf(1, "Finished writing %dKB (indirect)\n", total / 1000);
    }
  }
  printf(1, "Finished writing %dMB\n", total / 1000000);  
  close(fd);
  exit();
}
