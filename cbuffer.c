#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "cbuffer.h"

cbuf_t* cbuf_new(int size) {
  cbuf_t* buf = malloc(sizeof(cbuf_t));
  assert(buf);
  assert(0 < size);

  buf->data = malloc(size);
  assert(buf->data);

  buf->size = size;

  return buf;
}

void cbuf_destroy(cbuf_t* buf) {
  free(buf->data);
  free(buf);
}

char cbuf_put(cbuf_t* buf, int index, char value) {
  char retval;

  assert(0 <= index);

  retval = buf->data[cbuf_mod(buf, index)];
  buf->data[cbuf_mod(buf, index)] = value;
  
  return retval;
}

char cbuf_get(cbuf_t* buf, int index) {
  assert(0 <= index);
  return buf->data[cbuf_mod(buf, index)];
}

int cbuf_put_range(cbuf_t* buf, char* buffer, int start, int len) {
  int i, cnt = 0;

  assert(0 <= start);
  assert(0 <= len);
  assert(len <= cbuf_size(buf));

  cbuf_iterate_indices(buf, start, len, i) {
    cbuf_put(buf, i, buffer[cnt++]);
  } cbuf_iterate_indices_end();

  return 0;
}

char* cbuf_get_range(cbuf_t* buf, int start, int len) {
  char* buffer;
  char v;
  int cnt = 0;

  assert(0 <= start);
  assert(0 <= len);
  assert(len <= cbuf_size(buf));

  buffer = malloc(len);
  assert(buffer);

  cbuf_iterate_values(buf, start, len, v) {
    buffer[cnt++] = v;
  } cbuf_iterate_values_end();

  return buffer;
}

void cbuf_print(cbuf_t* buf) {
  char c;
  printf("[...] ");
  cbuf_iterate_values(buf, 0, cbuf_size(buf), c) {
    printf("%c ", c);
  } cbuf_iterate_values_end();
  printf("[...]\n");
}
