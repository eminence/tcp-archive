#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cbuffer.h"
#include "tcppacket.h"
#include "fancy_display.h"


cbuf_t* cbuf_new(int size) {
  cbuf_t* buf = malloc(sizeof(cbuf_t));
  assert(buf);
  assert(0 < size);

  buf->data = malloc(size*2);
  assert(buf->data);

  buf->size = size;

  return buf;
}

void cbuf_destroy(cbuf_t* buf) {
  free(buf->data);
  free(buf);
}

void __cbuf_put(cbuf_t* buf, int index, char value, uint8_t type) {
  assert(0 <= index);
  
  //assert(type & (CBUF_DATA | CBUF_FLAG));

  buf->data[__cbuf_type(cbuf_mod(buf, index))] = type;
  buf->data[__cbuf_data(cbuf_mod(buf, index))] = value;

  nlog(MSG_LOG, "cbuf_put", "set index %d to value %p with type %c", index, value, type == CBUF_DATA ? 'D' : 'F');
}

char __cbuf_get(cbuf_t* buf, int index, uint8_t* type) {
  char data;

  assert(0 <= index);
  assert(type);
  
  /* Obtain type. */
  *type = (uint8_t)buf->data[__cbuf_type(cbuf_mod(buf, index))];
  data = buf->data[__cbuf_data(cbuf_mod(buf, index))];

  assert(type);
  //assert(*type & (CBUF_DATA | CBUF_FLAG));
  
  nlog(MSG_LOG, "cbuf_get", "data at index %d has value %p and type %c", index, data, *type == CBUF_DATA ? 'D' : 'F');

  return data;
}

int cbuf_put_range(cbuf_t* buf, char* buffer, int start, int len) {
  int cnt = 0, i;

  assert(0 <= start);
  assert(0 <= len);
  assert(len <= cbuf_size(buf));

  cbuf_r_iterate_indices(buf, start, len, i) {
    __cbuf_put(buf, i, buffer[cnt++], CBUF_DATA);
  } cbuf_r_iterate_indices_end();

  return 0;
}

int cbuf_put_flag(cbuf_t* buf, int start, uint8_t flags) {
   __cbuf_put(buf, start, (char)flags, CBUF_FLAG);

   return 0;
}

int cbuf_get_range(cbuf_t* buf, int start, int len, void** vdata) {
  char** data = (char**)vdata;
  char v;
  int cnt = 0;
  uint8_t t;

  assert(0 <= start);
  assert(0 <= len);
  assert(len <= cbuf_size(buf));

  /* safe bet: never read more than len bytes */
  *data = malloc(len);
  assert(data);
  
  cbuf_iterate_values(buf, start, len, v, t) {
    (*data)[cnt++] = v;
  } cbuf_iterate_values_end();
  
  *data = realloc(*data, cnt);
  
  assert(*data);

  return t == CBUF_DATA ? cnt : -1;
}

void cbuf_print(cbuf_t* buf) {
  uint8_t c;
  int i=1;
  int t;

  printf("[...] ");
  cbuf_r_iterate_values(buf, 0, cbuf_size(buf), c) {
    printf("%d ", c);
  } cbuf_r_iterate_values_end();
  printf("[...]\n");
}
