ifndef DSLINK_SDK_DIR
$(error DSLINK_SDK_DIR must be defined.)
endif

#
# you should define TARGETS in your Makefile so that useful target
# in this file works properly.
#
TARGETS=pub

#     TEST_TARGETS
#     DIRS
#     DEFS
#         this environment variable will be passed to the Makefile in DIRS.

PUB_OBJS= pub.o pub_factory.o

#     RMFILES
#
# you must include this Makefile.common immediately after definition
# of above variables.
#
# Your Makefile shoud have the following line if you want
# to define the OS specific parameters.
#
#     ifeq ($(OS),Linux)
#     LDLIBS+= -lpthread
#     endif
#
CC = clang
CPPFLAGS += -I${DSLINK_SDK_DIR}/sdk/include
CPPFLAGS += -I${DSLINK_SDK_DIR}/deps/libuv/include
CPPFLAGS += -I${DSLINK_SDK_DIR}/deps/jansson/include
CPPFLAGS += -I${DSLINK_SDK_DIR}/deps/mbed/include
LDFLAGS += -L${DSLINK_SDK_DIR}/build
LDFLAGS += -fsanitize=address
LDLIBS += -llibuv -ljansson -lsdk_dslink_c

.PHONY: clean _dirs_

ifndef OS
OS = $(shell uname -s)
endif
ifndef ARCH
ARCH = $(shell arch)
endif
 
CPPFLAGS += -I.
CPPFLAGS += -I/usr/local/include

CFLAGS += -Werror -Wall
CFLAGS += -g -O2

LDFLAGS	+= -L/usr/local/lib

all: _dirs_ $(TARGETS)

_dirs_:
ifdef	DIRS
	for d in $(DIRS); do if test -d $$d ; then (cd $$d ; $(DEFS) make); fi ; done
endif

pub: $(PUB_OBJS)

#
#.SUFFIXES: .o .c
#
#.c.o:
#	$(CC) $(CFLAGS) -c $<

clean:
	-rm -rf a.out *.dSYM *.o
ifneq ($(RMFILES),)
	-rm -rf $(RMFILES)
endif
ifdef	DIRS
	for d in $(DIRS) ; do if test -d $$d ; then (cd $$d ; make clean); fi ; done
endif
ifneq ($(TARGETS),)
	-rm -f $(TARGETS)
endif
ifneq ($(TEST_TARGETS),)
	-rm -f $(TEST_TARGETS)
endif

distclean: clean
	-rm -f config.cache config.status config.log .depend
	if test -f Makefile.ini ; then rm -f Makefile ; fi

show-options:
	@echo CPPFLAGS =$(CPPFLAGS)
	@echo   CFLAGS =$(CFLAGS)
	@echo  LDFLAGS =$(LDFLAGS)
	@echo   LDLIBS =$(LDLIBS)

