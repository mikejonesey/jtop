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

//var
extern struct jthread *vmthread;

// func
int hex2int(const char *hexin);
struct jthread* getThread(const char* tpid);
void getExcludes(int *cnt_exclude, char *arr_exclude[]);
int getJavaStack(char *javaPID, int *cntThreadRunning_p, int *cntThreadWaiting_p, int *cntThreadBlocked_p, int cnt_excludes, char *arr_excludes[]);
void getJavaStackFiltered(int cnt_excludes, char* arr_excludes[]);

#endif //JTOP_JVMDATA_H
