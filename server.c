#include "segel.h"
#include "request.h"
#include <pthread.h>
#include "connfdList.h"
#include <stdbool.h>

pthread_cond_t cond;
pthread_cond_t cond_master;
pthread_mutex_t mutex;
unsigned int queueSize;
unsigned int num_of_working = 0;

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

typedef struct {
    unsigned int *thread_num;
    char *schedalg;
} worker_thread_input;

void* requestHandleByThread(void* args) {
    worker_thread_input * input = (worker_thread_input *)args;
    thread_stats threadStats = {*(input->thread_num), 0, 0, 0};
    stats_struct stats;
    stats.handler_thread_stats = &threadStats;
    while (1) {
        pthread_mutex_lock(&mutex);
        while (isListEmpty() || num_of_working == queueSize) {
            pthread_cond_wait(&cond, &mutex);
        }
        connfdNode connfd_node = popFromList();
        num_of_working++;
        pthread_mutex_unlock(&mutex);
        //calculate current stats
        stats.arrival_time = connfd_node.arrival_time;
        struct timeval curr_dispatch_time;
        gettimeofday(&curr_dispatch_time, NULL);
        timersub(&curr_dispatch_time, &connfd_node.arrival_time, &stats.dispatch_interval);
        //invoke request and close the connection after handling
        requestHandle(connfd_node.connfd, &stats);
        Close(connfd_node.connfd);
        pthread_mutex_lock(&mutex);
        num_of_working--;
        if ((strcmp(input->schedalg,"bf") == 0 && num_of_working == 0 && getNumOfNodes() == 0) || strcmp(input->schedalg,"block") == 0 || strcmp(input->schedalg,"random") == 0) {
            pthread_cond_signal(&cond_master);
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


bool handleMasterThreadInDynamicOrDropTail(int connfd, int maxSize, char* schedalg) {
    if (strcmp(schedalg, "dynamic")==0) {
        if (queueSize < maxSize) {
            (queueSize)++;
        }
    }
    if (strcmp(schedalg, "dynamic")==0 || strcmp(schedalg, "dt")==0) {
        Close(connfd);
        return true;
    }
    return false;
}

bool handleBlockInMasterThread(int connfd, char* schedalg) {
    if (strcmp(schedalg, "block")==0 || strcmp(schedalg, "bf")==0) {
        while (queueSize == getNumOfNodes() + num_of_working) {
                pthread_cond_wait(&cond_master, &mutex);
        }
    }
    if (strcmp(schedalg, "bf")==0) {
        Close(connfd);
        return true;
    }
    return false;
}

bool handleDropHeadInMasterThread(int connfd, char* schedalg) {
    if (strcmp(schedalg,"dh")==0) {
        if (getNumOfNodes() > 0) {
            int oldestRequest = popFromList().connfd;
            Close(oldestRequest);
        }
        else {
            Close(connfd);
            return true;
        }
    }
    return false;
}

void handleDropRandomInMasterThread(char* schedalg) {
    if (strcmp(schedalg,"random")==0) {
        dropHalfList();
        pthread_cond_wait(&cond_master, &mutex);
    }
}

int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numthreads, maxSize;
    struct sockaddr_in clientaddr;
    char* schedalg = (char*)malloc(sizeof(char)*(strlen(argv[4])+1));
    strcpy(schedalg,argv[4]);
    if (strcmp(schedalg, "block") != 0 && strcmp(schedalg, "bf") != 0 && strcmp(schedalg, "dh") != 0
    && strcmp(schedalg, "dt") != 0 && strcmp(schedalg, "dynamic") != 0 && strcmp(schedalg, "random") != 0) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&cond,NULL);
    pthread_cond_init(&cond_master,NULL);
    getargs(&port, argc, argv, 1);
    getargs(&numthreads,argc,argv, 2);
    getargs(&queueSize,argc,argv, 3);
    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t)*numthreads);
    for (int i = 0; i < numthreads; i++) {
        unsigned int* threadNum = (unsigned int*)malloc(sizeof(int));
        *threadNum = i;
        worker_thread_input* input = (worker_thread_input*)malloc(sizeof(worker_thread_input));
        input->thread_num = threadNum;
        input->schedalg = schedalg;
        pthread_create(&threads[i], NULL, requestHandleByThread, (void*)(input));
    }
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
        if (connfd >= 0) {
            if (queueSize <= getNumOfNodes() + num_of_working) {
                if (handleBlockInMasterThread(connfd,schedalg)) {
                    pthread_mutex_unlock(&mutex);
                    continue;
                }
                if (handleMasterThreadInDynamicOrDropTail(connfd, maxSize, schedalg)) {
                    pthread_mutex_unlock(&mutex);
                    continue;
                }
                if (handleDropHeadInMasterThread(connfd, schedalg)) {
                    pthread_mutex_unlock(&mutex);
                    continue;
                }
                handleDropRandomInMasterThread(schedalg);
            }
            insertList(connfd, curr_arrival_time);
        }
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
    }
}


    


 
