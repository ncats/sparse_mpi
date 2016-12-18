
CC = clang
MPICC = mpicc
DEBUG = -g
LMDB = /usr/local/lmdb/0.9.16
CFLAGS = -Wall $(DEBUG) 
LIBS = -L$(LMDB)/lib -llmdb
OBJS = sparse.o 

TARGETS = sparse_mpi sparse_mpi_mdb sparse_test

all: $(TARGETS)

.c.o: $(OBJS)
	$(CC) $(CFLAGS) -c $<

sparse_mpi.o: sparse_mpi.c
	$(MPICC) $(CFLAGS) -c $<

sparse_mpi_mdb.o: sparse_mpi.c
	$(MPICC) $(CFLAGS) -DHAVE_LMDB -I$(LMDB)/include -o $@ -c $<

sparse_mpi: $(OBJS) sparse_mpi.o
	$(MPICC) $(CFLAGS) -o $@ $(OBJS) sparse_mpi.o

sparse_mpi_mdb: $(OBJS) sparse_mpi_mdb.o
	$(MPICC) $(CFLAGS) -o $@ $(OBJS) sparse_mpi_mdb.o $(LIBS)

sparse_test: sparse.c
	$(CC) $(CFLAGS) -DSPARSE_MAIN -o $@ $<

clean:
	rm -f *.o $(TARGETS)
