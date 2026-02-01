/* vm/swap.c */
#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include <bitmap.h>

/* 한 페이지(4KB)에 해당하는 섹터 수 = 8 */
#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

/* 스왑 디스크 장치 */
static struct block *swap_block;
/* 스왑 슬롯 사용 여부를 관리하는 비트맵 (1 = 사용중, 0 = 비어있음) */
static struct bitmap *swap_map;
/* 동기화를 위한 락 */
static struct lock swap_lock;

/* 스왑 시스템 초기화 */
void vm_swap_init (void) {
    swap_block = block_get_role(BLOCK_SWAP);
    if (swap_block == NULL) {
        return; // 스왑 디스크가 없으면 초기화 중단
    }

    /* 스왑 영역의 크기를 페이지 단위로 계산 */
    size_t swap_size = block_size(swap_block) / SECTORS_PER_PAGE;
    
    /* 비트맵 생성 (모든 비트를 0(false)으로 초기화) */
    swap_map = bitmap_create(swap_size);
    bitmap_set_all(swap_map, false);
    
    lock_init(&swap_lock);
}

/* 스왑 영역(swap_index)에서 데이터를 읽어 메모리(kpage)로 복원 (Swap In) */
void vm_swap_in (size_t swap_index, void *kpage) {
    if (swap_block == NULL || swap_map == NULL) return;

    lock_acquire(&swap_lock);
    
    /* 해당 슬롯이 사용 중인지 확인 (사용 중이어야 데이터가 있음) */
    if (bitmap_test(swap_map, swap_index) == false) {
        lock_release(&swap_lock);
        return; // 에러 처리: 비어있는 슬롯을 읽으려 함
    }

    /* 8개의 섹터를 연속으로 읽어옴 */
    for (int i = 0; i < SECTORS_PER_PAGE; i++) {
        block_read(swap_block, swap_index * SECTORS_PER_PAGE + i, 
                   kpage + (i * BLOCK_SECTOR_SIZE));
    }

    /* 읽어온 슬롯은 이제 비어있는 것으로 처리 (Swap Slot 해제) */
    bitmap_flip(swap_map, swap_index);
    
    lock_release(&swap_lock);
}

/* 메모리(kpage)의 데이터를 스왑 영역으로 내보냄 (Swap Out) */
size_t vm_swap_out (void *kpage) {
    if (swap_block == NULL || swap_map == NULL) return -1; // PANIC 대신 에러 코드 리턴

    lock_acquire(&swap_lock);

    /* 빈 슬롯(false)을 찾아서 첫 번째 인덱스를 반환 (First Fit) */
    size_t swap_index = bitmap_scan_and_flip(swap_map, 0, 1, false);

    if (swap_index == BITMAP_ERROR) {
        lock_release(&swap_lock);
        return BITMAP_ERROR; // 스왑 공간 부족
    }

    /* 8개의 섹터에 나누어 씀 */
    for (int i = 0; i < SECTORS_PER_PAGE; i++) {
        block_write(swap_block, swap_index * SECTORS_PER_PAGE + i, 
                    kpage + (i * BLOCK_SECTOR_SIZE));
    }

    lock_release(&swap_lock);
    
    return swap_index; // 저장된 슬롯 번호 반환
}


/* 스왑 슬롯을 해제 (비트맵을 0으로 뒤집음) */
void vm_swap_free (size_t swap_index) {
    if (swap_block == NULL || swap_map == NULL) return;

    lock_acquire(&swap_lock);
    
    /* 사용 중인 슬롯이었다면 해제 */
    if (bitmap_test(swap_map, swap_index)) {
        bitmap_flip(swap_map, swap_index);
    }
    
    lock_release(&swap_lock);
}