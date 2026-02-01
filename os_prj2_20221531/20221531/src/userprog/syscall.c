#include "syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

#include "threads/vaddr.h"
#include "devices/shutdown.h" 
#include "userprog/process.h"   
#include "devices/input.h"    

#include <string.h>
#include "filesys/filesys.h" 
#include "filesys/file.h"    



static struct lock filesys_lock; // 동기화 위해 lock 추가

static void syscall_handler (struct intr_frame *);

void
check_addr(void *addr) {
  if (false == is_user_vaddr(addr)) exit(-1);
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock); // file system lock
}


// 스택에서 n번째 정수 인자를 가져오는 함수
static int get_int_arg(struct intr_frame *f, int offset) {
    void *arg_ptr = f->esp + offset;
    check_addr(arg_ptr); // 주소 유효성 검사
    return *(int *)arg_ptr;
}

// 스택에서 n번째 포인터 인자를 가져오는 함수
static void* get_ptr_arg(struct intr_frame *f, int offset) {
    void *arg_ptr = f->esp + offset;
    check_addr(arg_ptr); // 주소 유효성 검사
    check_addr(*(void **)arg_ptr); // 포인터가 가리키는 주소도 검사
    return *(void **)arg_ptr;
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  // printf ("system call!\n");
  
  check_addr(f->esp);
  int syscall_number = *(int *)(f->esp);

    switch (syscall_number) {
    case SYS_HALT:
      halt();
      break;

    case SYS_EXIT:
      exit(get_int_arg(f, 4));
      break;

    case SYS_EXEC:
      f->eax = exec(get_ptr_arg(f, 4));
      break;

    case SYS_WAIT:
      f->eax = wait(get_int_arg(f, 4));
      break;

    // case SYS_READ:
    //   f->eax = read(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
    //   break;

    // case SYS_WRITE:
    //   f->eax = write(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
    //   break;
    case SYS_FIBONACCI:
      f->eax = fibonacci(get_int_arg(f, 4));
      break;

    case SYS_MAX_OF_FOUR_INT:
      f->eax = max_of_four_int(get_int_arg(f, 4), 
                               get_int_arg(f, 8), 
                               get_int_arg(f, 12), 
                               get_int_arg(f, 16)); 
      break;

    // prj 2 system calls
    case SYS_CREATE:
      f->eax = create(get_ptr_arg(f,4), get_int_arg(f,8));
      break;
    case SYS_REMOVE:
      f->eax = remove(get_ptr_arg(f, 4));
      break;
    case SYS_OPEN:
      f->eax = open(get_ptr_arg(f, 4));
      break;
    case SYS_FILESIZE:
      f->eax = filesize(get_int_arg(f, 4));
      break;
    case SYS_READ:
      f->eax = read(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
      break;
    case SYS_WRITE:
      f->eax = write(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
      break;
    case SYS_SEEK:
      seek(get_int_arg(f, 4), get_int_arg(f, 8));
      break;
    case SYS_TELL:
      f->eax = tell(get_int_arg(f, 4));
      break;
    case SYS_CLOSE:
      close(get_int_arg(f, 4));
      break;  
  }
  // thread_exit ();
}
void halt (void) {
  shutdown_power_off();
}

void exit (int status) {
  printf("%s: exit(%d)\n", thread_current()->name, status);
  thread_current()->exit_status = status; 

  for (int i = 2; i < 128; i++) {
    if (thread_current()->FD[i] != NULL) {
      close(i);
    }
  }
  thread_exit();
}

int exec (const char *cmd_lime) {
  return process_execute(cmd_lime);
}

int wait (int pid) {
  return process_wait(pid);
}

// Reliability check needed
// int read (int fd, void *buffer, unsigned size) {
//   if (!fd) {
//     for (unsigned i = 0; i < size; i++) {
//       *((uint8_t *)buffer + i) = input_getc();
//     }
//     return size;
//   }
//   return -1;
// }

// int write (int fd, const void *buffer, unsigned size) {

//   if (fd == 1) {
//     putbuf(buffer, size);
//     return size;
//   }
//   return -1; 
// }

int fibonacci(int n) {
  if(n < 0){
    return 0;
  }
  if(n <= 1){
    return n;
  }

  return fibonacci(n-1) + fibonacci(n-2);
}

int max_of_four_int(int a, int b, int c, int d) {
  int MAX = a;

  if(b > MAX){
    MAX = b;
  }

  if(c > MAX){
    MAX = c;
  }

  if(d > MAX){
    MAX = d;
  }

  return MAX;
}

bool create (const char *file, unsigned initial_size)
{
  check_addr(file);
  if (file == NULL) exit(-1);

  filesys_create(file, initial_size);

}

bool remove(const char *file) {
  check_addr(file);
  if (file == NULL) exit(-1);

  filesys_remove(file);
}

int open(const char *file) {
  check_addr(file);
  if (file == NULL) return -1;

  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  if (f == NULL) {
    lock_release(&filesys_lock);
    return -1;
  }

  struct thread *cur = thread_current();
  for (int i = 2; i < 128; i++) {
    if (cur->FD[i] == NULL) {
      // 실행 중인 자신의 실행 파일인 경우 쓰기 금지
      if (strcmp(cur->name, file) == 0) {
        file_deny_write(f);
      }
      cur->FD[i] = f;
      lock_release(&filesys_lock);
      return i;
    }
  }

  file_close(f);
  lock_release(&filesys_lock);
  return -1;
}

int filesize(int fd) {
    if (fd < 2 || fd >= 128) return -1;
    
    struct file *f = thread_current()->FD[fd];
    if (f == NULL) return -1;
    return file_length(f);

}

int read (int fd, void *buffer, unsigned size)
{
  // 1. 버퍼 주소 유효성 검사
  check_addr(buffer);

  // 2. fd 값에 따른 분기 처리
  if (fd == 0) { // STDIN: 키보드 입력
    for (unsigned i = 0; i < size; i++) {
      *((uint8_t *)buffer + i) = input_getc();
    }
    return size;
  }
  
  // 파일 디스크립터가 유효한 범위에 있는지 확인
  if (fd < 2 || fd >= 128) {
    return -1;
  }

  // 3. 실제 파일에서 읽기
  struct file *f = thread_current()->FD[fd];
  if (f == NULL) { // 파일이 열려있는지 확인
    return -1;
  }
  
  lock_acquire(&filesys_lock);
  int bytes_read = file_read(f, buffer, size);
  lock_release(&filesys_lock);
  
  return bytes_read;
}

int write (int fd, const void *buffer, unsigned size)
{
  // 1. 버퍼 주소 유효성 검사
  check_addr(buffer);

  // 2. fd 값에 따른 분기 처리
  if (fd == 1) { // STDOUT: 모니터 출력
    putbuf(buffer, size);
    return size;
  }
  
  // 파일 디스크립터가 유효한 범위에 있는지 확인
  if (fd < 2 || fd >= 128) {
    return -1;
  }

  // 3. 실제 파일에 쓰기
  struct file *f = thread_current()->FD[fd];
  if (f == NULL) { // 파일이 열려있는지 확인
    return -1;
  }

  lock_acquire(&filesys_lock);
  int bytes_written = file_write(f, buffer, size);
  lock_release(&filesys_lock);
  
  return bytes_written;
}

void seek(int fd, unsigned position) {
    if (fd < 2 || fd >= 128) return;

    struct file *f = thread_current()->FD[fd];
    if (f == NULL) return;
    file_seek(f, position);
}

unsigned tell(int fd) {
    if (fd < 2 || fd >= 128) return 0; // Or some error indicator

    struct file *f = thread_current()->FD[fd];
    if (f == NULL) return 0; // Or some error indicator


    unsigned position = file_tell(f);
    return position;
}

void close(int fd) {
    if (fd < 2 || fd >= 128) return;
    
    struct thread *cur = thread_current();
    struct file *f = cur->FD[fd];
    if (f == NULL) return;

    file_close(f);
    cur->FD[fd] = NULL; // Mark file descriptor as free
  
}
