#!/bin/bash
LOG=/tmp/build.log
date > $LOG
make clean                2>&1 | tee -a $LOG
make clean TARGETOS=WINXP 2>&1 | tee -a $LOG
make clean TARGETOS=WIN32 2>&1 | tee -a $LOG
make clean TARGETOS=WIN64 2>&1 | tee -a $LOG
make                      2>&1 | tee -a $LOG
make TARGETOS=WINXP       2>&1 | tee -a $LOG
make TARGETOS=WIN32       2>&1 | tee -a $LOG
make TARGETOS=WIN64       2>&1 | tee -a $LOG
make setup TARGETOS=WINXP 2>&1 | tee -a $LOG
make setup TARGETOS=WIN32 2>&1 | tee -a $LOG
make setup TARGETOS=WIN64 2>&1 | tee -a $LOG
make debian               2>&1 | tee -a $LOG
