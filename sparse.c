/*
 * parsing sparse vector format
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "sparse.h"

struct _sparse_reader_s {
  FILE *fp;
  int32_t count;
  int32_t maxbuf;
  char *buf;
  int32_t maxvec;
  sparse_vector_t vec;
  char file[BUFSIZ];
  char err[BUFSIZ];
};

sparse_reader_t *
sparse_reader_create ()
{
  sparse_reader_t *reader = malloc (sizeof (struct _sparse_reader_s));
  assert (reader != 0);
  (void) memset (reader, 0, sizeof (*reader));
  return reader;
}

const char *
sparse_reader_error (const sparse_reader_t *reader)
{
  return reader->err;
}

void
sparse_reader_destroy (sparse_reader_t *reader)
{
  if (reader != 0)
    {
      if (reader->buf != 0)
        free (reader->buf);
      if (reader->fp != 0)
        fclose (reader->fp);
      if (reader->vec.data != 0)
        free (reader->vec.data);
      free (reader);
    }
}

int
sparse_reader_open (sparse_reader_t *reader, const char *file)
{
  if (reader->fp != 0)
    fclose (reader->fp);
  reader->err[0] = '\0';

  reader->count = 0;
  reader->fp = fopen (file, "r");
  if (reader->fp != 0)
    {
      (void) strcpy (reader->file, file);
      reader->count = 0;
    }
  else
    {
      reader->count = -1;
      (void) sprintf (reader->err,
                      "Unable to open file '%s' for reading!", file);
    }
  
  return reader->count;
}

sparse_vector_t *
sparse_reader_vector (const sparse_reader_t *reader)
{
  sparse_vector_t *sv = malloc
    (sizeof (sparse_vector_t)+reader->vec.size*sizeof (struct _sparse_el_s));
  assert (sv != 0);
  sv->data = (struct _sparse_el_s*)
    (((unsigned char*)sv)+ sizeof (sparse_vector_t));
  sv->id = reader->vec.id;
  sv->size = reader->vec.size;
  (void) memcpy (sv->data, reader->vec.data, sv->size*sizeof (*sv->data));
  return sv;
}

int
sparse_reader_next (sparse_reader_t *reader)
{
  int ok = 0;
  
  if (reader->fp != 0)
    {
      if (!feof (reader->fp))
        {
          long pos = ftell (reader->fp);
          if (pos < 0)
            {
              ok = -1;
              (void) sprintf (reader->err,
                              "bad input stream; ftell returns %ld", pos);
            }
          else
            {
              int32_t len = 0, ch;
              do
                {
                  ch = fgetc (reader->fp);
                  if (ch != EOF && ch != '\n')
                    {
                      if (len+1 >= reader->maxbuf)
                        {
                          reader->maxbuf = BUFSIZ+len;
                          reader->buf = realloc (reader->buf, reader->maxbuf);
                          assert (reader->buf != 0);
                        }
                      reader->buf[len++] = ch;
                    }
                  else
                    break;
                }
              while (1);
              
              reader->buf[len] = '\0';
              if (len > 0)
                {
                  char *ptr = 0, *tok;
                  int32_t size = 0, max, i;

                  reader->vec.id = strtol (reader->buf, &ptr, 10);
                  reader->vec.size = size = strtol (ptr, &ptr, 10); /* size */
                  if (size >= reader->maxvec)
                    {
                      reader->maxvec = size + 100;
                      reader->vec.data = realloc
                        (reader->vec.data, sizeof (*reader->vec.data)
                         * reader->maxvec);
                      assert (reader->vec.data != 0);
                    }
                  max = strtol (ptr, &ptr, 10); /* max index */
                  
                  while (*ptr == ' ')
                    ++ptr;
                  
                  for (i = 0; ((tok = strsep (&ptr, " ")) != 0); ++i)
                    {
                      sscanf (tok, "%d:%f", &reader->vec.data[i].pos,
                              &reader->vec.data[i].value);
                      /*
                      printf ("%d: %d %f\n", reader->vec.id,
                              reader->vec.data[i].pos,
                              reader->vec.data[i].value);
                      */
                    }
                  ok = ++reader->count;
                }
            }
        }
    }
  else
    {
      ok = -1;
      (void) strcpy (reader->err, "reader is not opened!");
    }
  
  return ok;
}

static int
compare_sparse_el_value_desc (const void *v1,
                              const void *v2)
{
  const struct _sparse_el_s *e1 = (const struct _sparse_el_s *)v1;
  const struct _sparse_el_s *e2 = (const struct _sparse_el_s *)v2;
  
  if (e2->value < e1->value) return -1;
  if (e2->value > e1->value) return 1;
  return e1->pos - e2->pos;
}

double
sparse_vector_tanimoto (const sparse_vector_t *v1,
                        const sparse_vector_t *v2,
                        sparse_vector_t **contrib)
{
  double a = 0., b =0., c = 0.;
  int i = 0, j = 0;

  if (contrib != 0)
    {
      int32_t size = v1->size < v2->size ? v1->size : v2->size;
      *contrib = malloc (sizeof (sparse_vector_t)
                         + size * sizeof (struct _sparse_el_s));
      (*contrib)->id = -1;
      (*contrib)->size = 0;
      (*contrib)->data = (struct _sparse_el_s *)
        ((unsigned char *)*contrib + sizeof (sparse_vector_t));
    }

  /* find overlapping elements */
  do
    {
      if (i < v1->size && j < v2->size)
        {
          if (v1->data[i].pos < v2->data[j].pos)
            {
              a += v1->data[i].value * v1->data[i].value;
              ++i;
            }
          else if (v2->data[j].pos < v1->data[i].pos)
            {
              b += v2->data[j].value * v2->data[j].value;
              ++j;
            }
          else /* if (v1->data[i].pos == v2->data[j].pos) */
            {
              double ab = v1->data[i].value * v2->data[j].value;
              if (contrib != 0)
                {
                  (*contrib)->data[(*contrib)->size].value = ab;
                  (*contrib)->data[(*contrib)->size].pos = v1->data[i].pos;
                  (*contrib)->size++;
                }
              c += ab;
              a += v1->data[i].value * v1->data[i].value;
              b += v2->data[j].value * v2->data[j].value;
              ++i;
              ++j;
            }
        }
      else if (i < v1->size)
        {
          a += v1->data[i].value * v1->data[i].value;
          ++i;
        }
      else if (j < v2->size)
        {
          b += v2->data[j].value * v2->data[j].value;
          ++j;
        }
    }
  while (i < v1->size || j < v2->size);

  /* now normalize contribution values */
  if (contrib != 0)
    {
      int32_t i;
      for (i = 0; i < (*contrib)->size; ++i)
        (*contrib)->data[i].value /= (a+b-c);
      
      /* sort by descreasing contribution */
      qsort ((*contrib)->data, (*contrib)->size, sizeof (struct _sparse_el_s),
             compare_sparse_el_value_desc);
    }
  
  return c / (a+b-c);
}

#ifdef SPARSE_MAIN

struct node_s {
  sparse_vector_t *v;
  struct node_s *next;
};

int
main (int argc, char *argv[])
{
  int n, err;
  sparse_reader_t *reader;
  
  if (argc == 1)
    {
      fprintf (stderr, "Usage: %s FILES...\n", argv[0]);
      exit (1);
    }

  reader = sparse_reader_create ();
  for (n = 1; n < argc; ++n)
    {
      err = sparse_reader_open (reader, argv[n]);
      if (err)
        fprintf (stderr, "error: %s\n", sparse_reader_error (reader));
      else
        {
          struct node_s *head = 0, *node;

          while (sparse_reader_next (reader) > 0)
            {
              struct node_s *n = malloc (sizeof (*node));
              n->v = sparse_reader_vector (reader);
              n->next = 0;
              
              printf ("reading %d...%d\n", n->v->id, n->v->size);
              { struct node_s *next;
                sparse_vector_t *contrib;
                int i;
                for (next = head; next != 0; next = next->next)
                  {
                    double d = sparse_vector_tanimoto (next->v, n->v, &contrib);
                    printf ("  (%d,%d) = %f\n", next->v->id, n->v->id, d);
                    printf ("     ");
                    for (i = 0; i < contrib->size; ++i)
                      printf (" %d:%.3e", contrib->data[i].pos,
                              contrib->data[i].value);
                    printf ("\n");
                    free (contrib);
                  }
              }

              if (head == 0)
                head = n;
              else
                node->next = n;
              node = n;
            }

          while (head != 0)
            {
              node = head->next;
              free (head->v);
              free (head);
              head = node;
            }
        }
    }
  sparse_reader_destroy (reader);
  
  return 0;
}
#endif
