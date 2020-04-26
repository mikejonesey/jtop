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

void getStat(const char *javapid){
    int thread_count = cnt_threads;
    char tstat_txt[1000];
    char tstat_path[256];
    memset(tstat_path, 0, sizeof(tstat_path));
    FILE *f_tstat;

    int cnt_proc_comm=0;
    int cnt_proc_state=0;
    int cnt_proc_minflt=0;
    int cnt_proc_majflt=0;
    int cnt_proc_utime=0;
    int cnt_proc_stime=0;
    int cnt_proc_startime=0;

    char proc_comm[100]="\0";
    char proc_state[100]="\0";
    char proc_minflt[100]="\0";
    char proc_majflt[100]="\0";
    char proc_utime[100]="\0";
    char proc_stime[100]="\0";
    char proc_startime[1000]="\0";

    struct sysinfo info;
    sysinfo(&info);
    const long int SYS_UPTIME = info.uptime;
    long SYS_JIFFPS = sysconf(_SC_CLK_TCK);

    for(int i=0; i<thread_count; i++){
        if(arr_jthreads[i].pid && arr_jthreads[i].pid[0]!='\0'){
            //////////////////////////////////////////////////
            // Get new props
            //////////////////////////////////////////////////
            strncpy(tstat_path, "/proc/", 255);
            strncat(tstat_path, javapid, 255-strlen(tstat_path));
            strncat(tstat_path, "/task/", 255-strlen(tstat_path));
            strncat(tstat_path, arr_jthreads[i].pid, 255-strlen(tstat_path));
            strncat(tstat_path, "/stat", 255-strlen(tstat_path));
            f_tstat = fopen(tstat_path, "r");
            if (f_tstat) {
                fgets(tstat_txt, sizeof(tstat_txt), f_tstat);
                fclose(f_tstat);
                int space_count = 0;

                cnt_proc_comm=0;
                cnt_proc_state=0;
                cnt_proc_minflt=0;
                cnt_proc_majflt=0;
                cnt_proc_utime=0;
                cnt_proc_stime=0;
                cnt_proc_startime=0;

                memset(proc_startime, 0, sizeof(proc_startime));
                if(tstat_txt&&strstr(tstat_txt," ")){
                    for(int filei=0; filei<512; filei++){
                        if (space_count > 21||tstat_txt[filei]=='\0') {
                            break;
                        }
                        if (tstat_txt[filei] == ')') {
                            // reset space_count
                            space_count=1;
                            continue;
                        }
                        if (tstat_txt[filei] == ' ') {
                            space_count++;
                            continue;
                        }
                        //space_count==0 //pid
                        if (space_count == 1 && tstat_txt[filei]!='(') {
                            //space_count==1 //comm
                            proc_comm[cnt_proc_comm] = tstat_txt[filei];
                            cnt_proc_comm++;
                        }
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

                    proc_comm[cnt_proc_comm]='\0';
                    proc_state[cnt_proc_state]='\0';
                    proc_minflt[cnt_proc_minflt]='\0';
                    proc_majflt[cnt_proc_majflt]='\0';
                    proc_utime[cnt_proc_utime]='\0';
                    proc_stime[cnt_proc_stime]='\0';
                    proc_startime[cnt_proc_startime]='\0';

                    if(strcmp(arr_jthreads[i].name, "New-thread")==0 && strcmp(proc_comm, "java")!= 0){
                        strncpy(arr_jthreads[i].name, proc_comm, sizeof(arr_jthreads[i].name)-1);
                    }

                    //state (R|S|D|Z|T|t|W|X|x|K|W|P)
                    if(arr_jthreads[i].state==NULL||strlen(arr_jthreads[i].state)<2){
                        if(proc_state[0]=='R'){
                            strncpy(arr_jthreads[i].state, "RUNNING", sizeof(arr_jthreads[i].state)-1);
                            cntThreadRunning++;
                        }else if(proc_state[0]=='W'){
                            strncpy(arr_jthreads[i].state, "WAITING", sizeof(arr_jthreads[i].state)-1);
                            cntThreadWaiting++;
                        }else if(proc_state[0]=='B'){
                            strncpy(arr_jthreads[i].state, "BLOCKED", sizeof(arr_jthreads[i].state)-1);
                            cntThreadBlocked++;
                        }else if(proc_state[0]=='S'||proc_state[0]=='D') {
                            strncpy(arr_jthreads[i].state, "SLEEP", sizeof(arr_jthreads[i].state)-1);
                            cntThreadWaiting++;
                        }else if(proc_state[0]=='T'){
                            strncpy(arr_jthreads[i].state, "Trace", sizeof(arr_jthreads[i].state)-1);
                            cntThreadWaiting++;
                        }else if(proc_state[0]=='Z'){
                            strncpy(arr_jthreads[i].state, "Zombie", sizeof(arr_jthreads[i].state)-1);
                            cntThreadBlocked++;
                        }else if(proc_state[0]=='D'){
                            strncpy(arr_jthreads[i].state, "Disk", sizeof(arr_jthreads[i].state)-1);
                        }
                    }
                    memset(proc_state, 0, sizeof(proc_state));

                    //store the minor page faults
                    long proc_minflt_l = atol(proc_minflt);
                    arr_jthreads[i].minfault = proc_minflt_l;
                    memset(proc_minflt, 0, sizeof(proc_minflt));

                    //store the major page faults
                    long proc_majflt_l = atol(proc_majflt);
                    arr_jthreads[i].majfault = proc_majflt_l;
                    memset(proc_majflt, 0, sizeof(proc_majflt));

                    //calc the secs the process has been running
                    unsigned long proc_startimei=0;
                    proc_startimei = atol(proc_startime);
                    int thread_etimei = (int)(SYS_UPTIME - (proc_startimei / SYS_JIFFPS));
                    //secs since last poll
                    int proc_time_diff=0;
                    int last_secs=arr_jthreads[i].secs;
                    if(arr_jthreads[i].secs==0){
                        last_secs = thread_etimei-6;
                    }
                    proc_time_diff=thread_etimei-last_secs;
                    //store secs running
                    arr_jthreads[i].secs=thread_etimei;
                    memset(proc_startime, 0, sizeof(proc_startime));

                    //calc CPU
                    int cputtime_last;
                    if(proc_stime[0]!='\0'&&proc_utime[0]!='\0'){
                        cnt_proc_stime=atoi(proc_stime);
                        cnt_proc_utime=atoi(proc_utime);

                        //get last cpu total
                        if(arr_jthreads[i].rawcpu==0){
                            cputtime_last=cnt_proc_stime+cnt_proc_utime;
                        }else{
                            cputtime_last=arr_jthreads[i].rawcpu;
                        }
                        //update cpu total
                        int cputime_useker=cnt_proc_stime+cnt_proc_utime;
                        arr_jthreads[i].rawcpu = cputime_useker;

                        // calc PCPU (CPCPU)
                        double cputime=(double)cputime_useker/thread_etimei;

                        arr_jthreads[i].pcpu = cputime;

                        memset(proc_stime, 0, sizeof(proc_stime));
                        memset(proc_utime, 0, sizeof(proc_utime));

                        // calc PCPU from last for a current cpu
                        int cputime_cur_tot = cputime_useker-cputtime_last;
                        float proc_time_diff_secs = proc_time_diff/1000;
                        if(proc_time_diff_secs<1){
                            proc_time_diff_secs=1;
                        }
                        double cputimecur=(double)(cputime_cur_tot/proc_time_diff_secs);
                        if(cputimecur>100){
                            cputimecur=100;
                        }
                        arr_jthreads[i].ccpu=cputimecur;

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

                //status
                strncpy(arr_jthreads[i].state, "CLOSED", 9);

                //minor page faults
                arr_jthreads[i].minfault=0;

                //major page faults
                arr_jthreads[i].majfault=0;

                //secs
                arr_jthreads[i].secs=0;

                //cpu
                arr_jthreads[i].pcpu=0;

                //cpuraw
                arr_jthreads[i].rawcpu=0;

                //ccpu
                arr_jthreads[i].ccpu=0;
            }
            // Clean up between each thread being processed
        }
        memset(tstat_path, 0, sizeof(tstat_path));
    }
}

void setThread_nonvoluntary_ctxt_switches(const unsigned int threadID, const char *status_txt){
    int istrpos = 0;
    int istrsav = 0;
    char tmpnvcs[1000];
    while(status_txt[istrpos]!='\0'&&istrpos<1000){
        if(status_txt[istrpos]<'0'||status_txt[istrpos]>'9'){
            istrpos++;
            continue;
        }
        tmpnvcs[istrsav]=status_txt[istrpos];
        istrpos++;
        istrsav++;
    }
    tmpnvcs[istrsav]='\0';
    long tmplnvcs=atol(tmpnvcs);
    int calcdiff=0;
    if (arr_jthreads[threadID].c_switch_nv){
        calcdiff = (tmplnvcs-arr_jthreads[threadID].c_switch_nv)/5;
    }
    arr_jthreads[threadID].c_switch_nv = tmplnvcs;
    arr_jthreads[threadID].cc_switch_nv = calcdiff;
}

void setThread_voluntary_ctxt_switches(const unsigned int threadID, const char *status_txt){
    int istrpos = 0;
    int istrsav = 0;
    char tmpvcs[1000];
    while(status_txt[istrpos]!='\0'&&istrpos<1000){
        if(status_txt[istrpos]<'0'||status_txt[istrpos]>'9'){
            istrpos++;
            continue;
        }
        tmpvcs[istrsav]=status_txt[istrpos];
        istrpos++;
        istrsav++;
    }
    tmpvcs[istrsav]='\0';
    long tmplvcs=atol(tmpvcs);
    int calcdiff=0;
    if (arr_jthreads[threadID].c_switch_v){
        calcdiff = (tmplvcs - arr_jthreads[threadID].c_switch_v)/5;
    }
    arr_jthreads[threadID].c_switch_v = tmplvcs;
    arr_jthreads[threadID].cc_switch_v = calcdiff;
}

void setThread_sigcgt(const unsigned int threadID, const char *status_txt){
    int charpos=7;
    char binarySigCaught[100];
    memset(binarySigCaught, 0, sizeof(binarySigCaught));
    while(status_txt[charpos]!='\0'){
        switch(status_txt[charpos]){
            case '0':
                strncat(binarySigCaught,"0000", 99-strlen(binarySigCaught)); break;
            case '1':
                strncat(binarySigCaught,"0001", 99-strlen(binarySigCaught)); break;
            case '2':
                strncat(binarySigCaught,"0010", 99-strlen(binarySigCaught)); break;
            case '3':
                strncat(binarySigCaught,"0011", 99-strlen(binarySigCaught)); break;
            case '4':
                strncat(binarySigCaught,"0100", 99-strlen(binarySigCaught)); break;
            case '5':
                strncat(binarySigCaught,"0101", 99-strlen(binarySigCaught)); break;
            case '6':
                strncat(binarySigCaught,"0110", 99-strlen(binarySigCaught)); break;
            case '7':
                strncat(binarySigCaught,"0111", 99-strlen(binarySigCaught)); break;
            case '8':
                strncat(binarySigCaught,"1000", 99-strlen(binarySigCaught)); break;
            case '9':
                strncat(binarySigCaught,"1001", 99-strlen(binarySigCaught)); break;
            case 'A':
                strncat(binarySigCaught,"1010", 99-strlen(binarySigCaught)); break;
            case 'B':
                strncat(binarySigCaught,"1011", 99-strlen(binarySigCaught)); break;
            case 'C':
                strncat(binarySigCaught,"1100", 99-strlen(binarySigCaught)); break;
            case 'D':
                strncat(binarySigCaught,"1101", 99-strlen(binarySigCaught)); break;
            case 'E':
                strncat(binarySigCaught,"1110", 99-strlen(binarySigCaught)); break;
            case 'F':
                strncat(binarySigCaught,"1111", 99-strlen(binarySigCaught)); break;
            case 'a':
                strncat(binarySigCaught,"1010", 99-strlen(binarySigCaught)); break;
            case 'b':
                strncat(binarySigCaught,"1011", 99-strlen(binarySigCaught)); break;
            case 'c':
                strncat(binarySigCaught,"1100", 99-strlen(binarySigCaught)); break;
            case 'd':
                strncat(binarySigCaught,"1101", 99-strlen(binarySigCaught)); break;
            case 'e':
                strncat(binarySigCaught,"1110", 99-strlen(binarySigCaught)); break;
            case 'f':
                strncat(binarySigCaught,"1111", 99-strlen(binarySigCaught)); break;
            default:
                break;
        }
        charpos++;
    }
    if(strlen(binarySigCaught)>11 && binarySigCaught[strlen(binarySigCaught)-11]=='1'){
        //Sigsegv on thread...
        strncpy(arr_jthreads[threadID].segv,"Y",1);
    }else{
        strncpy(arr_jthreads[threadID].segv,"N",1);
    }
}

void getStatus(const char *javapid){
    int thread_count = cnt_threads;
    char tstatus_txt[4096];
    char tstatus_path[256];
    memset(tstatus_path, 0, sizeof(tstatus_path));
    FILE *f_tstatus;

    for(int i=0; i<thread_count; i++) {
        if (arr_jthreads[i].pid == NULL) {
            continue;
        }
        strncpy(tstatus_path, "/proc/", 255);
        strncat(tstatus_path, javapid, 255-strlen(tstatus_path));
        strncat(tstatus_path, "/task/", 255-strlen(tstatus_path));
        strncat(tstatus_path, arr_jthreads[i].pid, 255-strlen(tstatus_path));
        strncat(tstatus_path, "/status", 255-strlen(tstatus_path));
        f_tstatus = fopen(tstatus_path, "r");
        if (!f_tstatus) {
            return;
        }

        while (fgets(tstatus_txt, sizeof(tstatus_txt), f_tstatus)) {
            if(strstr(tstatus_txt,"SigCgt:")){
                // save signals
                setThread_sigcgt(i,tstatus_txt);
            }
            if(strstr(tstatus_txt,"nonvoluntary_ctxt_switches:")){
                // save the non vol counter, and calculate a pct
                setThread_nonvoluntary_ctxt_switches(i,tstatus_txt);
            }else if(strstr(tstatus_txt,"voluntary_ctxt_switches:")){
                // save the vol count, and calc a pct
                setThread_voluntary_ctxt_switches(i,tstatus_txt);
            }
        }
        fclose(f_tstatus);
    }
}

void getNewThreads(const char *javapid){
    char tstat_path[256];
    char tstat_txt[256];
    memset(tstat_path, 0, sizeof(tstat_path));

    strncpy(tstat_path, "/proc/", 255);
    strncat(tstat_path, javapid, 255-strlen(tstat_path));
    strncat(tstat_path, "/task/", 255-strlen(tstat_path));

    DIR *procdir;
    FILE *procfile;
    const struct dirent *tasklist;
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
                while (i < cnt_threads && !strstr(arr_jthreads[i].pid, tasklist->d_name) ) {
                    i++;
                }
                if (i < cnt_threads) {
                    //already exists
                } else {
                    // new thread found

                    //check what the process is
                    strncpy(tstat_path, "/proc/", 255);
                    strncat(tstat_path, javapid, 255-strlen(tstat_path));
                    strncat(tstat_path, "/task/", 255-strlen(tstat_path));
                    strncat(tstat_path, tasklist->d_name, 255-strlen(tstat_path));
                    strncat(tstat_path, "/stat", 255-strlen(tstat_path));
                    procfile = fopen(tstat_path, "r");
                    if (procfile && cnt_threads < 9999) {
                        fgets(tstat_txt, sizeof(tstat_txt), procfile);
                        fclose(procfile);
                        if (strstr(tstat_txt, "(")) {
                            // add the thread
                            strncpy(arr_jthreads[cnt_threads].pid, tasklist->d_name, 9);
                            if(strcmp(javapid,tasklist->d_name)==0){
                                strncpy(arr_jthreads[cnt_threads].name, ">> JAVA <<", 255);
                            }else{
                                const char *threadNameSearch = strstr(tstat_txt, "(");
                                unsigned long threadNameEnd = (strstr(threadNameSearch, ")") - threadNameSearch)+1;
                                if(threadNameEnd<255){
                                    strncpy(arr_jthreads[cnt_threads].name, threadNameSearch, threadNameEnd);
                                }else{
                                    strncpy(arr_jthreads[cnt_threads].name, threadNameSearch, 255);
                                }
                            }
                            strncpy(arr_jthreads[cnt_threads].state, "U", 9);
                            arr_jthreads[cnt_threads].pcpu=0.00;
                            arr_jthreads[cnt_threads].ccpu=0;
                            arr_jthreads[cnt_threads].minfault=0;
                            arr_jthreads[cnt_threads].majfault=0;
                            arr_jthreads[cnt_threads].secs=0;
                            strncpy(arr_jthreads[cnt_threads].segv, "N", 1);
                            arr_jthreads[cnt_threads].rawcpu=0;
                            cnt_threads++;
                        }else{
                            // exclude the thread...
                            if(cnt_ignore_threads<99){
                                strncpy(arr_ignore_threads[cnt_ignore_threads].pid, tasklist->d_name, 9);
                                cnt_ignore_threads++;
                            }
                        }
                    }
                }
            }
        }
        closedir(procdir);
    }
}
