
//
//  lab2_list.c
//  lab2a
//
//  Created by Jingchi Ma on 5/4/19.
//  Copyright © 2019 Jingchi Ma. All rights reserved.
// gcc lab2_list.c SortedList.c -lpthread -lrt

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "SortedList.h"

char* USAGE = "Usage: valid options are --threads and --iterations. Default values are 1\n";
long long    counter     = 0;
int          num_iters   = 1;
int          num_threads = 1;
int          opt_yield   = 0;
char         sync_flag   = 0;
extern volatile int err_flag; // set to 1 if there's sync error in some thread
char*        yield_option = "none";
volatile long global_waiting_lock = 0;

struct timespec ts;

pthread_mutex_t *mutexs;    // pointer to the mutex array
volatile int    *spinlocks; // pointer to the spinlock array
SortedList_t    **lists;    // pointer to the list array
int             num_lists = 1;

void err_exit(char* err) {
    perror(err);
    exit(1);
}
void sync_err_exit(char* err) {
    fprintf(stderr, "%s\n", err);
    exit(2);
}
void segfault_handler(int signum);
void set_opt_yield(char *yield_option);
void *thread_func(void * vargs);
void print_results(char* testname, struct timespec start_time);
void test(void);

void get_lock(int list_index, long *waiting_time);
void release_lock(int list_index);
void acquire_spin_lock(volatile int *spinlock);
int get_list_index(int thread_index);

/* type of parameters to be passed into thread function */
typedef struct {
    int index;
    SortedList_t **nodeptr_loc;
} ThreadParam_t;

int main(int argc, char * argv[]) {
    signal(SIGSEGV, segfault_handler);
    struct option longopts[] = {
        { "iterations", required_argument, NULL, 'i' },
        { "threads",    required_argument, NULL, 't' },
        { "yield",      required_argument, NULL, 'y' },
        { "sync",       required_argument, NULL, 's' },
        { "lists",      required_argument, NULL, 'l' },
        { 0, 0, 0, 0 }
    };
    int c;
    while ((c = getopt_long(argc, argv, "i:t:ys:l:", longopts, NULL)) != -1) {
        switch(c) {
            case 'i':
                num_iters = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'y':
                yield_option = optarg;
                set_opt_yield(optarg);
                break;
            case 's':
                sync_flag = optarg[0];
                break;
            case 'l':
                num_lists = atoi(optarg);
                if (num_lists < 1) {
                    err_exit("list number must be >= 1");
                }
                break;
            case '?':
                err_exit(USAGE);
                break;
        }
    }
    
    test();
}

void test(void) {
    srand((unsigned int) time(NULL)); // setup random seed
    err_flag = 0;
    /* init lists and locks */
    lists = malloc(sizeof(SortedList_t*[num_lists]));
    mutexs = malloc(sizeof(pthread_mutex_t[num_lists]));
    spinlocks = malloc(sizeof(int[num_lists]));
    {
        int i;
        for (i = 0; i < num_lists; i++) {
            lists[i] = (SortedList_t *) malloc(sizeof(SortedList_t));
            lists[i]->key = NULL;
            lists[i]->prev = NULL;
            lists[i]->next = NULL;
            pthread_mutex_init(&mutexs[i], NULL);
            spinlocks[i] = 0;
        }
    }
    
    /* prepare list elements */
    SortedListElement_t *(*nodes)[num_iters] = malloc(sizeof(SortedListElement_t*[num_threads][num_iters]));
    {
        int i, j;
        for (i = 0; i < num_threads; i++) {
            for (j = 0; j < num_iters; j++) {
                nodes[i][j] = (SortedListElement_t *) malloc(sizeof(SortedListElement_t));
                // Need to make sure buffer is large enough, see
                // https://stackoverflow.com/a/8257728/8159477 for more info.
                char* key = malloc(10 * sizeof(char)); // also needs to create a buffer for each node!
                sprintf(key, "%d-%d", i, rand());
                nodes[i][j]->key = key;
                nodes[i][j]->prev = NULL;
                nodes[i][j]->next = NULL;
            }
        }
    }
    /* prepare and start threads */
    pthread_t thread_ids[num_threads];
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    {
        int i;
        for (i = 0; i < num_threads; i++) {
            ThreadParam_t *thread_param = malloc(sizeof(ThreadParam_t));
            thread_param->index = i;
            thread_param->nodeptr_loc = &nodes[i][0];
            pthread_create(&thread_ids[i], NULL, thread_func, (void *) thread_param);
            // NOTE: cannot free thread_param here!
        }
        for (i = 0; i < num_threads; i++) {
            pthread_join(thread_ids[i], NULL);
        }
    }
    if (err_flag == 1) {
        /* error occurs in some threads */
        exit(2);
    }
    /* check the final length */
    {
        int i;
        for (i = 0; i < num_lists; i++) {
            int len = SortedList_length(lists[i]);
            if (len != 0) {
                sync_err_exit("Sync Error: Sublist length is not 0");
            }
        }
    }
    /* all the lists have length 0 */
    free(lists);
    // TODO: destroy and free mutexes
    // TODO: free spinlocks
    // TODO: free list elements
    char testname[100];
    if (sync_flag == 0) {
        sprintf(testname, "list-%s-%s", yield_option, "none");
    } else {
        sprintf(testname, "list-%s-%c", yield_option, sync_flag);
    }
    print_results(testname, start_time);
}

// pass in the address of the array (i.e. address of the first SortedListElement pointer)
void *thread_func(void *vargs) {
    ThreadParam_t *thread_param = (ThreadParam_t *) vargs;
    int list_index = get_list_index(thread_param->index);
    SortedList_t *list = lists[list_index];
    long time_waiting_lock = 0;
    /* if err_flag has been set, directly return */
    if (__sync_val_compare_and_swap(&err_flag, 0, 0) == 1) {
        return NULL;
    }
    int i;
    SortedListElement_t **nodeptr_loc = thread_param->nodeptr_loc;
    /* insert the elements */
    for (i = 0; i < num_iters; i++) {
        get_lock(list_index, &time_waiting_lock);
        SortedList_insert(list, *nodeptr_loc);
        release_lock(list_index);
        nodeptr_loc++;
    }
    /* get the length */
    get_lock(list_index, &time_waiting_lock);
    int len = SortedList_length(list);
    release_lock(list_index);
    if (len == -1) {
        fprintf(stderr, "Sync Error: List length < 0\n");
        __sync_val_compare_and_swap(&err_flag, 0, 1);
        return NULL;
    }
    /* remove them all */
    nodeptr_loc = thread_param->nodeptr_loc;
    for (i = 0; i < num_iters; i++) {
        get_lock(list_index, &time_waiting_lock);
        SortedListElement_t *cur = SortedList_lookup(list, (*nodeptr_loc)->key);
        release_lock(list_index);
        if (cur == NULL) {
            /* error: cannot find the inserted key */
            fprintf(stderr, "Sync Error: Cannot find key %s which was inserted\n", (*nodeptr_loc)->key);
            __sync_val_compare_and_swap(&err_flag, 0, 1);
            return NULL;
        }
        get_lock(list_index, &time_waiting_lock);
        SortedList_delete(cur);
        release_lock(list_index);
        nodeptr_loc++;
    }
    /* add to global sum */
    __atomic_fetch_add(&global_waiting_lock, time_waiting_lock, __ATOMIC_SEQ_CST);
    return NULL;
}

void set_opt_yield(char *yield_option) {
    while (*yield_option != '\0') {
        switch (*yield_option) {
            case 'i':
                opt_yield |= INSERT_YIELD;
                break;
            case 'd':
                opt_yield |= DELETE_YIELD;
                break;
            case 'l':
                opt_yield |= LOOKUP_YIELD;
                break;
        }
        yield_option++;
    }
}
void print_results(char* testname, struct timespec start_time) {
    long num_operations = num_threads * num_iters * 3;
    struct timespec cur_ts;
    clock_gettime(CLOCK_MONOTONIC, &cur_ts);
    long total_runtime = (cur_ts.tv_sec - start_time.tv_sec) * 1000000000
    + cur_ts.tv_nsec - start_time.tv_nsec; // in nano seconds
    long avg_operation_time = total_runtime / num_operations;
    printf("%s,%d,%d,%d,%ld,%ld,%ld,%ld\n", testname, num_threads, num_iters, num_lists, num_operations, total_runtime, avg_operation_time, global_waiting_lock / num_operations);
}
void segfault_handler(int signum) {
    if (signum == SIGSEGV) {
        fprintf(stderr, "segmentation fault\n");
        exit(2);
    }
}
void acquire_spin_lock(volatile int *spinlock) {
    while (__sync_lock_test_and_set(spinlock, 1)) {
        while (*spinlock); // check if *spinlock is 1
    }
}

// setup lock based on sync_flag. May not get a real lock.
// must call release_lock with same list_index
// int *waiting_time is the total waiting-lock-time for a single thread
void get_lock(int list_index, long *waiting_time) {
    struct timespec start_ts;
    clock_gettime(CLOCK_MONOTONIC, &start_ts);
    switch(sync_flag) {
        case 's':
            acquire_spin_lock(&spinlocks[list_index]);
            break;
        case 'm':
            pthread_mutex_lock(&mutexs[list_index]);
            break;
        default:
            return; // no lock, no change to waiting_time
    }
    struct timespec cur_ts;
    clock_gettime(CLOCK_MONOTONIC, &cur_ts);
    *waiting_time += (cur_ts.tv_sec - start_ts.tv_sec) * 1000000000
                      + cur_ts.tv_nsec - start_ts.tv_nsec; // in nano seconds
}
void release_lock(int list_index) {
    switch(sync_flag) {
        case 's':
            __sync_lock_release(&spinlocks[list_index]);
            break;
        case 'm':
            pthread_mutex_unlock(&mutexs[list_index]);
            break;
    }
}
void get_testname(char testname[100], char *yield_option, char *sync_option) {
    sprintf(testname, "list-%s-%s", yield_option, sync_option);
}

// return the index of sublist the thread should be working on
int get_list_index(int thread_index) {
    return thread_index % num_lists;
}
