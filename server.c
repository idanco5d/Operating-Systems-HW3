#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "connfdList.h"
#include <stdbool.h>
#include <bits/types/time_t.h>
#include <sys/time.h>

pthread_cond_t cond;
pthread_mutex_t mutex;
unsigned int num_of_working = 0;

// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

// HW3: Parse the new arguments too
void getargs(void *arg, int argc, char *argv[], unsigned int pos)
{
    if (argc < pos + 1) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    if (pos != 4)
        *(int*)arg = atoi(argv[pos]);
    else
        *(char**)arg = argv[pos];
}

//void getnumthreads(int *numthreads, int argc, char *argv[])
//{
//    if (argc < 3) {
//        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
//        exit(1);
//    }
//    *numthreads = atoi(argv[2]);
//}

void* requestHandleByThread(void* args) {
    thread_stats threadStats = {*(int*)args, 0, 0, 0};
    stats_struct stats;
    free(args);
    while (1) {
        pthread_mutex_lock(&mutex);
        while (isListEmpty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        connfdNode connfd_node = popFromList();
        num_of_working++;
        pthread_mutex_unlock(&mutex);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //
        stats.arrival_time = connfd_node.arrival_time;
        stats.handler_thread_stats = &threadStats;
        struct timeval curr_dispatch_interval;
        gettimeofday(&curr_dispatch_interval, NULL);
        curr_dispatch_interval.tv_usec -= connfd_node.arrival_time.tv_usec;
        curr_dispatch_interval.tv_sec -= connfd_node.arrival_time.tv_sec;
        stats.dispatch_interval = curr_dispatch_interval;
        threadStats.handler_thread_req_count++;
        requestHandle(connfd_node.connfd, &stats);
        Close(connfd_node.connfd);
        pthread_mutex_lock(&mutex);
        num_of_working--;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numthreads, queueSize, maxSize;
    char* schedalg;
    struct sockaddr_in clientaddr;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    getargs(&port, argc, argv, 1);

    // 
    // HW3: Create some threads...
    //
    getargs(&numthreads,argc,argv, 2);
    for (int i = 0; i < numthreads; i++) {
        pthread_t t;
        int* threadNum = (int*)malloc(sizeof(int));
        *threadNum = i;
        pthread_create(&t, NULL, requestHandleByThread, (void*)threadNum);
    }
    getargs(&queueSize,argc,argv, 3);
    getargs(&schedalg,argc,argv, 4);
    if (strcmp(schedalg, "dynamic")==0) {
        getargs(&maxSize, argc, argv, 5);
    }
    listenfd = Open_listenfd(port);
    bool block_flush_threshold = false;
    while (1) {
        clientlen = sizeof(clientaddr);
        pthread_mutex_lock(&mutex);
        if (queueSize == getNumOfNodes()) {
            if (strcmp(schedalg, "block")==0) {
                pthread_mutex_unlock(&mutex);
                continue;
            }
            else if (strcmp(schedalg, "block_flush")==0) {
                pthread_mutex_unlock(&mutex);
                block_flush_threshold = true;
                continue;
            }
        }
        if (block_flush_threshold) {
            if (getNumOfNodes() > 0 || num_of_working > 0) {
                pthread_mutex_unlock(&mutex);
                continue;
            }
            block_flush_threshold = false;
        }
        pthread_mutex_unlock(&mutex);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        struct timeval curr_arrival_time;
        gettimeofday(&curr_arrival_time, NULL);
        pthread_mutex_lock(&mutex);
        if (queueSize == getNumOfNodes()) {
            if (strcmp(schedalg, "dynamic")==0) {
                if (queueSize < maxSize) {
                    queueSize++;
                }
            }
            if (strcmp(schedalg, "dynamic")==0 || strcmp(schedalg, "drop_tail")==0) {
                pthread_mutex_unlock(&mutex);
                Close(connfd);
                continue;
            }
        }
        if (connfd >= 0) {
            if (strcmp(schedalg,"drop_head")==0 && queueSize == getNumOfNodes()) {
                Close(popFromList().connfd);
            }
            insertList(connfd, curr_arrival_time);
        }
        if (!isListEmpty()) {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }
}


    


 
