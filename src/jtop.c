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

#include <pwd.h>
#include "jtop.h"
#include "procData.h"
#include "jvmData.h"
#include "topWindow.h"
#include "stackWindow.h"
#include "classcpuWindow.h"

// argp: Arguments - getopt
const char *argp_program_version = "jtop 1.0.1";
const char *argp_program_bug_address = "<michael.jones@linux.com>";
static char doc[] = "interactive java process viewer.";
static char args_doc[] = "...";
static struct argp_option options[] = {
        {"pid",     'p', "JPID", 0, "Select the java process."},
        {"verbose", 'v', 0,      0, "verbose mode."},
        {0}
};

struct arguments {
    enum {
        STD_MODE, VERB_MODE
    } mode;
    char *procid;
};

static error_t parse_opt(int key, char *arg, struct argp_state *state) {
    struct arguments *arguments = state->input;
    switch (key) {
        case 'p':
            arguments->procid = arg;
            break;
        case 'v':
            arguments->mode = VERB_MODE;
            break;
        case ARGP_KEY_ARG:
            return 0;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int cnt_threads = 0;
int cnt_ignore_threads = 0;
struct jthread arr_jthreads[10000];
int cnt_objects = 0;
struct objectsync arr_objectsync[2000];
int topActiveRow=0;
bool threadControl = false;
bool sleepMode = false;
bool filterMode = false;
int focusOn=0;
int orderMode=0;

bool procIsJava(const char *pid){
    char buffer[100];
    char procpath[100];
    FILE *f_procfs;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));

    strncpy(procpath, "/proc/", 99);
    strncat(procpath, pid, 99-strlen(procpath));
    strncat(procpath, "/comm", 99-strlen(procpath));
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        //comm file is open...
        fgets(buffer, sizeof(buffer), f_procfs);
        fclose(f_procfs);
        if(strcmp(buffer,"java\n")==0){
            return true;
        }
    }
    return false;
}

bool procIsCurrentUser(const char *pid, uid_t uid_curuser){
    char buffer[1000];
    const char *buffsrch;
    char procpath[100];
    FILE *f_procfs;
    char procUid[100];

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(procUid, 0, sizeof(procUid));

    strncpy(procpath, "/proc/", 99);
    strncat(procpath, pid, 99-strlen(procpath));
    strncat(procpath, "/status", 99-strlen(procpath));
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        //comm file is open...
        while(fgets(buffer, sizeof(buffer), f_procfs)){
            buffsrch = strstr(buffer,"Uid:");
            if(!buffsrch){
                continue;
            }
            int i=0;
            while(buffsrch[i]-'0'<0 || buffsrch[i]-'0'>9){
                i++;
            }
            int iresult=0;
            while(buffsrch[i]-'0'>=0 && buffsrch[i]-'0'<=9){
                procUid[iresult]=buffsrch[i];
                iresult++;
                i++;
            }
            break;
        }
        fclose(f_procfs);
        uid_t uid_procuid = (int)strtol(procUid, NULL, 0);
        if(uid_procuid==uid_curuser){
            return true;
        }
    }
    return false;
}

bool getProcUser(const char *pid, struct passwd *pw_procuser, char pw_procuser_buff[], size_t pw_procuser_buff_size){
    char buffer[1000];
    const char *buffsrch;
    char procpath[100];
    FILE *f_procfs;
    char procUid[100];
    struct passwd *pw_temp;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(procUid, 0, sizeof(procUid));

    strncpy(procpath, "/proc/", 99);
    strncat(procpath, pid, 99-strlen(procpath));
    strncat(procpath, "/status", 99-strlen(procpath));
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        //comm file is open...
        while(fgets(buffer, sizeof(buffer), f_procfs)){
            buffsrch = strstr(buffer,"Uid:");
            if(!buffsrch){
                continue;
            }
            int i=0;
            while(buffsrch[i]-'0'<0 || buffsrch[i]-'0'>9){
                i++;
            }
            int iresult=0;
            while(buffsrch[i]-'0'>=0 && buffsrch[i]-'0'<=9){
                procUid[iresult]=buffsrch[i];
                iresult++;
                i++;
            }
            break;
        }
        fclose(f_procfs);
        uid_t uid_procUid = (uid_t)strtol(procUid, NULL, 0);
        getpwuid_r(uid_procUid,pw_procuser,pw_procuser_buff,pw_procuser_buff_size,&pw_temp);
    } else{
        return false;
    }
    return true;
}

void getProcCmdline(const char *pid, char cmdline[], size_t cmdline_size){
    char buffer[1024];
    char procpath[100];
    FILE *f_procfs;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(cmdline, 0, cmdline_size);

    strncpy(procpath, "/proc/", 99);
    strncat(procpath, pid, 99-strlen(procpath));
    strncat(procpath, "/cmdline", 99-strlen(procpath));
    f_procfs = fopen(procpath, "r");
    if (!f_procfs) {
        return;
    }
    while(fgets(buffer, sizeof(buffer), f_procfs)){
        for(int i=0; i<sizeof(buffer)-2; i++){
            if(buffer[i]=='\0' && buffer[i+1]!='\0'){
                buffer[i]=' ';
            }else if(buffer[i]=='\0' && buffer[i+1]=='\0'){
                break;
            }
        }
        size_t max_write_size = cmdline_size-(strlen(cmdline)+1);
        strncat(cmdline,buffer,max_write_size);
    }
    fclose(f_procfs);
    return;
}

char *getJavaPid(char *javapid){
    char arr_javas[20][20];
    // temp is a pointer to results that gets discarded, but could be used to validate results...
    struct passwd* pw_temp;
    // struct and storage buffer of current user
    struct passwd pw_curuser;
    char pw_curuser_buff[200];
    // struct and storage buffer of process user
    struct passwd pw_procuser;
    char pw_procuser_buff[200];
    // get proc list
    struct dirent *proclist;
    int printRow = 1;
    DIR *procdir;
    procdir = opendir("/proc");

    // get current user info
    getpwuid_r(geteuid(),&pw_curuser,pw_curuser_buff,sizeof(pw_curuser_buff),&pw_temp);

    if (!procdir) {
        printf("Error checking for java process, exiting...\n");
        exit(1);
    }

    while (printRow<10) {
        proclist = readdir(procdir);
        if(proclist==NULL){
            break;
        }
        // filter for process id
        if (!(proclist->d_name[0]-'0' >0 && proclist->d_name[0]-'0'<=9)) {
            continue;
        }
        //check if process is java
        if(!procIsJava(proclist->d_name)){
            continue;
        }
        if(procIsCurrentUser(proclist->d_name,pw_curuser.pw_uid)){
            // store pid for later
            strncpy(arr_javas[printRow-1],proclist->d_name,10);
            // print commandline
            char cmdline[4096];
            getProcCmdline(proclist->d_name,cmdline,sizeof(cmdline));
            printf("%d. PID: %s\n   CMD: %s\n\n",printRow,proclist->d_name,cmdline);
            printRow++;
        }else{
            bool userRetrieved = getProcUser(proclist->d_name,&pw_procuser,pw_procuser_buff,sizeof(pw_procuser_buff));
            if(!userRetrieved){
                printf("N/A Wrong User... PID: %s is run by an unknown user, you are: %s\n\n", proclist->d_name,pw_curuser.pw_name);
            }else if(pw_procuser.pw_name){
                printf("N/A Wrong User... PID: %s is run by user: %s, you are: %s\n\n", proclist->d_name,pw_procuser.pw_name,pw_curuser.pw_name);
            }
        }
    }
    closedir(procdir);

    int userselection;
    if(printRow==1){
        printf("No Java process found, exiting...\n");
        exit(1);
    }else if(printRow==2){
        userselection=0;
    }else{
        printf("Select a Java Process to monitor: ");
        scanf("%d",&userselection);
        if(userselection>0){
            userselection-=1;
        }
        if(userselection>=printRow-1){
            printf("\nBad selection, exiting...\n");
            exit(1);
        }
    }
    strncpy(javapid,arr_javas[userselection],19);
    return javapid;
}

void *pollTopWindow(void *myargs){
    const struct jtopWindowObjects *jtopWinData = myargs;
    const char *javapid = jtopWinData->javapid;
    while(1){
        while(threadControl&&!sleepMode){
            getNewThreads(javapid);
            getStat(javapid);
            getStatus(javapid);
            if(orderMode==0){
                orderByCPU();
            }else if(orderMode==1){
                orderByBlocked();
            }
            while(!threadControl){
                sleep(1);
            }
            printTop(cnt_threads);
            printClassCPU();
            //sleep between processing interval
            sleep(2);
        }
        //sleep between sleeping interval
        sleep(1);
    }
    return NULL;
}

void toggleSleepMode(){
    if(sleepMode){
        wattron(jtopWindows.win_jtop, A_BOLD);
        wattron(jtopWindows.win_jtop, COLOR_PAIR(6));
        wmove(jtopWindows.win_jtop, 1, 1);

        char charwidth[jtopWindows.JTOP_WIN_MAX_COL];
        memset(charwidth, '\0', jtopWindows.JTOP_WIN_MAX_COL);
        memset(charwidth, ' ', jtopWindows.JTOP_WIN_MAX_COL-1);
        mvwprintw(jtopWindows.win_jtop, 1, 1, "%s", charwidth);

        mvwprintw(jtopWindows.win_jtop, 1, 1, " PID ");
        mvwprintw(jtopWindows.win_jtop, 1, 8, "STATE  ");
        mvwprintw(jtopWindows.win_jtop, 1, 16, "PCPU  ");
        mvwprintw(jtopWindows.win_jtop, 1, 23, "CCPU  ");
        mvwprintw(jtopWindows.win_jtop, 1, 30, "MinFault  ");
        mvwprintw(jtopWindows.win_jtop, 1, 41, "MajFault  ");
        mvwprintw(jtopWindows.win_jtop, 1, 52, "SECS  ");
        mvwprintw(jtopWindows.win_jtop, 1, 60, "cVS ");
        mvwprintw(jtopWindows.win_jtop, 1, 65, "cNVS ");
        mvwprintw(jtopWindows.win_jtop, 1, 70, "BLK  ");
        mvwprintw(jtopWindows.win_jtop, 1, 75, "Name                 ");
        mvwprintw(jtopWindows.win_jtop, 1, 102, "Command  ");
        wattroff(jtopWindows.win_jtop, A_BOLD);
        wattroff(jtopWindows.win_jtop, COLOR_PAIR(6));
        use_default_colors();
        sleepMode=false;
    }else{
        sleepMode=true;
        wattron(jtopWindows.win_jtop, A_BOLD);
        wattron(jtopWindows.win_jtop, COLOR_PAIR(4));
        wmove(jtopWindows.win_jtop, 1, 1);

        char charwidth[jtopWindows.JTOP_WIN_MAX_COL];
        memset(charwidth, '\0', jtopWindows.JTOP_WIN_MAX_COL);
        memset(charwidth, ' ', jtopWindows.JTOP_WIN_MAX_COL-1);
        mvwprintw(jtopWindows.win_jtop, 1, 1, "%s", charwidth);

        mvwprintw(jtopWindows.win_jtop, 1, 1, " PID ");
        mvwprintw(jtopWindows.win_jtop, 1, 8, "STATE  ");
        mvwprintw(jtopWindows.win_jtop, 1, 16, "PCPU  ");
        mvwprintw(jtopWindows.win_jtop, 1, 23, "CCPU  ");
        mvwprintw(jtopWindows.win_jtop, 1, 30, "MinFault  ");
        mvwprintw(jtopWindows.win_jtop, 1, 41, "MajFault  ");
        mvwprintw(jtopWindows.win_jtop, 1, 52, "SECS  ");
        mvwprintw(jtopWindows.win_jtop, 1, 60, "cVS ");
        mvwprintw(jtopWindows.win_jtop, 1, 65, "cNVS ");
        mvwprintw(jtopWindows.win_jtop, 1, 70, "BLK  ");
        mvwprintw(jtopWindows.win_jtop, 1, 75, "Name                 ");
        mvwprintw(jtopWindows.win_jtop, 1, 102, "Command  ");
        wattroff(jtopWindows.win_jtop, A_BOLD);
        wattroff(jtopWindows.win_jtop, COLOR_PAIR(6));
        use_default_colors();
        wrefresh(jtopWindows.win_jtop);
    }
}

void toggleOrderBy(){
    if(orderMode==0){
        orderMode=1;
    }else{
        orderMode=0;
    }
}

void toggleFilterMode(){
    if(filterMode){
        filterMode=false;
    }else{
        filterMode=true;
    }
    printJavaStack();
    printTop(cnt_threads);
}

void stackWindowScrollUp(int *cnt_scroll){
    if(focusOn==0){
        if(*cnt_scroll==0){
            return;
        }
        if (*cnt_scroll < 6) {
            *cnt_scroll = 0;
        } else {
            *cnt_scroll -= 5;
        }
        if (wscrl(jtopWindows.win_stack, -5) != OK) {
            return;
        }
        int ii = *cnt_scroll;

        wmove(jtopWindows.win_stack, 5, 1);
        wclrtoeol(jtopWindows.win_stack);

        for (int i = 0; i < 5; i++) {
            if(filterMode){
                mvwprintw(jtopWindows.win_stack, (i + 1), 1, "%d: %s", ii, javaThreadDump.arr_stacklines_filt[ii]);
            }else{
                mvwprintw(jtopWindows.win_stack, (i + 1), 1, "%d: %s", ii, javaThreadDump.arr_stacklines[ii]);
            }
            ii++;
        }
    }else if(focusOn==1){
        topActiveRow--;
        if(topActiveRow<0){
            topActiveRow=0;
        }
        printTop(cnt_threads);
        printJavaThreadStack(jtopWindows.win_stack, arr_jthreads[topActiveRow].name, javaThreadDump.arr_stacklines, jtopWindows.STACK_WIN_MAX_LINE);
    }
}

void stackWindowScrollDown(int *cnt_scroll){
    if(focusOn==0){
        unsigned int scroll_to = *cnt_scroll+5;
        unsigned int max_scroll;
        if(!filterMode) {
            max_scroll = javaThreadDump.cnt_stacklines - jtopWindows.STACK_WIN_MAX_LINE;
        }else {
            max_scroll = javaThreadDump.cnt_stacklines_filt - jtopWindows.STACK_WIN_MAX_LINE;
        }
        if (scroll_to > max_scroll) {
            scroll_to = max_scroll;
        }
        if(scroll_to==*cnt_scroll){
            return;
        }

        //start scroll...
        if (wscrl(jtopWindows.win_stack, 5) != OK) {
            return;
        }

        const unsigned int window_offset = jtopWindows.STACK_WIN_MAX_LINE-6;
        wmove(jtopWindows.win_stack, window_offset, 1);
        wclrtoeol(jtopWindows.win_stack);
        for (int i=0; i<5; i++) {
            if(filterMode){
                mvwprintw(jtopWindows.win_stack, window_offset+i, 1, "%d: %s", (scroll_to+window_offset+i), javaThreadDump.arr_stacklines_filt[scroll_to+window_offset+i+1]);
            }else if(!filterMode){
                mvwprintw(jtopWindows.win_stack, window_offset+i, 1, "%d: %s", (scroll_to+window_offset+i), javaThreadDump.arr_stacklines[scroll_to+window_offset+i+1]);
            }
        }
        wrefresh(jtopWindows.win_jtop);
        *cnt_scroll=scroll_to;
    }else if(focusOn==1 && topActiveRow<cnt_threads-1){
        topActiveRow++;
        printTop(cnt_threads);
        printJavaThreadStack(jtopWindows.win_stack, arr_jthreads[topActiveRow].name, javaThreadDump.arr_stacklines, jtopWindows.STACK_WIN_MAX_LINE);
    }
}

void navigateNextThread(int *cnt_scroll){
    ////////////////////
    // Next thread
    ////////////////////
    int search_cnt_scroll = *cnt_scroll + 2;
    while (javaThreadDump.arr_stacklines[search_cnt_scroll] && javaThreadDump.arr_stacklines[search_cnt_scroll][0] != '"' && *cnt_scroll < javaThreadDump.cnt_stacklines) {
        search_cnt_scroll++;
    }

    // validate search and print
    if(javaThreadDump.arr_stacklines[search_cnt_scroll]){
        //stackline exists
        if(javaThreadDump.arr_stacklines[search_cnt_scroll][0] != '"'){
            //search failed
            *cnt_scroll = javaThreadDump.cnt_stacklines-10;
        }else{
            //success
            *cnt_scroll = search_cnt_scroll;
        }
    }

    // scroll and print
    int ii = *cnt_scroll;
    for (int i = 1; i < jtopWindows.STACK_WIN_MAX_LINE; i++) {
        wmove(jtopWindows.win_stack, i, 1);
        wclrtoeol(jtopWindows.win_stack);
        if (ii>=0 && javaThreadDump.arr_stacklines[ii]) {
            mvwprintw(jtopWindows.win_stack, i, 1, "%d: %s", (*cnt_scroll+i-1), javaThreadDump.arr_stacklines[ii]);
        }
        ii++;
    }
}

void navigatePreviousThread(int *cnt_scroll){
    ////////////////////
    // Previous thread
    ////////////////////
    int search_cnt_scroll = *cnt_scroll - 2;
    while (search_cnt_scroll > 1 && javaThreadDump.arr_stacklines[search_cnt_scroll] && javaThreadDump.arr_stacklines[search_cnt_scroll][0] != '"' && cnt_scroll > 0) {
        search_cnt_scroll--;
    }

    //validate search result
    if(search_cnt_scroll > 1 && javaThreadDump.arr_stacklines[search_cnt_scroll]){
        //stackline exists
        if(javaThreadDump.arr_stacklines[search_cnt_scroll][0] != '"'){
            //search failed
            *cnt_scroll = 0;
        }else{
            //success
            *cnt_scroll = search_cnt_scroll;
        }
    }

    //scroll and print
    int ii = *cnt_scroll;
    for (int i = 1; i < jtopWindows.STACK_WIN_MAX_LINE; i++) {
        wmove(jtopWindows.win_stack, i, 1);
        wclrtoeol(jtopWindows.win_stack);
        if (javaThreadDump.arr_stacklines[ii]) {
            mvwprintw(jtopWindows.win_stack, i, 1, "%d: %s", (*cnt_scroll+i-1), javaThreadDump.arr_stacklines[ii]);
        }
        ii++;
    }
}

void navigateSearchTerm(int *cnt_scroll, const char *searchTerm){
    ////////////////////
    // Search
    ////////////////////
    unsigned int search_cnt_scroll = 0;
    unsigned int lastThreadPos=0;
    while (search_cnt_scroll < javaThreadDump.cnt_stacklines
            && javaThreadDump.arr_stacklines[search_cnt_scroll]
            && !strstr(javaThreadDump.arr_stacklines[search_cnt_scroll],searchTerm))
    {
        if(javaThreadDump.arr_stacklines[search_cnt_scroll][0] == '"'){
            lastThreadPos=search_cnt_scroll;
        }
        search_cnt_scroll++;
    }
    if(lastThreadPos>0 && javaThreadDump.arr_stacklines[search_cnt_scroll]){
        //search success
        *cnt_scroll = lastThreadPos;
    }else{
        //search failed
        return;
    }

    // scroll and print
    int ii = *cnt_scroll;
    for (int i = 1; i < jtopWindows.STACK_WIN_MAX_LINE; i++) {
        wmove(jtopWindows.win_stack, i, 1);
        wclrtoeol(jtopWindows.win_stack);
        if (ii>=0 && javaThreadDump.arr_stacklines[ii]) {
            mvwprintw(jtopWindows.win_stack, i, 1, "%d: %s", (*cnt_scroll+i-1), javaThreadDump.arr_stacklines[ii]);
        }
        ii++;
    }
}

void toggleActiveWindow(int *cnt_scroll){
    ////////////////////
    // Toggle controls
    ////////////////////
    if(focusOn==0){
        *cnt_scroll=0;
        focusOn=1;
        printJavaThreadStack(jtopWindows.win_stack, arr_jthreads[topActiveRow].name, javaThreadDump.arr_stacklines, jtopWindows.STACK_WIN_MAX_LINE);
    }else{
        focusOn=0;
        if(filterMode==true){
            // Get the active line number...
            int curlinenum = getLineJavaStack(arr_jthreads[topActiveRow].name, javaThreadDump.arr_stacklines_filt);
            // Set the current line number to use...
            *cnt_scroll=curlinenum-1;
            // Print the trace...
            printJavaStack();
        }else{
            // Get the active line number...
            int curlinenum = getLineJavaStack(arr_jthreads[topActiveRow].name, javaThreadDump.arr_stacklines);
            // Set the current line number to use...
            *cnt_scroll=curlinenum-1;
            // Print the trace...
            printJavaStack();
        }
    }
}

int main(int argc, char **argv) {
    int key_in;
    int cnt_scroll = 0;
    cntThreadRunning = 0;
    cntThreadWaiting = 0;
    cntThreadBlocked = 0;
    javaThreadDump.cnt_stacklines = 0;
    javaThreadDump.cnt_stacklines_filt = 0;
    unsigned int parent_x;
    unsigned int parent_y;
    unsigned int tstate_size = 3;
    char *arr_exclude[2000];
    int cnt_exclude=0;

    struct arguments arguments;
    arguments.mode = STD_MODE;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    //////////////////////////////////////////////////
    // Prep
    //////////////////////////////////////////////////

    // GET Stack Access
    struct stat statbuf;
    int checkJcmd = stat("/usr/bin/jcmd", &statbuf);
    int checkJstack = stat("/usr/bin/jstack", &statbuf);

    if(checkJcmd==-1 && checkJstack ==-1){
        // Machine has no trace method for the jvm...
        printf("JDK bins not found. Ensure you have the JDK installed (openjdk-8-jdk or oracle jdk).\nIf you have the JDK installed symbolic link jcmd or jstack to /usr/bin");
        return -1;
    }

    // GET Java PID
    char *javapid;
    javapid = arguments.procid;
    jtopWindows.javapid = javapid;

    if(javapid==NULL){
        javapid=malloc(200);
        memset(javapid,'\0',200);
        javapid = getJavaPid(javapid);
        jtopWindows.javapid = javapid;
    }

    //////////////////////////////////////////////////
    // Setup Curses
    //////////////////////////////////////////////////
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    getmaxyx(stdscr, parent_y, parent_x);
    if(parent_x<=10){
        parent_x=80;
    }
    if(parent_y<10){
        parent_y=80;
    }
    const int STACK_WIN_MAX_LINE = 20;
    jtopWindows.STACK_WIN_MAX_LINE = 20;
    const unsigned int STACK_WIN_MAX_COL = (parent_x) - 1;
    const unsigned int STACK_WIN_MAX_COLT = (parent_x/2);
    jtopWindows.JTOP_WIN_MAX_COL = parent_x;
    jtopWindows.STACK_WIN_MAX_COL = STACK_WIN_MAX_COLT;
    const unsigned int JTOP_WIN_MAX_LINE = parent_y - tstate_size - 21;
    jtopWindows.JTOP_WIN_MAX_LINE = JTOP_WIN_MAX_LINE;
    WINDOW *win_stack = newwin(STACK_WIN_MAX_LINE, STACK_WIN_MAX_COLT , 0, 0);
    WINDOW *win_ctop = newwin(STACK_WIN_MAX_LINE, STACK_WIN_MAX_COLT , 0, STACK_WIN_MAX_COLT+1);
    WINDOW *win_jtop = newwin(JTOP_WIN_MAX_LINE, parent_x, STACK_WIN_MAX_LINE, 0);
    WINDOW *win_tstate = newwin(tstate_size, parent_x, (parent_y - tstate_size) - 1, 0);

    jtopWindows.win_stack = win_stack;
    jtopWindows.win_jtop = win_jtop;
    jtopWindows.win_ctop = win_ctop;

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_GREEN);
    init_pair(6, COLOR_WHITE, COLOR_BLUE);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_BLACK, COLOR_WHITE);
    wattron(win_jtop, A_BOLD);
    wattron(win_jtop, COLOR_PAIR(6));
    wmove(win_jtop, 1, 1);
    char charwidth[STACK_WIN_MAX_COL];
    memset(charwidth, ' ', sizeof(charwidth));
    charwidth[STACK_WIN_MAX_COL-1]='\0';
    mvwprintw(win_jtop, 1, 1, "%s", charwidth);
    mvwprintw(win_jtop, 1, 1, " PID ");
    mvwprintw(win_jtop, 1, 8, "STATE  ");
    mvwprintw(win_jtop, 1, 16, "PCPU  ");
    mvwprintw(win_jtop, 1, 23, "CCPU  ");
    mvwprintw(win_jtop, 1, 30, "MinFault  ");
    mvwprintw(win_jtop, 1, 41, "MajFault  ");
    mvwprintw(win_jtop, 1, 52, "SECS  ");
    mvwprintw(win_jtop, 1, 60, "cVS ");
    mvwprintw(win_jtop, 1, 65, "cNVS ");
    mvwprintw(win_jtop, 1, 70, "BLK  ");
    mvwprintw(win_jtop, 1, 75, "Name                 ");
    mvwprintw(win_jtop, 1, 102, "Command  ");
    wattroff(win_jtop, A_BOLD);
    wattroff(win_jtop, COLOR_PAIR(6));
    use_default_colors();

    mvwprintw(win_ctop, 1, 1, "CPU By Class\n");
    whline(win_ctop, 0, STACK_WIN_MAX_COLT);
    box(win_ctop, 0, 0);

    box(win_stack, 0, 0);
    box(win_jtop, 0, 0);
    box(win_tstate, 0, 0);
    mvprintw((parent_y - 1), 0, "F3 / Search | [N]ext Thread | [T]oggle Window | [P]ause | [F]ilter Stack | [G]et Stack | [Q]uit");
    refresh();
    wrefresh(win_stack);
    wrefresh(win_jtop);
    wrefresh(win_ctop);
    wrefresh(win_tstate);

    scrollok(stdscr, TRUE);
    scrollok(win_stack, TRUE);

    //////////////////////////////////////////////////
    // Poll
    //////////////////////////////////////////////////

    pthread_t pollTopThread;
    if(pthread_create(&pollTopThread, NULL, pollTopWindow, &jtopWindows)) {
        mvwprintw(win_stack, 2, 1, "Error creating thread...");
        wrefresh(win_stack);
    }

    //////////////////////////////////////////////////
    // Get Stack
    //////////////////////////////////////////////////

    //populate exclude array from file
    getExcludes(&cnt_exclude, arr_exclude);

    //populate stack lines from jcmd
    getJavaStack(javapid, &cntThreadRunning, &cntThreadWaiting, &cntThreadBlocked, cnt_exclude, arr_exclude);

    //populate fileterd stack lines from stackline array
    getJavaStackFiltered(cnt_exclude, arr_exclude);

    //////////////////////////////////////////////////
    // Print Stack...
    //////////////////////////////////////////////////
    printJavaStack();
    wattron(win_stack, A_BOLD);
    wattron(win_stack, COLOR_PAIR(7));
    box(win_stack, 0, 0);
    wattroff(win_stack, A_BOLD);
    wattroff(win_stack, COLOR_PAIR(7));
    use_default_colors();
    wrefresh(win_stack);

    //////////////////////////////////////////////////
    // print the thread status bar...
    //////////////////////////////////////////////////
    int pct_running = cntThreadRunning*50/(cnt_threads-1);
    int pct_waiting = (cnt_threads-cntThreadRunning-cntThreadBlocked)*50/(cnt_threads-1);
    int pct_blocked = cntThreadBlocked*50/(cnt_threads-1);
    int ix = 1;
    if (start_color() == OK) {
        wattron(win_tstate, A_BOLD);
        wattron(win_tstate, COLOR_PAIR(4));
        mvwprintw(win_tstate, 1, ix, "Thread state: ");
        wattroff(win_tstate, A_BOLD);
        ix += 14;
        // RUNNING
        wattron(win_tstate, COLOR_PAIR(1));
        mvwprintw(win_tstate, 1, ix, "%d Running ", cntThreadRunning);
        int tmpint = cntThreadRunning;
        int intDig = 0;
        while (tmpint > 0) {
            intDig++;
            tmpint /= 10;
        }
        ix += intDig + 8;
        // WAITING
        wattron(win_tstate, COLOR_PAIR(2));
        mvwprintw(win_tstate, 1, ix, " %d Waiting ", cntThreadWaiting);
        tmpint = cntThreadWaiting;
        intDig = 0;
        while (tmpint > 0) {
            intDig++;
            tmpint /= 10;
        }
        ix += intDig + 9;
        // BLOCKED
        wattron(win_tstate, COLOR_PAIR(3));
        mvwprintw(win_tstate, 1, ix, " %d Blocked ", cntThreadBlocked);
        tmpint = cntThreadBlocked;
        intDig = 0;
        while (tmpint > 0) {
            intDig++;
            tmpint /= 10;
        }
        ix += intDig + 11;
        //////////////////////////////////////////////////
        //Graph...
        //////////////////////////////////////////////////
        wattron(win_tstate, COLOR_PAIR(7));
        mvwprintw(win_tstate, 1, ix, "[", cntThreadRunning);
        ix++;
        wattron(win_tstate, COLOR_PAIR(1));
        for (int i = 0; i < pct_running; i++) {
            mvwprintw(win_tstate, 1, ix, "|");
            ix++;
        }
        wattron(win_tstate, COLOR_PAIR(2));
        for (int i = 0; i < pct_waiting; i++) {
            mvwprintw(win_tstate, 1, ix, "|");
            ix++;
        }
        wattron(win_tstate, COLOR_PAIR(3));
        for (int i = 0; i < pct_blocked; i++) {
            mvwprintw(win_tstate, 1, ix, "|");
            ix++;
        }
        wattron(win_tstate, COLOR_PAIR(7));
        int unresolvedt=ix+50-(pct_running+pct_waiting+pct_blocked);
        while(ix<unresolvedt){
            mvwprintw(win_tstate, 1, ix, "_");
            ix++;
        }
        mvwprintw(win_tstate, 1, ix, "]");
        ix++;
    }
    wrefresh(win_tstate);
    wattroff(win_tstate, COLOR_PAIR(3));
    use_default_colors();
    box(win_tstate, 0, 0);

    //////////////////////////////////////////////////
    // Main LOOP catch keys...
    //////////////////////////////////////////////////
    refresh();
    while (1) {
        threadControl=true;
        key_in = getch();
        threadControl=false;
        mvwprintw(win_stack, 1, 1, "pressed key id: %d", key_in);
        if (key_in == 27 || key_in == 113) {
            //quit out (Esc)
            //quit out (q)
            break;
        } else if (key_in == 112) {
            // Toggle sleep mode (p)
            toggleSleepMode();
        } else if (key_in == 115) {
            // o
            toggleOrderBy();
        } else if (key_in == 102) {
            // f
            toggleFilterMode();
        } else if (key_in == 259 || key_in == 107) {
            // up
            stackWindowScrollUp(&cnt_scroll);
        } else if (key_in == 258 || key_in == 108 || key_in == 117) {
            // down
            stackWindowScrollDown(&cnt_scroll);
        } else if (key_in == 110) {
            // n
            navigateNextThread(&cnt_scroll);
        } else if (key_in == 78) {
            // Shift+n
            navigatePreviousThread(&cnt_scroll);
        } else if (key_in == 116) {
            // Toggle Window Control
            toggleActiveWindow(&cnt_scroll);
        } else if (key_in == 103) {
            ////////////////////
            // Get new stack from java...
            ////////////////////
            cnt_scroll=0;
            getJavaStack(javapid, &cntThreadRunning, &cntThreadWaiting, &cntThreadBlocked, cnt_exclude, arr_exclude);
            box(win_stack, 0, 0);
            wrefresh(win_stack);
            printJavaStack();
            getJavaStackFiltered(cnt_exclude, arr_exclude);
        } else if (key_in == 47 || key_in == 267) {
            //search input = '/' or 'F3'
            sleepMode=true;
            wattron(jtopWindows.win_stack, A_BOLD);
            wattron(jtopWindows.win_stack, COLOR_PAIR(8));
            wmove(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 1);
            wclrtoeol(jtopWindows.win_stack);

            char stackwinwidth[jtopWindows.STACK_WIN_MAX_COL-1];
            memset(stackwinwidth, '\0', jtopWindows.STACK_WIN_MAX_COL-1);
            memset(stackwinwidth, ' ', jtopWindows.STACK_WIN_MAX_COL-2);
            mvwprintw(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 1, "%s", stackwinwidth);

            wmove(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 1);
            mvwprintw(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 1, "Search: \n");
            curs_set(2);
            wrefresh(jtopWindows.win_stack);
            char searchTerm[100];
            wmove(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 9);
            keypad(jtopWindows.win_stack, TRUE);
            echo();
            mvwscanw(jtopWindows.win_stack, (jtopWindows.STACK_WIN_MAX_LINE - 2), 9, "%s", searchTerm);

            // back to normal
            wattroff(jtopWindows.win_stack, A_BOLD);
            wattroff(jtopWindows.win_stack, COLOR_PAIR(6));
            use_default_colors();
            sleepMode=false;
            noecho();
            curs_set(0);
            keypad(stdscr, TRUE);
            wrefresh(jtopWindows.win_stack);
            refresh();

            navigateSearchTerm(&cnt_scroll, searchTerm);

        }
        ////////////////////
        ////////////////////
        // End user input - refresh boxes
        ////////////////////
        ////////////////////
        if(focusOn==0){
            wattron(win_stack, A_BOLD);
            wattron(win_stack, COLOR_PAIR(7));
            box(win_stack, 0, 0);
            wattroff(win_stack, A_BOLD);
            wattroff(win_stack, COLOR_PAIR(7));
            use_default_colors();
            box(win_jtop, 0, 0);
        }else{
            box(win_stack, 0, 0);
            wattron(win_jtop, A_BOLD);
            wattron(win_jtop, COLOR_PAIR(7));
            box(win_jtop, 0, 0);
            wattroff(win_jtop, A_BOLD);
            wattroff(win_jtop, COLOR_PAIR(7));
            use_default_colors();
        }
        wrefresh(win_stack);
        wrefresh(win_jtop);
        refresh();
    }
    //////////////////////////////////////////////////
    // Destroy it all...
    //////////////////////////////////////////////////
    sleepMode=1;
    for (int i = 0; i <= javaThreadDump.cnt_stacklines; i++) {
        free(javaThreadDump.arr_stacklines[i]);
    }
    for (int i =0; i<= cnt_exclude; i++){
        free(arr_exclude[cnt_exclude]);
    }
    delwin(win_stack);
    delwin(win_tstate);
    endwin();
    return 0;
}
