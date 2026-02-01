#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <hash.h>
#include <list.h>
#include "threads/thread.h"
#include "filesys/file.h"

#define VM_BIN 0   /* 실행 파일 (Binary) */
#define VM_FILE 1  /* 메모리 매핑 파일 (mmap) */
#define VM_ANON 2  /* 스왑/스택 (Anonymous) */

/* 가상 페이지 정보를 담는 구조체 (Supplemental Page Table Entry) */
struct vm_entry {
    uint8_t type;       
    void *vaddr;        
    bool writable;      
    bool is_loaded;     
    

    struct file *file;  
    size_t offset;      
    size_t read_bytes;  
    size_t zero_bytes;  

    size_t swap_slot;   

    struct hash_elem elem;
};

/* mmap된 파일을 관리하기 위한 구조체 */
struct mmap_file {
    int mapid;              
    struct file *file;      
    void *vaddr;            
    size_t size;            
    struct list_elem elem;  
};


void vm_init (struct hash *vm);
void vm_destroy (struct hash *vm);
struct vm_entry *find_vme (void *vaddr);
bool insert_vme (struct hash *vm, struct vm_entry *vme);
bool delete_vme (struct hash *vm, struct vm_entry *vme);

#endif /* vm/page.h */