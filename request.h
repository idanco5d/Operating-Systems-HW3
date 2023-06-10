#ifndef __REQUEST_H__

typedef struct handler_thread_stats_t {
    unsigned int handler_thread_id;
    unsigned int handler_thread_req_count;
    unsigned int handler_thread_static_req_count;
    unsigned int handler_thread_dynamic_req_count;
} thread_stats;

typedef struct stats_t {
    struct timeval arrival_time;
    struct timeval dispatch_interval;
    thread_stats* handler_thread_stats;
} stats_struct;

void requestHandle(int fd, stats_struct* stats);

#endif
