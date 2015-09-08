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
	struct thread *cur;

	cur = thread_current();					// Get Running Thread

	cur->exit_status = status;				// Save Exit Status

	printf("%s: exit(%d)\n", cur->name, status);		// Output Exit Message

	thread_exit();						// Exit Thread
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
	int pid;
	struct thread *t;

	/* Create Process as call process_execute() */
	pid = process_execute(cmd_line);

	/* Search the Process Descriptor of Child Process Created */
	t = get_child_process(pid);

	/* Wait for Load of Child Process's Program */
	sema_down(&t->load_sema);

	if (!t->load_success)
	{
		return -1;
	}
	else if(t->load_success)
	{
		return pid;
	}
}
int wait(tid_t tid)
{
	return process_wait(tid);
}
int open (const char *file)
{
	int fd;
	struct file *f;

	lock_acquire(&filesys_lock);			// Lock

	f = filesys_open(file);				// File Open
	
	if (!f)
	{
		lock_release(&filesys_lock);		// Unlock
		return -1;				// If File isn't exist, Return -1
	}

	fd = process_add_file(f);			// Give File Descriptor to File Object

	lock_release(&filesys_lock);			// Unlock

	return fd;					// Return File Descriptor
}
int filesize (int fd)
{
	struct file *f;
	int length;

	lock_acquire(&filesys_lock);			// Lock

	f = process_get_file(fd);			// Search the File Object as use fd

	if (!f)
	{
		lock_release(&filesys_lock);		// Unlock
		return -1;				// If File isn't exist, Return -1
	}

	length = file_length(f);			// Save File Length

	lock_release(&filesys_lock);			// Unlock

	return length;					// Return Length of File
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
		if (!f)
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
		if (!f)
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
	struct file *f;

	lock_acquire(&filesys_lock);			// Lock

	f = process_get_file(fd);			// Search the File Object as use fd

	file_seek(f, position);				// seek

	lock_release(&filesys_lock);			// Unlock
}
unsigned tell (int fd)
{
	struct file *f;
	unsigned off_pos;

	lock_acquire(&filesys_lock);			// Lock
	
	f = process_get_file(fd);			// Search the File Object as use fd

	off_pos = file_tell(f);				// Get Off Position

	lock_release(&filesys_lock);			// Unlock

	return off_pos;					// tell
}
void close (int fd)
{
	process_close_file(fd);
}

struct vm_entry *check_address (void *addr, void* esp)
{
	uint32_t address = (uint32_t)addr;

	if (!(0x8048000 < address && address < 0xc0000000))
	{
		exit(-1);
	}

	return find_vme(addr);
}

void check_valid_buffer (void *buffer, unsigned size, void *esp, bool to_write)
{
	if (to_write)
	{
		int i;
		struct vm_entry *vme;

		for (i = 0; i<size; i++)
		{
			vme = check_address(buffer+i ,esp);

			if (vme != NULL)
			{
				if (vme->writable == false)
				{
					exit(-1);
				}
			}
			else
			{
				exit(-1);
			}
		}
	}
	else
	{
		exit(-1);
	}
}

void check_valid_string (const void *str, void *esp)
{
	int i, size = strlen((char*)str);
	struct vm_entry *vme;

	if (check_address(str,esp) != check_address(str+size,esp))
	{
		for (i = 0; i<size; i++)
		{
			vme = check_address(str+i, esp);
			if (vme == NULL)
			{
				exit(-1);
			}
		}
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
	check_address(esp, esp);

	/* Get SysCall Number from User Stack */
	syscall_number = *(int*)esp;

	// If 1 < argc : esp + 4 + (argc*4)
	// else if argc == 1 : esp
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
		//check_address((void*)arg[0]);
		check_valid_string((void*)arg[0], esp);
		f->eax = exec((const char *)arg[0]);
		unpin_string((void*)arg[0]);
		break;
	case SYS_WAIT:
		get_argument(esp, arg, 1);
		f->eax = wait(arg[0]);
		break;
	case SYS_CREATE:
		get_argument(esp+12, arg, 2);
		//check_address((void*)arg[0]);
		check_valid_string((void*)arg[0], esp);
		f->eax = create(arg[0], arg[1]);
		unpin_string((void*)arg[0]);
		break;
	case SYS_REMOVE:
		get_argument(esp, arg, 1);
		//check_address((void*)arg[0]);
		check_valid_string((void*)arg[0], esp);
		f->eax = remove(arg[0]);
		unpin_string((void*)arg[0]);
		break;
	case SYS_OPEN:
		get_argument(esp, arg, 1);
		//check_address((void*)arg[0]);
		check_valid_string((void*)arg[0], esp);
		f->eax = open(arg[0]);
		unpin_string((void*)arg[0]);
		break;
	case SYS_FILESIZE:
		get_argument(esp, arg, 1);
		f->eax = filesize(arg[0]);
		break;
	case SYS_READ:
		get_argument(esp+16, arg, 3);
		//check_address((void*)arg[1]);
		check_valid_buffer((void*)arg[1], *(unsigned*)arg[2], esp, true);
		f->eax = read(arg[0], arg[1], arg[2]);
		unpin_buffer((void*)arg[1], *(unsigned*)arg[2]);
		break;
	case SYS_WRITE:
		get_argument(esp+16, arg, 3);
		//check_address((void*)arg[1]);
		check_valid_string((void*)arg[1], esp);
		f->eax = write(arg[0], arg[1], arg[2]);
		unpin_string((void*)arg[1]);
		break;
	case SYS_SEEK:
		get_argument(esp+12, arg, 2);
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
	default:
		thread_exit();
		break;
	}
	unpin_ptr(esp);
}
