/*
 * Copyright (C) 2014 Mark Hills <mark@xwax.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>

#include "debug.h"
#include "status.h"
#include "cues.h"
#include "rig.h"

static struct list cuess = LIST_INIT(cuess);

void cues_reset(struct cues *q)
{
    size_t n;

    for (n = 0; n < MAX_CUES; n++)
        q->position[n] = CUE_UNSET;
}

/*
 * Unset the given cue point
 */

void cues_unset(struct cues *q, unsigned int label)
{
    debug("clearing cue point %d", label);
    q->position[label] = CUE_UNSET;
}

void cues_set(struct cues *q, unsigned int label, double position)
{
    debug("setting cue point %d to %0.2f", label, position);
    assert(label < MAX_CUES);
    q->position[label] = position;
}

double cues_get(const struct cues *q, unsigned int label)
{
    assert(label < MAX_CUES);
    return q->position[label];
}

/*
 * Return: the previous cue point before the current position, or CUE_UNSET
 */

double cues_prev(const struct cues *q, double current)
{
    size_t n;
    double r;

    r = CUE_UNSET;

    for (n = 0; n < MAX_CUES; n++) {
        double p;

        p = q->position[n];
        if (p == CUE_UNSET)
            continue;

        if (p > r && p < current)
            r = p;
    }

    return r;
}

/*
 * Return: the next cue point after the given position, or CUE_UNSET
 */

double cues_next(const struct cues *q, double current)
{
    size_t n;
    double r;

    r = CUE_UNSET;

    for (n = 0; n < MAX_CUES; n++) {
        double p;

        p = q->position[n];
        if (p == CUE_UNSET)
            continue;

        if (p < r && p > current)
            r = p;
    }

    return r;
}

static void cues_string_free(char **p, int offset)
{
    char **a = p + offset;

    while (*a) {
        free(*a);
        a++;
    }
}

static void cues_string(struct cues *q, char **p)
{
    int n;
    char **a = p;

    while (*a)
        a++;
    
    for (n = 0; n < MAX_CUES; n++) {

        if ((*a = malloc(15)) == NULL)
            return;

        if (q->position[n] == CUE_UNSET) 
            sprintf(*a, "-");
        else 
            sprintf(*a, "%lf", q->position[n]);

        a++;
    }
    *a = NULL;
}

static int cues_init(struct cues *q, const char *cueloader, const char *path, const char *cmd)
{
    pid_t pid;
    char *p[30] = {"cueloader", cmd, path, NULL};

    fprintf(stderr, "%sing cues for '%s'...\n", cmd, path);

    cues_string(q,p);

    pid = fork_pipe_nb_ar(&q->fd, cueloader, p);

    cues_string_free(p,3);

    if (pid == -1)
        return -1;

    q->pid = pid;
    q->pe = NULL;
    q->terminated = false;
    q->refcount = 0;
    rb_reset(&q->rb);
    event_init(&q->completion);

    q->index = 0;

    list_add(&q->cuess, &cuess);
    rig_post_cues(q);

    return 0;
}

int cues_set_by_cueloader(struct cues *q, const char *cueloader, const char *path)
{
    return cues_init(q, cueloader, path, "LOAD");
}

int cues_save_by_cueloader(struct cues *q, const char *cueloader, const char *path)
{
    return cues_init(q, cueloader, path, "SAVE");
}

void cues_acquire(struct cues *q)
{
    q->refcount++;
}

void cues_pollfd(struct cues *q, struct pollfd *pe)
{
    assert(q->pid != 0);

    pe->fd = q->fd;
    pe->events = POLLIN;

    q->pe = pe;
}

double get_cue_point(const char *s)
{
    char *endptr;
    double point;

    if (s[0] == '-') /* dash is explicit cuepoint unset */
        return CUE_UNSET;

    if (s[0] == '\0') /* empty string is unchanged cuepoint */
        return -1;

    errno = 0;
    point = strtod(s, &endptr);
    if (errno == ERANGE || *endptr != '\0' || !isfinite(point) || point <= 0.0)
        return CUE_UNSET;

    return point;
}

static int read_from_pipe(struct cues *q)
{
    for (;;) {
        char *line;
        ssize_t z;
        double p;

        if (q->index > MAX_CUES) 
            return -1; /* stop loading anything when all cues set */

        z = get_line(q->fd, &q->rb, &line);
        if (z == -1) {
            if (errno == EAGAIN)
                return 0;
            perror("get_line");
            return -1;
        }

        if (z == 0)
            return -1;

        debug("cues got line '%s'", line);

        p = get_cue_point(line);
        if (p == -1) { 
            free(line);
            q->index++;
            continue; 
        }
        
        q->position[q->index] = p;
        q->index++;

    }
}

static void do_wait(struct cues *q)
{
    int status;

    assert(q->pid != 0);
    debug("waiting on pid %d", q->pid);

    if (close(q->fd) == -1)
        abort();

    if (waitpid(q->pid, &status, 0) == -1)
        abort();

    debug("wait for pid %d returned %d", q->pid, status);

    if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
        fprintf(stderr, "Cue loading/saving completed\n");
    } else {
        fprintf(stderr, "Cue loading/saving completed with status %d\n", status);
        if (!q->terminated)
            status_printf(STATUS_ALERT, "Error loading/saving cues");
    }

    q->pid = 0;
}

static void terminate(struct cues *q)
{
    assert(q->pid != 0);
    debug("terminating %d", q->pid);

    if (kill(q->pid, SIGTERM) == -1)
        abort();

    q->terminated = true;
}

static void cues_clear(struct cues *q)
{
    assert(q->pid == 0);
    list_del(&q->cuess);
    event_clear(&q->completion);
}

void cues_release(struct cues *q)
{
    debug("put %p, refcount=%d", q, q->refcount);
    q->refcount--;

    /* Load must terminate before this object goes away */

    if (q->refcount == 1 && q->pid != 0) {
        debug("%p still executing but not longer required", q);
        terminate(q);
        return;
    }

    if (q->refcount == 0) {
        cues_clear(q);
    }
}

void cues_handle(struct cues *q)
{
    assert(q->pid != 0);

    if (q->pe == NULL)
        return;

    if (q->pe->revents == 0)
        return;

    if (read_from_pipe(q) != -1)
        return;

    do_wait(q);
    fire(&q->completion, NULL);
    list_del(&q->rig);
    cues_release(q); 
}
