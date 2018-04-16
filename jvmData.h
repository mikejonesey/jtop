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

#ifndef JTOP_JVMDATA_H
#define JTOP_JVMDATA_H

struct jthread *vmthread;

int hex2int(const char *hexin);

struct jthread* getThread(char* tpid, int cnt_threads);

void getExcludes(int *cnt_exclude, char *arr_exclude[]);
int getJavaStack(char *javaPID, int *cnt_win_stack_rows_p, char *stacklines[], int STACK_WIN_MAX_COL, int *cntThreadRunning_p, int *cntThreadWaiting_p, int *cntThreadBlocked_p, int cnt_excludes, char *arr_excludes[]);
void getJavaStackFiltered(int cnt_win_stack_rows, int *cnt_win_stack_rows_filtp, int cnt_excludes, char* arr_excludes[], char *stacklines[], char *stacklines_filt[]);

#endif //JTOP_JVMDATA_H
