#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"


uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
uint64
sys_shutdown(void)
{
    printf("Shutting down...\n");

    // Try ACPI poweroff
    volatile uint16 *acpi_pm1a_cnt = (volatile uint16 *)ACPI_PM1A_CNT;
    *acpi_pm1a_cnt = (1 << 13) | (1 << 14); // SLP_EN | SLP_TYP

    // If ACPI failed, try keyboard controller reset
    volatile uint8 *keyboard_ctrl = (volatile uint8 *)KBD_CTRL;
    *keyboard_ctrl = 0xFE;

    // If that didn't work, try the triple fault approach
    printf("Forcing shutdown...\n");
    asm volatile(
            "csrw satp, zero\n"    // Disable MMU
            "sfence.vma\n"         // Flush TLB
            "li a0, 0\n"           // NULL pointer
            "ld a0, (a0)\n"        // Cause fault
            );

    // Should never get here
    printf("Shutdown failed\n");
    return -1;
}



