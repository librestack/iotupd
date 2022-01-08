/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2020-2022 Brett Sheffield <bacs@librecast.net> */

#ifndef _JOB_H
#define _JOB_H 1

#include <pthread.h>
#include <semaphore.h>

#if 0
enum job_status {
	JOB_STATUS_READY,
	JOB_STATUS_DONE_OK,
	JOB_STATUS_DONE_ERR
};
#endif

enum job_flag {
	JOB_COPY = 1,	/* copy arg */
	JOB_FREE = 2,	/* free arg */
};

typedef struct job_s job_t;
typedef struct job_queue_s job_queue_t;
typedef struct job_thread_s job_thread_t;

struct job_s {
	void *(*f)(void *arg);		/* function for thread to call */
	void *arg;			/* pass this argument to f() */
	void *ret;			/* return value from f() */
	size_t len;			/* size of arg */
	job_t *next;			/* ptr to next job */
	int flags;			/* job flags */
	void (*callback)(void *);	/* callback when done */
	sem_t done;			/* set when job complete */
};

struct job_queue_s {
	job_t *next;			/* next job */
	job_t *last;			/* last job */
	size_t nthreads;		/* number of threads in pool */
	job_thread_t *thread;		/* array of threads */
	sem_t jobs;			/* semaphore of avail jobs */
	sem_t lock;			/* read/write lock */
	/* Optional semaphore - set manually when queue complete */
	sem_t done;			
};

struct job_thread_s {
	size_t id;
	pthread_t thread;
	job_queue_t *q;
};

/* Create new job queue with nthreads worker threads */
job_queue_t *job_queue_create(size_t nthreads);

/* Free a queue and join its threads */
void job_queue_destroy(job_queue_t *q);

/* Create new job.
 * callback is called with the job as argument. Can be called with &free to free
 * job when done if nothing else needs to access it */
job_t *job_new(void *(*f)(void *), void *arg, size_t len, void (*callback)(void *), int flags);

/* Push a new job onto the end of the queue. */
job_t *job_push(job_queue_t *q, job_t *job);

/* create a job and push onto the queue in one call */
job_t *job_push_new(job_queue_t *q, void *(*f)(void *), void *arg, size_t len, void (*callback)(void *), int flags);

/* Shift the next job from the front of the queue (FIFO) with no locks.
 * Use job_trywait() or job_wait() if lock required */
job_t *job_shift(job_queue_t *q);

/* Shift a job from the queue with locking. Return immediately if none
 * available */
job_t *job_trywait(job_queue_t *q);

/* Wait for a job. Does not return until one available */
job_t *job_wait(job_queue_t *q);

#endif /* _JOB_H */
