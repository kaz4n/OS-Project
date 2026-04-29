#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "scheduler.h"

JobQueue* queue_init(void) {
    JobQueue *q = (JobQueue *)malloc(sizeof(JobQueue));
    if (!q) return NULL;
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

void queue_destroy(JobQueue *q) {
    if (!q) return;
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_empty);
    free(q);
}

int queue_enqueue(JobQueue *q, const char *cmd, int fd) {
    if (!q || !cmd) return -1;
    pthread_mutex_lock(&q->mutex);
    if (q->count >= MAX_JOBS) { pthread_mutex_unlock(&q->mutex); return -1; }
    SchedulerJob *j = &q->jobs[q->tail];
    j->job_id = q->count + 1;
    strncpy(j->command, cmd, sizeof(j->command) - 1);
    j->client_fd = fd;
    j->completed = 0;
    clock_gettime(CLOCK_MONOTONIC, &j->submit_time);
    pthread_cond_init(&j->completion_cond, NULL);
    q->tail = (q->tail + 1) % MAX_JOBS;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return j->job_id;
}

int queue_dequeue(JobQueue *q, SchedulerJob *j) {
    if (!q || !j) return 0;
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) { pthread_cond_wait(&q->not_empty, &q->mutex); }
    *j = q->jobs[q->head];
    q->head = (q->head + 1) % MAX_JOBS;
    q->count--;
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

int queue_is_empty(JobQueue *q) {
    if (!q) return 1;
    pthread_mutex_lock(&q->mutex);
    int e = (q->count == 0);
    pthread_mutex_unlock(&q->mutex);
    return e;
}

long get_elapsed_ms(struct timespec start, struct timespec end) {
    long sec = end.tv_sec - start.tv_sec;
    long nsec = end.tv_nsec - start.tv_nsec;
    return (sec * 1000) + (nsec / 1000000);
}