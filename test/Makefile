.SUFFIXES: -omp -gomp -domp -dgomp -asm -dasm
CC = gcc

OPENMP	 = -fopenmp
CFLAGS   = -Wall -O -std=c99 -march=native -I. -g3
CLIBS    = -L$(MINIOMP)/lib -lminiomp

.c-omp:
	$(CC) $(CFLAGS) $(OPENMP) $< -o $@ $(CLIBS) -lm

.c-gomp:
	$(CC) $(CFLAGS) $(OPENMP) $< -o $@ -lm

.c-asm:
	$(CC) $(CFLAGS) $(OPENMP) -c -S $< -o $@ $(CLIBS) -lm

.c-domp:
	$(CC) -D_DISPLAY_ $(CFLAGS) $(OPENMP) $< -o $@ $(CLIBS) -lX11 -L/usr/X11R6/lib -lm

.c-dgomp:
	$(CC) -D_DISPLAY_ $(CFLAGS) $(OPENMP) $< -o $@ -lX11 -L/usr/X11R6/lib -lm

.c-dasm:
	$(CC) -D_DISPLAY_ $(CFLAGS) $(OPENMP) -c -S $< -o $@ $(CLIBS) -lm

clean:
	rm -rf *-omp *-gomp *-asm *-domp *-dgomp

ultraclean:
	rm -rf *-omp *-gomp *-asm *-domp *-dgomp *.prv *.pcf *.row TRACE.mpits set-0 TRACE.sym
