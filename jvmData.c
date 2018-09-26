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

struct jthread* getThread(char* tpid, int cnt_threads){
    for(int i=0; i<cnt_threads; i++){
        if(strcmp(arr_jthreads[i].pid,tpid) == 0) {
            return &arr_jthreads[i];
        }
    }
    return &arr_jthreads[cnt_threads];
}

void getExcludes(int *cnt_exclude, char *arr_exclude[]){
    int cnt_excludei = *cnt_exclude;
    // process file
    char exclude_txt[1000];
    char exclude_path[256];
    FILE *f_tstat;

    char *exclude_line;
    char *homepath = getenv("HOME");

    strcpy(exclude_path,homepath);
    strcat(exclude_path, "/.jtop-exclude");
    f_tstat = fopen(exclude_path, "r");
    if (f_tstat) {
        while(fgets(exclude_txt, sizeof(exclude_txt), f_tstat)){
            if(exclude_txt&&strstr(exclude_txt,"\n")){
                exclude_line = strtok(exclude_txt, "\n");
                while (exclude_line != NULL) {
                    arr_exclude[cnt_excludei]=malloc(strlen(exclude_line) + 1);
                    strcpy(arr_exclude[cnt_excludei], exclude_line);
                    cnt_excludei++;
                    exclude_line = strtok(NULL, "\n");
                }
            }
        }
        fclose(f_tstat);
    }
    //update count
    *cnt_exclude=cnt_excludei;
}

int getJavaStack(char *javaPID, int *cnt_win_stack_rows_p, char *stacklines[], int STACK_WIN_MAX_COL, int *cntThreadRunning_p, int *cntThreadWaiting_p, int *cntThreadBlocked_p, int cnt_excludes, char *arr_excludes[]){
    struct jthread *cur_jthread;
    int cnt_win_stack_rows = *cnt_win_stack_rows_p;
    int cntThreadRunning = 0;
    int cntThreadWaiting = 0;
    int cntThreadBlocked = 0;

    if(cnt_threads>0){
        for (int i = cnt_win_stack_rows-1; i > -1; i--) {
            if(stacklines[i]){
                free(stacklines[i]);
                //void *memloc = stacklines[i];
                //stacklines[i]=NULL;
                //free(memloc);
            }
        }

        for (int i = 0; i < cnt_threads; i++) {
            if(arr_jthreads[i].command!=NULL){
                //free(arr_jthreads[i].command);
                memset(arr_jthreads[i].command, 0, sizeof(arr_jthreads[i].command));
            }
            if(arr_jthreads[i].altcommand!=NULL){
                //free(arr_jthreads[i].altcommand);
                memset(arr_jthreads[i].altcommand, 0, sizeof(arr_jthreads[i].altcommand));
            }
        }
    }

    for(int i=cnt_objects-1; i>-1; i--){
        memset(arr_objectsync[i].object, 0, sizeof(arr_objectsync[i].object));
        //arr_objectsync[i].ptr_jthread=NULL;
    }

    cnt_objects=0;
    cnt_win_stack_rows=0;

    char outputbuf[8192];
    char outputbufpad[2];
    ssize_t nbytes;
    const char *vthreadstate = "Thread.State";
    unsigned long ipos;
    char *res2bufraw;
    //char res2buf[2048];
    char res2buf[8192];
    //char copylastbuff[2048];
    memset(outputbuf, 0, sizeof(outputbuf));
    memset(res2buf, 0, sizeof(res2buf));
    const char RUNNING_THREAD[9] = "RUNNABLE";
    const char WAITING_THREAD[8] = "WAITING";
    const char BLOCKED_THREAD[8] = "BLOCKED";

    //memset(copylastbuff, 0, sizeof(copylastbuff));

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
                res2bufraw = strtok(outputbuf, "\n");
                while(res2bufraw != NULL && cnt_win_stack_rows<10){
                    stacklines[cnt_win_stack_rows] = malloc(strlen(res2bufraw) + 1);
                    memset(stacklines[cnt_win_stack_rows], 0, sizeof(stacklines[cnt_win_stack_rows]));
                    strcpy(stacklines[cnt_win_stack_rows], res2bufraw);
                    cnt_win_stack_rows++;
                    res2bufraw = strtok(NULL, "\n");
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
                    strncat(outputbuf,outputbufpad,2);
                }
                strncat(outputbuf,outputbufpad,2);
            }
            res2bufraw = strtok(outputbuf, "\n");
            //////////////////////////////////////////////////
            // Read stack line by line
            //////////////////////////////////////////////////
            // process raw line buffer
            while (res2bufraw != NULL) {
                strcat(res2buf, res2bufraw);
                // GET A NEW LINE FROM BUFFER
                res2bufraw = strtok(NULL, "\n");
                if (res2bufraw == NULL && strlen(outputbuf)!=strlen(res2buf)) {
                    //break;
                    strcat(res2buf,"\n");
                }
                if (res2buf[0] == '"') {
                    //////////////////////////////////////////////////
                    // New Thread Line
                    //////////////////////////////////////////////////
                    stacklines[cnt_win_stack_rows] = malloc(strlen(" ") + 1);
                    memset(stacklines[cnt_win_stack_rows], 0, sizeof(stacklines[cnt_win_stack_rows]));
                    strcpy(stacklines[cnt_win_stack_rows], " ");
                    cnt_win_stack_rows++;
                    //get thread nid
                    if (strstr(res2buf, "nid=")) {
                        unsigned long nidloc = (strstr(res2buf, "nid=") - res2buf);
                        while (nidloc < strlen(res2buf)) {
                            if (res2buf[nidloc] == 'n' && res2buf[nidloc + 1] == 'i' && res2buf[nidloc + 2] == 'd' &&
                                res2buf[nidloc + 3] == '=') {
                                break;
                            }
                            nidloc++;
                        }
                        nidloc += 4;
                        char nidStr[10];
                        int ii;
                        ii = 0;
                        while (res2buf[nidloc] && res2buf[nidloc] != ' ') {
                            nidStr[ii] = res2buf[nidloc];
                            ii++;
                            nidloc++;
                        }
                        nidStr[ii] = '\0';
                        int tpidi=hex2int(nidStr);
                        char tpid[10];
                        sprintf(tpid, "%li", tpidi);
                        cur_jthread = getThread(tpid,cnt_threads);
                        if((*cur_jthread).pid[0]=='\0'){
                            strcpy(cur_jthread->pid, tpid);
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
                    strcpy(cur_jthread->name,threadName);
                    if(strcmp(threadName,"VM Thread")==0){
                        vmthread=cur_jthread;
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "VM Thread\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "VM Thread\0");
                    }else if(threadName[0]=='G'&&threadName[1]=='C'&&threadName[2]==' ' || strstr(threadName,"Gang worker")){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Garbage Collection\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Garbage Collection\0");
                    }else if(threadName[0]=='C'&&threadName[1]=='1'&&threadName[2]==' '){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Code Compiler\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Code Compiler\0");
                    }else if(threadName[0]=='C'&&threadName[1]=='2'&&threadName[2]==' '){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Code Cache Compiler\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Code Cache Compiler\0");
                    }else if(strcmp(threadName,"Signal Dispatcher")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Handle signals from OS\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Handle signals from OS\0");
                    }else if(strcmp(threadName,"VM Periodic Task Thre")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Performance sampling\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Performance sampling\0");
                    }else if(strcmp(threadName,"Attach Listener")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Debug: HotSpot Dynamic Attach - JVM TI Listener\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Debug: HotSpot Dynamic Attach - JVM TI Listener\0");
                    }else if(strcmp(threadName,"Service Thread")==0){
                        memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                        strcpy(cur_jthread->command, "Debug: HotSpot SA - proc+ptrace\0");
                        memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                        strcpy(cur_jthread->altcommand, "Debug: HotSpot SA - proc+ptrace\0");
                    }
                }// end new thread line
                //////////////////////////////////////////////////
                if (strstr(res2buf, vthreadstate)) {
                    //////////////////////////////////////////////////
                    // Thread status line...
                    //////////////////////////////////////////////////
                    ipos = (strstr(res2buf, vthreadstate) - res2buf);
                    ipos += sizeof(vthreadstate);
                    ipos += 6;
                    char cstrup[strlen(res2buf)];
                    strncpy(cstrup, res2buf, ipos);
                    cstrup[ipos] = '\0';
                    //////////////////////////////////////////////////
                    // Print Thread State in BOLD
                    //////////////////////////////////////////////////
                    char strend[2560];
                    int iposOrig = ipos;
                    while (res2buf[ipos]) {
                        strend[ipos - iposOrig] = res2buf[ipos];
                        ipos++;
                    }
                    strend[ipos - iposOrig] = '\0';
                    if (strstr(strend, RUNNING_THREAD)) {
                        cntThreadRunning++;
                        strcpy(cur_jthread->state, "RUNNING\0");
                    } else if (strstr(strend, WAITING_THREAD)) {
                        cntThreadWaiting++;
                        strcpy(cur_jthread->state, "WAIT\0");
                    } else if (strstr(strend, BLOCKED_THREAD)) {
                        strcpy(cur_jthread->state, "BLOCKED\0");
                        cntThreadBlocked++;
                    }
                    //////////////////////////////////////////////////
                    // Print anything remaining...
                    //////////////////////////////////////////////////
                    ipos++;
                    stacklines[cnt_win_stack_rows] = malloc(strlen(res2buf) + 1);
                    strcpy(stacklines[cnt_win_stack_rows], res2buf);
                    cnt_win_stack_rows++;
                    //////////////////////////////////////////////////
                    // cleanup ts...
                    //////////////////////////////////////////////////
                    ipos = 0;
                    iposOrig = 0;
                    memset(cstrup, 0, sizeof(cstrup));
                    memset(strend, 0, sizeof(strend));
                    // END Thread status line...
                } else if(strstr(res2buf, "- locked <")){
                    //////////////////////////////////////////////////
                    // locked resource...
                    //////////////////////////////////////////////////
                    int respos = (strstr(res2buf, " locked <") - res2buf)+9;
                    char temprecstr[20];
                    memset(temprecstr, 0, sizeof(temprecstr));
                    int i=0;
                    while (res2buf[respos] && res2buf[respos]!='>' && i < 20) {
                        temprecstr[i]=res2buf[respos];
                        respos++;
                        i++;
                    }
                    strcpy(arr_objectsync[cnt_objects].object,temprecstr);
                    arr_objectsync[cnt_objects].ptr_jthread=cur_jthread;
                    cnt_objects++;
                    // add to stacklines
                    stacklines[cnt_win_stack_rows] = malloc(strlen(res2buf) + 1);
                    strcpy(stacklines[cnt_win_stack_rows], res2buf);
                    cnt_win_stack_rows++;
                } else if(strstr(res2buf, "- waiting to lock <")){
                    //////////////////////////////////////////////////
                    // blocked on resource...
                    //////////////////////////////////////////////////
                    int respos = (strstr(res2buf, "to lock <") - res2buf)+9;
                    int i=0;
                    while (res2buf[respos] && res2buf[respos]!='>' && i < 20) {
                        cur_jthread->wlock[i]=res2buf[respos];
                        respos++;
                        i++;
                    }
                    cur_jthread->wlock[i]='\0';
                    // add to stacklines
                    stacklines[cnt_win_stack_rows] = malloc(strlen(res2buf) + 1);
                    strcpy(stacklines[cnt_win_stack_rows], res2buf);
                    cnt_win_stack_rows++;
                    //} else if(strcmp(res2buf, "- parking to wait for  <") == 0){
                    //////////////////////////////////////////////////
                    // waiting on resource...
                    //////////////////////////////////////////////////
                } else {
                    //////////////////////////////////////////////////
                    // Thread stack lines...
                    //////////////////////////////////////////////////
                    stacklines[cnt_win_stack_rows] = malloc(strlen(res2buf) + 1);
                    strcpy(stacklines[cnt_win_stack_rows], res2buf);
                    //##mvwprintw(win_stack, (cnt_win_stack_rows + 1), 1, "%s", res2buf);
                    if (strstr(res2buf, "\tat ") && (cur_jthread->command == NULL || cur_jthread->command[0] == '\0')) {
                        char thread_command[256];
                        memset(thread_command, 0, sizeof(thread_command));
                        int ii = 0;
                        int max_com_len;
                        //max_com_len = STACK_WIN_MAX_COL-73;
                        max_com_len = 250;
                        for (int i = 0; i < strlen(res2buf); i++) {
                            if (res2buf[i + 1] == '\0' || res2buf[i + 1] == '\n' || i > strlen(res2buf) || i > max_com_len) {
                                break;
                            }
                            if ((res2buf[i - 2] == 'a' && res2buf[i - 1] == 't' && res2buf[i] == ' ') ||
                                strlen(thread_command) > 0) {
                                thread_command[ii] = res2buf[i + 1];
                                ii++;
                            }
                        }
                        thread_command[ii] = '\0';
                        if(ii>2){
                            //cur_jthread->command = malloc(strlen(thread_command) + 1);
                            memset(cur_jthread->command, 0, sizeof(cur_jthread->command));
                            strcpy(cur_jthread->command, thread_command);
                        }
                    }
                    if (strstr(res2buf, "\tat ") && (cur_jthread->altcommand == NULL || cur_jthread->altcommand[0] == '\0')) {
                        bool filter_ok = true;
                        for (int ii = 0; ii < cnt_excludes; ii++) {
                            char exclude_string[2048];
                            strcpy(exclude_string, "at ");
                            strcat(exclude_string, arr_excludes[ii]);
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
                            //max_com_len = STACK_WIN_MAX_COL-73;
                            max_com_len = 250;
                            for (int i = 0; i < strlen(res2buf); i++) {
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
                            //cur_jthread->altcommand = malloc(strlen(thread_command_filt) + 1);
                            memset(cur_jthread->altcommand, 0, sizeof(cur_jthread->altcommand));
                            strcpy(cur_jthread->altcommand, thread_command_filt);
                        }
                    }
                    cnt_win_stack_rows++;
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

    *cnt_win_stack_rows_p = cnt_win_stack_rows;
    *cntThreadRunning_p = cntThreadRunning;
    *cntThreadWaiting_p = cntThreadWaiting;
    *cntThreadBlocked_p = cntThreadBlocked;

}

void getJavaStackFiltered(int cnt_win_stack_rows, int *cnt_win_stack_rows_filtp, int cnt_excludes, char* arr_excludes[], char *stacklines[], char *stacklines_filt[]){
    bool filter_ok=true;
    int cnt_win_stack_rows_filt=*cnt_win_stack_rows_filtp;

    cnt_win_stack_rows_filt=0;

    for(int i=0; i<cnt_win_stack_rows; i++){
        if(strstr(stacklines[i],"at ")){
            filter_ok=true;
            for(int ii=0; ii<cnt_excludes; ii++){
                char exclude_string[2048];
                strcpy(exclude_string,"at ");
                strcat(exclude_string,arr_excludes[ii]);
                if(strstr(stacklines[i],exclude_string)){
                    filter_ok=false;
                }
            }
            if(filter_ok){
                stacklines_filt[cnt_win_stack_rows_filt]=stacklines[i];
                cnt_win_stack_rows_filt++;
            }
        }else{
            stacklines_filt[cnt_win_stack_rows_filt]=stacklines[i];
            cnt_win_stack_rows_filt++;
        }
    }
    *cnt_win_stack_rows_filtp=cnt_win_stack_rows_filt;
}
