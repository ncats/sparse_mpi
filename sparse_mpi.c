
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>

#include "sparse.h"

struct node_s {
  sparse_vector_t *v;
  struct node_s *next;
};

int
main (int argc, char *argv[])
{
  int err, rank, numtasks, len;
  long total = 0l;
  sparse_reader_t *reader;
  struct node_s *head, *node, *next;
  char hostname[MPI_MAX_PROCESSOR_NAME]= {0};
  char outfile[BUFSIZ];
  FILE *outfp;
  
  MPI_Init (&argc, &argv);
  
  MPI_Comm_rank (MPI_COMM_WORLD, &rank);
  MPI_Comm_size (MPI_COMM_WORLD, &numtasks);
  MPI_Get_processor_name (hostname, &len);
  
  if (argc == 1)
    {
      fprintf (stderr, "Error: No sparse input file specified!\n");
      return 1;
    }
      
  reader = sparse_reader_create ();
  err = sparse_reader_open (reader, argv[1]);
  if (err)
    {
      fprintf (stderr, "Error: %s\n", sparse_reader_error (reader));
      sparse_reader_destroy (reader);
      return 1;
    }

  (void) sprintf (outfile, "sparse_mpi-%s-%d.txt", hostname, rank);
  outfp = fopen (outfile, "w");
  assert (outfp != 0);

  head = 0;
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

              fprintf (outfp, "%d %d %lf", next->v->id, n->v->id, sim);
              for (len = 0; len < contrib->size; ++len)
                fprintf (outfp, " %d:%f", contrib->data[len].pos,
                         contrib->data[len].value);
              fputc ('\n', outfp);
              //printf ("%d: (%d,%d)\n", rank, next->v->id, n->v->id);
              
              ++total;
            }
        }
      
      if (head == 0)
        head = n;
      else
        node->next = n;
      node = n;
    }
  printf ("** %s: rank=%d total=%ld\n", outfile, rank, total);
  fclose (outfp);

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
  
  return 0;
}
