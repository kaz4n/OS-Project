#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scheduler.h"
#include "shell.h"

static long diff_ms(const struct timespec *start, const struct timespec *end)
{
    long sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return sec * 1000L + nsec / 1000000L;
}

static void sleep_quantum_ms(int quantum_ms)
{
    struct timespec ts;

    ts.tv_sec = quantum_ms / 1000;
    ts.tv_nsec = (long)(quantum_ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static int append_data(char **buffer, size_t *len, size_t *cap, const char *data, size_t data_len)
{
    if (*len + data_len + 1 > *cap)
    {
        size_t new_cap = (*cap == 0) ? 1024 : *cap;
        while (*len + data_len + 1 > new_cap)
        {
            new_cap *= 2;
        }

        char *new_buffer = realloc(*buffer, new_cap);
        if (new_buffer == NULL)
        {
            return 0;
        }

        *buffer = new_buffer;
        *cap = new_cap;
    }

    memcpy(*buffer + *len, data, data_len);
    *len += data_len;
    (*buffer)[*len] = '\0';
    return 1;
}

static int append_text(char **buffer, size_t *len, size_t *cap, const char *text)
{
    return append_data(buffer, len, cap, text, strlen(text));
}

static int execute_command_capture_output(const char *command, int quantum_ms,
                                          char **captured, size_t *captured_len,
                                          int *exit_code)
{
    int pipefd[2];
    pid_t pid;
    int child_status = 0;
    char *buffer = NULL;
    size_t len = 0;
    size_t cap = 0;

    if (pipe(pipefd) != 0)
    {
        return 0;
    }

    pid = fork();
    if (pid < 0)
    {
        close(pipefd[0]);
        close(pipefd[1]);
        return 0;
    }

    if (pid == 0)
    {
        close(pipefd[0]);

        if (dup2(pipefd[1], STDOUT_FILENO) < 0)
        {
            perror("dup2 stdout");
            _exit(1);
        }

        if (dup2(pipefd[1], STDERR_FILENO) < 0)
        {
            perror("dup2 stderr");
            _exit(1);
        }

        close(pipefd[1]);
        process_input((char *)command);
        fflush(stdout);
        fflush(stderr);
        _exit(0);
    }

    close(pipefd[1]);

    int flags = fcntl(pipefd[0], F_GETFL, 0);
    if (flags != -1)
    {
        fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);
    }

    while (1)
    {
        char chunk[1024];
        ssize_t nread;
        pid_t wait_result;

        while ((nread = read(pipefd[0], chunk, sizeof(chunk))) > 0)
        {
            if (!append_data(&buffer, &len, &cap, chunk, (size_t)nread))
            {
                kill(pid, SIGKILL);
                waitpid(pid, NULL, 0);
                close(pipefd[0]);
                free(buffer);
                return 0;
            }
        }

        wait_result = waitpid(pid, &child_status, WNOHANG);
        if (wait_result == pid)
        {
            break;
        }

        if (wait_result < 0)
        {
            close(pipefd[0]);
            free(buffer);
            return 0;
        }

        sleep_quantum_ms(quantum_ms);
    }

    while (1)
    {
        char chunk[1024];
        ssize_t nread = read(pipefd[0], chunk, sizeof(chunk));
        if (nread > 0)
        {
            if (!append_data(&buffer, &len, &cap, chunk, (size_t)nread))
            {
                close(pipefd[0]);
                free(buffer);
                return 0;
            }
            continue;
        }

        if (nread == 0)
        {
            break;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            continue;
        }

        break;
    }

    close(pipefd[0]);

    if (buffer == NULL)
    {
        buffer = calloc(1, 1);
        if (buffer == NULL)
        {
            return 0;
        }
    }

    if (WIFEXITED(child_status))
    {
        *exit_code = WEXITSTATUS(child_status);
    }
    else if (WIFSIGNALED(child_status))
    {
        *exit_code = 128 + WTERMSIG(child_status);
    }
    else
    {
        *exit_code = 1;
    }

    *captured = buffer;
    *captured_len = len;
    return 1;
}

static void complete_job(Job *job)
{
    pthread_mutex_lock(&job->mutex);
    job->done = 1;
    pthread_cond_signal(&job->done_cond);
    pthread_mutex_unlock(&job->mutex);
}

static void run_job(Job *job, int quantum_ms)
{
    char *captured = NULL;
    size_t captured_len = 0;
    int exit_code = 1;
    char footer[256];

    clock_gettime(CLOCK_REALTIME, &job->started_at);

    if (!execute_command_capture_output(job->command, quantum_ms, &captured, &captured_len, &exit_code))
    {
        captured = strdup("scheduler: failed to execute command\n");
        if (captured == NULL)
        {
            captured = calloc(1, 1);
        }

        if (captured != NULL)
        {
            captured_len = strlen(captured);
        }

        exit_code = 1;
    }

    job->output = captured;
    job->output_len = captured_len;
    job->exit_code = exit_code;

    clock_gettime(CLOCK_REALTIME, &job->finished_at);

    long queue_wait = diff_ms(&job->submitted_at, &job->started_at);
    long runtime = diff_ms(&job->started_at, &job->finished_at);

    snprintf(footer, sizeof(footer),
             "\n[scheduler] job=%d wait_ms=%ld runtime_ms=%ld quantum_ms=%d exit=%d\n",
             job->id, queue_wait, runtime, quantum_ms, job->exit_code);

    if (job->output == NULL)
    {
        job->output = calloc(1, 1);
        if (job->output == NULL)
        {
            return;
        }
    }

    size_t cap = job->output_len + 1;
    if (!append_text(&job->output, &job->output_len, &cap, footer))
    {
        free(job->output);
        job->output = strdup("scheduler: failed to append scheduler metadata\n");
        if (job->output == NULL)
        {
            job->output = calloc(1, 1);
        }
        if (job->output != NULL)
        {
            job->output_len = strlen(job->output);
        }
    }
}

static void *dispatcher_main(void *arg)
{
    Scheduler *scheduler = (Scheduler *)arg;

    while (1)
    {
        Job *job = NULL;

        pthread_mutex_lock(&scheduler->mutex);
        while (!scheduler->stop && scheduler->head == NULL)
        {
            pthread_cond_wait(&scheduler->has_jobs, &scheduler->mutex);
        }

        if (scheduler->stop && scheduler->head == NULL)
        {
            pthread_mutex_unlock(&scheduler->mutex);
            break;
        }

        job = scheduler->head;
        scheduler->head = job->next;
        if (scheduler->head == NULL)
        {
            scheduler->tail = NULL;
        }
        job->next = NULL;
        pthread_mutex_unlock(&scheduler->mutex);

        run_job(job, scheduler->quantum_ms);
        complete_job(job);
    }

    return NULL;
}

int scheduler_init(Scheduler *scheduler, int quantum_ms)
{
    if (scheduler == NULL)
    {
        return 0;
    }

    memset(scheduler, 0, sizeof(*scheduler));
    scheduler->quantum_ms = (quantum_ms > 0) ? quantum_ms : DEFAULT_QUANTUM_MS;
    scheduler->next_job_id = 1;

    if (pthread_mutex_init(&scheduler->mutex, NULL) != 0)
    {
        return 0;
    }

    if (pthread_cond_init(&scheduler->has_jobs, NULL) != 0)
    {
        pthread_mutex_destroy(&scheduler->mutex);
        return 0;
    }

    if (pthread_create(&scheduler->dispatcher_tid, NULL, dispatcher_main, scheduler) != 0)
    {
        pthread_cond_destroy(&scheduler->has_jobs);
        pthread_mutex_destroy(&scheduler->mutex);
        return 0;
    }

    return 1;
}

void scheduler_shutdown(Scheduler *scheduler)
{
    if (scheduler == NULL)
    {
        return;
    }

    pthread_mutex_lock(&scheduler->mutex);
    scheduler->stop = 1;
    pthread_cond_broadcast(&scheduler->has_jobs);
    pthread_mutex_unlock(&scheduler->mutex);

    pthread_join(scheduler->dispatcher_tid, NULL);

    pthread_mutex_lock(&scheduler->mutex);
    Job *job = scheduler->head;
    scheduler->head = NULL;
    scheduler->tail = NULL;
    pthread_mutex_unlock(&scheduler->mutex);

    while (job != NULL)
    {
        Job *next = job->next;
        free(job->output);
        pthread_cond_destroy(&job->done_cond);
        pthread_mutex_destroy(&job->mutex);
        free(job);
        job = next;
    }

    pthread_cond_destroy(&scheduler->has_jobs);
    pthread_mutex_destroy(&scheduler->mutex);
}

Job *scheduler_submit(Scheduler *scheduler, int client_fd, const char *command)
{
    Job *job;

    if (scheduler == NULL || command == NULL)
    {
        return NULL;
    }

    job = calloc(1, sizeof(*job));
    if (job == NULL)
    {
        return NULL;
    }

    pthread_mutex_init(&job->mutex, NULL);
    pthread_cond_init(&job->done_cond, NULL);

    job->client_fd = client_fd;
    snprintf(job->command, sizeof(job->command), "%s", command);
    clock_gettime(CLOCK_REALTIME, &job->submitted_at);

    pthread_mutex_lock(&scheduler->mutex);
    job->id = scheduler->next_job_id++;

    if (scheduler->tail == NULL)
    {
        scheduler->head = job;
        scheduler->tail = job;
    }
    else
    {
        scheduler->tail->next = job;
        scheduler->tail = job;
    }

    pthread_cond_signal(&scheduler->has_jobs);
    pthread_mutex_unlock(&scheduler->mutex);

    return job;
}

void scheduler_wait(Job *job)
{
    if (job == NULL)
    {
        return;
    }

    pthread_mutex_lock(&job->mutex);
    while (!job->done)
    {
        pthread_cond_wait(&job->done_cond, &job->mutex);
    }
    pthread_mutex_unlock(&job->mutex);
}

void scheduler_destroy_job(Job *job)
{
    if (job == NULL)
    {
        return;
    }

    free(job->output);
    pthread_cond_destroy(&job->done_cond);
    pthread_mutex_destroy(&job->mutex);
    free(job);
}
