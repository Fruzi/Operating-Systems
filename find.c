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

int is_match(int fd, struct stat st, const char *filename) {
  int match = 1;
  char tag_key[10], tag_value[30], file_tag_value[30], *tag_sep;

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
    gettag(fd, tag_key, file_tag_value);
    if (gettag(fd, tag_key, file_tag_value) < 0
        || (strcmp(tag_value, "?") != 0 && strcmp(tag_value, file_tag_value) != 0)) {
      match = 0;
    }
  }
  return match;
}

void find(char *path, char *rel_path) {
  char *p, *rel_p, *filename, *filename_temp;
  int fd, file_fd;
  struct dirent de;
  struct stat st;

  if (follow) {
    if ((fd = open(path, O_RDONLY)) < 0) {
      return;
    }
  } else {
    if ((fd = open(path, O_RDONLY|O_NOLINKS)) < 0) {
      return;
    }
  }

  fstat(fd, &st);
  switch (st.type) {
  case T_FILE:
    filename = path;
    while ((filename_temp = strchr(filename, '/')) != 0) {
      filename = filename_temp + 1;
    }
    if (is_match(fd, st, filename)) {
      safestrcpy(&rel_path[strlen(rel_path)], filename, DIRSIZ);
      println(rel_path);
      *filename = 0;
    }
    break;
  case T_DIR:
    rel_p = rel_path + strlen(rel_path);
    if (strlen(rel_path) > 0) {
      *rel_p++ = '/';
    }
    p = path + strlen(path);
    if (strcmp(path, "/") != 0) {
      *p++ = '/';
    }
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
      if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
        continue;
      }
      safestrcpy(rel_p, de.name, DIRSIZ);
      safestrcpy(p, de.name, DIRSIZ);
      if ((file_fd = open(path, O_RDONLY)) < 0) {
        continue;
      }
      if(fstat(file_fd, &st) < 0){
        printf(1, "find: cannot stat %s\n", path);
        close(file_fd);
        continue;
      }
      if (is_match(file_fd, st, de.name)) {
        println(rel_path);
      }
      close(file_fd);
      if (st.type == T_DIR) {
        find(path, rel_path);
      }
      *rel_p = 0;
      *p = 0;
    }
    break;
  }
  close(fd);
}

int main(int argc, char *argv[]) {
  int arg_idx;
  char *arg, *path = 0, *rel_path = 0;

  if (argc < 2) {
    printf(2, "Usage: find <path> [<options>] [<tests>]\n");
    exit();
  }
  path = (char*)malloc(512);
  safestrcpy(path, argv[1], 512);
  rel_path = (char*)malloc(512);
  rel_path[0] = '\0';
  for (arg_idx = 2; arg_idx < argc; arg_idx++) {
    arg = argv[arg_idx];
    if (strcmp(arg, "-follow") == 0) {
      follow = 1;
    } else if (strcmp(arg, "-name") == 0) {
      arg = argv[++arg_idx];
      safestrcpy(name, arg, DIRSIZ);
    } else if (strcmp(arg, "-size") == 0) {
      arg = argv[++arg_idx];
      safestrcpy(size, arg, 12);
    } else if (strcmp(arg, "-type") == 0) {
      arg = argv[++arg_idx];
      if (strlen(arg) > 1) {
        continue;
      }
      type = types[(int)arg[0]];
    } else if (strcmp(arg, "-tag") == 0) {
      arg = argv[++arg_idx];
      safestrcpy(tag, arg, 45);
    }
  }
  find(path, rel_path);
  free(path);
  free(rel_path);
  exit();
}
