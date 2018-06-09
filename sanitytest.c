#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"
#include "fs.h"

void task1_sanity(void) {
  int fd;
  int i, n;
  int total = 0;
  int chunk = BSIZE * 4;
  char buf[chunk];
  memset(buf, 'a', chunk);

  if ((fd = open("textfile", O_CREATE|O_RDWR)) < 0) {
    printf(1, "error: open\n");
    exit();
  }
  for (i = 0; i < (1 << 20) / chunk; i++) {
    if ((n = write(fd, buf, chunk)) != chunk) {
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

void task2_maxderef_test(void) {
  int fd, i;
  char buf[10] = {0};
  char filename[3] = {'g', '0', 0};
  char linkname[3] = {'g', '1', 0};
  char *link[5] = {"ln", "-s", filename, linkname, 0};

  if ((fd = open(filename, O_CREATE|O_WRONLY)) < 0) {
    return;
  }
  memset(buf, '1', 9);
  buf[9] = '\0';
  write(fd, (void*)buf, 10);
  close(fd);
  for (i = 0; i < 50; i++) {
    filename[1]++;
    linkname[1]++;
    //printf(1, "%s %s %s %s\n", link[0], link[1], link[2], link[3]);
    switch (fork()) {
      case -1:
        return;
      case 0:
        exec(link[0], link);
        return;
      default:
        wait();
    }
  }
  printf(1, "%d\n", readlink(filename, buf, 10));  // should fail (print -1)
}

void task2_link_test(void) {
  int fd;
  char buf[512] = {0};
  char *link[5] = {"ln", "-s", "f1", "f2", 0};

  if ((fd = open("f1", O_CREATE|O_WRONLY)) < 0) {
    return;
  }
  memset(buf, '1', 9);
  buf[9] = '\0';
  write(fd, (void*)buf, 10);
  close(fd);
  switch (fork()) {
    case -1:
      return;
    case 0:
      exec(link[0], link);
      return;
    default:
      wait();
  }
  memset(buf, 0, sizeof(buf));
  readlink("f2", buf, 15);
  printf(1, "%s\n", buf);

  printf(1, "%d\n", readlink("f1", buf, 15));  // should fail (print -1)

  memset(buf, 0, sizeof(buf));
  if ((fd = open("f2", O_RDONLY)) < 0) {
    return;
  }
  read(fd, buf, 10);
  close(fd);
  printf(1, "%s\n", buf);
}

void task2_sanity(void) {
  task2_link_test();
  //task2_maxderef_test();
}

void task3_sanity(void) {
  int fd;
  char value[30];

  if ((fd = open("f1", O_CREATE|O_RDWR)) < 0) {
    return;
  }
  ftag(fd, "abc", "def");
  ftag(fd, "ghi", "jkl");
  ftag(fd, "abc", "sdfg");
  ftag(fd, "blbla", "aaaaaaaaa");
  printf(1, "%d\n", gettag(fd, "ghi", value));
  printf(1, "%s\n", value);
  funtag(fd, "abc");
  ftag(fd, "hello", "hi");
  close(fd);
}

int main(int argc, char **argv) {
  if (argc < 2 || strcmp(argv[1], "1") == 0) {
   task1_sanity();
  } else if (strcmp(argv[1], "2") == 0) {
    task2_sanity();
  } else if (strcmp(argv[1], "3") == 0) {
    task3_sanity();
  }
  exit();
}
