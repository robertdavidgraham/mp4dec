#include "qtatom.h"
#include <stdio.h>

size_t mp4_parse_hdlr(struct mp4_state *s, struct qtatom_hdlr *inner, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
    }

    while (offset < max) {
        unsigned number;
        
        /* Parse the next 4 bytes into a number */
        offset = mp4_parse_numbers(s, &inner->num, buf, offset, max, is_final);
        if (s->inner_state != 5)
            continue;
        number = inner->num.number;

        /* Handle the number that was returned */
        switch (inner->num.index) {
            case 1: /* version, flags */
                /* The first byte is a "version" that should equal 0. The following
                 * three bytes are reserved for flags, which all should be zero */
                if ((number>>24) != 0)
                    printf(" version=%u", number>>24);
                if ((number&0xFFFFFF) != 0)
                    printf(" flags=0x%06x", number&0xFFFFFF);
                break;
            case 2: /* predefined = 0 */
                /* the next 4 bytes are called "predefined" by the standard and should
                 * equal zero */
                if (number != 0)
                    printf(" predefined=0x%08x", number);
                break;
            case 3: /* handler type */
                printf(" handler=\"%c%c%c%c\"", (number>>24)&0xFF, (number>>16)&0xFF, (number>>8)&0xFF, (number>>0)&0xFF);
                break;
            case 4: 
            case 5:
            case 6:
                if (number != 0)
                    printf(" reserved[%u]=0x%08x", inner->num.index-4, number);
                break;
            default:
                printf(" %u=0x%08x", inner->num.index, number);
                break;
        }
    }

    if (is_final) {
        qtatom_debug_epilog(&s->atom);
    }
    return offset;
}
