#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "cbuffer.h"

int main() {
  int i;
  char c;
  int cnt;
  char* str;
  uint8_t t;

  cbuf_t* buffer = cbuf_new(10);

  for(i=0; i < 10; i++) {
	 __cbuf_put(buffer, i, 'a'+i, CBUF_DATA);
  }

  cbuf_put_flag(buffer, 5, 0x02);

  cnt=0;
  cbuf_iterate_indices(buffer, 0, 104, i, t) {
	 cnt++;
	 printf("%d ", i);
  } cbuf_iterate_indices_end();
  printf("Iterated %d times.\n", cnt);

  printf("\n\n\n");

  cnt=0;
  cbuf_iterate_values(buffer, 0, 107, c, t) {
	 cnt++;
	 printf("%c ", c);
  } cbuf_iterate_values_end();

  printf("Iterated %d times.\n", cnt);

  cbuf_print(buffer);

  printf("\n\n\n");
  cnt = cbuf_get_range(buffer, 5, 10, (void**)&str);

  if(0 > cnt) {
	 printf("[FLG] %p\n", *(int*)str);
  } else {
	 printf("[STR] Read %d bytes: \"%s\"\n", cnt, str);
  }

  cbuf_put_range(buffer, "hey", 5, 3);
  
  free(str);

  printf("\n\n\n");
  cnt = cbuf_get_range(buffer, 5, 10, (void**)&str);

  if(0 > cnt) {
	 printf("[FLG] %p\n", *(int*)str);
  } else {
	 printf("[STR] Read %d bytes: \"%s\"\n", cnt, str);
  }

  free(str);

  //printf("100 <= 101 < 4196\t%d\n", cbuf_btm_contains(buffer, 100, 4196, 101));
  // printf("5 <= 3 <= 1\t%d\n", cbuf_incl_contains(buffer, 5, 1, 3));
  //printf("5 <= 1 <= 3\t%d\n", cbuf_incl_contains(buffer, 5, 3, 1));
  //printf("4 <= 3 <= 4\t%d\n", cbuf_incl_contains(buffer, 4, 4, 3));
  //printf("1 <= 2 <= 3\t%d\n", cbuf_incl_contains(buffer, 1, 3, 2));
  //printf("3 <= 2 <= 1\t%d\n", cbuf_incl_contains(buffer, 3, 1, 2));

 // cbuf_destroy(buffer);

  return 0;
}
