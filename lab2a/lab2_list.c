
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
#include "SortedList.h"

char* USAGE = "Usage: valid options are --threads and --iterations. Default values are 1\n";
long long    counter     = 0;
int          num_iters   = 1;
int          num_threads = 1;
int          opt_yield   = 0;
volatile int err_flag    = 0; // set to 1 if there's sync error in some thread

struct timespec ts;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // statically initialize a mutex
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
    char sync_flag = 0;
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
    test();
}

void test(void) {
    /* init list */
    list = (SortedList_t *) malloc(sizeof(SortedList_t));
    list->key = NULL;
    list->prev = NULL;
    list->next = NULL;
    
    /* prepare list elements */
    SortedListElement_t *nodes[num_threads][num_iters];
    {
        int i, j;
        for (i = 0; i < num_threads; i++) {
            for (j = 0; j < num_iters; j++) {
                nodes[i][j] = (SortedListElement_t *) malloc(sizeof(SortedListElement_t));
                // Need to make sure buffer is large enough, see https://stackoverflow.com/a/8257728/8159477 for more info.
                char* key = malloc(10 * sizeof(char)); // also needs to create a buffer for each node!
                sprintf(key, "%d", j);
                nodes[i][j]->key = key;
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
    /* check the final length */
    int len = SortedList_length(list);
    if (len != 0) {
        sync_err_exit("Sync Error: List length is not 0");
    } else if (err_flag == 1) {
        /* error occurs in some threads */
        exit(2);
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
        SortedList_insert(list, *nodeptr_loc);
        nodeptr_loc++;
    }
    
    /* get the length */
    int len = SortedList_length(list);
    if (len == -1) {
        /* error found when checking length */
        fprintf(stderr, "Sync Error: List length < 0\n");
        __sync_val_compare_and_swap(&err_flag, 0, 1);
        return NULL;
    }
    /* remove them all */
    nodeptr_loc = (SortedListElement_t **) vargs;
    for (i = 0; i < num_iters; i++) {
        SortedListElement_t *cur = SortedList_lookup(list, (*nodeptr_loc)->key);
        if (cur == NULL) {
            /* error: cannot find the inserted key */
            fprintf(stderr, "Sync Error: Cannot find key %s which was inserted\n", (*nodeptr_loc)->key);
            __sync_val_compare_and_swap(&err_flag, 0, 1);
            return NULL;
        }
        SortedList_delete(cur);
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
