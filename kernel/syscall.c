#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"


int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if(copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_shutdown(void);

////
extern uint64 sys_llm_set_id(void);
extern uint64 sys_llm_get_id(void);
extern uint64 sys_llm_set_status(void);
extern uint64 sys_llm_get_status(void);
extern uint64 sys_llm_set_priority(void);
extern uint64 sys_llm_get_priority(void);


/////
extern uint64 sys_llm_generate(void);
/////
extern uint64 sys_llm_mem_alloc(void);
extern uint64 sys_llm_mem_read(void);
extern uint64 sys_llm_mem_write(void);
extern uint64 sys_llm_mem_clear(void);
//////
extern uint64 sys_llm_sto_create(void);
extern uint64 sys_llm_sto_read(void);
extern uint64 sys_llm_sto_write(void);
extern uint64 sys_llm_sto_retrieve(void);
extern uint64 sys_llm_sto_clear(void);
//////
extern uint64 sys_llm_tool_run(void);
//////
extern uint64 sys_llm_gen_snapshot(void);
extern uint64 sys_llm_gen_restore(void);
extern uint64 sys_llm_check_restore(void);
extern uint64 sys_llm_clear_restore(void);
//////


// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_shutdown] sys_shutdown,
/////
[SYS_llm_set_id] sys_llm_set_id,
[SYS_llm_get_id] sys_llm_get_id,
[SYS_llm_get_status] sys_llm_get_status,
[SYS_llm_set_status] sys_llm_set_status,
[SYS_llm_set_priority] sys_llm_set_priority,
[SYS_llm_get_priority] sys_llm_get_priority,
////
[SYS_llm_generate] sys_llm_generate,
///
[SYS_llm_gen_snapshot] sys_llm_gen_snapshot,
[SYS_llm_gen_restore] sys_llm_gen_restore,
[SYS_llm_check_restore] sys_llm_check_restore,
[SYS_llm_clear_restore] sys_llm_clear_restore,
/////
[SYS_llm_mem_alloc] sys_llm_mem_alloc,
[SYS_llm_mem_read] sys_llm_mem_read,
[SYS_llm_mem_write] sys_llm_mem_write,
[SYS_llm_mem_clear] sys_llm_mem_clear,
/////
[SYS_sto_create] sys_llm_sto_create,
[SYS_sto_read] sys_llm_sto_read,
[SYS_sto_write] sys_llm_sto_write,
[SYS_sto_retrieve] sys_llm_sto_retrieve,
[SYS_sto_clear] sys_llm_sto_clear,
/////
[SYS_tool_run] sys_llm_tool_run
};

void
syscall(void)
{
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // Use num to lookup the system call function for num, call it,
    // and store its return value in p->trapframe->a0
    p->trapframe->a0 = syscalls[num]();
  } else {
    printf("%d %s: unknown sys call %d\n",
            p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
