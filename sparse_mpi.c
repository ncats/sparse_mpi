
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <mpi.h>
#include <assert.h>

#ifdef HAVE_LMDB
#  include <lmdb.h>
#endif

#include "sparse.h"

struct node_s {
  sparse_vector_t *v;
  struct node_s *next;
};

int
main (int argc, char *argv[])
{
  int err = 1, rank, numtasks, len;
  long total = 0l;
  sparse_reader_t *reader;
  struct node_s *head = 0, *node, *next;
  char hostname[MPI_MAX_PROCESSOR_NAME]= {0};

#ifdef HAVE_LMDB
  MDB_env *env = 0;
  MDB_txn *txn = 0;
  MDB_dbi dbi = 0;
#else  
  char outfile[BUFSIZ];
  FILE *outfp;
#endif  
  
  MPI_Init (&argc, &argv);
  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &numtasks);
  MPI_Get_processor_name (hostname, &len);

  reader = sparse_reader_create ();
  assert (reader != 0);
  
#ifdef HAVE_LMDB  
  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s DBDIR FILE\n", argv[0]);
      goto done;
    }
  
  err = mdb_env_create (&env);
  if (err)
    {
      fprintf (stderr, "Error: can't create db env: %s\n", mdb_strerror (err));
      goto done;
    }

  err = mdb_env_open (env, argv[1], MDB_FIXEDMAP, 0664);
  if (err)
    {
      fprintf (stderr, "Error: can't open db path: %s (%s)\n",
               argv[1], mdb_strerror (err));
      goto done;
    }

  err = mdb_txn_begin (env, 0, 0, &txn);
  if (err)
    {
      fprintf (stderr, "Rank=%d: Error: can't start transaction (%s)\n",
	       rank, mdb_strerror (err));
      goto done;
    }

  err = mdb_dbi_open (txn, 0, MDB_CREATE | MDB_INTEGERKEY, &dbi);
  if (err)
    {
      fprintf (stderr, "Rank=%d: Error: can't open database (%s)\n", 
	       rank, mdb_strerror (err));
      goto done;
    }
  
  err = sparse_reader_open (reader, argv[2]);
#else
  if (argc == 1)
    {
      fprintf (stderr, "Usage: %s FILE\n", argv[0]);
      goto done;
    }

  (void) sprintf (outfile, "sparse_mpi-%s-%d.txt", hostname, rank);
  outfp = fopen (outfile, "w");
  assert (outfp != 0);
  
  err = sparse_reader_open (reader, argv[1]);  
#endif /* !HAVE_LMDB */
  
  if (err)
    {
      fprintf (stderr, "Error: %s\n", sparse_reader_error (reader));
      goto done;
    }

  while (sparse_reader_next (reader) > 0)
    {
      struct node_s *n = malloc (sizeof (*node));
      n->v = sparse_reader_vector (reader);
      n->next = 0;

      /* now do pairwise */
      for (next = head; next != 0; next = next->next)
        {
          if ((next->v->id + n->v->id) % numtasks == rank)
            {
              sparse_vector_t *contrib;
              double sim = sparse_vector_tanimoto (next->v, n->v, &contrib);
#ifdef HAVE_LMDB
	      uint64_t id;
	      MDB_val key, val;

	      if (next->v->id < n->v->id)
		{
		  id  = next->v->id;
		  id = (id << 32l) | n->v->id;
		}
	      else
		{
		  id = n->v->id;
		  id = (id << 32l) | next->v->id;
		}

	      if (total % 1000 == 0)
		{
		  err = mdb_txn_commit (txn);
		  if (err)
		    fprintf (stderr, "Rank=%d: Error: commiting transaction at record %ld (%s)\n",
			     rank, total, mdb_strerror (err));

		  err = mdb_txn_begin (env, 0, 0, &txn);
		  if (err)
		    fprintf (stderr, "Rank=%d: Error: can't start transaction at record %ld (%s)\n",
			     rank, total, mdb_strerror (err));
		}

	      key.mv_size = sizeof (id);
	      key.mv_data = &id;
	      val.mv_size = sizeof (sim);
	      val.mv_data = &sim;
	      err = mdb_put (txn, dbi, &key, &val, 0);
	      if (err)
		{
		  fprintf (stderr, "Rank=%d: Error: put at record %ld (%s)\n", 
			   rank, total, mdb_strerror (err));
		}
#else
              fprintf (outfp, "%d %d %lf", next->v->id, n->v->id, sim);
              for (len = 0; len < contrib->size; ++len)
                fprintf (outfp, " %d:%f", contrib->data[len].pos,
                         contrib->data[len].value);
              fputc ('\n', outfp);
              //printf ("%d: (%d,%d)\n", rank, next->v->id, n->v->id);
#endif /* !HAVE_LMDB */
              free (contrib);
              ++total;
            }
        }
      
      if (head == 0)
        head = n;
      else
        node->next = n;
      node = n;
    }

#ifdef HAVE_LMDB
  err = mdb_txn_commit (txn);
  if (err)
    fprintf (stderr, "Rank=%d: Error: commiting transaction at record %ld (%s)\n",
	     rank, total, mdb_strerror (err));
#endif

 done:
#ifdef HAVE_LMDB
  printf ("** rank=%d total=%ld\n", rank, total);  
  mdb_env_close (env);
#else
  printf ("** %s: rank=%d total=%ld\n", outfile, rank, total);  
  fclose (outfp);
#endif

  /* clean up */
  while (head != 0)
    {
      next = head->next;
      free (head->v);
      free (head);
      head = next;
    }

  sparse_reader_destroy (reader);
  
  MPI_Finalize();
  
  return err;
}
