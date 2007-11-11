#ifndef CBUFFER_H
#define CBUFFER_H

#include <assert.h>

typedef struct {
  char* data;
  int size;
} cbuf_t;

/* Creation. */
cbuf_t* cbuf_new(int size);
void cbuf_destroy(cbuf_t* buf);

/* Operations. */
char  cbuf_put(cbuf_t* buf, int index, char value);
char  cbuf_get(cbuf_t* buf, int index);
int   cbuf_put_range(cbuf_t* buf, char* buffer, int start, int len);
char* cbuf_get_range(cbuf_t* buf, int start, int len);
void  cbuf_print(cbuf_t* buf);

#define cbuf_size(c)                          ((c)->size)
#define cbuf_mod(c,n)                         ((n) % cbuf_size(c))
#define cbuf_add(c,i,n)                       cbuf_mod(c,i+n)
#define cbuf_sub(c,i,n)                       cbuf_mod(c,i-a)
#define cbuf_inc(c,i)                         ((i)=cbuf_add(c,i,1))
#define cbuf_dec(c,i)                         ((i)=cbuf_sub(c,i,1))
#define cbuf_cmp(c,a,b)                       ((cbuf_mod(c,a) == cbuf_mod(c,b)) ? 0 : \
                                                (((cbuf_mod(c,a) < cbuf_mod(c,b)) && ((cbuf_mod(c,b))-(cbuf_mod(c,a)) < cbuf_size(c)/2)) \
                                                   || ((cbuf_mod(c,a) > cbuf_mod(c,b)) && ((cbuf_mod(c,a))-(cbuf_mod(c,b)) > cbuf_size(c)/2))) ? -1 : 1)
#define cbuf_lt(c,a,b)                        (cbuf_cmp(c,a,b) == -1)                                    
#define cbuf_gt(c,a,b)                        (cbuf_cmp(c,a,b) ==  1) /* All comparisons cbuf_@(c,a,b) are a @ b. */                                    
#define cbuf_eq(c,a,b)                        (cbuf_cmp(c,a,b) ==  0)                                    
#define cbuf_lte(c,a,b)                       (cbuf_lt(c,a,b) || cbuf_eq(c,a,b))
#define cbuf_gte(c,a,b)                       (cbuf_gt(c,a,b) || cbuf_eq(c,a,b))
#define cbuf_incl_contains(c,a,b,n)           (cbuf_lte(c,a,n) && cbuf_gte(c,b,n))
#define cbuf_excl_contains(c,a,b,n)           (cbuf_lt(c,a,n) && cbuf_gt(c,b,n))
#define cbuf_btm_contains(c,a,b,n)            (cbuf_lte(c,a,n) && cbuf_gt(c,b,n))
#define cbuf_top_contains(c,a,b,n)            (cbuf_lt(c,a,n) && cbuf_gte(c,b,n))

#define cbuf_loop(c,start,len,i,v)            do { int __cbuf_c = 0; \
                                                for(i=cbuf_mod(c,start),v=cbuf_get(c,i); __cbuf_c < len; cbuf_inc(c,i),v=cbuf_get(c,i), __cbuf_c++)
#define cbuf_loop_end()                       } while(0)
#define cbuf_iterate_indices(c,start,len,var) do { char __cbuf_v; cbuf_loop(c,start,len,var,__cbuf_v)
#define cbuf_iterate_indices_end()            cbuf_loop_end(); } while(0)
#define cbuf_iterate_values(c,start,len,var)  do { int __cbuf_i; cbuf_loop(c,start,len,__cbuf_i,var)
#define cbuf_iterate_values_end()             cbuf_loop_end(); } while(0)

#endif
