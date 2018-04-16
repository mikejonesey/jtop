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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <ncurses.h>
#include <stdbool.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dirent.h>

//stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//custom
#include "bufferFun.h"

#ifndef JTOP_JTOP_H
#define JTOP_JTOP_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <argp.h>
#include <ncurses.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/sysinfo.h>
#include <wait.h>
#include <sys/mman.h>
#include <pthread.h>
#include <dirent.h>

//stat
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct jthread{
    char pid[6];
    char name[22];
    char state[10];
    char pcpu[6];
    char ccpu[6];
    char minfault[100];
    char majfault[100];
    char secs[10];
    char segv[2];
    char command[400];
    char altcommand[400];
    int rawcpu;
    char cpu_total[50];
    int blocking;
    char wlock[20];
};

struct ignore_thread {
    char pid[6];
};

struct objectsync{
    char object[20]; //0x00000005507a65a0
    struct jthread* ptr_jthread;
};

struct class_cpu{
    char className[400];
    int totalcpu;
};

struct pollTopWinArgs{
    WINDOW *win_stack;
    WINDOW *win_jtop;
    WINDOW *win_ctop;
    char *javapid;
    int JTOP_WIN_MAX_LINE;
    int STACK_WIN_MAX_COL;
};

int cnt_threads;
int cnt_ignore_threads;
int cnt_objects;

unsigned int cntThreadRunning;
unsigned int cntThreadWaiting;
unsigned int cntThreadBlocked;

struct jthread *vmthread;
struct jthread arr_jthreads[10000];
struct ignore_thread arr_ignore_threads[100];
struct objectsync arr_objectsync[2000];
struct class_cpu arr_class_cpu[10];
int topActiveRow;
bool threadControl;
bool sleepMode;
bool filterMode;
int focusOn;
int orderMode;

struct pollTopWinArgs jtopWinData;

//cmdline options
static error_t parse_opt(int key, char *arg, struct argp_state *state);

//prep
char *getJavaPid(char *javapid);

//runtime
void *pollTopWindow(void *myargs);
int main(int argc, char **argv);

#endif //JTOP_JTOP_H
