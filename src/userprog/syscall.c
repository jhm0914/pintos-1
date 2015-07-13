#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"

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
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
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
int open (const char *file)
{
	int fd;
	struct file *f = filesys_open(file);		// File Open
	
	if (f == NULL)
	{
		return -1;				// If File isn't exist, Return -1
	}

	fd = process_add_file(f);			// Give File Descriptor to File Object

	return fd;					// Return File Descriptor
}
int filesize (int fd)
{
	struct file *f = process_get_file(fd);		// Search the File Object as use fd

	if (f == NULL)
	{
		return -1;				// If File isn't exist, Return -1
	}

	return file_length(f);				// Return Length of File
}
int read (int fd, void *buffer, unsigned size)
{
	struct file *f = process_get_file(fd);
	char ch, *buffer_ = (char*)buffer;
	int i = 0;

	lock_acquire(&filesys_lock);			// Lock

	f = process_get_file(fd);			// Search the File Object as use fd

	if (fd == 0)					// If fd == 0, Save Keyboard's Input to buffer and return the size that saved to buffer
	{
		for (ch = input_getc(); ch != NULL; ch = input_getc())
		{
			buffer_[i++] = ch;
		}

		lock_release(&filesys_lock);

		return i;
	}
	else						// If fd != 0, Return the Bytes after Save Data of File.
	{
		if (f == NULL)
		{
			lock_release(&filesys_lock);
			return -1;
		}
		else
		{
			i = file_read(f, buffer, size);
			
			lock_release(&filesys_lock);

			return i;
		}
	}
}
int write (int fd, void *buffer, unsigned size)
{
	struct file *f;
	int i;

	lock_acquire(&filesys_lock);			// Lock
	
	f = process_get_file(fd);			// Search the File Object as use fd

	if (fd == 1)					// If fd == 1, Output the data that saved in buffer, and return size of buffer
	{
		putbuf(buffer, size);
		lock_release(&filesys_lock);
		return size;
	}
	else						// If fd != 1, Record the data that saved in buffer, and return size that recorded
	{
		if (f == NULL)
		{
			lock_release(&filesys_lock);
			return -1;
		}
		else
		{
			i = file_write(f, buffer, size);
			
			lock_release(&filesys_lock);

			return i;
		}
	}
}
void seek (int fd, unsigned position)
{
	struct file *f = process_get_file(fd);		// Search the File Object as use fd
	file_seek(f, position);				// seek
}
unsigned tell (int fd)
{
	struct file *f = process_get_file(fd);		// Search the File Object as use fd
	file_tell(f);					// tell
}
void close (int fd)
{
	process_close_file(fd);
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
  lock_init(&filesys_lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int syscall_number, arg[MAX_SYSTEMCALL_ARGUMENT];
	char *ptr;

	/* Get Stack Pointer from interrupt frame */
	void *esp = f->esp;
	
	/* Check Stack Pointer Address */
	check_address(esp);

	/* Get SysCall Number from User Stack */
	syscall_number = *(int*)esp;

	hex_dump(esp, esp, PHYS_BASE - esp, true);

	esp += 16;

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
		get_argument(esp, arg, 1);
		check_address((void*)arg[0]);
		f->eax = open(arg[0]);
		break;
	case SYS_FILESIZE:
		get_argument(esp, arg, 1);
		f->eax = filesize(arg[0]);
		break;
	case SYS_READ:
		get_argument(esp, arg, 3);
		check_address((void*)arg[1]);
		f->eax = read(arg[0], arg[1], arg[2]);
		break;
	case SYS_WRITE:
		get_argument(esp, arg, 3);
		check_address((void*)arg[1]);
		f->eax = write(arg[0], arg[1], arg[2]);
		break;
	case SYS_SEEK:
		get_argument(esp, arg, 2);
		seek(arg[0], arg[1]);
		break;
	case SYS_TELL:
		get_argument(esp, arg, 1);
		f->eax = tell(arg[0]);
		break;
	case SYS_CLOSE:
		get_argument(esp, arg, 1);
		close(arg[0]);
		break;
	}

	thread_exit ();
}
