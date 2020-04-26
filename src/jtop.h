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
    char pid[10]; // Thread ID
    char name[256]; // Name of Thread / "C2 CompilerThread1"
    char state[10]; // Running / Sleeping / Zombie...
    double pcpu; // CPU Usage... 0.0
    double ccpu; // Sampled CPU Usage... 0.0
    long minfault; // Minor page fault counter
    long majfault; // Major page fault counter
    long secs; // Seconds running / 123
    char segv[2]; // not used
    char command[400]; // Current Java Call / "org.eclipse.swt.internal.gtk.OS.Call(Native Method)"
    char altcommand[400]; // Filtered Java Call for project relevant code / "org.eclipse.swt.internal.gtk.OS.Call(Native Method)"
    int rawcpu; // CPU counter, internal / 123
    int blocking; // Count of threads blocked by this thread / 0
    char wlock[20];
    long c_switch_nv; // Non Volantary Context Switch Count / 123456
    long c_switch_v; // Volantary Context Switch Count / 123456
    double cc_switch_nv; // Sampled NVCS / 0.0
    double cc_switch_v; // Sampled VCS / 0.0
};

struct ignore_thread {
    char pid[10];
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
