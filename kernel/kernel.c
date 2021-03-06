#include <stdlib.h>
#include "kernel.h"
#include "list.h"

#ifndef NULL
#define NULL 0
#endif

/*****************************************************************************
 * Global variables
 *****************************************************************************/

static uint32_t id = 1;
Task *tsk_running = NULL; /* pointer to ready task list : the first
                                     node is the running task descriptor */
Task *tsk_prev = NULL;
Task *tsk_sleeping = NULL; /* pointer to sleeping task list */

/*****************************************************************************
 * SVC dispatch
 *****************************************************************************/
/* sys_add
 *   test function
 */
int sys_add(int a, int b)
{
	return a + b;
}

/* syscall_dispatch
 *   dispatch syscalls
 *   n      : syscall number
 *   args[] : array of the parameters (4 max)
 */
int32_t svc_dispatch(uint32_t n, uint32_t args[]) // répartir les appels systèmes (genre read, write,..)
{
	switch (n)
	{
	case 0:
		return sys_add((int)args[0], (int)args[1]);

	case 1:
		return (int32_t)malloc(args[0]);

	case 2:
		free((void *)args[0]);
		return 0;

	case 3:
		return sys_os_start();

	case 4:
		return sys_task_new((TaskCode)args[0], args[1]);

	case 5:
		return sys_task_id();

	case 6:
		return sys_task_wait((uint32_t)args[0]);

	case 7:
		return sys_task_kill();

	case 8:
		return (int32_t)sys_sem_new((int32_t)args[0]);

	case 9:
		return sys_sem_p((Semaphore *)args[0]);

	case 10:
		return sys_sem_v((Semaphore *)args[0]);
	}
	return -1;
}

void sys_switch_ctx()
{
	SCB->ICSR |= 1 << 28; // set PendSV to pending
}
/*****************************************************************************
 * Round robin algorithm
 *****************************************************************************/
#define SYS_TICK 10 // system tick in ms

uint32_t sys_tick_cnt = 0;

/* tick_cb
 *   system tick callback: task switching, ...
 */
void sys_tick_cb()
{
	Task* tsk = tsk_sleeping;

	tsk_prev = tsk_running;
	tsk_running->status = TASK_READY;

	for(uint32_t i = 0; i < list_size(tsk_sleeping); i++)
	{
		if(!(tsk->delay-=SYS_TICK))
		{
			tsk_sleeping = list_remove_head(tsk_sleeping, &tsk);
			tsk_running = list_insert_head(tsk_running, tsk);
			tsk_running->status = TASK_RUNNING;
		}
	}
	// if no task woken up, perform a standard context switch
	if(tsk_prev == tsk_running)
	{
		tsk_running = tsk_running->next;
		tsk_running->status = TASK_RUNNING;
	}
	sys_switch_ctx();
}

/* void sys_tick_cb()
{
    Task *t;

    tsk_prev = tsk_running;
    tsk_running -> status = TASK_READY;

    int lsize = list_size(tsk_sleeping);


    while( tsk_sleeping && lsize)
    {
        tsk_sleeping -> delay -= SYS_TICK;
        if(tsk_sleeping-> delay <=0)
        {
            tsk_sleeping = list_remove_head(tsk_sleeping,&t);
            tsk_running = list_insert_tail(tsk_running,t);
            t -> status = TASK_READY;
        }
        if(tsk_sleeping) tsk_sleeping = tsk_sleeping -> next;
        lsize--;
    }

    tsk_running = tsk_running->next;
    tsk_running -> status = TASK_RUNNING;

    sys_switch_ctx();


//    list_display(tsk_running);
} */

void SysTick_Handler(void)
{
	sys_tick_cnt++;

	if (sys_tick_cnt == SYS_TICK)
	{
		sys_tick_cnt = 0;
		sys_tick_cb();
	}
}

/*****************************************************************************
 * General OS handling functions
 *****************************************************************************/

/* sys_os_start
 *   start the first created task
 */
int32_t sys_os_start()
{
	tsk_running->status = TASK_RUNNING;
	sys_switch_ctx(); // generer interruption systeme de type pendSV

	// Reset BASEPRI
	__set_BASEPRI(0);

	// Set systick reload value to generate 1ms interrupt
	SysTick_Config(SystemCoreClock / 1000U);
	return 0;
}

/*****************************************************************************
 * Task handling functions
 *****************************************************************************/

/* sys_task_new
 *   create a new task :
 *   func      : task code to be run
 *   stacksize : task stack size
 *
 *   Stack frame:
 *      |    xPSR    |
 *      |     PC     |
 *      |     LR     |
 *      |     R12    |    ^
 *      |     R3     |    ^
 *      |     R2     |    | @++
 *      |     R1     |
 *      |     R0     |
 *      +------------+
 *      |     R11    |
 *      |     R10    |
 *      |     R9     |
 *      |     R8     |
 *      |     R7     |
 *      |     R6     |
 *      |     R5     |
 *      |     R4     |
 *      +------------+
 *      | EXC_RETURN |
 *      |   CONTROL  | <- sp
 *      +------------+
 */
int32_t sys_task_new(TaskCode func, uint32_t stacksize) // create new tasks
{
	// get a stack with size multiple of 8 bytes (cf. page 13)
	uint32_t size = stacksize > 96 ? 8 * (((stacksize - 1) / 8) + 1) : 96;

	Task *t = (Task *)malloc(sizeof(Task) + size); // pointe sur le bas du bloc memoire

	if (t) // si bloc memoire alloué (ie mallock successful)
	{
		t->id = id++;					// identifier
		t->status = TASK_READY;			// task status : running, ready, ...
		t->splim = (uint32_t *)(t + 1);  // stack limit is at t+1*sizeof(Task), but t is Task* so => t+1
		// t->splim = (uint32_t*)((uint8_t*)(t) + sizeof(Task));
		t->sp = t->splim + (size >> 2) - 18; // get the top of the stack & divide size by 4 to get it in multiple of 32 bits chunks format
		// save stack frame // -18 car sinon on sort de la pile
		t->sp[17] = 0x01000000; // set reset in xPSR
		t->sp[16] = (uint32_t)func;
		// t->sp[15]= (uint32_t)task_kill;  // LR
		t->sp[1] = 0xFFFFFFFD; // EXC_RETURN code to return in thread mode and use PSP
		t->sp[0] = 0x1;		   // CONTROL thread mode privileged level => unprivileged
		t->delay = 0;		   // waiting delay (for timeouts)
		tsk_running = list_insert_tail(tsk_running, t); // on la fout dans la liste

		return t->id;
	}
	return -1;
}

/* sys_task_kill
 *   kill oneself
 */
int32_t sys_task_kill()
{
    Task* old_tsk_running = NULL;
    /* replace tsk_running by next task and remove tsk_running from list
     * but keep the deleted tsk_running in old_tsk_running
     * to free it from memory
     */
    tsk_running = list_remove_head(tsk_running, &old_tsk_running);
    tsk_running->status = TASK_RUNNING;
    // free the removed task memory space
    free(old_tsk_running);
    // switch context to run tsk_running
    sys_switch_ctx();

    return 0;
}

/* int32_t sys_task_kill()
{
    Task* t;
    tsk_running = list_remove_head(tsk_running, &t);
    tsk_prev=NULL;
    free(t);
    tsk_running -> status = TASK_RUNNING;
    sys_switch_ctx();

    return 0;
} */

/* sys_task_id
 *   returns id of task
 */
int32_t sys_task_id()
{
	if (tsk_running)
		return tsk_running->id;
	else
		return -1;
}

/* sys_task_yield
 *   run scheduler to switch to another task
 */
int32_t sys_task_yield()
{

	return -1;
}

/* task_wait
 *   suspend the current task until timeout
 */
int32_t sys_task_wait(uint32_t ms)
{
    // if no token available
    if(ms)
    {
        Task* tsk;
        // update the current task to sleep
        tsk_running->status = TASK_SLEEPING;
        tsk_running->delay = ms;
        tsk_prev = tsk_running;
        // perform the list transfer
        tsk_running = list_remove_head(tsk_running, &tsk);
        tsk_sleeping = list_insert_tail(tsk_sleeping, tsk);
        tsk_running -> status = TASK_RUNNING;
        // update the new task to run
        sys_switch_ctx();
    }
    return 0;
}

/*****************************************************************************
 * Semaphore handling functions
 *****************************************************************************/

/* sys_sem_new
 *   create a semaphore
 *   init    : initial value
 */
Semaphore * sys_sem_new(int32_t init)
{
    Semaphore* sem = (Semaphore*) malloc(sizeof(Semaphore));

    if(sem)
    {
        sem->count = init;
        sem->waiting = NULL;
        return sem;
    }
    return sem;
}

/* sys_sem_p
 *   take a token
 */
int32_t sys_sem_p(Semaphore *sem)
{
	// enlever la tache courante de tskrunning
	// et placer la tache dans sem waiting
	Task* tsk;
	if(sem)
	{
		sem->count--;
		if(sem->count < 0)
		{
			tsk_running->status = TASK_WAITING;
			tsk_prev = tsk_running;

			tsk_running = list_remove_head(tsk_running, &tsk);
			sem->waiting = list_insert_tail(sem->waiting, tsk);
			tsk_running->status = TASK_RUNNING;
			sys_switch_ctx();
		}
		return 0;
	}
	return -1;
}

/* sys_sem_v
 *   release a token
 */
int32_t sys_sem_v(Semaphore *sem)
{
	// tache en tete de sem waiting
	// a placer dans tsk_running (en tete)
	Task* tsk;
	if(sem)
	{
		sem->count++;
		if(sem->waiting)
		{
			tsk_running->status = TASK_READY;
			tsk_prev = tsk_running;
			sem->waiting = list_remove_head(sem->waiting, &tsk);
			tsk_running = list_insert_head(tsk_running, tsk);
			tsk_running->status = TASK_RUNNING;
			sys_switch_ctx();
		}
		return 0;
	}
	return -1;
}
