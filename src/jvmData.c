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

#include <fcntl.h>
#include "jvmData.h"

#define die(e) do { fprintf(stderr, "%s\n", e); exit(EXIT_FAILURE); } while (0);

int hex2int(const char *hexin){
    int intout;
    intout = (int)strtol(hexin, NULL, 0);
    return intout;
}

struct jthread* getThread(const char* tpid){
    for(int i=0; i<cnt_threads; i++){
        if(strcmp(arr_jthreads[i].pid,tpid) == 0) {
            return &arr_jthreads[i];
        }
    }
    return &arr_jthreads[cnt_threads];
}

void getExcludes(int *cnt_exclude, char *arr_exclude[]){
    // process file
    char exclude_txt[1000];
    char exclude_path[256];
    char *save_ptr_exclude_line;
    int cnt_excludei = 0;
    FILE *f_tstat;

    const char *exclude_line;
    const char *homepath = getenv("HOME");

    for(int i=0; i<*cnt_exclude; i++){
        free(arr_exclude[i]);
    }
    *cnt_exclude=0;

    strncpy(exclude_path,homepath,255);
    strncat(exclude_path, "/.jtop-exclude",255-strlen(exclude_path));
    f_tstat = fopen(exclude_path, "r");
    if (!f_tstat) {
        return;
    }
    while(fgets(exclude_txt, sizeof(exclude_txt), f_tstat)){
        if(exclude_txt&&strstr(exclude_txt,"\n")){
            exclude_line = strtok_r(exclude_txt, "\n", &save_ptr_exclude_line);
            while (exclude_line != NULL) {
                arr_exclude[cnt_excludei]=malloc(strlen(exclude_line)+1);
                memset(arr_exclude[cnt_excludei],0,strlen(exclude_line));
                strncpy(arr_exclude[cnt_excludei], exclude_line, strlen(exclude_line));
                cnt_excludei++;
                exclude_line = strtok_r(NULL, "\n", &save_ptr_exclude_line);
            }
        }
    }
    fclose(f_tstat);
    //update count
    *cnt_exclude=cnt_excludei;
}

int getJavaStack(char *javaPID, int *cntThreadRunning_p, int *cntThreadWaiting_p, int *cntThreadBlocked_p, int cnt_excludes, char *arr_excludes[]){
    struct jthread *cur_jthread = &arr_jthreads[0];
    cntThreadRunning = 0;
    cntThreadWaiting = 0;
    cntThreadBlocked = 0;

    if(cnt_threads>0){
        for (int i = 0; i < javaThreadDump.cnt_stacklines; i++) {
            if(javaThreadDump.arr_stacklines[i]){
                free(javaThreadDump.arr_stacklines[i]);
            }
        }

        for (int i = 0; i < cnt_threads; i++) {
            if(arr_jthreads[i].command!=NULL){
                memset(arr_jthreads[i].command, '\0', sizeof(arr_jthreads[i].command));
            }
            if(arr_jthreads[i].altcommand!=NULL){
                memset(arr_jthreads[i].altcommand, '\0', sizeof(arr_jthreads[i].altcommand));
            }
        }
    }

    for(int i=cnt_objects-1; i>-1; i--){
        memset(arr_objectsync[i].object, 0, sizeof(arr_objectsync[i].object));
    }

    cnt_objects=0;
    javaThreadDump.cnt_stacklines=0;

    char outputbuf[8192];
    char outputbufpad[2]="\0";
    ssize_t nbytes;
    const char *vthreadstate = "Thread.State";
    unsigned long ipos;
    const char *res2bufraw;
    char res2buf[8192];
    char *save_ptr_res2buf1;
    char *save_ptr_res2buf2;
    memset(outputbuf, '\0', sizeof(outputbuf));
    memset(res2buf, '\0', sizeof(res2buf));
    const char RUNNING_THREAD[9] = "RUNNABLE";
    const char WAITING_THREAD[8] = "WAITING";
    const char BLOCKED_THREAD[8] = "BLOCKED";

    // Check for trace method avail...
    struct stat statbuf;
    int checkJcmd = stat("/usr/bin/jcmd", &statbuf);
    int checkJstack = stat("/usr/bin/jstack", &statbuf);

    if(checkJcmd==-1 && checkJstack ==-1){
        // Machine has no trace method for the jvm...
        return -1;
    }

    // Get stack
    int link[2];
    int errpipe[2];
    pid_t pid;

    if (pipe(link) == -1){
        die("link");
    }
    if (pipe(errpipe) == -1){
        die("errpipe");
    }

    if ((pid = fork()) == -1){
        die("fork");
    }

    if (pid == 0) {
        dup2(link[1], STDOUT_FILENO);
        dup2(errpipe[1], STDERR_FILENO);
        write(errpipe[1],"\0",1);
        close(link[0]);
        close(link[1]);
        close(errpipe[0]);
        close(errpipe[1]);
        if(checkJcmd!=-1){
            execl("/usr/bin/jcmd", "/usr/bin/jcmd", javaPID, "Thread.print", "-l", (char *) 0);
        }else if(checkJstack!=-1){
            execl("/usr/bin/jstack", "/usr/bin/jstack", "-l", javaPID, (char *) 0);
        }
        die("Failed to get trace...");
    } else {
        close(link[1]);
        close(errpipe[1]);
        do{
            nbytes = read(errpipe[0], outputbuf, 4096);
            if(nbytes>1){
                if(outputbuf[nbytes-1]!='\n'){
                    while(nbytes>0 && outputbufpad[0] != '\n' && strlen(outputbuf)<sizeof(outputbuf)-4){
                        nbytes = read(errpipe[0], outputbufpad, 1);
                        strncat(outputbuf,outputbufpad,2);
                    }
                }
                res2bufraw = strtok_r(outputbuf, "\n", &save_ptr_res2buf2);
                while(res2bufraw != NULL && javaThreadDump.cnt_stacklines<10){
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(strlen(res2bufraw) + 1);
                    memset(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], 0, strlen(res2bufraw));
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], res2bufraw, strlen(res2bufraw));
                    javaThreadDump.cnt_stacklines++;
                    res2bufraw = strtok_r(NULL, "\n", &save_ptr_res2buf2);
                }
            }
            memset(outputbuf, 0, sizeof(outputbuf));
        } while(nbytes>1);
        close(errpipe[0]);
        // Loop Through file read...
        do {
            nbytes = read(link[0], outputbuf, 4096);
            if(outputbuf[nbytes-1]!='\n'){
                while(nbytes>0 && outputbufpad[0] != '\n' && strlen(outputbuf)<sizeof(outputbuf)-4){
                    nbytes = read(link[0], outputbufpad, 1);
                    strncat(outputbuf,outputbufpad,1);
                }
                strncat(outputbuf,outputbufpad,1);
            }
            res2bufraw = strtok_r(outputbuf, "\n", &save_ptr_res2buf1);
            //////////////////////////////////////////////////
            // Read stack line by line
            //////////////////////////////////////////////////
            // process raw line buffer
            while (res2bufraw != NULL) {
                strncat(res2buf, res2bufraw, 8191-strlen(res2buf));
                // GET A NEW LINE FROM BUFFER
                res2bufraw = strtok_r(NULL, "\n", &save_ptr_res2buf1);
                if (res2bufraw == NULL && strlen(outputbuf)!=strlen(res2buf)) {
                    strncat(res2buf,"\n", 8191-strlen(res2buf));
                }
                if (res2buf[0] == '"') {
                    //////////////////////////////////////////////////
                    // New Thread Line
                    //////////////////////////////////////////////////
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(2);
                    memset(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], 0, 2);
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], " ", 1);
                    javaThreadDump.cnt_stacklines++;
                    //get thread nid
                    if (strstr(res2buf, "nid=")) {
                        const char *nidsrch = strstr(res2buf, "nid=");
                        unsigned long nidloc = 4;
                        unsigned long nidend = (strstr(nidsrch, " ") - nidsrch);
                        int ii = 0;
                        char nidStr[10]="\0";
                        while (nidloc<nidend){
                            nidStr[ii]=nidsrch[nidloc];
                            ii++;
                            nidloc++;
                        }
                        unsigned int tpidi=hex2int(nidStr);
                        char tpid[10];
                        snprintf(tpid, 9, "%i", tpidi);
                        cur_jthread = getThread(tpid);
                        if((*cur_jthread).pid[0]=='\0'){
                            strncpy(cur_jthread->pid, tpid, 9);
                            cnt_threads++;
                        }
                    }//end nid
                    // update thread name
                    char threadName[256];
                    memset(threadName, 0, sizeof(threadName));
                    int i = 0;
                    while (res2buf[i + 1] != '"' && i < 255) {
                        threadName[i] = res2buf[i + 1];
                        i++;
                    }
                    threadName[i] = '\0';
                    strncpy(cur_jthread->name,threadName,255);
                    if(strcmp(threadName,"VM Thread")==0){
                        vmthread=cur_jthread;
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "VM Thread\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "VM Thread\0",399);
                    }else if(threadName[0]=='G'&&threadName[1]=='C'&&threadName[2]==' ' || strstr(threadName,"Gang worker")){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Garbage Collection\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Garbage Collection\0",399);
                    }else if(threadName[0]=='C'&&threadName[1]=='1'&&threadName[2]==' '){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Code Compiler\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Code Compiler\0",399);
                    }else if(threadName[0]=='C'&&threadName[1]=='2'&&threadName[2]==' '){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Code Cache Compiler\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Code Cache Compiler\0",399);
                    }else if(strcmp(threadName,"Signal Dispatcher")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Handle signals from OS\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Handle signals from OS\0",399);
                    }else if(strcmp(threadName,"VM Periodic Task Thre")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Performance sampling\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Performance sampling\0",399);
                    }else if(strcmp(threadName,"Attach Listener")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Debug: HotSpot Dynamic Attach - JVM TI Listener\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Debug: HotSpot Dynamic Attach - JVM TI Listener\0",399);
                    }else if(strcmp(threadName,"Service Thread")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strncpy(cur_jthread->command, "Debug: HotSpot SA - proc+ptrace\0",399);
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strncpy(cur_jthread->altcommand, "Debug: HotSpot SA - proc+ptrace\0",399);
                    }
                }// end new thread line
                //////////////////////////////////////////////////
                if (strstr(res2buf, vthreadstate)) {
                    //////////////////////////////////////////////////
                    // Thread status line...
                    //////////////////////////////////////////////////
                    ipos = (strstr(res2buf, vthreadstate) - res2buf);
                    ipos += strlen(vthreadstate);
                    ipos += 6;
                    char cstrup[strlen(res2buf)];
                    strncpy(cstrup, res2buf, ipos);
                    cstrup[ipos] = '\0';
                    //////////////////////////////////////////////////
                    // Print Thread State in BOLD
                    //////////////////////////////////////////////////
                    if (strstr(res2buf, RUNNING_THREAD)) {
                        cntThreadRunning++;
                        strncpy(cur_jthread->state, "RUNNING\0", 9);
                    } else if (strstr(res2buf, WAITING_THREAD)) {
                        cntThreadWaiting++;
                        strncpy(cur_jthread->state, "WAIT\0", 9);
                    } else if (strstr(res2buf, BLOCKED_THREAD)) {
                        strncpy(cur_jthread->state, "BLOCKED\0", 9);
                        cntThreadBlocked++;
                    }
                    //////////////////////////////////////////////////
                    // Print anything remaining...
                    //////////////////////////////////////////////////
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(strlen(res2buf) + 1);
                    memset(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines],0,strlen(res2buf));
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], res2buf, strlen(res2buf));
                    javaThreadDump.cnt_stacklines++;
                    //////////////////////////////////////////////////
                    // cleanup ts...
                    //////////////////////////////////////////////////
                    memset(cstrup, 0, sizeof(cstrup));
                    // END Thread status line...
                } else if(strstr(res2buf, "- locked <")){
                    //////////////////////////////////////////////////
                    // locked resource...
                    //////////////////////////////////////////////////
                    const char *ressrch = strstr(res2buf, " locked <");
                    unsigned long respos = 9;
                    unsigned long resposend = (strstr(ressrch, ">") - ressrch);
                    char temprecstr[20]="\0";
                    int i=0;
                    while (i<20 && respos<resposend) {
                        temprecstr[i]=ressrch[respos];
                        respos++;
                        i++;
                    }
                    strncpy(arr_objectsync[cnt_objects].object,temprecstr,19);
                    arr_objectsync[cnt_objects].ptr_jthread=cur_jthread;
                    cnt_objects++;
                    // add to javaThreadDump.arr_stacklines
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(strlen(res2buf) + 1);
                    memset(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines],0,strlen(res2buf));
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], res2buf, strlen(res2buf));
                    javaThreadDump.cnt_stacklines++;
                } else if(strstr(res2buf, "- waiting to lock <")){
                    //////////////////////////////////////////////////
                    // blocked on resource...
                    //////////////////////////////////////////////////
                    const char *ressrch = strstr(res2buf, "to lock <");
                    unsigned long respos = 9;
                    unsigned long resposend = (strstr(ressrch, ">") - ressrch);
                    int i=0;
                    while (i<20 && respos<resposend) {
                        cur_jthread->wlock[i]=ressrch[respos];
                        respos++;
                        i++;
                    }
                    cur_jthread->wlock[i]='\0';
                    // add to javaThreadDump.arr_stacklines
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(strlen(res2buf) + 1);
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], res2buf, strlen(res2buf));
                    javaThreadDump.cnt_stacklines++;
                    //////////////////////////////////////////////////
                    // waiting on resource...
                    //////////////////////////////////////////////////
                } else {
                    //////////////////////////////////////////////////
                    // Thread stack lines...
                    //////////////////////////////////////////////////
                    javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines] = malloc(strlen(res2buf) + 1);
                    memset(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], 0, strlen(res2buf));
                    strncpy(javaThreadDump.arr_stacklines[javaThreadDump.cnt_stacklines], res2buf, strlen(res2buf));
                    if ((strstr(res2buf, "\tat ") != NULL && cnt_threads>0) && (cur_jthread->command[0] == '\0')) {
                        char thread_command[256];
                        memset(thread_command, 0, sizeof(thread_command));
                        int ii = 0;
                        int max_com_len;
                        max_com_len = 250;
                        for (int i = 0; i < strlen(res2buf); i++) {
                            if (res2buf[i + 1] == '\0' || res2buf[i + 1] == '\n' || i > strlen(res2buf) || i > max_com_len) {
                                break;
                            }
                            if(i<=2){
                                continue;
                            }
                            if ((res2buf[i - 2] == 'a' && res2buf[i - 1] == 't' && res2buf[i] == ' ') ||
                                    strlen(thread_command) > 0)
                            {
                                thread_command[ii] = res2buf[i + 1];
                                ii++;
                            }
                        }
                        thread_command[ii] = '\0';
                        if(ii>2){
                            memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                            strncpy(cur_jthread->command, thread_command, 399);
                        }
                    }
                    if (strstr(res2buf, "\tat ") && (cur_jthread->altcommand == NULL || cur_jthread->altcommand[0] == '\0')) {
                        bool filter_ok = true;
                        for (int ii = 0; ii < cnt_excludes; ii++) {
                            char exclude_string[2048];
                            strncpy(exclude_string, "at ",2047);
                            strncat(exclude_string, arr_excludes[ii],2047-strlen(exclude_string));
                            if (strstr(res2buf, exclude_string)) {
                                filter_ok = false;
                                break;
                            }
                        }
                        if (filter_ok) {
                            char thread_command_filt[256];
                            memset(thread_command_filt, 0, sizeof(thread_command_filt));
                            int ii = 0;
                            int max_com_len;
                            max_com_len = 250;
                            for (int i = 2; i < strlen(res2buf); i++) {
                                if (res2buf[i + 1] == '\0' || res2buf[i + 1] == '\n' || i > strlen(res2buf) || i > max_com_len) {
                                    break;
                                }
                                if ((res2buf[i - 2] == 'a' && res2buf[i - 1] == 't' && res2buf[i] == ' ') ||
                                        strlen(thread_command_filt) > 0) {
                                    thread_command_filt[ii] = res2buf[i + 1];
                                    ii++;
                                }
                            }
                            thread_command_filt[ii] = '\0';
                            memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                            strncpy(cur_jthread->altcommand, thread_command_filt, 399);
                        }
                    }
                    javaThreadDump.cnt_stacklines++;
                }
                // other per line...
                //////////////////////////////////////////////////
                memset(res2buf, 0, sizeof(res2buf));
            }
            memset(outputbuf, 0, sizeof(outputbuf));
            outputbufpad[0]='\0';
        } while (nbytes > 0);
    }

    // check blocked
    for(int i=0; i<cnt_threads; i++) {
        arr_jthreads[i].blocking=0;
    }
    for(int i=0; i<cnt_threads; i++){
        if(arr_jthreads[i].wlock && arr_jthreads[i].wlock[0]!='\0'){
            for(int ii=0; ii<cnt_objects; ii++){
                if(strcmp(arr_jthreads[i].wlock,arr_objectsync[ii].object)==0){
                    arr_objectsync[ii].ptr_jthread->blocking++;
                    break;
                }
            }
        }
    }

    *cntThreadRunning_p = cntThreadRunning;
    *cntThreadWaiting_p = cntThreadWaiting;
    *cntThreadBlocked_p = cntThreadBlocked;
}

void getJavaStackFiltered(int cnt_excludes, char* arr_excludes[]){
    bool filter_ok;
    javaThreadDump.cnt_stacklines_filt=0;

    for(int i=0; i<javaThreadDump.cnt_stacklines; i++){
        if(strstr(javaThreadDump.arr_stacklines[i],"at ")){
            filter_ok=true;
            for(int ii=0; ii<cnt_excludes; ii++){
                char exclude_string[2048];
                strncpy(exclude_string,"at ",2047);
                strncat(exclude_string,arr_excludes[ii],2047-strlen(exclude_string));
                if(strstr(javaThreadDump.arr_stacklines[i],exclude_string)){
                    filter_ok=false;
                }
            }
            if(filter_ok){
                javaThreadDump.arr_stacklines_filt[javaThreadDump.cnt_stacklines_filt]=javaThreadDump.arr_stacklines[i];
                javaThreadDump.cnt_stacklines_filt++;
            }
        }else{
            javaThreadDump.arr_stacklines_filt[javaThreadDump.cnt_stacklines_filt]=javaThreadDump.arr_stacklines[i];
            javaThreadDump.cnt_stacklines_filt++;
        }
    }
}
