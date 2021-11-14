// -------------------------------------------------
// debug macro. assert and memory allocations checks

#ifdef _DEBUG
void win_break(void);                            // used in W_ASSERT for bug detection

void *w_malloc(int size);                        // alloc with check
void *w_calloc(int num_obj, int obj_sizeof);
void w_free(void *ptr);
void w_check_alloc(void);

#define W_ASSERT(c) ((c) ? (void)0 : win_break())
#define W_MALLOC w_malloc
#define W_CALLOC w_calloc
#define W_FREE w_free
#else
#define W_ASSERT(c)
#define W_MALLOC malloc
#define W_CALLOC calloc
#define W_FREE free
#endif
