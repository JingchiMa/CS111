//
//  SortedList.c
//  lab2a
//
//  Created by Jingchi Ma on 5/4/19.
//  Copyright © 2019 Jingchi Ma. All rights reserved.
//

#include "SortedList.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int opt_yield;

void SortedList_insert(SortedList_t *list, SortedListElement_t *element) {
    SortedListElement_t *prev = list; // header is dummy header
    SortedListElement_t *cur = list->next;
    while (cur != NULL && strcmp(cur->key, element->key) < 0) {
        prev = prev->next;
        cur = cur->next;
        if (strcmp(prev->key, element->key) > 0) {
            /* synchronization problem */
            fprintf(stderr, "Sync Error: sorted list out of order\n");
            exit(2);
        }
    }
    if (opt_yield & INSERT_YIELD) {
        sched_yield();
    }
    prev->next = element;
    element->prev = prev;
    element->next = cur;
    if (cur != NULL) {
        cur->prev = element;
    }
}

int SortedList_delete(SortedListElement_t *element) {
    // cannot delete header
    if (element->prev == NULL || element->prev->next != element) {
        return 1;
    } else if (element->next != NULL && element->next->prev != element) {
        return 1;
    }
    if (opt_yield & DELETE_YIELD) {
        sched_yield();
    }
    element->prev->next = element->next;
    if (element->next != NULL) {
        element->next->prev = element->prev;
    }
    return 0;
}

SortedListElement_t *SortedList_lookup(SortedList_t *list, const char *key) {
    int res;
    SortedListElement_t *cur = list->next;
    if (opt_yield & LOOKUP_YIELD) {
        sched_yield();
    }
    while (cur != NULL) {
        res = strcmp(cur->key, key);
        if (res == 0) {
            return cur;
        } else if (res > 0) {
            return NULL;
        } else {
            cur = cur->next;
        }
    }
    return NULL;
}

int SortedList_length(SortedList_t *list) {
    int len = 0;
    SortedListElement_t *cur = list->next;
    while (cur != NULL) {
        if (cur->prev->next != cur) {
            return -1;
        }
        if (cur->next != NULL && cur->next->prev != cur) {
            return -1;
        }
        // TODO: may also check if list is still in increasing order.
        len++;
        cur = cur->next;
    }
    return len;
}

void print_sortedList(SortedList_t *list) {
    SortedListElement_t *cur = list->next;
    printf("dummy ");
    while (cur != NULL) {
        printf("<-> %s ", cur->key);
        cur = cur->next;
    }
    printf("\n");
}

int unit_test() {
    SortedListElement_t dummy;
    dummy.key = NULL;
    dummy.prev = NULL; // have to set to NULL
    dummy.next = NULL; // have to set to NULL
    SortedList_t *list = &dummy;
    char *strings[] = {"good", "bad", "int", "hey!"};
    int i;
    for (i = 0; i < 4; i++) {
        SortedListElement_t *node = (SortedListElement_t *) malloc(sizeof(SortedListElement_t));
        node->key = strings[i];
        SortedList_insert(list, node);
    }
    printf("List length is %d\n", SortedList_length(list));
    for (i = 0; i < 4; i++) {
        SortedListElement_t *node = SortedList_lookup(list, strings[i]);
        if (node == NULL) {
            printf("ERROR! %s is not found in list\n", node->key);
        }
        SortedList_delete(node);
    }
    print_sortedList(list);
    return 0;
}
