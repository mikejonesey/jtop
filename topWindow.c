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

int printTop(WINDOW *topwindow, int thread_count, int STACK_WIN_MAX_COL) {
    //topActiveRow == global, current selected row from top menu
    int printRow;
    int topOffset=0; //which line to print from...
    if(topActiveRow>=(jtopWinData.JTOP_WIN_MAX_LINE-3)){
        topOffset = topActiveRow-(jtopWinData.JTOP_WIN_MAX_LINE-4);
    }
    //mvwprintw(topwindow, 1, 150, "%d", topOffset);
    for (int i = topOffset; i < thread_count; i++) {
        printRow=i+2;
        printRow-=topOffset;

        //clear line...
        wmove(topwindow, printRow, 1);
        wclrtoeol(topwindow);

        if(topActiveRow==i){
            wattron(topwindow, COLOR_PAIR(5));

            // print whole line...
            char charwidth[STACK_WIN_MAX_COL];
            memset(charwidth, ' ', sizeof(charwidth));
            charwidth[STACK_WIN_MAX_COL-1]='\0';
            mvwprintw(topwindow, printRow, 1, "%s", charwidth);
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>1){
            wattron(topwindow, COLOR_PAIR(3));
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>0){
            wattron(topwindow, COLOR_PAIR(2));
        }

        if (arr_jthreads[i].pid){

            //PID
            if (arr_jthreads[i].pid && arr_jthreads[i].pid[0] != '\0') {
                mvwprintw(topwindow, printRow, 1, "%s", arr_jthreads[i].pid);
            }

            //STATE
            if (arr_jthreads[i].state){
                mvwprintw(topwindow, printRow, 8, "%s", arr_jthreads[i].state);
            }else{
                mvwprintw(topwindow, printRow, 8, "?");
            }

            //PCPU
            if (arr_jthreads[i].pcpu){
                mvwprintw(topwindow, printRow, 16, "%s", arr_jthreads[i].pcpu);
            }else{
                mvwprintw(topwindow, printRow, 16, "?");
            }

            //CCPU
            if (arr_jthreads[i].ccpu){
                mvwprintw(topwindow, printRow, 23, "%s", arr_jthreads[i].ccpu);
            }else{
                mvwprintw(topwindow, printRow, 23, "?");
            }

            //MinFault
            if (arr_jthreads[i].minfault && arr_jthreads[i].minfault[0] != '\0'){
                mvwprintw(topwindow, printRow, 30, "%s", arr_jthreads[i].minfault);

            } else {
                mvwprintw(topwindow, printRow, 30, "?");
            }

            //MajFault
            if (arr_jthreads[i].majfault && arr_jthreads[i].majfault[0] != '\0'){
                mvwprintw(topwindow, printRow, 41, "%s", arr_jthreads[i].majfault);
            }else{
                mvwprintw(topwindow, printRow, 41, "?");
            }

            //SECS
            if (arr_jthreads[i].secs){
                if(strlen(arr_jthreads[i].secs)>7){
                    mvwprintw(topwindow, printRow, 52, "E");
                }else{
                    mvwprintw(topwindow, printRow, 52, "%s", arr_jthreads[i].secs);
                }
            }else{
                mvwprintw(topwindow, printRow, 52, "?");
            }

            //SEGV
            /*
            if (arr_jthreads[i].segv && arr_jthreads[i].segv[0] != '\0'){
                mvwprintw(topwindow, printRow, 60, "%s", arr_jthreads[i].segv);
            }else{
                mvwprintw(topwindow, printRow, 60, "0");
            }
            */

            //Volantary Conext Switch
            if (arr_jthreads[i].cc_switch_v && arr_jthreads[i].cc_switch_v[0] != '\0'){
                mvwprintw(topwindow, printRow, 60, "%s", arr_jthreads[i].cc_switch_v);
            }else {
                mvwprintw(topwindow, printRow, 60, "0");
            }

            //Non Volantary Conext Switch
            if (arr_jthreads[i].cc_switch_nv && arr_jthreads[i].cc_switch_nv[0] != '\0'){
                mvwprintw(topwindow, printRow, 65, "%s", arr_jthreads[i].cc_switch_nv);
            }else {
                mvwprintw(topwindow, printRow, 65, "0");
            }

            //Blocking Count
            if (arr_jthreads[i].blocking){
                mvwprintw(topwindow, printRow, 70, "%d", arr_jthreads[i].blocking);
            }else{
                mvwprintw(topwindow, printRow, 70, "0");
            }

            //Name
            if (arr_jthreads[i].name && arr_jthreads[i].name[0] != '\0'){
                mvwprintw(topwindow, printRow, 75, "%s", arr_jthreads[i].name);
                if(strlen(arr_jthreads[i].name)>26){
                    mvwprintw(topwindow, printRow, 98, "... ");
                }
            }else{
                mvwprintw(topwindow, printRow, 75, "?");
            }

            //Command
            if(filterMode && arr_jthreads[i].altcommand && arr_jthreads[i].altcommand[0] != '\0'){
                mvwprintw(topwindow, printRow, 102, "%s", arr_jthreads[i].altcommand);
            }else if (arr_jthreads[i].command && arr_jthreads[i].command[0] != '\0'){
                mvwprintw(topwindow, printRow, 102, "%s", arr_jthreads[i].command);
            }else{
                mvwprintw(topwindow, printRow, 102, "n/a");
            }

            //raw cpu
            //if (arr_jthreads[i].rawcpu!=NULL){
            //    mvwprintw(topwindow, printRow, 173, "%s", arr_jthreads[i].rawcpu);
            //}else{
            //    mvwprintw(topwindow, printRow, 173, "?");
            //}
        }else{
            mvwprintw(topwindow, printRow, 7, "pid null");
        }

        if(topActiveRow==i){
            wattroff(topwindow, COLOR_PAIR(5));
            use_default_colors();
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>1){
            wattroff(topwindow, COLOR_PAIR(3));
            use_default_colors();
        }else if(arr_jthreads[i].blocking && arr_jthreads[i].blocking>0){
            wattroff(topwindow, COLOR_PAIR(2));
            use_default_colors();
        }

    }

    if(focusOn==0){
        box(topwindow, 0, 0);
    }else{
        wattron(topwindow, A_BOLD);
        wattron(topwindow, COLOR_PAIR(7));
        box(topwindow, 0, 0);
        wattroff(topwindow, A_BOLD);
        wattroff(topwindow, COLOR_PAIR(7));
        use_default_colors();
    }
    wrefresh(topwindow);
    return 0;
}

void orderByCPU(){
    int thread_count = cnt_threads;
    struct jthread tmp_jthread;
    int blockedi,blockedii,cpufulli,cpufullii;
    double cpunowi, cpunowii;
    for(int i=0; i<thread_count; i++){
        // set variables from Set A
        blockedi=arr_jthreads[i].blocking;
        if(arr_jthreads[i].rawcpu){
            cpufulli=arr_jthreads[i].rawcpu;
        }else{
            cpufulli=0;
        }
        if(arr_jthreads[i].ccpu){
            cpunowi=strtod(arr_jthreads[i].ccpu,NULL);
        }else{
            cpunowi=0;
        }
        for(int ii=i+1; ii<thread_count; ii++){
            // set variables from Set B
            blockedii=arr_jthreads[ii].blocking;
            if(arr_jthreads[ii].rawcpu){
                cpufullii=arr_jthreads[ii].rawcpu;
            }else{
                cpufullii=0;
            }
            if(arr_jthreads[ii].ccpu){
                cpunowii=strtod(arr_jthreads[ii].ccpu,NULL);
            }else{
                cpunowii=0;
            }
            // compare vars...
            if((blockedi<blockedii)||
               (cpunowi<cpunowii && blockedi<=blockedii)||
               (cpufulli<cpufullii && cpunowi<=cpunowii && blockedi<=blockedii)){
                // off to temp
                tmp_jthread = arr_jthreads[i];
                // shift
                arr_jthreads[i] = arr_jthreads[ii];
                // back from temp
                arr_jthreads[ii] = tmp_jthread;
                cpunowi=cpunowii;
                cpufulli=cpufullii;
                blockedi=blockedii;
            }
        }

    }
}

void orderByBlocked(){
    int thread_count = cnt_threads;
    struct jthread tmp_jthread;
    int blockedi,blockedii;
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
