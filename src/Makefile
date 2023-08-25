#子目录的Makefile直接读取其子目录就行
SUBDIRS=$(shell ls -l | grep ^d | awk '{print $$9}')
#需要忽略的目录
Other_DIR = inc .vscode 
SUBDIRS := $(filter-out $(Other_DIR),$(SUBDIRS))
#以下同根目录下的makefile的相同代码的解释
CUR_SOURCE=${wildcard *.c}
CUR_OBJS=${patsubst %.c, %.o, $(CUR_SOURCE)}
all:$(SUBDIRS) $(CUR_OBJS)
$(SUBDIRS):ECHO
	make -C $@
$(CUR_OBJS):%.o:%.c
	$(CC) -shared -fPIC  $(CUR_SOURCE) -o $(TARGET_MDXZLIB_PATH)/$(TARGET_LIB_NAME) $(CFLAGS) 

ECHO:
	@echo $(SUBDIRS)