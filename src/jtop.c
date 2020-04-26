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
const char *argp_program_version = "jtop 1.0";
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

bool procIsJava(char *pid){
    char buffer[100];
    char procpath[100];
    FILE *f_procfs;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));

    strcpy(procpath, "/proc/");
    strncat(procpath, pid, 10);
    strcat(procpath, "/comm");
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

bool procIsCurrentUser(char *pid, uid_t uid_curuser){
    char buffer[1000];
    char *buffsrch;
    char procpath[100];
    FILE *f_procfs;
    char procUid[100];

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(procUid, 0, sizeof(procUid));

    strcpy(procpath, "/proc/");
    strncat(procpath, pid, 10);
    strcat(procpath, "/status");
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        //comm file is open...
        while(fgets(buffer, sizeof(buffer), f_procfs)){
            buffsrch = strstr(buffer,"Uid:");
            if(buffsrch){
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
        }
        fclose(f_procfs);
        uid_t uid_procuid = (int)strtol(procUid, NULL, 0);
        if(uid_procuid==uid_curuser){
            return true;
        }
    }
    return false;
}

void getProcUser(char *pid, struct passwd *pw_procuser, char pw_procuser_buff[], size_t pw_procuser_buff_size){
    char buffer[1000];
    char *buffsrch;
    char procpath[100];
    FILE *f_procfs;
    char procUid[100];
    struct passwd *pw_temp;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(procUid, 0, sizeof(procUid));

    strcpy(procpath, "/proc/");
    strncat(procpath, pid, 10);
    strcat(procpath, "/status");
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        //comm file is open...
        while(fgets(buffer, sizeof(buffer), f_procfs)){
            buffsrch = strstr(buffer,"Uid:");
            if(buffsrch){
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
        }
        fclose(f_procfs);
        uid_t uid_procUid = (uid_t)strtol(procUid, NULL, 0);
        getpwuid_r(uid_procUid,pw_procuser,pw_procuser_buff,pw_procuser_buff_size,&pw_temp);
    }
}

char *getProcCmdline(char *pid, char cmdline[], size_t cmdline_size){
    char buffer[1024];
    char procpath[100];
    FILE *f_procfs;

    memset(buffer, 0, sizeof(buffer));
    memset(procpath, 0, sizeof(procpath));
    memset(cmdline, 0, cmdline_size);

    strcpy(procpath, "/proc/");
    strncat(procpath, pid, 10);
    strcat(procpath, "/cmdline");
    f_procfs = fopen(procpath, "r");
    if (f_procfs) {
        while(fgets(buffer, sizeof(buffer), f_procfs)){
            for(int i=0; i<sizeof(buffer)-2; i++){
                if(buffer[i]=='\0' && buffer[i+1]!='\0'){
                    buffer[i]=' ';
                }else if(buffer[i]=='\0' && buffer[i+1]=='\0'){
                    break;
                }
            }
            buffer[1023]='\0';
            strncat(cmdline,buffer,(cmdline_size-strlen(cmdline)));
        }
        fclose(f_procfs);
    }
    return cmdline;
}

char *getJavaPid(char *javapid){
    char arr_javas[10][20];
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

    if (procdir) {
        while (printRow<10 && (proclist = readdir(procdir)) != NULL) {
            if (strcmp(proclist->d_name, ".") != 0 && strcmp(proclist->d_name, "..") != 0 && (proclist->d_name[0]-'0'>=0 && proclist->d_name[0]-'0'<=9)) {
                //check if process is java
                if(procIsJava(proclist->d_name)){
                    if(procIsCurrentUser(proclist->d_name,pw_curuser.pw_uid)){
                        // store pid for later
                        strncpy(arr_javas[printRow-1],proclist->d_name,10);
                        // print commandline
                        char cmdline[4096];
                        getProcCmdline(proclist->d_name,cmdline,sizeof(cmdline));
                        printf("%d. PID: %s\n   CMD: %s\n\n",printRow,proclist->d_name,cmdline);
                        printRow++;
                    }else{
                        getProcUser(proclist->d_name,&pw_procuser,pw_procuser_buff,sizeof(pw_procuser_buff));
                        if(pw_procuser.pw_name){
                            printf("N/A Wrong User... PID: %s is run by user: %s, you are: %s\n\n", proclist->d_name,pw_procuser.pw_name,pw_curuser.pw_name);
                        }else{
                            printf("N/A Wrong User... PID: %s is run by an unknown user, you are: %s\n\n", proclist->d_name,pw_curuser.pw_name);
                        }
                    }
                }
            }
        }
        closedir(procdir);
    }

    int userselection;
    if(printRow==1){
        printf("No Java process found, exiting...\n");
        exit(1);
    }else if(printRow==2){
        userselection=1;
    }else{
        printf("Select a Java Process to monitor: ");
        scanf("%d",&userselection);
    }
    if(arr_javas[userselection-1] && arr_javas[userselection-1][0] != '\0'){
        for(int i=0; i<strlen(arr_javas[userselection - 1]); i++){
            javapid[i]=arr_javas[userselection - 1][i];
        }
        javapid[strlen(arr_javas[userselection - 1])]='\0';
        return javapid;
    } else{
        printf("\nError...\n");
        exit(1);
    }
}

void *pollTopWindow(void *myargs){
    while(1){
        while(threadControl&&!sleepMode){
            struct pollTopWinArgs *jtopWinData = myargs;
            WINDOW *win_jtop = jtopWinData->win_jtop;
            WINDOW *win_classcpu = jtopWinData->win_ctop;
            char *javapid = jtopWinData->javapid;
            int STACK_WIN_MAX_COL = jtopWinData->STACK_WIN_MAX_COL;
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
            printTop(win_jtop, cnt_threads, STACK_WIN_MAX_COL);

            printClassCPU(win_classcpu);
            sleep(5);
        }
        sleep(1);
    }
    return NULL;
}

void toggleSleepMode(WINDOW *win_jtop, int STACK_WIN_MAX_COL){
    if(sleepMode){
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
        //mvwchgat(win_jtop,"test1001");
        wattroff(win_jtop, A_BOLD);
        wattroff(win_jtop, COLOR_PAIR(6));
        use_default_colors();
        sleepMode=false;
    }else{
        sleepMode=true;
        wattron(win_jtop, A_BOLD);
        wattron(win_jtop, COLOR_PAIR(4));
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
        //mvwchgat(win_jtop,"test1001");
        wattroff(win_jtop, A_BOLD);
        wattroff(win_jtop, COLOR_PAIR(6));
        use_default_colors();
        wrefresh(win_jtop);
    }
}

void toggleFilterMode(WINDOW *win_stack, WINDOW *win_jtop, int STACK_WIN_MAX_LINE, int cnt_win_stack_rows, char *arr_stacklines[], int cnt_win_stack_rows_filt, char *arr_stacklines_filt[], const int *STACK_WIN_MAX_COL){
    if(filterMode){
        filterMode=false;
        printJavaStack(win_stack, cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_LINE);
        printTop(win_jtop, cnt_threads,*STACK_WIN_MAX_COL);
    }else{
        filterMode=true;
        printJavaStack(win_stack, cnt_win_stack_rows_filt, arr_stacklines_filt, STACK_WIN_MAX_LINE);
        printTop(win_jtop, cnt_threads, *STACK_WIN_MAX_COL);
    }
}

void scrollUp(WINDOW *win_stack, WINDOW *win_jtop, int *cnt_scroll, char *arr_stacklines[], char *arr_stacklines_filt[], const int *STACK_WIN_MAX_COL, int *cnt_win_stack_rows, const int *STACK_WIN_MAX_LINE){
    if(focusOn==0){
        if(*cnt_scroll>0){
            if (*cnt_scroll < 6) {
                *cnt_scroll = 0;
            } else {
                *cnt_scroll -= 5;
            }
            if (wscrl(win_stack, -5) == OK) {
                int ii = *cnt_scroll;

                wmove(win_stack, 5, 1);
                wclrtoeol(win_stack);

                for (int i = 0; i < 5; i++) {
                    if(filterMode){
                        mvwprintw(win_stack, (i + 1), 1, "%s", arr_stacklines_filt[ii]);
                    }else{
                        mvwprintw(win_stack, (i + 1), 1, "%s", arr_stacklines[ii]);
                    }
                    ii++;
                }
            }
        }
    }else if(focusOn==1){
        topActiveRow--;
        if(topActiveRow<0){
            topActiveRow=0;
        }
        printTop(win_jtop, cnt_threads, *STACK_WIN_MAX_COL);
        printJavaThreadStack(win_stack, arr_jthreads[topActiveRow].name, *cnt_win_stack_rows, arr_stacklines, *STACK_WIN_MAX_LINE);
    }
}

void scrollDown(WINDOW *win_stack, WINDOW *win_jtop, int *cnt_scroll, char *arr_stacklines[], char *arr_stacklines_filt[], int *cnt_win_stack_rows, const int *STACK_WIN_MAX_LINE, const int *JTOP_WIN_MAX_LINE, const int *STACK_WIN_MAX_COL){
    if(focusOn==0){
        if(*cnt_scroll<(*cnt_win_stack_rows-*STACK_WIN_MAX_LINE)-6){
            *cnt_scroll += 5;
            if (*cnt_scroll > (*cnt_win_stack_rows - *STACK_WIN_MAX_LINE) + 6) {
                *cnt_scroll = (*cnt_win_stack_rows - *STACK_WIN_MAX_LINE) + 6;
            }
            if (wscrl(win_stack, 5) == OK) {
                int startprintdata=*cnt_scroll+(*STACK_WIN_MAX_LINE-7);
                int startprintline=*STACK_WIN_MAX_LINE-6;

                wmove(win_stack, startprintline, 1);
                wclrtoeol(win_stack);

                for (int i = startprintline; i < startprintline+5; i++) {
                    if(filterMode && arr_stacklines_filt[startprintdata]){
                        mvwprintw(win_stack, i, 1, "%s", arr_stacklines_filt[startprintdata]);
                    }else if(!filterMode && arr_stacklines[startprintdata]){
                        mvwprintw(win_stack, i, 1, "%s", arr_stacklines[startprintdata]);
                    }
                    startprintdata++;
                }
                wrefresh(win_jtop);
            }
        }
    }else if(focusOn==1){
        if(topActiveRow<cnt_threads-1){
            topActiveRow++;
            printTop(win_jtop, cnt_threads, *STACK_WIN_MAX_COL);
            printJavaThreadStack(win_stack, arr_jthreads[topActiveRow].name, *cnt_win_stack_rows, arr_stacklines, *STACK_WIN_MAX_LINE);
        }
        /*
        if(topOffset<cnt_threads-(*JTOP_WIN_MAX_LINE-3)){
            topOffset++;
            printTop(win_jtop, topOffset, cnt_threads, *STACK_WIN_MAX_COL);
            printJavaThreadStack(win_stack, arr_jthreads[topOffset].name, *cnt_win_stack_rows, arr_stacklines, *STACK_WIN_MAX_LINE);
        }
        */
    }
}

int main(int argc, char **argv) {
    int key_in;
    int cnt_scroll = 0;
    unsigned int cntThreadRunning = 0;
    unsigned int cntThreadWaiting = 0;
    unsigned int cntThreadBlocked = 0;
    unsigned int cnt_win_stack_rows = 0;
    unsigned int cnt_win_stack_rows_filt = 0;
    unsigned int parent_x, parent_y, new_x, new_y;
    unsigned int tstate_size = 3;
    char *arr_stacklines[100000];
    char *arr_stacklines_filt[100000];
    char *arr_exclude[2000];
    int cnt_exclude=0;
    char searchString[256];
    bool searchMode = 0;
    bool pausedThreadReload = 0;
    //mmap
    struct sysinfo info;
    sysinfo(&info);
    const long int SYS_UPTIME = info.uptime;
    long SYS_JIFFPS = sysconf(_SC_CLK_TCK);
    const long int SYS_CURTIME = time(NULL);
    const long int SYS_BOOTTIME = (SYS_CURTIME - SYS_UPTIME);

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
    jtopWinData.javapid = javapid;

    if(javapid==NULL){
        //printf("No Java process specified...\n");
        javapid=malloc(200);
        memset(javapid,0,sizeof(javapid));
        javapid = getJavaPid(javapid);
        jtopWinData.javapid = javapid;
    }

    //////////////////////////////////////////////////
    // Setup Curses
    //////////////////////////////////////////////////
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    getmaxyx(stdscr, parent_y, parent_x);
    const int STACK_WIN_MAX_LINE = 20;
    const int STACK_WIN_MAX_COL = (parent_x) - 1;
    const int STACK_WIN_MAX_COLT = (parent_x/2);
    const int JTOP_WIN_MAX_COL = parent_x;
    jtopWinData.STACK_WIN_MAX_COL = STACK_WIN_MAX_COL;
    const int JTOP_WIN_MAX_LINE = parent_y - tstate_size - 21;
    jtopWinData.JTOP_WIN_MAX_LINE = JTOP_WIN_MAX_LINE;
    WINDOW *win_stack = newwin(STACK_WIN_MAX_LINE, STACK_WIN_MAX_COLT , 0, 0);
    WINDOW *win_ctop = newwin(STACK_WIN_MAX_LINE, STACK_WIN_MAX_COLT , 0, STACK_WIN_MAX_COLT+1);
    WINDOW *win_jtop = newwin(JTOP_WIN_MAX_LINE, parent_x, STACK_WIN_MAX_LINE, 0);
    WINDOW *win_tstate = newwin(tstate_size, parent_x, (parent_y - tstate_size) - 1, 0);

    jtopWinData.win_stack = win_stack;
    jtopWinData.win_jtop = win_jtop;
    jtopWinData.win_ctop = win_ctop;

    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_WHITE, COLOR_GREEN);
    init_pair(6, COLOR_WHITE, COLOR_BLUE);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
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
    //mvwchgat(win_jtop,"test1001");
    wattroff(win_jtop, A_BOLD);
    wattroff(win_jtop, COLOR_PAIR(6));
    use_default_colors();

    mvwprintw(win_ctop, 1, 1, "CPU By Class\n");
    whline(win_ctop, 0, STACK_WIN_MAX_COLT);
    box(win_ctop, 0, 0);

    box(win_stack, 0, 0);
    box(win_jtop, 0, 0);
    box(win_tstate, 0, 0);
    mvprintw((parent_y - 1), 0, "F3 Search | [N]ext Thread | [T]oggle Window | [P]ause | [F]ilter Stack | [G]et Stack | [Q]uit");
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
    if(pthread_create(&pollTopThread, NULL, pollTopWindow, &jtopWinData)) {
        mvwprintw(win_stack, 2, 1, "Error creating thread...");
        wrefresh(win_stack);
    }

    //////////////////////////////////////////////////
    // Get Stack
    //////////////////////////////////////////////////

    //populate exclude array from file
    getExcludes(&cnt_exclude, arr_exclude);

    //populate stack lines from jcmd
    getJavaStack(javapid, &cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_COL, &cntThreadRunning, &cntThreadWaiting, &cntThreadBlocked, cnt_exclude, arr_exclude);
    //waitpid();

    //populate fileterd stack lines from stackline array
    getJavaStackFiltered(cnt_win_stack_rows, &cnt_win_stack_rows_filt, cnt_exclude, arr_exclude, arr_stacklines, arr_stacklines_filt);

    //////////////////////////////////////////////////
    // Print Stack...
    //////////////////////////////////////////////////
    printJavaStack(win_stack, cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_LINE);
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
    char *bars[1000];
    int waste=20;
    int pct_running = cntThreadRunning*50/(cnt_threads-1);
    //int pct_waiting = cntThreadWaiting*50/(cnt_threads-1);
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
        //mvwprintw(win_stack, 1, 1, "pressed key id: %d", key_in);
        if (searchMode == 1) {
            if (key_in == 104) {
                ////////////////////
                //run search
                ////////////////////
                int search_cnt_scroll = cnt_scroll;
                search_cnt_scroll += 1;
                while (!strstr(arr_stacklines[search_cnt_scroll], searchString) ||
                       search_cnt_scroll == cnt_scroll + 1) {
                    search_cnt_scroll++;
                }
                if (search_cnt_scroll > cnt_scroll) {
                    if ((search_cnt_scroll > cnt_win_stack_rows - STACK_WIN_MAX_LINE)) {
                        cnt_scroll = cnt_win_stack_rows - STACK_WIN_MAX_LINE;
                    } else {
                        cnt_scroll = search_cnt_scroll - 1;
                    }
                    int ii = cnt_scroll;
                    for (int i = 0; i < STACK_WIN_MAX_LINE; i++) {
                        if (arr_stacklines[ii]) {
                            mvwprintw(win_stack, (i + 1), 1, "%s", arr_stacklines[ii]);
                            ii++;
                        }
                    }
                }
            } else {
                //append and print string
                //strcat(searchString, &key_in);
                //mvwprintw(win_stack, (STACK_WIN_MAX_LINE + 1), 1, "CHECK A CHECK: %s\n", key_in);
                //mvwprintw(win_stack, (STACK_WIN_MAX_LINE - 1), 1, "Search: %s\n", key_in);
            }
        }else if (key_in == 113) {
            ////////////////////
            //quit out
            ////////////////////
            break;
        } else if (key_in == 112) {
            ////////////////////////////////////////
            // Toggle sleep mode
            ////////////////////////////////////////
            toggleSleepMode(win_jtop, STACK_WIN_MAX_COL);
        } else if (key_in == 115) {
            ////////////////////////////////////////
            // Toggle order by
            ////////////////////////////////////////
            if(orderMode==0){
                orderMode=1;
            }else{
                orderMode=0;
            }
        } else if (key_in == 102) {
            ////////////////////////////////////////
            // Toggle filter mode
            ////////////////////////////////////////
            toggleFilterMode(win_stack, win_jtop, STACK_WIN_MAX_LINE, cnt_win_stack_rows, arr_stacklines, cnt_win_stack_rows_filt, arr_stacklines_filt, &STACK_WIN_MAX_COL);
        } else if (key_in == 259 || key_in == 107) {
            ////////////////////
            //scroll up
            ////////////////////
            scrollUp(win_stack, win_jtop, &cnt_scroll, arr_stacklines, arr_stacklines_filt, &STACK_WIN_MAX_COL, &cnt_win_stack_rows, &STACK_WIN_MAX_LINE);
        } else if (key_in == 258 || key_in == 108) {
            ////////////////////
            //scroll down
            ////////////////////
            scrollDown(win_stack, win_jtop, &cnt_scroll, arr_stacklines, arr_stacklines_filt, &cnt_win_stack_rows, &STACK_WIN_MAX_LINE, &JTOP_WIN_MAX_LINE, &STACK_WIN_MAX_COL);
        } else if (key_in == 110) {
            ////////////////////
            // Next thread
            ////////////////////
            int search_cnt_scroll = cnt_scroll + 2;
            while (arr_stacklines[search_cnt_scroll] && arr_stacklines[search_cnt_scroll][0] != '"' && cnt_scroll < cnt_win_stack_rows) {
                search_cnt_scroll++;
            }

            // validate search and print
            if(arr_stacklines[search_cnt_scroll]){
                //stackline exists
                if(arr_stacklines[search_cnt_scroll][0] != '"'){
                    //search failed
                    cnt_scroll = cnt_win_stack_rows-10;
                }else{
                    //success
                    cnt_scroll = search_cnt_scroll;
                }
            }

            // scroll and print
            int ii = cnt_scroll;
            for (int i = 1; i < STACK_WIN_MAX_LINE; i++) {
                wmove(win_stack, i, 1);
                wclrtoeol(win_stack);
                if (arr_stacklines[ii]) {
                    mvwprintw(win_stack, i, 1, "%s", arr_stacklines[ii]);
                }
                ii++;
            }
        } else if (key_in == 78) {
            ////////////////////
            // Previous thread
            ////////////////////
            int search_cnt_scroll = cnt_scroll - 2;
            while (arr_stacklines[search_cnt_scroll] && arr_stacklines[search_cnt_scroll][0] != '"' && cnt_scroll > 0) {
                search_cnt_scroll--;
            }

            //validate search result
            if(arr_stacklines[search_cnt_scroll]){
                //stackline exists
                if(arr_stacklines[search_cnt_scroll][0] != '"'){
                    //search failed
                    //flash();
                    //beep();
                    cnt_scroll = 0;
                }else{
                    //success
                    cnt_scroll = search_cnt_scroll;
                }
            }

            //scroll and print
            int ii = cnt_scroll;
            for (int i = 1; i < STACK_WIN_MAX_LINE; i++) {
                wmove(win_stack, i, 1);
                wclrtoeol(win_stack);
                if (arr_stacklines[ii]) {
                    mvwprintw(win_stack, i, 1, "%s", arr_stacklines[ii]);
                }
                ii++;
            }
        } else if (key_in == 116) {
            ////////////////////
            // Toggle controls
            ////////////////////
            if(focusOn==0){
                cnt_scroll=0;
                focusOn=1;
                printJavaThreadStack(win_stack, arr_jthreads[topActiveRow].name, cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_LINE);
            }else{
                focusOn=0;
                if(filterMode){
                    // Get the active line number...
                    int curlinenum = getLineJavaStack(cnt_win_stack_rows_filt, arr_jthreads[topActiveRow].name, arr_stacklines_filt);
                    // Set the current line number to use...
                    cnt_scroll=curlinenum-1;
                    // Print the trace...
                    printJavaStack(win_stack, cnt_win_stack_rows_filt, arr_stacklines_filt, STACK_WIN_MAX_LINE);
                }else{
                    // Get the active line number...
                    int curlinenum = getLineJavaStack(cnt_win_stack_rows, arr_jthreads[topActiveRow].name, arr_stacklines);
                    // Set the current line number to use...
                    //cnt_scroll=curlinenum-1;
                    // Print the trace...
                    printJavaStack(win_stack, cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_LINE);
                    int limitloop=0;
                    while (cnt_scroll <= (curlinenum-5) && limitloop<100){
                        scrollDown(win_stack, win_jtop, &cnt_scroll, arr_stacklines, arr_stacklines_filt, &cnt_win_stack_rows, &STACK_WIN_MAX_LINE, &JTOP_WIN_MAX_LINE, &STACK_WIN_MAX_COL);
                        limitloop++;
                    }
                }
            }
        } else if (key_in == 103) {
            ////////////////////
            // Get new stack from java...
            ////////////////////
            pausedThreadReload=1;
            cnt_scroll=0;
            getJavaStack(javapid, &cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_COL, &cntThreadRunning, &cntThreadWaiting, &cntThreadBlocked, cnt_exclude, arr_exclude);
            box(win_stack, 0, 0);
            wrefresh(win_stack);
            if(filterMode){
                printJavaStack(win_stack, cnt_win_stack_rows_filt, arr_stacklines_filt, STACK_WIN_MAX_LINE);
            }else{
                printJavaStack(win_stack, cnt_win_stack_rows, arr_stacklines, STACK_WIN_MAX_LINE);
            }
            getJavaStackFiltered(cnt_win_stack_rows, &cnt_win_stack_rows_filt, cnt_exclude, arr_exclude, arr_stacklines, arr_stacklines_filt);
        } else if (key_in == 47) {
            ////////////////////
            //search input
            ////////////////////
            sleepMode=true;
            searchMode = 1;
            mvwprintw(win_stack, (STACK_WIN_MAX_LINE - 1), 1, "Search: \n");
            echo();
            curs_set(2);
            setsyx((STACK_WIN_MAX_LINE - 1), 8);
            wmove(win_stack, (STACK_WIN_MAX_LINE - 1), 8);
            wclrtoeol(win_stack);
            wrefresh(win_stack);
            wgetch(win_stack);
            refresh();
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
        //box(win_side2, 0, 0);
        wrefresh(win_stack);
        wrefresh(win_jtop);
        refresh();
    }
    //////////////////////////////////////////////////
    // Destroy it all...
    //////////////////////////////////////////////////
    //kill(pid, SIGKILL);
    //pthread_kill(pollTopThread, SIGKILL);
    for (int i = 0; i < cnt_win_stack_rows; i++) {
        free(arr_stacklines[i]);
    }
    for (int i =0; i< cnt_exclude; i++){
        free(arr_exclude[cnt_exclude]);
    }
    /*
    for (int i = 0; i < cnt_threads; i++) {
        if(arr_jthreads[i].command!=NULL){
            free(arr_jthreads[i].command);
        }
        if(arr_jthreads[i].altcommand!=NULL){
            free(arr_jthreads[i].altcommand);
        }
    }*/
    delwin(win_stack);
    delwin(win_tstate);
    endwin();
    return 0;
}
