#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "list.h"
#include "process.h"
static void syscall_handler (struct intr_frame *);

struct proc_file {			// structure for a file process
	struct file* ptr;
	int fd;
	struct list_elem elem;
};

void exit_process(int status)	//to exit a process
{
	struct list_elem *e;
	e = list_begin(&thread_current()->parent->childs);
  	while(e != list_end (&thread_current()->parent->childs))       	//in the childs of the parent, find the child with same id as that of current
    {
      struct child *f = list_entry (e, struct child, elem);
      if(f->tid == thread_current()->tid)
      {
      	f->flag = true;			// flag that indicates a child process ran atleast once
      	f->error_exit = status;
      }
      e = list_next (e);
    }
	thread_current()->error_exit = status;
	if(thread_current()->parent->waitingon == thread_current()->tid)		// turning up semaphore since the child process has exited
		sema_up(&thread_current()->parent->child_lock);
	thread_exit();
}

int exec_process(char *file_name)		//to execute a process
{
	acquire_filesys_lock();
	char * fn_cp = malloc (strlen(file_name)+1);
	strlcpy(fn_cp, file_name, strlen(file_name)+1);
	char * save_ptr;
	fn_cp = strtok_r(fn_cp," ",&save_ptr);
	struct file* f = filesys_open (fn_cp);
	if(f==NULL)
	{
		release_filesys_lock();
		return -1;
	}
	else
	{
		file_close(f);
		release_filesys_lock();
		return process_execute(file_name);
	}
}
void* valid_addr(const void *vaddr)
{
	if (is_user_vaddr(vaddr))
	{
		void *ptr = pagedir_get_page(thread_current()->pagedir, vaddr);
		if (ptr==NULL)
		{
			exit_process(-1);
			return 0;
		}
		return ptr;
	}	
	exit_process(-1);
	return 0;
}


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 		//based on the passed syscall number, execute that call.
{
	int * p = f->esp;
	if(!valid_addr(p))
			thread_exit ();
	int syscall_number = *p;
	if(syscall_number == SYS_EXIT)
	{
		if(valid_addr(p+1))
			exit_process(*(p+1));
	}
	else if(syscall_number == SYS_EXEC)
	{
		if(valid_addr(p+1) && valid_addr(*(p+1)))
			f->eax = exec_process(*(p+1));
	}
	else if(syscall_number == SYS_WRITE)
	{
		if( valid_addr(p+7)&&valid_addr(*(p+6)))
		{
			if(*(p+5)==1)
			{
				putbuf(*(p+6),*(p+7));
				f->eax = *(p+7);
			}
			else
			{
				struct list* files = &thread_current()->files;
				int fd = *(p+5);
				struct proc_file* fptr;
				struct list_elem *e;
				e = list_begin (files);
				int flag = 1;
		      	for (; e != list_end (files);e = list_next (e))
		        {
		          struct proc_file *ff = list_entry (e, struct proc_file, elem);
		          if(ff->fd == fd)
		          	{
		          		fptr = ff;
		          		flag = 0;
		          		acquire_filesys_lock();
						f->eax = file_write (fptr->ptr, *(p+6), *(p+7));
						release_filesys_lock();
		          	}
		        }
				if(flag)
					f->eax=-1;
			}
		}
	}
}


void close_file(struct list* files, int fd)			//close and remove a file from files lise
{
	struct list_elem *e;
	struct proc_file *f;
	e = list_begin (files);
	while(e != list_end (files))
	{
	  f = list_entry (e, struct proc_file, elem);
	  if(f->fd == fd)
	  {
	  	file_close(f->ptr);
	  	list_remove(e);
	  }
	   e = list_next (e);
	}
    free(f);
}

void close_all_files(struct list* files)		//close and remove all files from files lise
{
	struct list_elem *e;
	while(!list_empty(files))
	{
		e = list_pop_front(files);
		struct proc_file *f = list_entry (e, struct proc_file, elem);
	  	file_close(f->ptr);
	  	list_remove(e);
	  	free(f);
	} 
}