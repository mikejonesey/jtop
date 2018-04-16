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

#include "procData.h"
#include "classcpuWindow.h"

void getStat(char *javapid){
    int thread_count = cnt_threads;
    char tstat_txt[1000];
    char tstat_path[256];
    memset(tstat_path, 0, sizeof(tstat_path));
    FILE *f_tstat;

    struct sysinfo info;
    sysinfo(&info);
    const long int SYS_UPTIME = info.uptime;
    long SYS_JIFFPS = sysconf(_SC_CLK_TCK);
    const long int SYS_CURTIME = time(NULL);
    const long int SYS_BOOTTIME = (SYS_CURTIME - SYS_UPTIME);

    for(int i=0; i<thread_count; i++){
        if(arr_jthreads[i].pid && arr_jthreads[i].pid[0]!='\0'){
            //////////////////////////////////////////////////
            // Get new props
            //////////////////////////////////////////////////
            strcat(tstat_path, "/proc/");
            strcat(tstat_path, javapid);
            strcat(tstat_path, "/task/");
            //strcat(tstat_path, threads[i][0]);
            strcat(tstat_path, arr_jthreads[i].pid);
            strcat(tstat_path, "/stat");
            f_tstat = fopen(tstat_path, "r");
            if (f_tstat) {
                fgets(tstat_txt, sizeof(tstat_txt), f_tstat);
                fclose(f_tstat);
                int space_count = 0;

                int cnt_proc_state=0;
                char proc_state[100];

                int cnt_proc_minflt=0;
                char proc_minflt[100];

                int cnt_proc_majflt=0;
                char proc_majflt[100];

                int cnt_proc_utime=0;
                char proc_utime[100];

                int cnt_proc_stime=0;
                char proc_stime[100];

                int cnt_proc_startime=0;
                char proc_startime[100];
                if(tstat_txt&&strstr(tstat_txt," ")){
                    for(int filei=0; filei<512; filei++){
                        if (space_count > 21||tstat_txt[filei]=='\0') {
                            break;
                        }
                        if (tstat_txt[filei] == ' ') {
                            space_count++;
                            continue;
                        }
                        //space_count==0 //pid
                        //space_count==1 //comm
                        if (space_count == 2) {
                            //state (R|S|D|Z|T|t|W|X|x|K|W|P)
                            proc_state[cnt_proc_state] = tstat_txt[filei];
                            cnt_proc_state++;
                        }
                        //space_count==3 //ppid
                        //space_count==4 //pgrp
                        //space_count==5 //session
                        //space_count==6 //tty_nr
                        //space_count==7 //tpgid
                        //space_count==8 //flags (include/linux/sched.h)
                        if (space_count == 9) {
                            //minflt - The number of minor faults the process has made which have not required loading a memory page from disk
                            proc_minflt[cnt_proc_minflt] = tstat_txt[filei];
                            cnt_proc_minflt++;
                        }
                        //space_count==10 //cminflt
                        if (space_count == 11) {
                            //majflt - The number of major faults the process has made which have required loading a memory page from disk
                            proc_majflt[cnt_proc_majflt] = tstat_txt[filei];
                            cnt_proc_majflt++;
                        }
                        //space_count==12 //cmajflt
                        if (space_count == 13) {
                            //utime - Amount of time that this process has been scheduled in user mode, measured in clock ticks (divide  by  sysconf(_SC_CLK_TCK)).   This  includes  guest  time,                                 guest_time  (time  spent running a virtual CPU, see below), so that applications that are not aware of the guest time field do not lose that time from their calculations.
                            proc_utime[cnt_proc_utime]=tstat_txt[filei];
                            cnt_proc_utime++;
                        }
                        if (space_count == 14) {
                            //stime - Amount of time that this process has been scheduled in kernel mode, measured in clock ticks (divide by sysconf(_SC_CLK_TCK))
                            proc_stime[cnt_proc_stime]=tstat_txt[filei];
                            cnt_proc_stime++;
                        }
                        //space_count==15 //cutime
                        //space_count==16 //cstime
                        //space_count==17 //priority
                        //space_count==18 //nice
                        //space_count==19 //num_threads
                        //space_count==20 //itrealvalue
                        if (space_count == 21) {
                            //starttime - The time the process started after system boot.
                            proc_startime[cnt_proc_startime] = tstat_txt[filei];
                            cnt_proc_startime++;
                        }
                        //space_count==22 //vsize
                        //space_count==23 //rss
                        //space_count==24 //rsslim
                        //space_count==25 //startcode
                        //space_count==26 //endcode
                        //space_count==27 //startstack
                        //space_count==28 //kstkesp
                        //space_count==29 //kstkeip
                        //space_count==30 //signal - Obsolete, use /proc/[pid]/status
                        //space_count==31 //blocked - Obsolete, use /proc/[pid]/status
                        //space_count==32 //sigignore - Obsolete, use /proc/[pid]/status
                        //space_count==33 //sigcatch - Obsolete, use /proc/[pid]/status
                        //space_count==34 //wchan - This is the "channel" in which the process is waiting.  It is the address of a location in the kernel where the process is sleeping.  The corresponding symbolic name can be found in /proc/[pid]/wchan
                        //space_count==35 //nswap - Number of pages swapped (not maintained)
                        //space_count==36 //cnswap
                        //space_count==37 //exit_signal
                        //space_count==38 //processor
                        //space_count==39 //rt_priority
                        //space_count==40 //policy
                        //space_count==41 //delayacct_blkio_ticks
                        //space_count==42 //guest_time - Guest time of the process (time spent running a virtual CPU for a guest operating system), measured in clock ticks (divide by sysconf(_SC_CLK_TCK))
                        //space_count==43 //cguest_time
                        //space_count==44 //start_data
                        //space_count==45 //end_data
                        //space_count==46 //start_brk
                        //space_count==47 //arg_start
                        //space_count==48 //arg_end
                        //space_count==49 //env_start
                        //space_count==50 //env_end
                        //space_count==51 //exit_code - The thread's exit status in the form reported by waitpid
                    }

                    proc_state[cnt_proc_state]='\0';
                    proc_minflt[cnt_proc_minflt]='\0';
                    proc_majflt[cnt_proc_majflt]='\0';
                    proc_utime[cnt_proc_utime]='\0';
                    proc_stime[cnt_proc_stime]='\0';
                    proc_startime[cnt_proc_startime]='\0';

                    if(arr_jthreads[i].state==NULL||strlen(arr_jthreads[i].state)<2){
                        if(proc_state[0]=='R'){
                            strcpy(arr_jthreads[i].state, "RUNNING\0");
                            cntThreadRunning++;
                        }else if(proc_state[0]=='W'){
                            strcpy(arr_jthreads[i].state, "WAITING\0");
                            cntThreadWaiting++;
                        }else if(proc_state[0]=='B'){
                            strcpy(arr_jthreads[i].state, "BLOCKED\0");
                            cntThreadBlocked++;
                        }else if(proc_state[0]=='S'){
                            strcpy(arr_jthreads[i].state, "SLEEP\0");
                            cntThreadWaiting++;
                        }
                    }
                    memset(proc_state, 0, sizeof(proc_state));

                    //store the minor page faults
                    strcpy(arr_jthreads[i].minfault, proc_minflt);
                    memset(proc_minflt, 0, sizeof(proc_minflt));

                    //store the major page faults
                    strcpy(arr_jthreads[i].majfault, proc_majflt);
                    memset(proc_majflt, 0, sizeof(proc_majflt));

                    //calc the secs the process has been running
                    int proc_startimei=0;
                    proc_startimei = atoi(proc_startime);
                    int thread_etimei = (int)(SYS_UPTIME - (proc_startimei / SYS_JIFFPS));
                    //secs since last poll
                    int proc_time_diff=0;
                    int last_secs=0;
                    if(arr_jthreads[i].secs){
                        last_secs = atoi(arr_jthreads[i].secs);
                    }else{
                        last_secs = thread_etimei-6;
                    }
                    proc_time_diff=thread_etimei-last_secs;
                    //store secs running
                    char thread_secs[100];
                    sprintf(thread_secs, "%d", thread_etimei);
                    strcpy(arr_jthreads[i].secs, thread_secs);
                    memset(proc_startime, 0, sizeof(proc_startime));
                    memset(thread_secs, 0, sizeof(thread_secs));

                    //calc CPU
                    if(proc_stime&&proc_utime){
                        char cpu_time_str[100];
                        char cpu_time_cur_str[100];
                        cnt_proc_stime=atoi(proc_stime);
                        cnt_proc_utime=atoi(proc_utime);

                        //get last cpu total
                        int cputtime_last=0;
                        if(arr_jthreads[i].cpu_total!=NULL){
                            cputtime_last=atoi(arr_jthreads[i].cpu_total);
                        }else{
                            cputtime_last=0;
                        }
                        //update cpu total
                        int cputime_useker=cnt_proc_stime+cnt_proc_utime;
                        sprintf(cpu_time_str, "%d", cputime_useker);
                        arr_jthreads[i].rawcpu = cputime_useker;
                        strcpy(arr_jthreads[i].cpu_total, cpu_time_str);
                        memset(cpu_time_str, 0, sizeof(cpu_time_str));

                        // calc PCPU (CPCPU)
                        //double cputime=(((cputime_useker)/SYS_JIFFPS)/proc_startimei);
                        //double cputime=((cputime_useker)/SYS_JIFFPS);
                        double cputime=(double)cputime_useker/thread_etimei;
                        sprintf(cpu_time_str, "%0.2f", cputime);
                        strcpy(arr_jthreads[i].pcpu, cpu_time_str);
                        memset(proc_stime, 0, sizeof(proc_stime));
                        memset(proc_utime, 0, sizeof(proc_utime));
                        memset(cpu_time_str, 0, sizeof(cpu_time_str));

                        // calc PCPU from last for a current cpu
                        int cputime_cur_tot = cputime_useker-cputtime_last;
                        //double cputimecur=(double)(cputime_cur_tot/proc_time_diff)*SYS_JIFFPS;
                        float proc_time_diff_secs = proc_time_diff/1000;
                        if(proc_time_diff_secs<1){
                            proc_time_diff_secs=1;
                        }
                        double cputimecur=(double)(cputime_cur_tot/proc_time_diff_secs);
                        if(cputimecur>100){
                            cputimecur=100;
                        }
                        sprintf(cpu_time_cur_str, "%0.2f", cputimecur);
                        while(cpu_time_cur_str[strlen(cpu_time_cur_str)-1]=='0'){
                            cpu_time_cur_str[strlen(cpu_time_cur_str)-1]='\0';
                        }
                        if(cpu_time_cur_str[strlen(cpu_time_cur_str)-1]=='.'){
                            cpu_time_cur_str[strlen(cpu_time_cur_str)-1]='\0';
                        }
                        strcpy(arr_jthreads[i].ccpu, cpu_time_cur_str);
                        memset(cpu_time_cur_str, 0, sizeof(cpu_time_cur_str));

                        if(cputimecur>10){
                            updateClassInfo(cputimecur,arr_jthreads[i].altcommand);
                        }
                    }

                    // clean up between each file read
                    memset(tstat_txt, 0, sizeof(tstat_txt));

                }
            }else{
                //////////////////////////////////////////////////
                // process has finished already or unable to check (no file)
                //////////////////////////////////////////////////
                //command replace to path for debug:
                //strcpy(arr_jthreads[i].command, tstat_path);

                //status
                strcpy(arr_jthreads[i].state, "CLOSED");

                //minor page faults
                strcpy(arr_jthreads[i].minfault, "0\0");

                //major page faults
                strcpy(arr_jthreads[i].majfault, "0");

                //secs
                strcpy(arr_jthreads[i].secs, "0\0");

                //cpu
                strcpy(arr_jthreads[i].pcpu, "0.00");

                //cpuraw
                strcpy(arr_jthreads[i].cpu_total, "0\0");

                //ccpu
                strcpy(arr_jthreads[i].ccpu, "0\0");
            }
            // Clean up between each thread being processed
        }
        memset(tstat_path, 0, sizeof(tstat_path));
    }
}

void getStatus(char *javapid){
    int thread_count = cnt_threads;
    char tstatus_txt[4096];
    char tstatus_path[256];
    char binarySigCaught[100];
    memset(tstatus_path, 0, sizeof(tstatus_path));
    memset(binarySigCaught, 0, sizeof(binarySigCaught));
    FILE *f_tstatus;

    for(int i=0; i<thread_count; i++) {
        if (arr_jthreads[i].pid != NULL) {
            strcpy(tstatus_path, "/proc/");
            strcat(tstatus_path, javapid);
            strcat(tstatus_path, "/task/");
            strcat(tstatus_path, arr_jthreads[i].pid);
            strcat(tstatus_path, "/status");
            f_tstatus = fopen(tstatus_path, "r");
            if (f_tstatus) {
                while (fgets(tstatus_txt, sizeof(tstatus_txt), f_tstatus)) {
                    if(strstr(tstatus_txt,"SigCgt:")){
                        int charpos=7;
                        while(tstatus_txt[charpos]!='\0'){
                            switch(tstatus_txt[charpos]){
                                case '0': strcat(&binarySigCaught[charpos-7],"0000"); break;
                                case '1': strcat(&binarySigCaught[charpos-7],"0001"); break;
                                case '2': strcat(&binarySigCaught[charpos-7],"0010"); break;
                                case '3': strcat(&binarySigCaught[charpos-7],"0011"); break;
                                case '4': strcat(&binarySigCaught[charpos-7],"0100"); break;
                                case '5': strcat(&binarySigCaught[charpos-7],"0101"); break;
                                case '6': strcat(&binarySigCaught[charpos-7],"0110"); break;
                                case '7': strcat(&binarySigCaught[charpos-7],"0111"); break;
                                case '8': strcat(&binarySigCaught[charpos-7],"1000"); break;
                                case '9': strcat(&binarySigCaught[charpos-7],"1001"); break;
                                case 'A': strcat(&binarySigCaught[charpos-7],"1010"); break;
                                case 'B': strcat(&binarySigCaught[charpos-7],"1011"); break;
                                case 'C': strcat(&binarySigCaught[charpos-7],"1100"); break;
                                case 'D': strcat(&binarySigCaught[charpos-7],"1101"); break;
                                case 'E': strcat(&binarySigCaught[charpos-7],"1110"); break;
                                case 'F': strcat(&binarySigCaught[charpos-7],"1111"); break;
                                case 'a': strcat(&binarySigCaught[charpos-7],"1010"); break;
                                case 'b': strcat(&binarySigCaught[charpos-7],"1011"); break;
                                case 'c': strcat(&binarySigCaught[charpos-7],"1100"); break;
                                case 'd': strcat(&binarySigCaught[charpos-7],"1101"); break;
                                case 'e': strcat(&binarySigCaught[charpos-7],"1110"); break;
                                case 'f': strcat(&binarySigCaught[charpos-7],"1111"); break;
                            }
                            charpos++;
                        }
                        binarySigCaught[charpos-7]='\0';
                        if(binarySigCaught[strlen(binarySigCaught)-11]=='1'){
                            //Sigsegv on thread...
                            strcpy(arr_jthreads[i].segv,"Y");
                        }else{
                            strcpy(arr_jthreads[i].segv,"N");
                        }
                        break;
                    }
                }
                fclose(f_tstatus);
            } else{
                //failed to open status file...
                //strcpy(threads[i][7],tstatus_path);
            }
        }
    }
}

void getNewThreads(char *javapid){
    char tstat_path[256];
    char tstat_txt[256];
    memset(tstat_path, 0, sizeof(tstat_path));

    strcpy(tstat_path, "/proc/");
    strcat(tstat_path, javapid);
    strcat(tstat_path, "/task/");

    DIR *procdir;
    FILE *procfile;
    struct dirent *tasklist;
    procdir = opendir(tstat_path);
    if (procdir) {
        while ((tasklist = readdir(procdir)) != NULL) {
            if (strcmp(tasklist->d_name, ".") != 0 && strcmp(tasklist->d_name, "..") != 0) {
                for(int ichk_ignore=0; ichk_ignore<cnt_ignore_threads; ichk_ignore++){
                    if(strcmp(tasklist->d_name,arr_ignore_threads[ichk_ignore].pid)==0){
                        continue;
                    }
                }
                int i = 0;
                //char *okok = tasklist->d_name;
                while (i < cnt_threads && !strstr(arr_jthreads[i].pid, tasklist->d_name) ) {
                    i++;
                }
                if (i < cnt_threads) {
                    //already exists
                } else {
                    // new thread found

                    //check what the process is
                    strcpy(tstat_path, "/proc/");
                    strcat(tstat_path, javapid);
                    strcat(tstat_path, "/task/");
                    strcat(tstat_path, tasklist->d_name);
                    strcat(tstat_path, "/stat");
                    procfile = fopen(tstat_path, "r");
                    if (procfile) {
                        fgets(tstat_txt, sizeof(tstat_txt), procfile);
                        fclose(procfile);
                        if (strstr(tstat_txt, " (java) ")) {
                            // add the thread
                            strcpy(arr_jthreads[cnt_threads].pid, tasklist->d_name);
                            if(strcmp(javapid,tasklist->d_name)==0){
                                strcpy(arr_jthreads[cnt_threads].name, ">> JAVA <<");
                            }else{
                                strcpy(arr_jthreads[cnt_threads].name, "New-thread");
                            }
                            strcpy(arr_jthreads[cnt_threads].state, "U");
                            strcpy(arr_jthreads[cnt_threads].pcpu, "0");
                            strcpy(arr_jthreads[cnt_threads].ccpu, "0");
                            strcpy(arr_jthreads[cnt_threads].minfault, "0");
                            strcpy(arr_jthreads[cnt_threads].majfault, "0");
                            strcpy(arr_jthreads[cnt_threads].secs, "0");
                            strcpy(arr_jthreads[cnt_threads].segv, "N");
                            arr_jthreads[cnt_threads].rawcpu=0;
                            strcpy(arr_jthreads[cnt_threads].cpu_total, "0");
                            cnt_threads++;
                        }else{
                            // exclude the thread...
                            strcpy(arr_ignore_threads[cnt_ignore_threads].pid, tasklist->d_name);
                            cnt_ignore_threads++;
                        }
                    }
                }
            }
        }
        closedir(procdir);
    }
}
