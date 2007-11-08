#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define HEADER_SIZE 8

#define PROTO_DATA 0
#define PROTO_RIP 1
#define PROTO_TCP 2

#define get_version(p)        ((unsigned char)(((p)[0] & 0xF0)>>4))
#define set_version(p, v)     ((p)[0] &= 0x0F, (p)[0] |= ((v)<<4))

#define get_protocol(p)       ((unsigned char)((p)[0] & 0x0F))
#define set_protocol(p, v)    ((p)[0] &= 0xF0, (p)[0] |= (v))

#define get_total_len(p)      (*((uint16_t*)((p)+1)))
#define set_total_len(p, v)   do {uint16_t _tmp=(v); memcpy((p)+1, &_tmp, 2);} while(0)

#define get_ttl(p)            ((unsigned char)((p)[3]))
#define set_ttl(p, v)         ((p)[3] = ((unsigned char)(v)))

#define get_checksum(p)       (*((uint16_t*)((p)+4)))
#define set_checksum(p, v)    do {uint16_t _tmp=(v); memcpy((p)+4, &_tmp, 2);} while(0)

#define get_src(p)            ((unsigned char)((p)[6]))
#define set_src(p, v)         ((p)[6] = ((unsigned char)(v)))

#define get_dst(p)            ((unsigned char)((p)[7]))
#define set_dst(p, v)         ((p)[7] = ((unsigned char)(v)))

#define dump_bits(p, l)       do {int _tmp; for(_tmp=0;_tmp<(l);_tmp++){ \
                                 ((p)[_tmp / 8] & (0x80>>(_tmp%8))) ? putchar('1') : putchar('0');}putchar('\n');} while(0)

#endif
