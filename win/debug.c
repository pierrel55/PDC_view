#include <string.h>
#include <stdlib.h>
#include "debug.h"

#ifdef _DEBUG

// debug util
#ifdef _MSVC
void win_break(void)
{
  __asm int 3;
}
#else
// if no IDE, print console message
#include <stdio.h>
void win_break(void)
{
  printf("ERROR: break occured..\n");
}
#endif

// ----------------------------------------------
// alloc check code
static int allocated_sum = 0;
struct alloc_hdr_t
{
  void *ptr;
  int size;
};

// alloc
void *w_malloc(int size)
{
  struct alloc_hdr_t *hdr;
  W_ASSERT(size > 0);
  hdr = (struct alloc_hdr_t *)malloc(sizeof(struct alloc_hdr_t) + size);
  if (!hdr)
  {
    win_break();
    return NULL;
  }
  hdr->ptr = hdr;
  hdr->size = size;
  allocated_sum += size;
  return hdr + 1;
}

void *w_calloc(int num_obj, int obj_sizeof)
{
  int size = num_obj*obj_sizeof;
  void *ptr = w_malloc(size);
  if (ptr)
    memset(ptr, 0, size);
  return ptr;
}

// free + check
void w_free(void *ptr)
{
  if (ptr)
  {
    struct alloc_hdr_t *hdr = (struct alloc_hdr_t *)ptr - 1;
    W_ASSERT((hdr->ptr == hdr) && (hdr->size > 0));
    allocated_sum -= hdr->size;
    W_ASSERT(allocated_sum >= 0);
    hdr->ptr = 0;
    free(hdr);
    return;
  }
  // null ptr, allowed but should not occur
  W_ASSERT(0);
}

// exit check
void w_check_alloc(void)
{
  W_ASSERT(allocated_sum == 0);
}

#endif
