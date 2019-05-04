//
//  main.c
//  lab2a
//
//  Created by Jingchi Ma on 5/2/19.
//  Copyright © 2019 Jingchi Ma. All rights reserved.
// gcc lab2_add.c -lpthread -lrt

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

char* USAGE = "Usage: valid options are --threads and --iterations. Default values are 0\n";
long long value = 0;
int num_iters = 1;
int num_threads = 1;
int opt_yield = 0;
struct timespec ts;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; // statically initialize a mutex
volatile int spinlock = 0;

void err_exit(char* err) {
    perror(err);
    exit(1);
}

void add_none(long long *pointer, long long value);
void add_m(long long *pointer, long long value);
void add_s(long long *pointer, long long value);
void add_c(long long *pointer, long long value);

void acquire_spin_lock(void);

/* thread functions */
void *thread_func_none(void *vargp);
void *thread_func_c(void *vargp);
void *thread_func_s(void *vargp);
void *thread_func_m(void *vargp);

void print_results(char* testname, struct timespec start_time);

void test(char* testname, void* (thread_func)(void*));

int main(int argc, char * argv[]) {
    struct option longopts[] = {
        { "iterations", required_argument, NULL, 'i' },
        { "threads",    required_argument, NULL, 't' },
        { "yield",      no_argument,       NULL, 'y' },
        { "sync",       required_argument, NULL, 's' },
        { 0, 0, 0, 0 }
    };
    int c;
    char sync = 0;
    while ((c = getopt_long(argc, argv, "i:t:ys:", longopts, NULL)) != -1) {
        switch(c) {
            case 'i':
                num_iters = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'y':
                opt_yield = 1;
                break;
            case 's':
                sync = optarg[0];
                break;
            case '?':
                err_exit(USAGE);
                break;
        }
    }
    if (sync) {
        switch(sync) {
            case 'm':
                test(&sync, thread_func_m);
                break;
            case 's':
                test(&sync, thread_func_s);
                break;
            case 'c':
                test(&sync, thread_func_c);
                break;
            default:
                err_exit("invalid sync option, only m, s, and c are allowed");
        }
    } else {
        test("none", thread_func_none);
    }

    return 0;
}

void *thread_func_none(void *vargp) {
    int i;
    for (i = 0; i < num_iters; i++) {
        add_none((long long *) vargp, 1);
        add_none((long long *) vargp, -1);
    }
    return NULL;
}

void *thread_func_m(void *vargp) {
    int i;
    for (i = 0; i < num_iters; i++) {
        add_m((long long *) vargp, 1);
        add_m((long long *) vargp, -1);
    }
    return NULL;
}

void *thread_func_s(void *vargp) {
    int i;
    for (i = 0; i < num_iters; i++) {
        add_s((long long *) vargp, 1);
        add_s((long long *) vargp, -1);
    }
    return NULL;
}

void *thread_func_c(void *vargp) {
    int i;
    for (i = 0; i < num_iters; i++) {
        add_c((long long *) vargp, 1);
        add_c((long long *) vargp, -1);
    }
    return NULL;
}

void print_results(char* testname, struct timespec start_time) {
    long num_operations = num_threads * num_iters * 2;
    struct timespec cur_ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cur_ts);
    long total_runtime = (cur_ts.tv_sec - start_time.tv_sec) * 1000000000
                         + cur_ts.tv_nsec - start_time.tv_nsec; // in nano seconds
    long avg_operation_time = total_runtime / num_operations;
    printf("%s,%d,%d,%ld,%ld,%ld,%lld\n", testname, num_threads, num_iters, num_operations, total_runtime, avg_operation_time, value);
}

// test_option should be one of "none", "m", "s", and "c".
void test(char* test_option, void* (thread_func)(void*)) {
    pthread_t thread_ids[num_threads];
    int i;
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    for (i = 0; i < num_threads; i++) {
        pthread_create(&thread_ids[i], NULL, thread_func, (void *) &value);
    }
    for (i = 0; i < num_threads; i++) {
        pthread_join(thread_ids[i], NULL);
    }
    char testname[100];
    if (opt_yield) {
        sprintf(testname, "add-yield-%s", test_option);
    } else {
        sprintf(testname, "add-%s", test_option);
    }
    print_results(testname, start_time);
}

void add_none(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
}
// mutex
void add_m(long long *pointer, long long value) {
    pthread_mutex_lock(&lock);
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
    pthread_mutex_unlock(&lock);
}
// spin lock
void add_s(long long *pointer, long long value) {
    acquire_spin_lock();
    long long sum = *pointer + value;
    if (opt_yield) {
        sched_yield();
    }
    *pointer = sum;
    __sync_lock_release(&spinlock);
}
// atomic operation
void add_c(long long *pointer, long long value) {
    long long oldsum = __sync_val_compare_and_swap(pointer, 0, 0);
    long long newsum = oldsum + value;
    if (opt_yield) {
        sched_yield();
    }
    while (__sync_val_compare_and_swap(pointer, oldsum, newsum) != oldsum) {
        /* loop until before-operation value in pointer is oldsum */
    }
}

// spin lock implementation
void acquire_spin_lock() {
    while (__sync_lock_test_and_set(&spinlock, 1)) {
        while (spinlock);
    }
}


