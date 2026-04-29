#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <limits.h>
#include <time.h>
#include <pthread.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define TIME_QUANTUM_MS 200
#define MAX_JOBS 1024

typedef struct {
    int job_id;
    char command[1024];
    int client_fd;
    int session_id;
    char cwd[PATH_MAX];
    struct timespec submit_time;
    struct timespec start_time;
    struct timespec end_time;
    int completed;
    int exit_code;
    pthread_cond_t completion_cond;
} SchedulerJob;

typedef struct {
    SchedulerJob jobs[MAX_JOBS];
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} JobQueue;

JobQueue* queue_init(void);
void queue_destroy(JobQueue *queue);
int queue_enqueue(JobQueue *queue, const char *command, int client_fd, const char *cwd, int session_id);
int queue_dequeue(JobQueue *queue, SchedulerJob *job);
int queue_is_empty(JobQueue *queue);
long get_elapsed_ms(struct timespec start, struct timespec end);

#endif
