#include "minunit.h"

#include "filechunk.h"

char *test_file_chunk_create() {

  FileChunk *chunk = file_chunk_create();
  mu_assert(chunk != NULL, "Could not create file chunk");

  file_chunk_destroy(chunk);
  return NULL;
}

char *test_file_chunk_extend() {

  FileChunk *chunk = file_chunk_create();
  mu_assert(chunk != NULL, "Could not create file chunk");

  int addlength = 1000;

  unsigned char *addition1 = calloc(addlength, sizeof(unsigned char));
  FileChunk *out1 = file_chunk_extend(chunk, addition1, addlength);
  mu_assert(out1 != NULL, "Invalid response returned from extend");
  mu_assert(out1->length == addlength, "Did not extend length");
  mu_assert(chunk->length == addlength, "Did not extend length");
  free(addition1);

  unsigned char *addition2 = calloc(addlength, sizeof(unsigned char));
  FileChunk *out2 = file_chunk_extend(chunk, addition2, addlength);
  mu_assert(out2 != NULL, "Invalid response returned from extend");
  mu_assert(out2->length == addlength * 2, "Did not extend length");
  mu_assert(chunk->length == addlength * 2, "Did not extend length");
  free(addition2);

  file_chunk_destroy(chunk);

  return NULL;
}


char *all_tests() {
  mu_suite_start();

  mu_run_test(test_file_chunk_create);
  mu_run_test(test_file_chunk_extend);

  return NULL;
}

RUN_TESTS(all_tests);
