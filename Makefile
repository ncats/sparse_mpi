
CC = clang
MPICC = mpicc
DEBUG = -g
CFLAGS = -Wall $(DEBUG)
OBJS = sparse.o sparse_mpi.o

TARGETS = sparse_mpi sparse_test

all: $(TARGETS)

.c.o: $(OBJS)
	$(CC) $(CFLAGS) -c $<

sparse_mpi.o: sparse_mpi.c
	$(MPICC) $(CFLAGS) -c $<

sparse_mpi: $(OBJS)
	$(MPICC) $(CFLAGS) -o $@ $(OBJS)

sparse_test: sparse.c
	$(CC) $(CFLAGS) -DSPARSE_MAIN -o $@ $<

clean:
	rm -f $(OBJS) $(TARGETS)
