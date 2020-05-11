#include <stdlib.h>
#include <string.h>
#include "vfs.h"
//#include <fat.h>

/***************************************************************************
 * File name manipulation utilities
 ***************************************************************************/
// strdup
//   duplicates a string (warning: allocates memory!)
char* strdup(const char *str)
{
	size_t siz;
	char *copy;

	siz = strlen(str) + 1;
	if ((copy = os_alloc(siz)) == NULL)
		return NULL;
	memcpy(copy, str, siz);
	return copy;
}

// -------------------- dirname ------------------------------------------
// returns parent's name (everything before the last '/')
char * dirname(char *path)
{
	char *dname=strdup(path);
	char *slash=NULL;
	char *p=dname;

	while(*p){
		if(*p=='/') slash=p;
		p++;
	}
  
	if(slash) *slash=0;
	if(*dname==0) strcpy(dname, "/");
	return dname;
}
  
// -------------------- basename ------------------------------------------
// returns the filename (everything after the last '/')
char * basename(char *path)
{
	char *p=path;
	char *slash=NULL;
	while(*p){
		if(*p=='/') slash=p;
		p++;
	}
	if(slash) return slash+1;
	else return path;
}

/***************************************************************************
 * Opened file descriptor array with Device associated to file descriptor
 ***************************************************************************/
FileObject * opened_fds[MAX_OPENED_FDS];

/***************************************************************************
 * Registered device table
 ***************************************************************************/
extern Device* device_table[];

/* dev_lookup
 *   search for a device represented by its name in the device table
 */
static Device *dev_lookup(char *path)
{
	char * dir = dirname(path);
	
	if (!strcmp(dir,"/dev")) {
		free(dir);
	    int i=0;
	    char *devname = basename(path);
	    
    	Device *dev=device_table[0];
    	while (dev) {
        	if (!strcmp(devname,dev->name))
            	return dev;
        	dev=device_table[++i];
    	}
    } else os_free(dir);
    return NULL;
}

#ifdef _FAT_H_
/***************************************************************************
 * FAT Object
 ***************************************************************************/
extern Device dev_fs;
static FatFS *fs=NULL;

int mount()
{
	return dev_fs.init(&dev_fs);
	return 0;
}
#endif
/***************************************************************************
 * Generic device functions
 ***************************************************************************/
Semaphore *vfs_mutex;

/* open
 *   returns a file descriptor for path name
 */
int open(char *path, unsigned int flags)
{
    int fd;
    // operations must be atomic
    sem_p(vfs_mutex);
    // check if we can open something
    for(fd = 0; fd < MAX_OPENED_FDS; fd++)
    {
        // soon as a free fd has been found use it
        if(opened_fds[fd] == NULL)
        {
            FileObject* f = (FileObject*) os_alloc(sizeof(FileObject));
            memset(f, 0, sizeof(FileObject));
            // check if os_alloc succeeded
            if(f)
            {
                f->name = strdup(path);
                f->flags = flags;
                // check if it's a hardware device or the /dev directory
                if(!strcmp("/dev", f->name))
                {
                    f->flags |= F_IS_DEVDIR;
                    break; // goto end:
                }
                // check if its a hardware device or a regular file
                if(!strncmp("/dev/", f->name, 5)) f->flags |= F_IS_DEV;
                // find device
                f->dev = dev_lookup(path);
                if(f->dev)
                {
                    // open and check if it fails or succeed
                    if((fd = f->dev->open(f))>0) opened_fds[fd] = f;
                    break; // goto end:
                }
                // if device finding failed don't forget to free allocated memory
                os_free(f->name); // allocated in strdup
                os_free(f);
            }
            // something must have fail if it goes here
            fd = -1;
            break; // goto end:
        }
    }
    // end:
    sem_v(vfs_mutex);
    return fd;
}
/* int open(char *path, unsigned int flags)
{
	for(int fd=0; fd < MAX_OPENED_FDS; fd++)
	{
		// as soon as an empty space is found, we use it
		if(opened_fds[fd] == NULL)
		{
			FileObject* f = (FileObject*)os_alloc(sizeof(FileObject));
			// check if malloc succeeded
			if(f)
			{
				f->name = strdup(path);
				f->offset = 0;
				f->flags = flags;
				f->dev = NULL;
				if(strcmp(f->name, "/dev") == 0)
				{
					return fd;
				}
				f->dev = dev_lookup(path);
				if(f->dev)
				{
					f->flags |= F_IS_DEV;
				}
				if(f->dev && f->dev->open && f->dev->open(f))
				{
					opened_fds[fd] = f;
					return fd;
				}
				else
				{
					os_free(f);
					return -1;
				}
			}
			break;
		}
	}
    return -1;
}*/

/* close
 *   close the file descriptor
 */
int close(int fd)
{
    FileObject* f = opened_fds[fd];
    // free every dynamically allocated fields
    os_free(f->name);
    os_free(f);
    // mark the fd cell as free
    opened_fds[fd] = NULL;
    return -1;
}

/* read
 *   read len bytes from fd to buf, returns actually read bytes
 */
int read(int fd, void *buf, size_t len)
{
    FileObject* f = opened_fds[fd];
    if(f)
    {
        // dev can't be null because open would have failed instead
        Device* dev = f->dev;
        sem_v(vfs_mutex);
        if(dev->read) return dev->read(f, buf, len);
    }
    return -1;
}

/* write
 *   write len bytes from buf to fd, returns actually written bytes
 */
int write(int fd, void *buf, size_t len)
{
    FileObject* f = opened_fds[fd];
    if(f)
    {
        // dev can't be null because open would have fail instead
        Device* dev = f->dev;
        if(dev->write)
        {
        	sem_v(vfs_mutex);
        	return dev->write(f, buf, len);
        }
    }
    return -1;
}

/* ioctl
 *   set/get parameter for fd
 */
int ioctl(int fd, int op, void** data)
{
	/* A COMPLETER */

    return -1;
}

/* lseek
 *   set the offset in fd
 */
int lseek(int fd, unsigned int offset)
{
	/* A COMPLETER */

	return -1;	
}

#ifdef _FAT_H_
/***************************************************************************
 * Directory handling functions
 ***************************************************************************/
int dev_fs_next_dir(DIR *dir);

int readdir(int fd, DIR **dir)
{
	FileObject *f=opened_fds[fd];
	
	if (f) {
		if (f->flags & F_IS_ROOTDIR) {
			f->flags &=~F_IS_ROOTDIR;
			strcpy(f->dir->entry, "dev");
			f->dir->entry_isdir=1;
			f->dir->entry_size=0;
			*dir=f->dir;
			return 0;
		} else if (f->flags & F_IS_DEVDIR) {
			if (device_table[f->offset]) {
				strcpy(f->dir->entry, device_table[f->offset]->name);
				f->dir->entry_isdir=0;
				f->dir->entry_size=0;
				*dir=f->dir;
				f->offset++;
				return 0;
			}
		} else if ((f->flags & F_IS_DIR) && dev_fs_next_dir(f->dir)) {
			*dir=f->dir;
			return 0;
		}
	}
	return -1;
}
#endif
