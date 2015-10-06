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

#ifndef RIG_H
#define RIG_H

#include "excrate.h"
#include "track.h"
#include "cues.h"

int rig_init();
void rig_clear();

int rig_main();

int rig_quit();

void rig_lock();
void rig_unlock();

void rig_post_track(struct track *t);
void rig_post_excrate(struct excrate *e);
void rig_post_cues(struct cues *q);

#endif
