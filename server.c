#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "connfdList.h"
#include <stdbool.h>

pthread_cond_t cond;
pthread_mutex_t mutex;

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
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
}

void getnumthreads(int *numthreads, int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    *numthreads = atoi(argv[2]);
}

void* requestHandleByThread(void* args) {
    while (1) {

        pthread_mutex_lock(&mutex);
        while (isListEmpty()) {
            pthread_mutex_unlock(&mutex);
            pthread_cond_wait(&cond, &mutex);
            pthread_mutex_lock(&mutex);
        }
        int connfd = popFromList();
        pthread_mutex_unlock(&mutex);

        //
        // HW3: In general, don't handle the request in the main thread.
        // Save the relevant info in a buffer and have one of the worker threads
        // do the work.
        //

        requestHandle(connfd);
        Close(connfd);
    }
    return NULL;
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numthreads;
    struct sockaddr_in clientaddr;
    initializeList();

    getargs(&port, argc, argv);

    // 
    // HW3: Create some threads...
    //
    getnumthreads(&numthreads,argc,argv);
    for (int i = 0; i < numthreads; i++) {
        pthread_t t;
        pthread_create(&t, NULL, requestHandleByThread, NULL);
    }
    while (1) {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *) &clientaddr, (socklen_t *) &clientlen);
        if (connfd >= 0) {
            pthread_mutex_lock(&mutex);
            insertList(connfd);
            pthread_mutex_unlock(&mutex);
        }
        pthread_mutex_lock(&mutex);
        if (!isListEmpty()) {
            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }
}


    


 
