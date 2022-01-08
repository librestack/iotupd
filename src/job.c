/* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only */
/* Copyright (c) 2020-2022 Brett Sheffield <bacs@librecast.net> */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "job.h"
#include "log.h"

job_t *job_shift(job_queue_t *q)
{
	job_t *job;
	sem_wait(&q->lock);
	if ((job = q->next)) {
		q->next = q->next->next;
		if (!q->next) q->last = NULL;
	}
	sem_post(&q->lock);
	return job;
}

static job_t *job_shiftlock(job_queue_t *q, int(*lockf)(sem_t *sem))
{
	job_t *job = NULL;
	if (!lockf(&q->jobs)) {
		job = job_shift(q);
	}
	return job;
}

job_t *job_trywait(job_queue_t *q)
{
	return job_shiftlock(q, &sem_trywait);
}

job_t *job_wait(job_queue_t *q)
{
	return job_shiftlock(q, &sem_wait);
}

job_t *job_new(void *(*f)(void *), void *arg, size_t len, void (*callback)(void *), int flags)
{
	job_t *job = calloc(1, sizeof(job_t));
	job->f = f;
	if ((flags & JOB_COPY) == JOB_COPY) {
		job->arg = malloc(len);
		memcpy(job->arg, arg, len);
	}
	else {
		job->arg = arg;
	}
	job->len = len;
	job->flags = flags;
	job->callback = callback;
	sem_init(&job->done, 0, 0);
	return job;
}

job_t *job_push(job_queue_t *q, job_t *job)
{
	sem_wait(&q->lock);
	if (!q->next)
		q->next = job;
	if (q->last)
		q->last->next = job;
	q->last = job;
	sem_post(&q->jobs);
	sem_post(&q->lock);
	return job;
}

job_t *job_push_new(job_queue_t *q, void *(*f)(void *), void *arg, size_t len, void (*callback)(void *), int flags)
{
	job_t *job = job_new(f, arg, len, callback, flags);
	return job_push(q, job);
}

static void *job_seek(void *arg)
{
	job_thread_t *jt = (job_thread_t *)arg;
	job_t *job;
	void(*callback)(void *);
	while((job = job_wait(jt->q))) {
		if (job->f) job->ret = job->f(job->arg);
		if (job->flags & JOB_FREE) free(job->arg);
		callback = job->callback; /* avoid race */
		sem_post(&job->done);
		if (callback) callback(job);
	}
	/* never reached */
	return jt;
}

job_queue_t *job_queue_create(size_t nthreads)
{
	pthread_attr_t attr;
	job_queue_t *q = calloc(1, sizeof (job_queue_t));
	if (!q) return NULL;
	q->thread = calloc(nthreads, sizeof (job_thread_t));
	if (!q->thread) {
		free(q);
		return NULL;
	}
	sem_init(&q->done, 0, 0);
	sem_init(&q->jobs, 0, 0);
	sem_init(&q->lock, 0, 1);
	q->nthreads = nthreads;
	pthread_attr_init(&attr);
	DEBUG("creating %zu threads", nthreads);
	for (size_t z = 0; z < nthreads; z++) {
		q->thread[z].id = z;
		q->thread[z].q = q;
		pthread_create(&q->thread[z].thread, &attr, &job_seek, &q->thread[z]);
	}
	pthread_attr_destroy(&attr);
	return q;
}

void job_queue_destroy(job_queue_t *q)
{
	job_t *job;
	while ((job = job_shift(q))) {
		if (job->flags & JOB_FREE) free(job->arg);
		free(job);
	}
	DEBUG("destroying %zu threads", q->nthreads);
	for (size_t z = 0; z < q->nthreads; z++) {
		pthread_cancel(q->thread[z].thread);
	}
	for (size_t z = 0; z < q->nthreads; z++) {
		pthread_join(q->thread[z].thread, NULL);
	}
	sem_destroy(&q->lock);
	free(q->thread);
	free(q);
}
