
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
#include "SortedList.h"

char* USAGE = "Usage: valid options are --threads and --iterations. Default values are 1\n";
long long    counter     = 0;
int          num_iters   = 1;
int          num_threads = 1;
int          opt_yield   = 0;
char         sync_flag   = 0;
extern volatile int err_flag; // set to 1 if there's sync error in some thread

struct timespec ts;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // statically initialize a mutex
volatile int spinlock = 0;
SortedList_t *list;

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

void get_lock(void);
void release_lock(void);
void acquire_spin_lock(void);

int main(int argc, char * argv[]) {
    signal(SIGSEGV, segfault_handler);
    struct option longopts[] = {
        { "iterations", required_argument, NULL, 'i' },
        { "threads",    required_argument, NULL, 't' },
        { "yield",      required_argument, NULL, 'y' },
        { "sync",       required_argument, NULL, 's' },
        { 0, 0, 0, 0 }
    };
    int c;
    while ((c = getopt_long(argc, argv, "i:t:ys:", longopts, NULL)) != -1) {
        switch(c) {
            case 'i':
                num_iters = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'y':
                set_opt_yield(optarg);
                break;
            case 's':
                sync_flag = optarg[0];
                break;
            case '?':
                err_exit(USAGE);
                break;
        }
    }
    /* IO redirection */
    int csv = open("list.csv", O_CREAT | O_WRONLY);
    dup2(csv, 1); // redirec stdout to the csv file
    
    test();
}

void test(void) {
    err_flag = 0;
    /* init list */
    list = (SortedList_t *) malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->prev = NULL;
    list->next = NULL;
    
    /* prepare list elements */
    SortedListElement_t *(*nodes)[num_iters] = malloc(sizeof(SortedListElement_t*[num_threads][num_iters]));
    {
        int i, j;
        for (i = 0; i < num_threads; i++) {
            for (j = 0; j < num_iters; j++) {
                nodes[i][j] = (SortedListElement_t *) malloc(sizeof(SortedListElement_t));
                // Need to make sure buffer is large enough, see https://stackoverflow.com/a/8257728/8159477 for more info.
                char* key = malloc(10 * sizeof(char)); // also needs to create a buffer for each node!
                sprintf(key, "%d-%d", i, j);
                nodes[i][j]->key = key;
                nodes[i][j]->prev = NULL;
                nodes[i][j]->next = NULL;
            }
        }
    }
    /* prepare and start threads */
    pthread_t thread_ids[num_threads];
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    {
        int i;
        for (i = 0; i < num_threads; i++) {
            pthread_create(&thread_ids[i], NULL, thread_func, (void *) &nodes[i][0]);
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
    int len = SortedList_length(list);
    if (len != 0) {
        sync_err_exit("Sync Error: List length is not 0");
    } else {
        print_results("list-none-none", start_time);
    }
}

// pass in the address of the array (i.e. address of the first SortedListElement pointer)
void *thread_func(void *vargs) {
    /* if err_flag has been set, directly return */
    if (__sync_val_compare_and_swap(&err_flag, 0, 0) == 1) {
        return NULL;
    }
    int i;
    SortedListElement_t **nodeptr_loc = (SortedListElement_t **) vargs;
    
    /* insert the elements */
    for (i = 0; i < num_iters; i++) {
        get_lock();
        SortedList_insert(list, *nodeptr_loc);
        release_lock();
        nodeptr_loc++;
    }
    
    /* get the length */
    // get_lock();
    int len = SortedList_length(list);
    // release_lock();
    if (len == -1) {
        /* error found when checking length */
        fprintf(stderr, "Sync Error: List length < 0\n");
        __sync_val_compare_and_swap(&err_flag, 0, 1);
        return NULL;
    }
    /* remove them all */
    nodeptr_loc = (SortedListElement_t **) vargs;
    for (i = 0; i < num_iters; i++) {
        get_lock();
        SortedListElement_t *cur = SortedList_lookup(list, (*nodeptr_loc)->key);
        release_lock();
        if (cur == NULL) {
            /* error: cannot find the inserted key */
            fprintf(stderr, "Sync Error: Cannot find key %s which was inserted\n", (*nodeptr_loc)->key);
            __sync_val_compare_and_swap(&err_flag, 0, 1);
            return NULL;
        }
        get_lock();
        SortedList_delete(cur);
        release_lock();
        nodeptr_loc++;
    }
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
    int num_lists = 1;
    long num_operations = num_threads * num_iters * 3;
    struct timespec cur_ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cur_ts);
    long total_runtime = (cur_ts.tv_sec - start_time.tv_sec) * 1000000000
                         + cur_ts.tv_nsec - start_time.tv_nsec; // in nano seconds
    long avg_operation_time = total_runtime / num_operations;
    printf("%s,%d,%d,%d,%ld,%ld,%ld\n", testname, num_threads, num_iters, num_lists, num_operations, total_runtime, avg_operation_time);
}
void segfault_handler(int signum) {
    if (signum == SIGSEGV) {
        fprintf(stderr, "segmentation fault\n");
        exit(2);
    }
}
void acquire_spin_lock() {
    while (__sync_lock_test_and_set(&spinlock, 1)) {
        while (spinlock);
    }
}

// setup lock based on sync_flag. May not get a real lock.
// must call release_lock
void get_lock() {
    switch(sync_flag) {
        case 's':
            /* spin lock */
            acquire_spin_lock();
            break;
        case 'm':
            pthread_mutex_lock(&lock);
            break;
    }
}
void release_lock() {
    switch(sync_flag) {
        case 's':
            __sync_lock_release(&spinlock);
            break;
        case 'm':
            pthread_mutex_unlock(&lock);
            break;
    }
}
