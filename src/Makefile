TARGET		= libminiomp.so
OBJS		= libminiomp.o env.o intrinsic.o parallel.o synchronization.o single.o loop.o task.o tasksync.o taskloop.o list.o
DEPS		= $(OBJS:.o=.d)
CC		= gcc
CFLAGS		= -fPIC -Wall -O -std=c99 -march=native -I. -D_GNU_SOURCE
CFLAGS_SO	= --shared
LIB_DIR		= $(MINIOMP)/lib
VPATH		= $(LIB_DIR)

all: $(LIB_DIR) $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS_SO) $(CFLAGS) $^ -o $(LIB_DIR)/$@ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(LIB_DIR):
	@mkdir -p $@

.PHONY: clean

clean:
	rm -rf $(LIB_DIR)/$(TARGET) $(OBJS) $(DEPS)

-include $(DEPS)
