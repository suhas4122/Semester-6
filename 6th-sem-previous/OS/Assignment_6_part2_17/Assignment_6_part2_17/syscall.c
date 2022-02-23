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
			exit_process(*(p+1));		// call exit_process after checking validity
	}
	else if(syscall_number == SYS_EXEC)
	{
		if(valid_addr(p+1) && valid_addr(*(p+1)))
			f->eax = exec_process(*(p+1));		// call exec_process after checking validity
	}
	else if(syscall_number == SYS_WRITE)
	{
		if( valid_addr(p+7)&&valid_addr(*(p+6)))		// check validity
		{
			if(*(p+5)==1)			// if file descriptor is 1, use putbuf 
			{
				putbuf(*(p+6),*(p+7));		
				f->eax = *(p+7);
			}
			else
			{
				struct list* files = &thread_current()->files;
				struct list_elem *temp;
				int flag = 1;
		      	for (temp = list_begin(files); temp != list_end (files); temp = list_next (temp))	//else iterate on the files list to find file with same fd
		        {
					struct proc_file *fl = list_entry (temp, struct proc_file, elem);
					if(fl->fd == *(p+5))
					{
						flag = 0;
						acquire_filesys_lock();
						f->eax = file_write (fl->ptr, *(p+6), *(p+7));			// use file_write defined in file.c and set the return value
						release_filesys_lock();
					}
		        }
				if(flag) f->eax=-1;
			}
		}
	}
	else if(syscall_number == SYS_CREATE)
	{
		if(valid_addr(p+5)&&valid_addr(*(p+4)))			// check validity and then call filesys_create defined in filesys.c
		{
			acquire_filesys_lock();
			f->eax = filesys_create(*(p+4),*(p+5));
			release_filesys_lock();
		}
	}
	else if(syscall_number==SYS_REMOVE)
	{
		if(valid_addr(p+1)&&valid_addr(*(p+1)))			// check validity and then call filesys_remove defined in filesys.c
		{
			acquire_filesys_lock();
			if(filesys_remove(*(p+1))==NULL)
				f->eax = false;
			else
				f->eax = true;
			release_filesys_lock();
		}
	}
	else if(syscall_number==SYS_OPEN)
	{
		if(valid_addr(p+1)&&valid_addr(*(p+1)))			// check validity 
		{
			acquire_filesys_lock();
			struct file* file_ptr = filesys_open (*(p+1));
			release_filesys_lock();
			if(file_ptr==NULL)				// if NULL ptr, return -1
				f->eax = -1;
			else				// else append the file in the current thread's file list
			{
				struct proc_file *processfile = malloc(sizeof(*processfile));		
				processfile->ptr = file_ptr;
				processfile->fd = thread_current()->fd_count;
				thread_current()->fd_count++;
				list_push_back (&thread_current()->files, &processfile->elem);
				f->eax = processfile->fd;
	
			}
		}
	}
	else if(syscall_number==SYS_FILESIZE)
	{
		if(valid_addr(p+1))			//check validity and iterate on the files list to find the file with same fd
		{							//if found, set the return value as it's length other wise set return value as file_length(NULL);
			acquire_filesys_lock();
			struct list_elem *temp;
			int ff=0;
			for(temp = list_begin(&thread_current()->files); temp!=list_end(&thread_current()->files);temp=list_next(temp))
			{
				struct proc_file *fl = list_entry (temp, struct proc_file, elem);
				if(fl->fd == *(p+1))
				{
					ff = 1;
					f->eax = file_length(fl->ptr);
				}
			}
			if(ff==0) f->eax = file_length(NULL);
			release_filesys_lock();
		}
	}
	else if(syscall_number==SYS_READ)
	{
		valid_addr(p+7);				// check validity
		valid_addr(*(p+6));
		if(*(p+5)==0)		// if file descriptor is 0, use input_getc() 
		{
			uint8_t* buffer = *(p+6);
			int i =0;
			for(;i<*(p+7);i++) buffer[i] = input_getc();
			f->eax = *(p+7);
		}
		else
		{
			int flag=1;
			struct list_elem *temp;
			struct list *files = &thread_current()->files;
			for(temp=list_begin(files);temp!=list_end(files);temp=list_next(temp))	//else iterate on the files list to find file with same fd
			{
				struct proc_file *fl = list_entry(temp,struct proc_file,elem);
				if(fl->fd == *(p+5))
				{
					flag=0;
					acquire_filesys_lock();
					f->eax = file_read (fl->ptr, *(p+6), *(p+7));			// use file_read defined in file.c and set the return value
					release_filesys_lock();
				}
			}
			if(flag) f->eax=-1;
		}
	}
	else if(syscall_number==SYS_CLOSE)
	{
		if(valid_addr(p+1))			//check validity
		{
			acquire_filesys_lock();
			struct list_elem *temp;
			struct proc_file *fl;
			struct list * files = &thread_current()->files;
			for(temp=list_begin(files);temp!=list_end(files);temp=list_next(temp))		//iterate on files list to find the file with same fd
			{
				fl = list_entry(temp, struct proc_file, elem);
				if(fl->fd == *(p+1))				// renove the file with same fd
				{
					file_close(fl->ptr);
					list_remove(temp);
				}
			}
			free(fl);		//free the file pointer
			release_filesys_lock();
		}
	}
}
