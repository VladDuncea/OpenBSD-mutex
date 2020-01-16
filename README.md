# OpenBSD-mutex
Custom openBSD **_mutex_** implementation made by Vlad Duncea and Vlad Stoleru. This was an assignment received during our Operating Systems Course.

### Architecture used
The implementation was designed and written on a custom openBSD architercture provided by our Teaching Assistant.
The code was written and compiled inside the Kernel, the attached folders being taken from inside it.

### A quick look through the code
The first part to be implemented was the additional syscall used to operate our mutex implementation, which can be found at the end of the _kern/syscalls.master_: _void umutex(void \**mtx, int op)_. It takes two parameters, the _address_ of a mutex pointer and the _operation_ to be performed.

#### Looking into _sys/mutex.h_ we can see that we can perform three operations using our mutex:
1. **MUTEX_INIT** (which is a define for _3_): initializez the mutex, handling the allocation of the lock;
2. **MUTEX_LOCK** (which is a define for _1_): locks the mutex and remembers the process which holds the lock;
3. **MUTEX_UNLOCK** (which is a define for _2_): unlocks the mutex and wakes up one of the sleeping processes (memorized inside the mutex);

#### The implementation is found inside _kern/sys\_umutex.c_, which is linked to the syscall within _conf/files_:
- the _u_(ser)_mutex_ structure contains four fields:
  - **SIMPLEQ_HEAD(pids_head,pidentry) head**: a queue structure which holds all the processes waiting at the umutex;
    - the **pidentry** structure has two fields: a queue entry for the process (**SIMPLEQ_ENTRY(pidentry)**) and the process id (**pid_t pid**);
  - **int lock**: the current state of the lock - 1: _locked_, 0: _unlocked_;
  - **LIST_ENTRY(umutex) umutexes**: a list entry for the mutex;
    - a global mutex list (**LIST_HEAD(umutex_head,umutex) head**) is used as a container for all the mutexes - the list is initialized using the _umutex\_init_ function which is called at Kernel initialization (inside _kern/init\_main.c_).
  - **pid_t lockPid**: the process identifier of the process that currently holds the lock.
- two different kinds of locks are used in the implementation of _umutex\_lock_ and _umutex\_unlock_:
  - a mutex used to lock the Kernel: **KERNEL_LOCK()** and **KERNEL_UNLOCK()** are two macros that expand to a mutex structure that cease/continue the Kernel activity;
  - a readers-writers lock used to secure the part where a process is put to sleep/woken up, to prevent concurrency in the sleeping process list: **struct rwlock umtxlock = RWLOCK_INITIALIZER("umutex");**.
