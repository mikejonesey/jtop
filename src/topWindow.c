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


#include "topWindow.h"

void clearLine(const unsigned int lineno){
    char charwidth[jtopWindows.JTOP_WIN_MAX_COL];
    memset(charwidth, '\0', sizeof(charwidth));
    memset(charwidth, ' ', sizeof(charwidth)-1);
    mvwprintw(jtopWindows.win_jtop, lineno, 1, "%s", charwidth);
}

void topPrintPid(const int threadID, const int row, int *col){
    //PID
    mvwprintw(jtopWindows.win_jtop, row, *col, "%s", arr_jthreads[threadID].pid);
    *col+=7;
}

void topPrintState(const int threadID, const int row, int *col){
    //STATE
    if (arr_jthreads[threadID].state){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%s", arr_jthreads[threadID].state);
    }
    *col+=8;
}

void topPrintCPUInfo(const int threadID, const int row, int *col){
    //PCPU
    mvwprintw(jtopWindows.win_jtop, row, *col, "%0.2f", arr_jthreads[threadID].pcpu);
    *col+=7;

    //CCPU
    mvwprintw(jtopWindows.win_jtop, row, *col, "%0.2f", arr_jthreads[threadID].ccpu);
    *col+=7;
}

void topPrintFaultInfo(const int threadID, const int row, int *col){
    //MinFault
    mvwprintw(jtopWindows.win_jtop, row, *col, "%o", arr_jthreads[threadID].minfault);
    *col+=11;

    //MajFault
    mvwprintw(jtopWindows.win_jtop, row, *col, "%o", arr_jthreads[threadID].majfault);
    *col+=11;
}

void topPrintSecs(const int threadID, const int row, int *col){
    //SECS
    if(arr_jthreads[threadID].secs<9999999){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%d", arr_jthreads[threadID].secs);
    }else{
        mvwprintw(jtopWindows.win_jtop, row, *col, "E");
    }
    *col+=8;
}

void topPrintContextSwitch(const int threadID, const int row, int *col){
    //Volantary Conext Switch
    if (arr_jthreads[threadID].cc_switch_v<999){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%d", arr_jthreads[threadID].cc_switch_v);
    }else {
        mvwprintw(jtopWindows.win_jtop, row, *col, "M");
    }
    *col+=5;

    //Non Volantary Conext Switch
    if (arr_jthreads[threadID].cc_switch_nv<999){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%d", arr_jthreads[threadID].cc_switch_nv);
    }else {
        mvwprintw(jtopWindows.win_jtop, row, *col, "M");
    }
    *col+=5;
}

void topPrintBlocks(const int threadID, const int row, int *col){
    //Blocking Count
    if (arr_jthreads[threadID].blocking){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%d", arr_jthreads[threadID].blocking);
    }else{
        mvwprintw(jtopWindows.win_jtop, row, *col, "0");
    }
    *col+=5;
}

void topPrintName(const int threadID, const int row, int *col){
    //Name
    if (arr_jthreads[threadID].name && arr_jthreads[threadID].name[0] != '\0'){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%s", arr_jthreads[threadID].name);
        if(strlen(arr_jthreads[threadID].name)>26){
            mvwprintw(jtopWindows.win_jtop, row, *col+23, "... ");
        }
    }
    *col+=27;
}

void topPrintCommand(const int threadID, const int row, int *col){
    //Command
    if(filterMode && arr_jthreads[threadID].altcommand && arr_jthreads[threadID].altcommand[0] != '\0'){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%s", arr_jthreads[threadID].altcommand);
    }else if (arr_jthreads[threadID].command && arr_jthreads[threadID].command[0] != '\0'){
        mvwprintw(jtopWindows.win_jtop, row, *col, "%s", arr_jthreads[threadID].command);
    }else{
        mvwprintw(jtopWindows.win_jtop, row, *col, "n/a          ");
    }
    *col+=97;
}

int printTop(int thread_count) {
    int printRow;
    int printCol=1;
    int topOffset=0; //which line to print from...
    if(topActiveRow>=(jtopWindows.JTOP_WIN_MAX_LINE-3)){
        topOffset = topActiveRow-(jtopWindows.JTOP_WIN_MAX_LINE-4);
    }
    for (int i = topOffset; i < thread_count; i++) {
        printRow=i+2;
        printRow-=topOffset;

        if (!arr_jthreads[i].pid){
            continue;
        }

        //clear line...
        wmove(jtopWindows.win_jtop, printRow, 1);
        wclrtoeol(jtopWindows.win_jtop);

        if(topActiveRow==i){
            wattron(jtopWindows.win_jtop, COLOR_PAIR(5));
            clearLine(printRow);
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>1){
            wattron(jtopWindows.win_jtop, COLOR_PAIR(3));
            clearLine(printRow);
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>0){
            wattron(jtopWindows.win_jtop, COLOR_PAIR(2));
            clearLine(printRow);
        }

        printCol=1;

        topPrintPid(i, printRow, &printCol);
        topPrintState(i, printRow, &printCol);
        topPrintCPUInfo(i, printRow, &printCol);
        topPrintFaultInfo(i, printRow, &printCol);
        topPrintSecs(i, printRow, &printCol);
        topPrintContextSwitch(i, printRow, &printCol);
        topPrintBlocks(i, printRow, &printCol);
        topPrintName(i,printRow, &printCol);
        topPrintCommand(i,printRow, &printCol);

        if(topActiveRow==i){
            wattroff(jtopWindows.win_jtop, COLOR_PAIR(5));
            use_default_colors();
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>1){
            wattroff(jtopWindows.win_jtop, COLOR_PAIR(3));
            use_default_colors();
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>0){
            wattroff(jtopWindows.win_jtop, COLOR_PAIR(2));
            use_default_colors();
        }

    }

    if(focusOn==0){
        box(jtopWindows.win_jtop, 0, 0);
    }else{
        wattron(jtopWindows.win_jtop, A_BOLD);
        wattron(jtopWindows.win_jtop, COLOR_PAIR(7));
        box(jtopWindows.win_jtop, 0, 0);
        wattroff(jtopWindows.win_jtop, A_BOLD);
        wattroff(jtopWindows.win_jtop, COLOR_PAIR(7));
        use_default_colors();
    }
    wrefresh(jtopWindows.win_jtop);
    return 0;
}

void orderByCPU(){
    unsigned int thread_count = cnt_threads;
    struct jthread tmp_jthread;
    unsigned int cpufulli;
    unsigned int cpufullii;
    for(int i=0; i<thread_count; i++){
        // set variables from Set A
        if(arr_jthreads[i].rawcpu){
            cpufulli=arr_jthreads[i].rawcpu;
        }else{
            cpufulli=0;
        }
        for(int ii=i+1; ii<thread_count; ii++){
            // set variables from Set B
            if(arr_jthreads[ii].rawcpu){
                cpufullii=arr_jthreads[ii].rawcpu;
            }else{
                cpufullii=0;
            }
            // compare vars...
            if((arr_jthreads[i].blocking<arr_jthreads[ii].blocking)||
               (arr_jthreads[i].ccpu<arr_jthreads[ii].ccpu && arr_jthreads[i].blocking<=arr_jthreads[ii].blocking)||
               (cpufulli<cpufullii && arr_jthreads[i].ccpu<=arr_jthreads[ii].ccpu && arr_jthreads[i].blocking<=arr_jthreads[ii].blocking)){
                // off to temp
                tmp_jthread = arr_jthreads[i];
                // shift
                arr_jthreads[i] = arr_jthreads[ii];
                // back from temp
                arr_jthreads[ii] = tmp_jthread;
                cpufulli=cpufullii;
            }
        }

    }
}

void orderByBlocked(){
    unsigned int thread_count = cnt_threads;
    struct jthread tmp_jthread;
    unsigned int blockedi;
    unsigned int blockedii;
    for(int i=0; i<thread_count; i++){
        // set variables from Set A
        blockedi=arr_jthreads[i].blocking;
        for(int ii=i+1; ii<thread_count; ii++){
            // set variables from Set B
            blockedii=arr_jthreads[ii].blocking;
            // compare vars...
            if(blockedi<blockedii){
                // off to temp
                tmp_jthread = arr_jthreads[i];
                // shift
                arr_jthreads[i] = arr_jthreads[ii];
                // back from temp
                arr_jthreads[ii] = tmp_jthread;
            }
        }

    }
}
