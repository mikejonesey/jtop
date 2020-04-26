//
// Created by mike on 2/11/18.
//

#include "classcpuWindow.h"
#include "jtop.h"

void updateClassInfo(int classccpu, const char *ptr_classname){
    int i = 0;

    // return if there is no class to track...
    if(ptr_classname==NULL || ptr_classname[0]=='\0'){
        return;
    }

    // check if class is already tracked...
    while(i<10 && arr_class_cpu[i].className !=NULL && arr_class_cpu[i].className[0] != '\0'){
        if(strcmp(arr_class_cpu[i].className,ptr_classname)==0){
            arr_class_cpu[i].totalcpu+=classccpu;
            break;
        }
        i++;
    }

    // add a new class to track...
    if(arr_class_cpu[i].className==NULL || arr_class_cpu[i].className[0] == '\0'){
        //new class deff...
        strncpy(arr_class_cpu[i].className,ptr_classname,399);
        arr_class_cpu[i].totalcpu=classccpu;
    }
}

void orderClassCPU(){
    struct class_cpu tmp_class_cpu;
    for(int i=0; i<10; i++){
        for(int ii=0; ii<10; ii++){
            if(arr_class_cpu[i].totalcpu && arr_class_cpu[ii].totalcpu && arr_class_cpu[i].totalcpu>arr_class_cpu[ii].totalcpu){
                tmp_class_cpu = arr_class_cpu[i];
                arr_class_cpu[i] = arr_class_cpu[ii];
                arr_class_cpu[ii] = tmp_class_cpu;
            }
        }
    }
}

void printClassCPU(){
    orderClassCPU();

    int totalcpu=0;
    int classcount=0;
    int pct_cpu;

    for(int i=0; i<10; i++){
        if(arr_class_cpu[i].totalcpu){
            totalcpu+=arr_class_cpu[i].totalcpu;
            classcount++;
        }
    }

    mvwprintw(jtopWindows.win_ctop, 1, 1, "CPU By Class\n");
    whline(jtopWindows.win_ctop, 0, 30);

    int i = 0;
    while(i<10 && arr_class_cpu[i].className && arr_class_cpu[i].className[0] != '\0'){
        //clear line...
        wmove(jtopWindows.win_ctop, i+3, 1);
        wclrtoeol(jtopWindows.win_ctop);

        pct_cpu=arr_class_cpu[i].totalcpu*100/totalcpu;

        if(pct_cpu==0){
            arr_class_cpu[i].totalcpu=0;
            memset(arr_class_cpu[i].className, 0, sizeof(arr_class_cpu[i].className));
            continue;
        }

        //print line...
        mvwprintw(jtopWindows.win_ctop, (i + 3), 1, "%d. %d%% %s", i+1, pct_cpu, arr_class_cpu[i].className);
        i++;

    }
    box(jtopWindows.win_ctop, 0, 0);
    wrefresh(jtopWindows.win_ctop);
}

void printBlocks(){
    mvwprintw(jtopWindows.win_ctop, 1, 1, "Threads blocked by: %s\n",arr_jthreads[topActiveRow].name);
    whline(jtopWindows.win_ctop, 0, 30);
    box(jtopWindows.win_ctop, 0, 0);
}

