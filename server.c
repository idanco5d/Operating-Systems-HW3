#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "connfdList.h"
#include <stdbool.h>

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
    while (1) {
        pthread_mutex_lock(&mutex);
        while (isListEmpty()) {
            pthread_cond_wait(&cond, &mutex);
        }
        int connfd = popFromList();
        num_of_working++;
        pthread_mutex_unlock(&mutex);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //
        requestHandle(connfd);
        Close(connfd);
        pthread_mutex_lock(&mutex);
        num_of_working--;
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numthreads, queueSize;
    char* schedalg;
    struct sockaddr_in clientaddr;

    getargs(&port, argc, argv, 1);

    // 
    // HW3: Create some threads...
    //
    getargs(&numthreads,argc,argv, 2);
    for (int i = 0; i < numthreads; i++) {
        pthread_t t;
        pthread_create(&t, NULL, requestHandleByThread, NULL);
    }
    getargs(&queueSize,argc,argv, 3);
    getargs(&schedalg,argc,argv, 4);
    listenfd = Open_listenfd(port);
    bool block_flush_threshold = false;
    while (1) {
        clientlen = sizeof(clientaddr);
        pthread_mutex_lock(&mutex);
        if (strcmp(schedalg, "block")==0 && queueSize == getNumOfNodes()) {
            pthread_mutex_unlock(&mutex);
            continue;
        }
        if (strcmp(schedalg, "block_flush")==0 && queueSize == getNumOfNodes()) {
            pthread_mutex_unlock(&mutex);
            block_flush_threshold = true;
            continue;
        }
        if (block_flush_threshold) {
            if (queueSize > 0 || num_of_working > 0) {
                pthread_mutex_unlock(&mutex);
                continue;
            }
            block_flush_threshold = false;
        }
        pthread_mutex_unlock(&mutex);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        pthread_mutex_lock(&mutex);
        if (strcmp(schedalg, "drop_tail")==0 && queueSize == getNumOfNodes()) {
            pthread_mutex_unlock(&mutex);
            Close(connfd);
            continue;
        }
        if (connfd >= 0) {
            if (strcmp(schedalg,"drop_head")==0 && queueSize == getNumOfNodes()) {
                Close(popFromList());
            }
            insertList(connfd);
        }
        if (!isListEmpty()) {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }
}


    


 
