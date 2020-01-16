#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/kernel.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/pool.h>
#include <sys/time.h>
#include <sys/rwlock.h>
#include <sys/umutex.h>
#include <sys/types.h>
#include <sys/malloc.h>

#ifdef KTRACE
#include <sys/ktrace.h>
#endif


struct rwlock umtxlock = RWLOCK_INITIALIZER("umutex");

LIST_HEAD(umutex_head,umutex) head;

struct pidentry{
	SIMPLEQ_ENTRY(pidentry) pidentries;
	pid_t pid;
};

struct umutex {
	SIMPLEQ_HEAD(pids_head,pidentry) head;	/* list of all waiting threads */
	int	lock;	
	LIST_ENTRY(umutex) umutexes;
	pid_t lockPid;		 
};



/* Syscall helpers. */
void	 umutex_init(void);
int		 umutex_construct(struct umutex **);
int		 umutex_lock(struct umutex *,pid_t);
int		 umutex_unlock(struct umutex *,pid_t);

int
sys_umutex(struct proc *p, void *v, register_t *retval)
{
	struct sys_umutex_args *uap = v;

	//get wanted instruction
	int instr = SCARG(uap,op);

	//get mutex
	struct umutex ** mtx = (struct umutex **) SCARG(uap,mtx);
	
	switch(instr)
	{
		case UMUTEX_INIT:
			umutex_construct(mtx);
			//uprintf("Adresa din kernel: %p\n",*mtx);
			*retval = 1;
			break;
		case UMUTEX_LOCK:
			if (*mtx==NULL)
			{
				*retval = 0;
				return 0;
			}	
			//uprintf("Adresa din kernel la lock: %p\n",*mtx);
			umutex_lock(*mtx,p->p_tid);
			*retval = 1;
			break;
		case UMUTEX_UNLOCK:
			if (*mtx==NULL)
			{
				*retval = 0;
				return 0;
			}
			//uprintf("Adresa din kernel la unlock: %p\n",*mtx);
			umutex_unlock(*mtx,p->p_tid);
			*retval = 1;
			break;
		default:
			*retval=0;
			break;

	}
	return 0;
}

void umutex_init(void)
{
	LIST_INIT(&head);
}


int umutex_construct(struct umutex ** mtx)
{
	//create mutex
	*mtx = malloc(sizeof(struct umutex),M_TEMP,M_NOWAIT);
	if(*mtx==NULL)
		return -1;

	//add mutex to list
	LIST_INSERT_HEAD(&head,*mtx,umutexes);

	SIMPLEQ_INIT(&((*mtx)->head));
	(*mtx)->lockPid =0;
	(*mtx)->lock =0;
	return 0;
}

int umutex_lock(struct umutex * mtx,pid_t pid)
{	
	while(true)
	{
		rw_enter_write(&umtxlock);
		KERNEL_LOCK();
		uprintf("%d a incercat lock - mtx are lock:%d lockPid:%d\n",pid,mtx->lock,mtx->lockPid);
		if(mtx->lock ==0 || mtx->lockPid==pid)
		{
			mtx->lock =1;
			mtx->lockPid = pid; //this thread pid
			uprintf("%d a luat lock\n",pid);
			rw_exit_write(&umtxlock);
			KERNEL_UNLOCK();
			return 0;
		}
		else
		{
			struct pidentry * pidelem = malloc(sizeof(struct pidentry),M_TEMP,M_NOWAIT);
			if(pidelem == NULL)
			{
				KERNEL_UNLOCK();
				return -1;
			}
			pidelem->pid = pid;
			SIMPLEQ_INSERT_TAIL(&(mtx->head),pidelem,pidentries);
			
			
			KERNEL_UNLOCK();
			
			rwsleep(pidelem,&umtxlock,PUSER | PNORELOCK,"UMUTEX",0);
		}
	}
	return 0;
}

int umutex_unlock(struct umutex *mtx,pid_t pid)
{
	rw_enter_write(&umtxlock);
	KERNEL_LOCK();
	uprintf("%d a chemat unlock - mtx are lock:%d lockPid:%d\n",pid,mtx->lock,mtx->lockPid);
	if(mtx->lock ==1 && mtx->lockPid==pid)
	{	
		mtx->lock =0;
		mtx->lockPid = 0;
		if(SIMPLEQ_EMPTY(&(mtx->head)))
		{
			rw_exit_write(&umtxlock);
			KERNEL_UNLOCK();
			return 0;
		}
		
		struct pidentry * pidelem = SIMPLEQ_FIRST(&(mtx->head));
		SIMPLEQ_REMOVE_HEAD(&(mtx->head),pidentries);
		
		//poti sa dai wake in lock ???
		uprintf("%d a dat unlock si o sa se trezeasca %d\n\n",pid,pidelem->pid);
		
		KERNEL_UNLOCK();
		wakeup_one(pidelem);
		rw_exit_write(&umtxlock);
		
		free(pidelem,M_TEMP,sizeof(struct pidentry));
		return 0;
	}
	else
	{
		KERNEL_UNLOCK();
		rw_exit_write(&umtxlock);
		return -1;
	}
	
	return 0;
}




