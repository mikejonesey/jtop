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

/*
 * memory notes...
 * mallocated variables...
 * arr_exclude[N] = used to store excluded class, don't know the length of these...
 * javaThreadDump.arr_stacklines[N] = used to store stack traces from java, don't know the length of these...
 * everything else is pre-allocated.
*/

//struct defs
struct jthread{
    char pid[10]; // Thread ID
    char name[256]; // Name of Thread / "C2 CompilerThread1"
    char state[10]; // Running / Sleeping / Zombie...
    double pcpu; // CPU Usage... 0.0
    double ccpu; // Sampled CPU Usage... 0.0
    unsigned long minfault; // Minor page fault counter
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

struct jtopWindowObjects{
    WINDOW *win_stack;
    WINDOW *win_jtop;
    WINDOW *win_ctop;
    char *javapid;
    unsigned int JTOP_WIN_MAX_COL;
    unsigned int JTOP_WIN_MAX_LINE;
    unsigned int STACK_WIN_MAX_COL;
    unsigned int STACK_WIN_MAX_LINE;
    unsigned int stack_win_scroll_cnt;
};

struct struct_javaThreadDump{
    unsigned int cnt_stacklines;
    unsigned int cnt_stacklines_filt;
    char *arr_stacklines[100000];
    char *arr_stacklines_filt[100000];
};

//gvars
extern struct jtopWindowObjects jtopWindows;
extern struct struct_javaThreadDump javaThreadDump;

extern int cnt_threads;
extern int cnt_ignore_threads;
extern int cnt_objects;

extern unsigned int cntThreadRunning;
extern unsigned int cntThreadWaiting;
extern unsigned int cntThreadBlocked;

extern struct jthread *vmthread;
extern struct jthread arr_jthreads[10000];
extern struct ignore_thread arr_ignore_threads[100];
extern struct objectsync arr_objectsync[2000];
extern struct class_cpu arr_class_cpu[10];
extern int topActiveRow;
extern bool threadControl;
extern bool sleepMode;
extern bool filterMode;
extern int focusOn;
extern int orderMode;

//functions
char *getJavaPid(char *javapid);
void *pollTopWindow(void *myargs);
int main(int argc, char **argv);

#endif //JTOP_JTOP_H
