#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"
#include "fcntl.h"

#define println(s) do { printf(1, "%s\n", (s)); } while(0)

char name[DIRSIZ] = {0}, size[12] = {0}, tag[45] = {0};
int follow = 0, type = 0;
int types[] = {
  [(int)'d'] T_DIR,
  [(int)'f'] T_FILE,
  [(int)'s'] T_SYM
};
uint og_path_size = 0;

int is_match(int fd, const char *filename) {
  int match = 1;
  struct stat st;
  char tag_key[10], tag_value[30], file_tag_value[30], *tag_sep;

  if (fstat(fd, &st) < 0) {
    printf(2, "find: cannot stat %s\n", filename);
    return 0;
  }

  if (*name && strcmp(filename, name) != 0) {
    match = 0;
  }
  if (*size) {
    switch (size[0]) {
    case '+':
      if (st.size <= atoi(&size[1])) {
        match = 0;
      }
      break;
    case '-':
      if (st.size >= atoi(&size[1])) {
        match = 0;
      }
      break;
    default:
      if (st.size != atoi(size)) {
        match = 0;
      }
    }
  }
  if (type && type != st.type) {
    match = 0;
  }
  if (*tag) {
    tag_sep = strchr(tag, '=');
    safestrcpy(tag_key, tag, (int)(tag_sep - tag) + 1);
    safestrcpy(tag_value, tag_sep + 1, 30);
    if (gettag(fd, tag_key, file_tag_value) < 0
        || (strcmp(tag_value, "?") != 0 && strcmp(tag_value, file_tag_value) != 0)) {
      match = 0;
    }
  }
  return match;
}

void find(const char *path) {
  char cont_path[512], *p, *filename, *filename_temp;
  int fd;
  struct dirent de;
  struct stat st;

  if (follow) {
    if ((fd = open((char*)path, O_RDONLY)) < 0) {
      return;
    }
  } else {
    if ((fd = open((char*)path, O_RDONLY|O_NOLINKS)) < 0) {
      return;
    }
  }
  if (fstat(fd, &st) < 0) {
    printf(2, "find: cannot stat %s\n", path);
    close(fd);
    return;
  }

  filename = (char*)path;
  while ((filename_temp = strchr(filename, '/')) != 0) {
    filename = filename_temp + 1;
  }
  if (is_match(fd, filename)) {
    println(path);
  }

  if (st.type == T_DIR) {
    safestrcpy(cont_path, path, 512);
    p = &cont_path[strlen(cont_path) - 1];
    if (*p++ != '/') {
      *p++ = '/';
    }
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
        continue;
      }
      safestrcpy(p, de.name, DIRSIZ);
      find(cont_path);
    }
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  char **arg;
  char path[512] = {0};

  if (argc < 2) {
    printf(2, "Usage: find <path> [<options>] [<tests>]\n");
    exit();
  }
  safestrcpy(path, argv[1], 512);
  og_path_size = strlen(path);
  for (arg = &argv[2]; arg < &argv[argc]; arg++) {
    if (strcmp(*arg, "-follow") == 0) {
      follow = 1;
    } else if (strcmp(*arg, "-name") == 0) {
      safestrcpy(name, *(++arg), DIRSIZ);
    } else if (strcmp(*arg, "-size") == 0) {
      safestrcpy(size, *(++arg), 12);
    } else if (strcmp(*arg, "-type") == 0) {
      if (strlen(*(++arg)) > 1) {
        continue;
      }
      type = types[(int)*arg[0]];
    } else if (strcmp(*arg, "-tag") == 0) {
      safestrcpy(tag, *(++arg), 45);
    }
  }
  find(path);
  exit();
}
