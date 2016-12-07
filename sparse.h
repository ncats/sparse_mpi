
#ifndef __SPARSE_H__
#define __SPARSE_H__

#include <stdint.h>

struct _sparse_el_s {
  int32_t pos; /* position */
  float value;
};

typedef struct _sparse_vector_s {
  int32_t id;
  int32_t size;
  struct _sparse_el_s *data;
} sparse_vector_t;

typedef struct _sparse_reader_s sparse_reader_t;

extern sparse_reader_t *sparse_reader_create ();
extern const char *sparse_reader_error (const sparse_reader_t *);
extern void sparse_reader_destroy (sparse_reader_t *);
extern int sparse_reader_open (sparse_reader_t *reader, const char *file);
extern sparse_vector_t *sparse_reader_vector (const sparse_reader_t *);
extern int sparse_reader_next (sparse_reader_t *);

extern double sparse_vector_tanimoto (const sparse_vector_t *v1,
                                      const sparse_vector_t *v2,
                                      sparse_vector_t **contrib);

#endif /* !__SPARSE_H__ */
