#ifndef CBUFFER_H
#define CBUFFER_H

#include <assert.h>

typedef struct {
  char* data;
  int size;
} cbuf_t;

/* Creation. */
cbuf_t* cbuf_new(int size);
void cbuf_destroy();

/* Operations. */
char  cbuf_put(cbuf_t* buf, int index, char value);
char  cbuf_get(cbuf_t* buf, int index);
char  cbuf_put_range(cbuf_t* buf, int start, int len);
char* cbuf_get_range(cbuf_t* buf, int start, int len);

#define cbuf_size(c)                ((c)->size)
#define cbuf_mod(c,n)               ((n) % cbuf_size(c))
#define cbuf_add(c,i,n)             cbuf_mod(c,i+n)
#define cbuf_sub(c,i,n)             cbuf_mod(c,i-a)
#define cbuf_inc(c,i)               cbuf_add(c,i,1)
#define cbuf_dec(c,i)               cbuf_sub(c,i,1)
#define cbuf_cmp(c,a,b)             ((cbuf_mod(c,a) == cbuf_mod(c,b)) ? 0 : \
                                      (((cbuf_mod(c,a) < cbuf_mod(c,b)) && ((cbuf_mod(c,b))-(cbuf_mod(c,a)) < cbuf_size(c)/2)) \
                                        || ((cbuf_mod(c,a) > cbuf_mod(c,b)) && ((cbuf_mod(c,a))-(cbuf_mod(c,b)) > cbuf_size(c)/2))) ? -1 : 1)
#define cbuf_lt(c,a,b)              (cbuf_cmp(c,a,b) == -1)                                    
#define cbuf_gt(c,a,b)              (cbuf_cmp(c,a,b) ==  1) /* All comparisons cbuf_@(c,a,b) are a @ b. */                                    
#define cbuf_eq(c,a,b)              (cbuf_cmp(c,a,b) ==  0)                                    
#define cbuf_lte(c,a,b)             (cbuf_lt(c,a,b) || cbuf_eq(c,a,b))
#define cbuf_gte(c,a,b)             (cbuf_gt(c,a,b) || cbuf_eq(c,a,b))
#define cbuf_incl_contains(c,a,b,n) (cbuf_lte(c,a,n) && cbuf_gte(c,b,n))
#define cbuf_excl_contains(c,a,b,n) (cbuf_lt(c,a,n) && cbuf_gt(c,b,n))
#define cbuf_btm_contains(c,a,b,n)  (cbuf_lte(c,a,n) && cbuf_gt(c,b,n))
#define cbuf_top_contains(c,a,b,n)  (cbuf_lt(c,a,n) && cbuf_gte(c,b,n))

#endif
