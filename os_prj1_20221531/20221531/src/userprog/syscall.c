#include "syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"   

void
check_addr(void *addr) {
  if (false == is_user_vaddr(addr)) exit(-1);
}

static void syscall_handler (struct intr_frame *);


void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
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

    case SYS_READ:
      f->eax = read(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
      break;

    case SYS_WRITE:
      f->eax = write(get_int_arg(f, 4), get_ptr_arg(f, 8), get_int_arg(f, 12));
      break;
    case SYS_FIBONACCI:
      f->eax = fibonacci(get_int_arg(f, 4));
      break;

    case SYS_MAX_OF_FOUR_INT:
      f->eax = max_of_four_int(get_int_arg(f, 4), 
                               get_int_arg(f, 8), 
                               get_int_arg(f, 12), 
                               get_int_arg(f, 16)); 
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
  thread_exit();
}

int exec (const char *cmd_lime) {
  return process_execute(cmd_lime);
}

int wait (int pid) {
  return process_wait(pid);
}

// Reliability check needed
int read (int fd, void *buffer, unsigned size) {
  if (!fd) {
    for (unsigned i = 0; i < size; i++) {
      *((uint8_t *)buffer + i) = input_getc();
    }
    return size;
  }
  return -1;
}

int write (int fd, const void *buffer, unsigned size) {

  if (fd == 1) {
    putbuf(buffer, size);
    return size;
  }
  return -1; 
}

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
