#include "qtatom.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>


size_t mp4_parse_ilst(struct mp4_state *s, struct qtatom_ilst *inner, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    unsigned state = s->inner_state;

    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
        
        inner->subitem = malloc(sizeof(inner->subitem[0]));
        memset(inner->subitem, 0, sizeof(inner->subitem[0]));
        inner->subitem->atom.indent = s->atom.indent + 1;
        inner->subitem->atom.file_offset = s->atom.file_offset + s->atom.header_length;
    }

    while (offset < max) {
        switch (state) {
            case 0:
            case 1:
            case 2:
            case 3:
                inner->size <<= 8;
                inner->size |= buf[offset++];
                if (state == 3) {
                    printf(" size=%u", inner->size);
                }
                state++;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                inner->index <<= 8;
                inner->index |= buf[offset++];
                if (state == 7) {
                    printf(" index=%u", inner->index);
                    inner->size = 0;
                    qtatom_debug_epilog(&s->atom);
                }
                state++;
                break;
            case 8:
                offset = mp4_parse(inner->subitem, buf, offset, max);
                break;

            default:
                printf(" %02x(%c)", buf[offset], buf[offset]);
                offset++;
                break;
        }
    }

    if (is_final) {
        if (state != 8)
            qtatom_debug_epilog(&s->atom);
    }
    s->inner_state = state;
    return offset;
}
