/*
 * Java process viewer, ordered by sampled cpu.
 *
 * Copyright (C) 2018, Michael Jones <mj@mikejonesey.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with the source; if not, see
 *  <http://www.gnu.org/licenses/>.
 */

#include "jtop.h"

#ifndef JTOP_TOPWINDOW_H
#define JTOP_TOPWINDOW_H

int printTop(WINDOW *topwindow, int thread_count, int STACK_WIN_MAX_COL);
void orderByCPU();
void orderByBlocked();

#endif //JTOP_TOPWINDOW_H
