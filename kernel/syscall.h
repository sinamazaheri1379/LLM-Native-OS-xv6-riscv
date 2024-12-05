// System call numbers
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
#define SYS_read    5
#define SYS_kill    6
#define SYS_exec    7
#define SYS_fstat   8
#define SYS_chdir   9
#define SYS_dup    10
#define SYS_getpid 11
#define SYS_sbrk   12
#define SYS_sleep  13
#define SYS_uptime 14
#define SYS_open   15
#define SYS_write  16
#define SYS_mknod  17
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
#define SYS_shutdown  23  // Add after other syscalls

// AIOS Kernel scheduler syscalls
#define SYS_llm_set_id 24
#define SYS_llm_get_id 25
#define SYS_llm_get_status 26
#define SYS_llm_set_status 27
#define SYS_llm_set_priority 28
#define SYS_llm_get_priority 29

// AIOS Kernel LLM Core(s) syscalls

#define SYS_llm_generate 30

// AIOS Kernel Context Manager

#define SYS_llm_gen_snapshot 31
#define SYS_llm_gen_restore 32
#define SYS_llm_check_restore 33
#define SYS_llm_clear_restore 34

///////////////////////////////



// AIOS Kernel Memory Manager

#define SYS_llm_mem_alloc 35
#define SYS_llm_mem_read 36
#define SYS_llm_mem_write 37
#define SYS_llm_mem_clear 38

// AIOS Kernel Storage Manager

#define SYS_sto_create 39
#define SYS_sto_read 40
#define SYS_sto_write 41
#define SYS_sto_retrieve 42
#define SYS_sto_clear 43

// AIOS Kernel Tool Manager

#define SYS_tool_run 44


// AIOS Kernel Access Manager
