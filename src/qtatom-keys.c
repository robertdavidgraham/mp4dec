#include "qtatom.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


size_t mp4_parse_keys(struct mp4_state *s, struct qtatom_keys *inner, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    unsigned state = s->inner_state;

    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
    }

    while (offset < max) {
        unsigned char c = buf[offset++];
        //printf(" %02x(%c)", c, c);
        switch (state) {
            case 0:
                if (c != 0)
                    printf(" version=%u", c);
                break;
            case 1:
            case 2:
            case 3:
                if (c != 0)
                    printf(" flag=%02x", c);
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                inner->count <<= 8;
                inner->count |= c;
                if (state == 7) {
                    printf(" count=%u", inner->count);
                    inner->size = 0;
                }
                break;
            case 8:
            case 9:
            case 10:
            case 11:
                inner->size <<= 8;
                inner->size |= c;
                if (state == 11) {
                    printf(" size=%u", inner->size);
                    if (inner->size < 8) {
                        printf(" keys.size=invalid");
                        state = 1000;
                    } else {
                        inner->size -= 4;
                    }
                }
                break;
            case 12:
            case 13:
            case 14:
            case 15:
                inner->type <<= 8;
                inner->type |= c;
                inner->size--;
                inner->index++;
                if (state == 15) {
                    printf(" {index=%u '%c%c%c%c'=\'", inner->index, (inner->type>>24)&0xFF, (inner->type>>16)&0xFF, (inner->type>>8)&0xFF, (inner->type>>0)&0xFF);
                }
                break;
            case 16:
                printf("%c", c);
                if (--inner->size == 0) {
                    printf("\'}");
                    state = 4-1;
                } else
                    state = 16-1;
                break;
            default:
                break;
        }
        state++;
    }

    if (is_final) {
        qtatom_debug_epilog(&s->atom);
    }
    s->inner_state = state;
    return offset;
}
