#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#define MAX_SYSTEMCALL_ARGUMENT 10

static void syscall_handler (struct intr_frame *);
void check_address (void *addr);
void get_argument (void *esp, int *arg, int count);

/********************************** System Call's Prototype *****************************************/
void halt (void);
void exit (int status);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
tid_t exec (const char *cmd_line);
int wait (tid_t tid);
/****************************************************************************************************/

/**********************************************************************************/
void halt (void)
{
	shutdown_power_off();
}
void exit (int status)
{
	struct thread *cur = thread_current();
	cur->exit_status = status;
	printf("%s : exit(%d)\n", cur->name, status);
	thread_exit();
}
bool create (const char *file, unsigned initial_size)
{
	return filesys_create(file, initial_size);
}
bool remove (const char *file)
{
	return filesys_remove(file);
}
tid_t exec (const char *cmd_line)
{
	/* Create Process as call process_execute() */
	int pid = process_execute(cmd_line);

	/* Search the Process Descriptor of Child Process Created */
	struct thread *t = get_child_process(pid);

	/* Wait for Load of Child Process's Program */
	sema_down(&t->load_sema);

	if (t->load_success == false)
	{
		return -1;
	}
	else if(t->load_success == true)
	{
		return pid;
	}
}
int wait(tid_t tid)
{
	process_wait(tid);
}


void check_address (void *addr)
{
	uint32_t address = (uint32_t)addr;

	if (!(0x8048000 <= address && address <=0xc0000000))
	{
		printf("Invalid Memory Access : %p\n", address);
		exit(-1);
	}
}
void get_argument (void *esp, int *arg, int count)
{
	int i;

	for (i = 0; i<count; i++)
	{
		esp = esp + 4;
		check_address(esp);
		arg[i] = *(int*)esp;
	}
}
/**********************************************************************************/

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int syscall_number, arg[MAX_SYSTEMCALL_ARGUMENT];

	/* Get Stack Pointer from interrupt frame */
	void *esp = f->esp;
	
	/* Check Stack Pointer Address */
	check_address(esp);

	/* Get SysCall Number from User Stack */
	syscall_number = *(int*)esp;

	switch (syscall_number)
	{
	case SYS_HALT:
		halt();
		break;
	case SYS_EXIT:
		get_argument(esp, arg, 1);
		exit(arg[0]);
		break;
	case SYS_EXEC:
		get_argument(esp, arg, 1);
		check_address((void*)arg[0]);
		f->eax = exec((const char *)arg[0]);
		break;
	case SYS_WAIT:
		get_argument(esp, arg, 1);
		f->eax = wait(arg[0]);
		break;
	case SYS_CREATE:
		get_argument(esp, arg, 2);
		check_address((void*)arg[0]);
		f->eax = create(arg[0], arg[1]);
		break;
	case SYS_REMOVE:
		get_argument(esp, arg, 1);
		check_address((void*)arg[0]);
		f->eax = remove(arg[0]);
		break;
	case SYS_OPEN:
		break;
	case SYS_FILESIZE:
		break;
	case SYS_READ:
		break;
	case SYS_WRITE:
		break;
	case SYS_SEEK:
		break;
	case SYS_TELL:
		break;
	case SYS_CLOSE:
		break;
	}

	thread_exit ();
}
