#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stddef.h>
#include <time.h>
#include <pthread.h>

#define DEFAULT_QUANTUM_MS 200
#define MAX_COMMAND_SIZE 1024

typedef struct Job {
    int id;
    int client_fd;
    char command[MAX_COMMAND_SIZE];

    struct timespec submitted_at;
    struct timespec started_at;
    struct timespec finished_at;

    char *output;
    size_t output_len;
    int exit_code;

    int done;
    pthread_mutex_t mutex;
    pthread_cond_t done_cond;

    struct Job *next;
} Job;

typedef struct Scheduler {
    pthread_t dispatcher_tid;
    pthread_mutex_t mutex;
    pthread_cond_t has_jobs;

    Job *head;
    Job *tail;

    int stop;
    int quantum_ms;
    int next_job_id;
} Scheduler;

int scheduler_init(Scheduler *scheduler, int quantum_ms);
void scheduler_shutdown(Scheduler *scheduler);
Job *scheduler_submit(Scheduler *scheduler, int client_fd, const char *command);
void scheduler_wait(Job *job);
void scheduler_destroy_job(Job *job);

#endif
