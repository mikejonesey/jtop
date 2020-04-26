//
// Created by mike on 2/11/18.
//

#ifndef JTOP_CLASSCPUWINDOW_H
#define JTOP_CLASSCPUWINDOW_H

#include "jtop.h"

void updateClassInfo(int classccpu, char *ptr_classname);
void printClassCPU(WINDOW *win_classcpu);
void printBlocks(WINDOW *win_classcpu);

#endif //JTOP_CLASSCPUWINDOW_H
