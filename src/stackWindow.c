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

#include "stackWindow.h"

void printJavaStack(){
    int printline=0;
    int stacklines;
    if(filterMode==false){
        stacklines = javaThreadDump.cnt_stacklines;
    }else{
        stacklines = javaThreadDump.cnt_stacklines_filt;
    }
    for(int i=0; i<stacklines; i++){
        if(printline>jtopWindows.STACK_WIN_MAX_LINE-2){
            break;
        }
        //clear line...
        wmove(jtopWindows.win_stack, printline+1, 1);
        wclrtoeol(jtopWindows.win_stack);

        //print line...
        if(filterMode==false){
            mvwprintw(jtopWindows.win_stack, (printline + 1), 1, "%d: %s", i, javaThreadDump.arr_stacklines[i]);
        }else{
            mvwprintw(jtopWindows.win_stack, (printline + 1), 1, "%d: %s", i, javaThreadDump.arr_stacklines_filt[i]);
        }
        printline++;
    }
    wrefresh(jtopWindows.win_stack);
}

void printJavaThreadStack(WINDOW *win_stack, const char *threadName, char *stacklines[], int STACK_WIN_MAX_LINE){
    int printline=0;
    if(strcmp(threadName,"New-thread")==0){
        while(printline<STACK_WIN_MAX_LINE) {
            wmove(win_stack, printline + 1, 1);
            wclrtoeol(win_stack);
            printline++;
        }
        return;
    }

    //search string
    char threadSearch[23];
    memset(threadSearch, 0, sizeof(threadSearch));
    strncpy(threadSearch,threadName,22);
    if(strlen(threadName)<21){
        threadSearch[strlen(threadSearch)]='"';
    }

    //vars
    bool thread_found=false;

    //loop rows
    for(int i=0; i<=javaThreadDump.cnt_stacklines; i++){
        // check for thread found
        if(thread_found==false && i<javaThreadDump.cnt_stacklines){
            if(strstr(stacklines[i],threadSearch)){
                thread_found=true;
            }else{
                continue;
            }
        }
        // check for thread end or end of file
        if(!stacklines[i] || strlen(stacklines[i])<2 || i>=javaThreadDump.cnt_stacklines-1){
            while(printline<STACK_WIN_MAX_LINE+2){
                wmove(win_stack, printline+1, 1);
                wclrtoeol(win_stack);
                printline++;
            }
            break;
        }
        //clear line...
        wmove(win_stack, printline+1, 1);
        wclrtoeol(win_stack);

        //print line...
        mvwprintw(jtopWindows.win_stack, (printline + 1), 1, "%d: %s", (i), javaThreadDump.arr_stacklines[i]);

        printline++;
    }
    wrefresh(win_stack);
}

int getLineJavaStack(const char * srchString, char *stacklines[]){
    int printline=0;
    for(int i=0; i<javaThreadDump.cnt_stacklines; i++){
        if(strstr(stacklines[i],srchString)){
            break;
        }
        printline++;
    }
    printline--;
    printline--;
    printline--;
    return printline;
}