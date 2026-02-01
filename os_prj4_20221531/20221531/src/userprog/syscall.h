#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "lib/user/syscall.h"
#include "threads/thread.h"

typedef int pid_t;

void syscall_init (void);

/* Projects 1 and 2 system calls */
void halt (void) __attribute__ ((noreturn));
void exit (int status) __attribute__ ((noreturn));
pid_t exec (const char *cmd_line);
int wait (pid_t pid);
bool create (const char *file, unsigned initial_size);
bool remove (const char *file);
int open (const char *file);
int filesize (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);

//project 4
mapid_t mmap (int fd, void *addr);
void munmap (mapid_t mapping);
extern struct lock filesys_lock;

#endif /* userprog/syscall.h */
