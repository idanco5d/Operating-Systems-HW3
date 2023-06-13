#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "connfdList.h"
#include <stdbool.h>
#include <bits/types/time_t.h>
#include <sys/time.h>

//#define MAX_SCHEDALG_SIZE 12

pthread_cond_t cond;
pthread_cond_t cond_master;
pthread_mutex_t mutex;
unsigned int num_of_working = 0;
char* schedalg;


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
        stats.arrival_time = connfd_node.arrival_time;
        stats.handler_thread_stats = &threadStats;
        struct timeval curr_dispatch_time;
        gettimeofday(&curr_dispatch_time, NULL);
        timersub(&curr_dispatch_time, &connfd_node.arrival_time, &stats.dispatch_interval);
        //threadStats.handler_thread_req_count++;
        //to delete
//        printf("Dispatch time is %lu in sec and %lu in usec\n", curr_dispatch_interval.tv_sec, curr_dispatch_interval.tv_usec);
        //not to delete
        requestHandle(connfd_node.connfd, &stats);
        Close(connfd_node.connfd);
        pthread_mutex_lock(&mutex);
        num_of_working--;
        if ((strcmp(schedalg,"block_flush") == 0 && num_of_working == 0 && getNumOfNodes() == 0) || strcmp(schedalg,"block") == 0) {
            pthread_cond_signal(&cond_master);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


bool handleMasterThreadInDynamicOrDropTail(int* queueSize, int connfd, int maxSize, int numthreads) {
    if (*queueSize == getNumOfNodes() + num_of_working) {
        if (strcmp(schedalg, "dynamic")==0) {
            if (*queueSize < maxSize) {
                (*queueSize)++;
            }
        }
        if (strcmp(schedalg, "dynamic")==0 || strcmp(schedalg, "drop_tail")==0) {
            pthread_mutex_unlock(&mutex);
            Close(connfd);
            return true;
        }
    }
    return false;
}

void handleBlockInMasterThread(int queueSize, int numthreads) {
    if (strcmp(schedalg, "block")==0 || strcmp(schedalg, "block_flush")==0) {
        while (queueSize == getNumOfNodes() + num_of_working) {
                pthread_cond_wait(&cond_master, &mutex);
        }
    }
}

void insertToListOrDropHead(int connfd, int queueSize, struct timeval curr_arrival_time) {
    if (connfd >= 0) {
        if (strcmp(schedalg,"drop_head")==0 && queueSize == getNumOfNodes() + num_of_working) {
            Close(popFromList().connfd);
        }
        insertList(connfd, curr_arrival_time);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numthreads, queueSize, maxSize;
    struct sockaddr_in clientaddr;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    pthread_cond_init(&cond_master,NULL);
    getargs(&port, argc, argv, 1);
    getargs(&numthreads,argc,argv, 2);
    getargs(&schedalg,argc,argv, 4);
    for (int i = 0; i < numthreads; i++) {
        pthread_t t;
        int* threadNum = (int*)malloc(sizeof(int));
        *threadNum = i;
        pthread_create(&t, NULL, requestHandleByThread, (void*)threadNum);
    }
    getargs(&queueSize,argc,argv, 3);
    if (strcmp(schedalg, "dynamic")==0) {
        getargs(&maxSize, argc, argv, 5);
    }

    listenfd = Open_listenfd(port);
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        struct timeval curr_arrival_time;
        gettimeofday(&curr_arrival_time, NULL);
        pthread_mutex_lock(&mutex);
        handleBlockInMasterThread(queueSize, numthreads);
        //to delete
//        printf("We got a request on time %lu in sec and %lu in usec\n", curr_arrival_time.tv_sec, curr_arrival_time.tv_usec);
        //not to delete
        if (handleMasterThreadInDynamicOrDropTail(&queueSize, connfd, maxSize, numthreads)) {
            continue;
        }
        insertToListOrDropHead(connfd,queueSize,curr_arrival_time);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}


    


 
