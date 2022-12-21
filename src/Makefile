# ######################################################################
# #
# #                       Author: Annie Wang & Reeda Mroue
# #
# # The autograder will run the following command to build the program:
# #
# #       make -B
# #
# ######################################################################

# # name of the program to build
# #

# utils.o : utils.c utils.h
# 	clang -c -Wall utils.c
# filesystem.o : filesystem.c filesystem.h
# 	clang -c -Wall filesystem.c
# pennfat.o : pennfat.c
# 	clang -c -Wall pennfat.c
# filefunc.o : filefunc.c filefunc.h
# 	clang -c -Wall filefunc.c
# pennfat : pennfat.o filesystem.o utils.o
# 	clang -o pennfat pennfat.o filesystem.o utils.o
# built_ins.o : built_ins.c built_ins.h
# 	clang -c -Wall built_ins.c
# kernel_func.o : kernel_func.c kernel_func.h
# 	clang -c -Wall kernel_func.c
# kernel.o : kernel.c kernel.h
# 	clang -c -Wall kernel.c
# log.o : log.c log.h
# 	clang -c -Wall log.c
# pcb_list.o : pcb_list.c pcb_list.h pcb.h
# 	clang -c -Wall pcb_list.c
# fsf : filefunc.o filesystem.o utils.o
# 	clang -o fsf filefunc.o filesystem.o utils.o
# clean :
# 	rm *.o fsf pennfat \.
# all : fsf


PROG=pennos

PROG_2=pennfat

BUILDDIR = bin

SOURCE_DIR = bin

NAME=$

PROMPT='"$(NAME) "'

# Remove -DNDEBUG during development if assert(3) is used
#
override CPPFLAGS += -DNDEBUG -DPROMPT=$(PROMPT)

CC = clang

# Replace -O1 with -g for a debug version during development
#
# CFLAGS = -Wall -Werror -O1

# SRCS = $(wildcard *.c)
# SRCS_FILTERED = $(filter-out pennfat.c, $(SRCS)) 
# OBJS = $(patsubst %,$(BIN)/%, $(SRCS_FILTERED:.c=.o) )
# SRCS_PF = pennfat.c filesystem.c utils.c
# OBJS_PF = $(patsubst %,$(BIN)/%, $(SRCS_FILTERED:.c=.o) )


# .PHONY : clean

# $(BIN)/%.o : %.c
# 	$(CC) -o $@ $^

# $(BIN)/$(PROG) : $(OBJS)
# 	$(CC) -o $@ $^ ./parser/parser-$(shell uname -p).o

# $(BIN)/$(PROG_2) : $(OBJS_PF)
# 	$(CC) -o $@ $^

# clean :
# 	$(RM) $(OBJS) $(PROG) $(PROG_2)



CFLAGS = -Wall -Werror -O1

SRCS = $(wildcard *.c)
SRCS_FILTERED = $(filter-out pennfat.c, $(SRCS)) 
OBJS = $(SRCS_FILTERED:.c=.o) 
SRCS_PF = pennfat.c filesystem.c utils.c
OBJS_PF = $(SRCS_PF:.c=.o) 

.PHONY : clean

$(PROG) : $(OBJS)
	$(CC) -o $@ $^ ./parser/parser-$(shell uname -p).o
	mv *.o ../$(BUILDDIR)
	mv $(PROG) ../$(BUILDDIR)
	rm -f ../$(BUILDDIR)/minfs
	cp -f minfs ../$(BUILDDIR)

$(PROG_2) : $(OBJS_PF)
	$(CC) -o $@ $^
	mv *.o ../$(BUILDDIR)
	mv $(PROG_2) ../$(BUILDDIR)
	rm -f ../$(BUILDDIR)/minfs
	cp minfs ../$(BUILDDIR)

clean :
	$(RM) $(OBJS) $(PROG) $(PROG_2)

