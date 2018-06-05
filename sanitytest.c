#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

void task1_sanity(void) {
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
}

void task2_sanity(void) {
  int fd;
  char buf1[10] = {0}, buf2[10] = {0};
  char *link1[5] = { "ln", "-s", "f1", "f2", 0 };
  char *link2[5] = { "ln", "-s", "f2", "f3", 0 };

  if ((fd = open("f1", O_CREATE|O_WRONLY)) < 0) {
    return;
  }
  memset(buf1, '1', 9);
  buf1[9] = '\0';
  write(fd, (void*)buf1, 10);
  close(fd);
  switch (fork()) {
    case -1:
      return;
    case 0:
      exec(link1[0], link1);
      return;
    default:
      wait();
  }
  switch (fork()) {
    case -1:
      return;
    case 0:
      exec(link2[0], link2);
      return;
    default:
      wait();
  }
  if ((fd = open("f3", O_RDONLY)) < 0) {
    return;
  }
  read(fd, buf2, 10);
  close(fd);
  printf(1, "%s\n", buf2);
  buf2[0] = '\0';

  readlink("f3", buf2, 15);
  printf(1, "%s\n", buf2);
}

int main(int argc, char **argv) {
  if (argc < 2 || strcmp(argv[1], "1") == 0) {
   task1_sanity();
  } else if (strcmp(argv[1], "2") == 0) {
    task2_sanity();
  }
  exit();
}
