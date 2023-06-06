//
// Created by illus on 03/06/2023.
//

#include <stdbool.h>

#ifndef WEBSERVER_FILES_CONNFDLIST_H

typedef struct connfdNode {
    int connfd;
    struct connfdNode *next;
} connfdNode;

void initializeList();
void insertList(int connfd);
int popFromList();
void destroyList();
void printList();
bool isListEmpty();

#define WEBSERVER_FILES_CONNFDLIST_H

#endif //WEBSERVER_FILES_CONNFDLIST_H
