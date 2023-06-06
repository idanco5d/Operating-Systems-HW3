//
// Created by illus on 03/06/2023.
//

#include "connfdList.h"

#include <stdio.h>
#include <stdlib.h>

connfdNode *head = NULL;
connfdNode *tail = NULL;

void initializeList() {
    head = NULL;
    tail = NULL;
}

void insertList(int connfd) {
    connfdNode *new_node = (connfdNode*)malloc(sizeof(connfdNode));
    new_node->connfd = connfd;
    new_node->next = NULL;
    if (head == NULL) {
        head = new_node;
        tail = new_node;
    } else {
        tail->next = new_node;
        tail = new_node;
    }
}

int popFromList() {
    if (head == NULL) {
        printf("List is empty\n");
        return -1;
    }
    int connfd = head->connfd;
    connfdNode *temp = head;
    head = head->next;
    free(temp);
    return connfd;
}

void destroyList() {
    connfdNode *temp = head;
    while(temp != NULL) {
        connfdNode *next = temp->next;
        free(temp);
        temp = next;
    }
    head = NULL;
    tail = NULL;
}

void printList() {
    connfdNode *temp = head;
    while(temp != NULL) {
        printf("%d ", temp->connfd);
        temp = temp->next;
    }
}

bool isListEmpty() {
    return head == NULL && tail == NULL;
}