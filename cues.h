/*
 * Copyright (C) 2015 Mark Hills <mark@xwax.org>
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

#ifndef CUES_H
#define CUES_H

#include <math.h>

#include "external.h"
#include "list.h"
#include "library.h"

#define MAX_CUES 16
#define CUE_UNSET (HUGE_VAL)

/*
 * A set of cue points
 */

struct cues {
    struct deck *deck;
    struct list cuess;
    unsigned int refcount;

    double position[MAX_CUES];

    struct event completion;
   /* State of cues loading */

    struct list rig;
    pid_t pid;
    int fd;
    struct pollfd *pe;
    bool terminated;
    unsigned int index;

    /* State of reader */

    struct rb rb;
};

void cues_reset(struct cues *q);

void cues_unset(struct cues *q, unsigned int label);
void cues_set(struct cues *q, unsigned int label, double position);
double cues_get(const struct cues *q, unsigned int label);
double cues_prev(const struct cues *q, double current);
double cues_next(const struct cues *q, double current);
int cues_set_by_cueloader(struct cues *q, const char *cueloader, const char *path);
int cues_save_by_cueloader(struct cues *q, const char *cueloader, const char *path);
void cues_acquire(struct cues *q);
void cues_pollfd(struct cues *q, struct pollfd *pe);
void cues_handle(struct cues *q);
#endif
