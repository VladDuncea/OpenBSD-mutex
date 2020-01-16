# OpenBSD-mutex
Custom openBSD **_mutex_** implementation made by Vlad Duncea and Vlad Stoleru. This was an assignment received during our Operating Systems Course.

### Architecture used
The implementation was designed and written on a custom openBSD architercture provided by our Teaching Assistant.
The code was written and compiled inside the Kernel, the attached folders being taken from inside it.

### A quick look through the code
The first part to be implemented was the additional syscall used to operate our mutex implementation, which can be found at the end of the _kern/syscalls.master_: _void umutex(void \**mtx, int op)_. It takes two parameters, the _address_ of a mutex pointer and the _operation_ to be performed.

#### Looking into _sys/mutex.h_ we can see that we can perform three operations using our mutex:
1. **UMUTEX_INIT** (which is a define for _3_): initializez the mutex, handling the allocation of the lock;
2. **UMUTEX_LOCK** (which is a define for _1_): 
 - if umutex is open: locks the mutex and remembers the process which holds the lock;
 - if umutex is locked: puts the process on a waiting list and stops the process until lock is aquired. 
3. **UMUTEX_UNLOCK** (which is a define for _2_): if mutex is owned by calling process unlocks the mutex and wakes up one of the sleeping processes (memorized inside the mutex);

#### The implementation is found inside _kern/sys\_umutex.c_, which is linked to the syscall within _conf/files_:
- the _u_(ser)_mutex_ structure contains four fields:
  - **SIMPLEQ_HEAD(pids_head,pidentry) head**: a queue structure which holds all the processes waiting for the umutex;
    - the **pidentry** structure has two fields: a queue entry for the process (**SIMPLEQ_ENTRY(pidentry)**) and the process id (**pid_t pid**);
  - **int lock**: the current state of the lock: 1= _locked_, 0= _unlocked_;
  - **LIST_ENTRY(umutex) umutexes**: a list entry for the mutex;
    - a global mutex list (**LIST_HEAD(umutex_head,umutex) head**) is used as a container for all the mutexes - the list is initialized using the _umutex\_init_ function which is called at Kernel initialization (inside _kern/init\_main.c_).
  - **pid_t lockPid**: the process identifier of the process that currently holds the lock.
  
- two different kinds of locks are used in the implementation of _umutex\_lock_ and _umutex\_unlock_:
  - a mutex used to lock the Kernel: **KERNEL_LOCK()** and **KERNEL_UNLOCK()**;
  - a readers-writers lock used to secure the part where a process is put to sleep/woken up, to prevent losing wakeup calls: **struct rwlock umtxlock = RWLOCK_INITIALIZER("umutex");**.

### Implementing the mutex operations
- **int umutex\_construct(struct umutex \*\* mtx);**
  - creates the mutex, allocating memory for the _umutex_ structure;
  - inserts the mutex into the mutex list: _LIST_INSERT\_HEAD(&head,\*mtx,umutexes);_;
  - initializes the inner sleeping-process queue: _SIMPLEQ\_INIT(&((\*mtx)->head));_;
  - sets the initial values for the **lockPID** and **lock** fields as 0.
  
- **int umutex\_lock(struct umutex \* mtx,pid_t pid);**
  - the requesting process enters an infinite _while_ loop, which will be left only after the aquisition of the lock or if an error occurs;
  - when entering, the process locks both the rwlock and the Kernel;
  - if the lock is unlocked or the current owner is the requesting process:
    - the lock is aquired by the process;
    - the rwlock and the Kernel are both unlocked;
    - the process exits the loop.
  - otherwise:
    - a _struct pidentry_ pointer is created which will hold the requesting process data;
    - the process is inserted into the mutex sleeping-queue (_SIMPLEQ_INSERT\_TAIL(&(mtx->head),pidelem,pidentries);_);
    - the Kernel is unlocked;
    - the process is put to sleep indefinetly and the _rwlock_ is unlocked afterwards.

- **int umutex\_unlock(struct umutex \*mtx,pid_t pid);**
  - the requesting process locks both the rwlock and the Kernel;
  - if the requesting process is the lock owner:
    - the process unlocks the lock;
    - if the lock's sleeping-queue is empty, the process unlocks both the rwlock and the Kernel then exits.
    - otherwise:
      - a process _p_ is removed from the list;
      - the Kernel is unlocked;
      - _p_ process is woken up: _wakeup_one(pidelem)_;
      - the rwlock is unlocked;
      - the pidentry containing the woken process is memory-free'd.
  - if the process making the request is not the lock owner, the process unlocks both the rwlock and the Kernel then exits.
      
