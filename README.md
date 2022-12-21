# PennOS

#### Names: Annie Wang (anniwang), Joseph Zhang (jzhang25), Mohamed Abaker (alnasir7), Reeda Mroue (mroue), Sofia Wawrzyniak (fiawawrz)

## Submitted Source Files: 
built_ins.c
built_ins.h
errno.c
errno.h
filefunc.c
filefunc.h
filesystem.c
filesystem.h
kernel.c
kernel.h
kernel_func.c
kernel_func.h
log.c
log.h 
parser.h 
pcb.h 
pcb_list.c 
pcb_list.h
pennfat.c
sched.c
sched.h
shell.c
shell.h 
shell_execute.c
shell_list.c
shell_notif.c
signals.c
signals.h
stress.c
stress.h 
user_func.c
user_func.h
utils.c
utils.h

## Extra Credit Answers: 
n/a

## Compilation Instructions:
In the src folder, type:
make

Then type:
make pennfat

To run the os, type:
./pennos fatfs

To run the pennfat, type: ./pennfat
Make sure to write in mkfs minfs 1 0
or mount minfs, as the first command into pennfat, otherwise a seg fault will occur.

## Overview of Work Accomplished:
2 people worked on the filesystem:
- We made a filesystem api that can read/write from a binary file. Then, we used those filesystem API calls to implement
the functions for pennfat. Then, we used the filesystem API calls to implement the user-level functions in filefunc.c.

3 people worked on the kernel:
- We made a priority scheduler that organizes processes (which are represented by a process control block structure). Then we created the functions required to interact with processes, including the kernel functions and the system calls. Via the PCB structure, we were able to handle the queue, idle processes, and differentiate between running, stopped, blocked, and zombied processes. Finally, we defined macros to represent the process statuses and signals.

All 5 people integrated and worked on the shell
- We adapted PennShell to make the shell. We implemented all the built in functions, and integrated the filesystem and kernel together
 into the shell.

## Description of Code:
This is a makeshift operating system called PennOS, similar to the UNIX operating systems we work with in our day to day. 

The PennOS code handles two primary environments: the user and kernel environments. The user environment is represented by the file system, which is called PennFAT here. This is how the OS handles file reads and writes. 

The kernel environment is represented by the kernel, which creates process control blocks to hold process information. This contains a priority scheduler to organize processes, as well as implementations for the system calls for switching between the user and kernel contexts.

The PennShell is the final component, which allows the user to interact with both the filesystem and the kernel (indirectly) via standard input and output. It is the part of the code we will interact with in our demo.

## Code Layout:
./src contains all of the source code

For the filesystem, the main code is contained in filefunc.c, filesystem.c, and pennfat.c.

For the kernel, the main code is contained in kernel.c, kernel_func.c, user_func.c, pcb.h, pcb_list.c, sched.c, signals.c, utils.c

For the shell, the main code is in shell.c, shell_execute.c, shell_list.c, shell_notif.c, stress.c, errno.c, and built_ins.c

./doc contains all of the documentation including the readme and companion document

./log contains all the logs in log.txt

## General Comments: 
It was fulfilling to build a working operating system. We definitely learned a lot and gained an admiration for the complexity of a working kernel and filesystem.