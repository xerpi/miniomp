CC = gcc

CFLAGS   = -Wall -O -std=c99 -march=native -I. -D_GNU_SOURCE
CFLAGS_SO = -fPIC --shared

TARGETS  = libminiomp.so
all: $(TARGETS) 

libminiomp.so: libminiomp.h libminiomp.c env.c intrinsic.c parallel.c synchronization.c single.c loop.c task.c taskloop.c
	$(CC) $(CFLAGS_SO) $(CFLAGS) $+ -o $(MINIOMP)/lib/$@ -lpthread

clean:
	rm -rf $(MINIOMP)/lib/$(TARGETS)
