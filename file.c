//
// File descriptors
//

#include "types.h"
#include "defs.h"
#include "param.h"
#include "fs.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "file.h"

struct devsw devsw[NDEV];
struct {
  struct spinlock lock;
  struct file file[NFILE];
} ftable;

void
fileinit(void)
{
  initlock(&ftable.lock, "ftable");
}

// Allocate a file structure.
struct file*
filealloc(void)
{
  struct file *f;

  acquire(&ftable.lock);
  for(f = ftable.file; f < ftable.file + NFILE; f++){
    if(f->ref == 0){
      f->ref = 1;
      release(&ftable.lock);
      return f;
    }
  }
  release(&ftable.lock);
  return 0;
}

// Increment ref count for file f.
struct file*
filedup(struct file *f)
{
  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("filedup");
  f->ref++;
  release(&ftable.lock);
  return f;
}

// Close file f.  (Decrement ref count, close when reaches 0.)
void
fileclose(struct file *f)
{
  struct file ff;

  acquire(&ftable.lock);
  if(f->ref < 1)
    panic("fileclose");
  if(--f->ref > 0){
    release(&ftable.lock);
    return;
  }
  ff = *f;
  f->ref = 0;
  f->type = FD_NONE;
  release(&ftable.lock);

  if(ff.type == FD_PIPE)
    pipeclose(ff.pipe, ff.writable);
  else if(ff.type == FD_INODE){
    begin_op();
    iput(ff.ip);
    end_op();
  }
}

// Get metadata about file f.
int
filestat(struct file *f, struct stat *st)
{
  if(f->type == FD_INODE){
    ilock(f->ip);
    stati(f->ip, st);
    iunlock(f->ip);
    return 0;
  }
  return -1;
}

// Read from file f.
int
fileread(struct file *f, char *addr, int n)
{
  int r;

  if(f->readable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return piperead(f->pipe, addr, n);
  if(f->type == FD_INODE){
    ilock(f->ip);
    if((r = readi(f->ip, addr, f->off, n)) > 0)
      f->off += r;
    iunlock(f->ip);
    return r;
  }
  panic("fileread");
}

//PAGEBREAK!
// Write to file f.
int
filewrite(struct file *f, char *addr, int n)
{
  int r;

  if(f->writable == 0)
    return -1;
  if(f->type == FD_PIPE)
    return pipewrite(f->pipe, addr, n);
  if(f->type == FD_INODE){
    // write a few blocks at a time to avoid exceeding
    // the maximum log transaction size, including
    // i-node, indirect block, allocation blocks,
    // and 2 blocks of slop for non-aligned writes.
    // this really belongs lower down, since writei()
    // might be writing a device like the console.
    int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
    int i = 0;
    while(i < n){
      int n1 = n - i;
      if(n1 > max)
        n1 = max;

      begin_op();
      ilock(f->ip);
      if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
        f->off += r;
      iunlock(f->ip);
      end_op();

      if(r < 0)
        break;
      if(r != n1)
        panic("short filewrite");
      i += r;
    }
    return i == n ? n : -1;
  }
  panic("filewrite");
}

/* Assignment 4 */

static int find_tag_off(char *tag_block, const char *key) {
  int off;
  char *tag;

  for (off = 0; off + TAG_SIZE < BSIZE; off += TAG_SIZE) {
    tag = &tag_block[off];
    if (strncmp(tag, key, strlen(key)) == 0) {
      return off;
    }
  }
  return -1;
}

static int new_tag_off(char *tag_block)  {
  int off;
  char *tag;

  for (off = 0; off + TAG_SIZE < BSIZE; off += TAG_SIZE) {
    tag = &tag_block[off];
    if (tag[0] == '\0') {
      return off;
    }
  }
  return -1;
}

// Debug
static void print_tags(char *tag_block) {
  int off;
  char *tag;

  for (off = 0; off + TAG_SIZE < BSIZE; off += TAG_SIZE) {
    tag = &tag_block[off];
    cprintf("%s=%s ", tag, TAG_VALUE(tag));
  }
  cprintf("\n");
}

int ftag(struct file *f, const char *key, const char *value) {
  char tag_block[BSIZE], *tag;
  int tag_off;

  if (strlen(key) >= 10 || strlen(value) >= 30) {
    return -1;
  }
  begin_op();
  ilock(f->ip);
  readtagi(f->ip, tag_block);
  if ((tag_off = find_tag_off(tag_block, key)) < 0) {
    if ((tag_off = new_tag_off(tag_block)) < 0) {
      iunlock(f->ip);
      end_op();
      return -1;
    }
  }
  tag = &tag_block[tag_off];
  safestrcpy(tag, key, strlen(key) + 1);
  safestrcpy(TAG_VALUE(tag), value, strlen(value) + 1);
  writetagi(f->ip, tag_block);
  iunlock(f->ip);
  end_op();
  print_tags(tag_block); // Debug
  return 0;
}

int funtag(struct file *f, const char *key) {
  char tag_block[BSIZE], *tag;
  int tag_off;

  if (strlen(key) >= 10) {
    return -1;
  }
  begin_op();
  ilock(f->ip);
  readtagi(f->ip, tag_block);
  if ((tag_off = find_tag_off(tag_block, key)) < 0) {
    iunlock(f->ip);
    end_op();
    return -1;
  }
  tag = &tag_block[tag_off];
  memset(tag, 0, TAG_SIZE);
  writetagi(f->ip, tag_block);
  iunlock(f->ip);
  end_op();
  print_tags(tag_block); // Debug
  return 0;
}

int gettag(struct file *f, const char *key, char *buf) {
  char tag_block[BSIZE], *tag;
  int tag_off;

  if (strlen(key) >= 10) {
    return -1;
  }
  begin_op();
  ilock(f->ip);
  readtagi(f->ip, tag_block);
  if ((tag_off = find_tag_off(tag_block, key)) < 0) {
    iunlock(f->ip);
    end_op();
    return -1;
  }
  tag = &tag_block[tag_off];
  safestrcpy(buf, TAG_VALUE(tag), 30);
  writetagi(f->ip, tag_block);
  iunlock(f->ip);
  end_op();
  return strlen(TAG_VALUE(tag));
}
