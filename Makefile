#输出文件夹
OUTFILE=debug
#可执行程序文件夹
OUTBIN=bin
#连接目标文件夹
OBJFILE=obj
#demo目标文件夹
OBJS_DIR_DEMO=obj_demo
#编译出库的名字和路径
TARGET_MDXZLIB_PATH = $(ROOT_DIR)/lib
#判断输出文件夹是否存在，如果不存在自动填充
ifneq ($(OUTFILE), $(wildcard $(OUTFILE)))
$(shell mkdir -p $(OUTFILE) $(OUTFILE)/$(OUTBIN) $(OUTFILE)/$(OBJFILE))
$(shell echo 'OBJS=*.o\nODIR=obj\n$$(ROOT_DIR)/$$(BIN_DIR)/$$(BIN):$$(ODIR)/$$(OBJS)\n\t$$(CC) -o $$@ $$^ $$(CFLAGS) $$(LDFLAGS)'>$(OUTFILE)/Makefile)
endif
#输出文件夹下Makefile文件是否存在
ifneq ($(OUTFILE)/Makefile, $(wildcard $(OUTFILE)/Makefile))
$(shell echo 'OBJS=*.o\nODIR=obj\n$$(ROOT_DIR)/$$(BIN_DIR)/$$(BIN):$$(ODIR)/$$(OBJS)\n\t$$(CC) -o $$@ $$^ $$(CFLAGS) $$(LDFLAGS)'>$(OUTFILE)/Makefile)
endif
#判断可执行程序文件夹是否存在
ifneq ($(OUTFILE)/$(OUTBIN), $(wildcard $(OUTFILE)/$(OUTBIN)))
$(shell mkdir -p $(OUTFILE)/$(OUTBIN))
endif
#判断连接目标文件夹是否存在
ifneq ($(OUTFILE)/$(OBJFILE), $(wildcard $(OUTFILE)/$(OBJFILE)))
$(shell mkdir -p $(OUTFILE)/$(OBJFILE))
endif
#判断demo目标文件夹是否存在
ifneq ($(OUTFILE)/$(OBJS_DIR_DEMO), $(wildcard $(OUTFILE)/$(OBJS_DIR_DEMO)))
$(shell mkdir -p $(OUTFILE)/$(OBJS_DIR_DEMO))
endif
#判断TARGET_MDXZLIB_PATH目标文件夹是否存在
ifneq ($(TARGET_MDXZLIB_PATH), $(wildcard $(TARGET_MDXZLIB_PATH)))
$(shell mkdir -p $(TARGET_MDXZLIB_PATH))
endif



#设置编译器
#CC= arm-linux-gnueabihf-gcc
CPP=g++
#debug文件夹里的makefile文件需要最后执行，所以这里需要执行的子目录要排除debug文件夹，这里使用awk排除了debug文件夹，读取剩下的文件夹
SUBDIRS=$(shell ls -l | grep ^d | awk '{if($$9 != "debug") print $$9}')
#SUBDIRS删除includes文件夹，因为这个文件中是头文件，不需要make
SUBDIRS:=$(patsubst includes,,$(SUBDIRS))
Other_DIR = include .vscode lib hpp
SUBDIRS := $(filter-out $(Other_DIR),$(SUBDIRS))
#无需下一行的注释代码，因为我们已经知道debug里的makefile是最后执行的，所以最后直接去debug目录下执行指定的makefile文件就行，具体下面有注释
#DEBUG=$(shell ls -l | grep ^d | awk '{if($$9 == "debug") print $$9}')
#记住当前工程的根目录路径
ROOT_DIR=$(shell pwd)
#最终bin文件的名字，可以更改为自己需要的
BIN=libmdxzntripclienttest

#目标文件所在的目录
OBJS_DIR=debug/obj
OBJS_DIR_DEMO=debug/obj_demo
#bin文件所在的目录
BIN_DIR=debug/bin
#编译出库的名字和路径
TARGET_LIB_NAME = libinfiniteBuffer.so

#获取当前目录下的c文件集，放在变量CUR_SOURCE中
CUR_SOURCE=${wildcard *.c}
#将对应的c文件名转为o文件后放在下面的CUR_OBJS变量中
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
#删除
RM=rm -rf
## debug flag
DBG_ENABLE   = 0
## need libs, add at here
LIBS := infiniteBuffer
## used headers  file path
INCLUDE_PATH := $(ROOT_DIR)/include .
#使用的库目录，静态库和动态库添加在这里
LIBRARY_PATH := $(ROOT_DIR)/lib  
#/usr/local/DIAS_Toolchain/sysroots/aarch64-poky-linux/usr/lib/
## debug for debug info, when use gdb to debug
ifeq (1, ${DBG_ENABLE}) 
	CFLAGS += -D_DEBUG -O0 -g -DDEBUG=1 -Wno-unused-result
endif
## get all include path
CFLAGS  += $(foreach dir, $(INCLUDE_PATH), -I$(dir)) 
## get all library path
LDFLAGS += $(foreach lib, $(LIBRARY_PATH), -L$(lib))
## get all librarys
LDFLAGS += $(foreach lib, $(LIBS), -l$(lib))
CFLAGS += 	-g \
			-pthread -pg
#将以下变量导出到子shell中，本次相当于导出到子目录下的makefile中
export CC BIN OBJS_DIR BIN_DIR ROOT_DIR LDFLAGS CFLAGS TARGET_MDXZLIB_PATH TARGET_LIB_NAME OBJS_DIR_DEMO
#注意这里的顺序，需要先执行SUBDIRS最后才能是DEBUG
all:$(clean) $(SUBDIRS) $(CUR_OBJS) DEBUG
#递归执行子目录下的makefile文件，这是递归执行的关键
$(SUBDIRS):ECHO
	make -C $@
DEBUG:ECHO
#直接去debug目录下执行makefile文件
	make -C debug
ECHO:
	@echo $(SUBDIRS)
#将c文件编译为o文件，并放在指定放置目标文件的目录中即OBJS_DIR
$(CUR_OBJS):%.o:%.c
	$(CC) -c $^ -o $(ROOT_DIR)/$(OBJS_DIR_DEMO)/$@ $(CFLAGS)
clean:
	@$(RM) $(BIN_DIR)/*
	@$(RM) $(OBJS_DIR_DEMO)/*
	@$(RM) $(TARGET_MDXZLIB_PATH)/$(TARGET_LIB_NAME)
install:
	sudo cp $(TARGET_MDXZLIB_PATH)/$(TARGET_LIB_NAME) /usr/local/lib
	sudo ldconfig