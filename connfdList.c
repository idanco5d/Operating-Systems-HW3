//
// Created by illus on 03/06/2023.
//

#include "connfdList.h"

#include <stdio.h>
#include <stdlib.h>
#include "segel.h"

connfdNode *head = NULL;
unsigned int num_of_nodes = 0;

void insertList(int connfd, struct timeval arrival_time) {
    connfdNode *new_node = (connfdNode*)malloc(sizeof(connfdNode));
    new_node->connfd = connfd;
    new_node->arrival_time = arrival_time;
    new_node->next = NULL;
    num_of_nodes++;
    if (!head) {
        head = new_node;
        return;
    }
    connfdNode *temp = head;
    while (temp->next) {
        temp = temp->next;
    }
    temp->next = new_node;
}

connfdNode popFromList() {
    if (head == NULL) {
        printf("List is empty\n");
        connfdNode nullNode;
        nullNode.next = NULL;
        nullNode.connfd = -1;
        return nullNode;
    }
    connfdNode toReturn;
    toReturn.connfd = head->connfd;
    toReturn.arrival_time = head->arrival_time;
    connfdNode *temp = head;
    head = head->next;
    free(temp);
    num_of_nodes--;
    return toReturn;
}

void destroyList() {
    connfdNode *temp = head;
    while(temp != NULL) {
        connfdNode *next = temp->next;
        free(temp);
        temp = next;
    }
    num_of_nodes = 0;
    head = NULL;
}

void printList() {
    connfdNode *temp = head;
    while(temp != NULL) {
        temp = temp->next;
    }
}

bool isListEmpty() {
    return head == NULL;
}

unsigned int getNumOfNodes() {
    return num_of_nodes;
}

void removeFromListAtPlace(unsigned int n) {
    if (num_of_nodes == 0) {
        return;
    }
    if (n == 0) {
        connfdNode node = popFromList();
        Close(node.connfd);
        return;
    }
    if (n == num_of_nodes - 1) {
        connfdNode *temp = head;
        while (temp->next && temp->next->next) {
            temp = temp->next;
        }
        connfdNode *toRemove = temp->next;
        temp->next = NULL;
        Close(toRemove->connfd);
        free(toRemove);
        num_of_nodes--;
        return;
    }
    connfdNode *temp = head, *prev = head;
    for (int i = 0; i < n; i++) {
        prev = temp;
        temp = temp -> next;
    }
    prev->next = temp->next;
    Close(temp->connfd);
    free(temp);
    num_of_nodes--;
}

void dropHalfList() {
    if (num_of_nodes == 1) {
        connfdNode toRemove = popFromList();
        Close(toRemove.connfd);
        return;
    }
    unsigned int halfNodesAmount = ceil(num_of_nodes/2);
    for (unsigned int i = 0; i < halfNodesAmount; i++) {
        unsigned int random_placement = rand() % num_of_nodes;
        removeFromListAtPlace(random_placement);
    }
}