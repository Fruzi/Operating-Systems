#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define abs(x) (x < 0 ? (-1 * x) : x)

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

extern void handle_signals();
extern uint sigret_start;
extern uint sigret_end;
asm (".globl sigret_start\n"
     ".globl sigret_end\n"
     "sigret_start:\n\t"
     "movl $24, %eax\n\t"
     "int $64\n"
     "sigret_end:");

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}


int 
allocpid(void) 
{
  int old;
  /* Assignment 2 */
  pushcli();
  do {
    old = nextpid;
  } while (!cas(&nextpid, old, old + 1));
  popcli();
  return old + 1;
}


//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  /* Assignment 2 */
  pushcli();
  do {
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(abs(p->state) == UNUSED) {
        break;
      }
    }
    if (p == &ptable.proc[NPROC]) {
      popcli();
      return 0;
    }
  } while (!cas(&p->state, UNUSED, EMBRYO));
  popcli();

  p->pid = allocpid();


  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  /* Assignment 2 */
  // Initialize signal-related fields.
  p->pending_sigs = 0;
  p->sig_mask = ~0;
  memset(p->sig_handlers, SIG_DFL, sizeof(p->sig_handlers));
  p->suspended = 0;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  /* Assignment 2 */
  //pushcli();
  if (!cas(&p->state, EMBRYO, RUNNABLE)) {
    panic("userinit: state should be EMBRYO");
  }
  //popcli();
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  /* Assignment 2 */
  // Inherit the parent’s signal mask and signal handlers.
  np->sig_mask = curproc->sig_mask;
  memmove(np->sig_handlers, curproc->sig_handlers, sizeof(curproc->sig_handlers));

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  /* Assignment 2 */
  //pushcli();
  if (!cas(&np->state, EMBRYO, RUNNABLE)) {
    panic("fork: state should be EMBRYO");
  }
  //popcli();

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  /* Assignment 2 */
  pushcli();
  if (!cas(&curproc->state, RUNNING, -ZOMBIE)) {
  	panic("exit: state should be RUNNING");
  }
  // Parent might be sleeping in wait().
  //DIFFF
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(abs(p->state) == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");

}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  /* Assignment 2 */
  pushcli();
  for(;;){
    if (!cas(&curproc->state, RUNNING, -SLEEPING)) {
      panic("wait: state should be RUNNING");
    }

    curproc->chan = curproc;
    // Scan through table looking for exited children.
    havekids = 0;
    do {
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->parent != curproc)
          continue;
        havekids = 1;
        if(abs(p->state) == ZOMBIE){
          // Found one.
          break;
        }
      }
    } while (p < &ptable.proc[NPROC] && !cas(&p->state, ZOMBIE, -UNUSED));

    if (havekids && p < &ptable.proc[NPROC]) {
      pid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      freevm(p->pgdir);
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      cas(&p->state, -UNUSED, UNUSED);
      curproc->chan = 0;
      cas(&curproc->state, -SLEEPING, RUNNING);
      popcli();
      return pid;
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      curproc->chan = 0;
      cas(&curproc->state, -SLEEPING, RUNNING);
      popcli();
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sched();
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  int found;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    /* Assignment 2 */
    pushcli();
    do {
      found = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == RUNNABLE
           /* Assignment 2 */
           // Choose a different process if p is suspended and a SIGCONT is not pending.
           && (!p->suspended || is_pending_sig(p, SIGCONT))) {
          found = 1;
          break;
        }
      }
    } while (found && !cas(&p->state, RUNNABLE, RUNNING));
    if (!found) {
      popcli();
      continue;
    }

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);

    swtch(&(c->scheduler), p->context);
    switchkvm();
    
    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    cas(&p->state, -RUNNABLE, RUNNABLE);
    if (cas(&p->state, -SLEEPING, SLEEPING)) {
      if (p->killed || is_pending_sig(p, SIGKILL)) {
        cas(&p->state, SLEEPING, RUNNABLE);
      }
    }
    if (cas(&p->state, -ZOMBIE, ZOMBIE)) {
      wakeup1(p->parent);
    }

    popcli();

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  pushcli();
  if (!cas(&myproc()->state, RUNNING, -RUNNABLE)) {
    panic("yield: state should be RUNNING");
  }
  sched();
  popcli();
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  popcli();

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  pushcli();

  if(lk != &ptable.lock){
    release(lk);
  }

  if (!cas(&p->state, RUNNING, -SLEEPING)) {
    panic("sleep: state should be RUNNING");
  }

  // Go to sleep.
  p->chan = chan;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    acquire(lk);
  }

  popcli();
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  do {
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if(abs(p->state) == SLEEPING && p->chan == chan) {
        break;
      }
    }
  } while (p < &ptable.proc[NPROC] && !cas(&p->state, SLEEPING, -RUNNABLE));
  if (p == &ptable.proc[NPROC]) {
    return;
  }
  p->chan = 0;
  if (!cas(&p->state, -RUNNABLE, RUNNABLE)) {
    panic("wakeup1: state should be -RUNNABLE");
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  pushcli();
  wakeup1(chan);
  popcli();
}

/* Assignment 2 */
// Add the given signal to the process with the given pid's pending signals array.
int
kill(int pid, int signum)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->pending_sigs |= (1 << signum);
      return 0;
    }
  }
  return -1;
}

/* Assignment 2 */
uint
sigprocmask(uint sigmask)
{
  struct proc *p = myproc();
  uint prev = p->sig_mask;
  p->sig_mask = sigmask;
  return prev;
}

sighandler_t
signal(int signum, sighandler_t handler)
{
  struct proc *p = myproc();
  sighandler_t prev;

  if (signum < 0 || signum >= 32)
    return (sighandler_t)-2;

  prev = p->sig_handlers[signum];
  p->sig_handlers[signum] = handler;
  return prev;
}

void
sigret(void)
{
  struct proc *p = myproc();
  memmove(p->tf, &p->user_tf_backup, sizeof(struct trapframebackup));
  sigprocmask(p->sig_mask_backup);
}

// Default SIGKILL handler.
void sigkill(void) {
  struct proc *p = myproc();
  p->killed = 1;
  sigprocmask(p->sig_mask_backup);
}

// Default SIGSTOP handler.
void sigstop(void) {
  struct proc *p = myproc();
  p->suspended = 1;
  sigprocmask(p->sig_mask_backup);
  yield();
}

// Default SIGCONT handler.
void sigcont(void) {
  struct proc *p = myproc();
  p->suspended = 0;
  sigprocmask(p->sig_mask_backup);
}

void execute_user_signal_handler(int signum) {
  struct proc *p = myproc();
  void *handler = p->sig_handlers[signum];

  memmove(&p->user_tf_backup, p->tf, sizeof(struct trapframe));

  p->tf->esp -= (uint)&sigret_end - (uint)&sigret_start;
  memmove((void*)p->tf->esp, (void*)&sigret_start, (uint)&sigret_end - (uint)&sigret_start);
  *((uint*)(p->tf->esp - 1 * sizeof(uint))) = signum;
  *((uint*)(p->tf->esp - 2 * sizeof(uint))) = p->tf->esp;
  p->tf->esp -= 2 * sizeof(uint);
  p->tf->eip = (uint)handler;
}

// Execute default signal handlers. Called from trapret, before returning to user space.
void handle_signals(struct trapframe *tf) {
  struct proc *p = myproc();

  if (p == 0)
    return;

  for (int i = 0; i < 32; i++) {
    if (is_pending_sig(p, i)) {
      // Block other signals.
      p->sig_mask_backup = sigprocmask(0);

      if (p->sig_handlers[i] == (void*)SIG_DFL) {
        // Execute the default behavior for this signal.
        switch (i) {
        case SIGSTOP:
          sigstop();
          break;
        case SIGCONT:
          sigcont();
          break;
        default:
          sigkill();
          break;
        }
      } else if (p->sig_handlers[i] == (void*)SIG_IGN) {
        // Ignore this signal.
      } else {
        execute_user_signal_handler(i);
      }

      // Reset the pending signal bit.
      p->pending_sigs ^= (1 << i);
    }
  }
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
