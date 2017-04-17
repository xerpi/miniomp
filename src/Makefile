TARGET		= libminiomp.so
CC		= gcc
CFLAGS		= -Wall -O -std=c99 -march=native -I. -D_GNU_SOURCE
CFLAGS_SO	= -fPIC --shared
LIB_DIR		= $(MINIOMP)/lib
VPATH		= $(LIB_DIR)

all: $(LIB_DIR) $(TARGET)

libminiomp.so: libminiomp.h libminiomp.c env.c intrinsic.c parallel.c synchronization.c single.c loop.c task.c taskloop.c
	$(CC) $(CFLAGS_SO) $(CFLAGS) $+ -o $(LIB_DIR)/$@ -lpthread

$(LIB_DIR):
	@mkdir -p $@

.PHONY: clean

clean:
	rm -rf $(LIB_DIR)/$(TARGET)
