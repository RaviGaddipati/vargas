//
// Created by gaddra on 8/6/15.
//

#ifndef VMATCH_H
#define VMATCH_H

#include <string>
#include <stdlib.h>
#include <iostream>
#include "getopt_pp.h"
#include "graph.h"
#include "readsim.h"
#include "readfile.h"


int build_main(int argc, char *argv[]);

void printMainHelp();
void printBuildHelp();
void printSimHelp();
void printAlignHelp();

#endif //VMATCH_H
