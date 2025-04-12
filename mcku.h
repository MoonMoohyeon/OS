#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READY 10
#define RUNNING 11
#define TERMINATED 12
#define NUM_PAGES 16
#define NUM_FRAMES 64
#define MAX_PROC 256

struct pcb {
    char pid;
    FILE* fd;
    char* pgtable;
    struct pcb* next;
    int state;
    /* Add more fields if needed */
};

struct pcb* head = NULL;
struct pcb* tail = NULL;
// struct pcb PCBS[MAX_PROC];
unsigned char pt[NUM_FRAMES]; // 물리 메모리 페이지 테이블
long free_list[NUM_FRAMES];
extern struct pcb* current;
extern char* ptbr;
int NUM_PROCESS = MAX_PROC;

void ku_proc_init(int nprocs, char* flist) {
    NUM_PROCESS = nprocs;
    // Initialize free list
    for (int i = 0; i < NUM_PAGES; i++) {
        free_list[i] = 0;
    }

    FILE* fd = fopen(flist, "r");
    if (fd == NULL) {
        printf("Failed to open file %s\n", flist);
        exit(1);
    }
    else{
        printf("file open success\n");
    }

    // Initialize process control blocks
    for (int i = 0; i < nprocs; i++) {
        // Open process file
        char filename[NUM_PAGES];
        fgets(filename, NUM_PAGES, fd);
        int len = strlen(filename);
        if (filename[len - 1] == '\n') {
            filename[len - 1] = '\0';
        }

        FILE* process = fopen(filename, "r");
        if (process == NULL) {
            printf("Error: failed to open file %s\n", filename);
            exit(1);
        }
        else{
            printf("sub file open success\n");
        }

        // Allocate page table
        char* pgtable = (char*)malloc(NUM_PAGES * sizeof(char));
        if (pgtable == NULL) {
            printf("Failed to allocate page table for process %d\n", i);
            exit(EXIT_FAILURE);
        }
        else{
            printf("malloc success\n");
        }
        memset(pgtable, '0', NUM_PAGES);

        // Initialize PCB
        for (int i = 0; i < nprocs; i++) {
        struct pcb* new_pcb = (struct pcb*) malloc(sizeof(struct pcb));
        new_pcb->pid = i;
        new_pcb->fd = process;
        new_pcb->pgtable = pgtable;
        new_pcb->state = 0;

        // 연결 리스트에 삽입
        if (head == NULL) {
            head = new_pcb;
            tail = new_pcb;
            new_pcb->next = head;
        } else {
            tail->next = new_pcb;
            tail = new_pcb;
            new_pcb->next = head;
        }
    }

        // PCBS[i].pid = i;
        // PCBS[i].fd = process;
        // PCBS[i].pgtable = pgtable;
        // // PCBS[i].next = NULL;
        // PCBS[i].state = READY;
    //}

    // Set up initial process to run
    current = head;
    current->state = RUNNING;
    ptbr = current->pgtable;
    printf("init complete\n");
}
}


void ku_scheduler(char pid) {
    printf("scheduler running\n");

    if (head == NULL) {
        printf("Process list is empty.\n");
        return;
    }

    struct pcb* cur_pcb = head;

    // 현재 state가 running인 노드 찾기
    while (cur_pcb != NULL && cur_pcb->state != 1) {
        cur_pcb = cur_pcb->next;
    }

    if (cur_pcb == NULL) {
        printf("No running process found.\n");
        return;
    }

    // 현재 노드의 state를 ready로 변경
    cur_pcb->state = 0;

    // 다음 노드의 state를 running으로 변경
    if (cur_pcb->next == NULL) {
        // tail 노드인 경우 head 노드부터 다시 탐색
        cur_pcb = head;
        while (cur_pcb->state != 0) {
            cur_pcb = cur_pcb->next;
        }
    } else {
        cur_pcb = cur_pcb->next;
    }

    cur_pcb->state = 1;

    // int current_pid = current->pid;
    // int next_pid = (current_pid + 1) % NUM_PAGES;
    // for(int i=0; i<NUM_PAGES; i++)
    // {
    //     if(PCBS[i].pid == TERMINATED) return;
    //     if(PCBS[i].state == READY)
    //     {
    //         next_pid = i;
    //         break;
    //     }
    // }

    // printf("sched current = %d, next = %d\n", current_pid, next_pid);
    // if (next_pid != current_pid) {
    //     // 현재 프로세스의 상태를 READY로 변경
    //     PCBS[current_pid].state = READY;

    //     // 다음 프로세스를 RUNNING 상태로 변경
    //     PCBS[next_pid].state = RUNNING;
    //     current = &PCBS[next_pid];
    //     ptbr = current->pgtable;
    printf("scheduler complete\n");
}

void ku_pgfault_handler(char va) {
    printf("pg_handler running\n");
    ptbr = current->pgtable;
    int pt_index, pa;
    char* pte;

    pt_index = (va & 0xF0) >> 4;
    pte = ptbr + pt_index;
    pa = ((*pte & 0xFC) << 2) + (va & 0x0F) | 0x01; // dirty bit

    // 비어있는 프레임을 찾아 매핑
    for (int i = 0; i < NUM_FRAMES; i++) {
            if (free_list[i] == 0) {
                free_list[i] = pa;
                *pte = (char)i; // 페이지 테이블 최신화(프레임 번호)
                printf("Page fault at virtual address %hhu, page frame allocated at %d\n", va, i);
                break;
            }
        }
    printf("pg_handler complete\n");
}


void ku_proc_exit(char pid) {
    printf("proc_exit running pid = %d\n", pid);
    struct pcb* cur_pcb = head;
    struct pcb* prev_pcb = NULL;
    // pid가 일치하는 노드 찾기
    while (cur_pcb != NULL && cur_pcb->pid != pid) {
        prev_pcb = cur_pcb;
        cur_pcb = cur_pcb->next;
    }

    if (cur_pcb == NULL) {
        printf("pid %d not found.\n", pid);
        return;
    }

    // 삭제할 노드가 head인 경우
    if (prev_pcb == NULL) {
        head = cur_pcb->next;
    } else {
        prev_pcb->next = cur_pcb->next;
    }

    // 삭제할 노드가 tail인 경우
    if (cur_pcb->next == NULL) {
        prev_pcb->next = NULL;
    }

    free(cur_pcb);

    // for (int i = 0; i < NUM_PROCESS; i++)
    // {
    //     if (PCBS[i].pid == pid)
    //     {
    //         current = &PCBS[i];
    //     }
    //     if(i == NUM_PROCESS)
    //     {
    //         printf("exit error\n");
    //         return;
    //     }
    // }

    // PCB에서 해당 프로세스가 사용하던 페이지 프레임 해제
    for (int i = 0; i < NUM_PAGES; i++) {
        if (current->pgtable[i] != 0)
        {
            free_list[current->pgtable[i]] = '0'; // 프리 리스트 업데이트
            current->pgtable[i] = '0';
        }
    }

    if (current->state == RUNNING)
    {
        ku_scheduler('a'); // 다른 프로세스 실행을 위해
    }

    // current->pid = TERMINATED;
    // fclose(current->fd);
    // current->state = TERMINATED;
    // free(current->pgtable);
    printf("proc_exit complete\n");
}
