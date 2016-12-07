Sparse MPI
==========

This repository contains a simple code that makes use of the (open-)
MPI library to perform all pairwise calculation of [Tanimoto
similarity](https://en.wikipedia.org/wiki/Jaccard_index) 
of sparse numerical vectors. To compile this code, you first need to
have the [openmpi](https://www.open-mpi.org/) library installed. To
build, simply type

```
make
```

This should build standalone executables ```sparse_mpi``` and
```sparse_test```.  The directory ```data``` contains an example input
sparse format that can be used for testing, e.g.,

```
mpirun sparse_mpi data/descriptors.txt
```

To run ```sparse_mpi``` in an HPC environment with
[Slurm](https://slurm.schedmd.com/), try something like the following:

```
salloc -N 10 --sockets-per-node=2 --cores-per-socket=12 --threads-per-core=1 mpirun sparse_mpi data/descriptors.txt
```

Note this command assumes you have at least 24 cores per node.