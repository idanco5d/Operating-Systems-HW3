//
// Created by illus on 03/06/2023.
//

#include <stdbool.h>
#include <bits/types/time_t.h>
#include <sys/time.h>

#ifndef WEBSERVER_FILES_CONNFDLIST_H

typedef struct connfdNode {
    int connfd;
    struct timeval arrival_time;
    struct connfdNode *next;
} connfdNode;

void initializeList();
void insertList(int connfd, struct timeval arrival_time);
connfdNode popFromList();
void destroyList();
void printList();
bool isListEmpty();
unsigned int getNumOfNodes();

#define WEBSERVER_FILES_CONNFDLIST_H

#endif //WEBSERVER_FILES_CONNFDLIST_H
