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
struct timespec ts;

void err_exit(char* err) {
    perror(err);
    exit(1);
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    *pointer = sum;
}

void *thread_func(void *vargp);
void print_results(char* testname, struct timespec start_time);

int main(int argc, char * argv[]) {
    struct option longopts[] = {
        { "iterations", optional_argument, NULL, 'i' },
        { "threads",    optional_argument, NULL, 't' },
        { 0, 0, 0, 0 }
    };
    int c;

    while ((c = getopt_long(argc, argv, "i:t:", longopts, NULL)) != -1) {
        switch(c) {
            case 'i':
                num_iters = atoi(optarg);
                break;
            case 't':
                num_threads = atoi(optarg);
                break;
            case '?':
                err_exit(USAGE);
                break;
        }
    }
    pthread_t thread_ids[num_threads];
    int i, j;
    struct timespec start_time;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start_time);
    for (i = 0; i < num_iters; i++) {
        for (j = 0; j < num_threads; j++) {
            pthread_create(&thread_ids[j], NULL, thread_func, (void *) &value);
        }
        for (j = 0; j < num_threads; j++) {
            pthread_join(thread_ids[j], NULL);
        }
    }
    print_results("add-none", start_time);
    return 0;
}

void *thread_func(void *vargp) {
    add((long long *) vargp, 1);
    add((long long *) vargp, -1);
    return NULL;
}

void print_results(char* testname, struct timespec start_time) {
    long num_operations = num_threads * num_iters * 2;
    struct timespec cur_ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cur_ts);
    long total_runtime = cur_ts.tv_nsec - start_time.tv_nsec;
    long avg_operation_time = total_runtime / num_operations;
    printf("%s,%d,%d,%ld,%ld,%ld,%lld\n", testname, num_threads, num_iters, num_operations, total_runtime, avg_operation_time, value);
}
