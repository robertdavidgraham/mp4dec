#include "qtatom.h"
#include <ctype.h>
#include <stdio.h>

size_t mp4_parse_numbers(struct mp4_state *s, struct numbers *num, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    unsigned state = s->inner_state;

    while (offset < max) {
        switch (state) {
            case 0:
                num->index = 0;
                state++;
                break;
            case 1:
                num->number = buf[offset++];
                state++;
                break;
            case 2:
                num->number <<= 8;
                num->number |= buf[offset++];
                state++;
                break;
            case 3:
                num->number <<= 8;
                num->number |= buf[offset++];
                state++;
                break;
            case 4:
                num->number <<= 8;
                num->number |= buf[offset++];
                num->index++;
                state++;
                goto end;
                break;
            case 5:
                state = 1;
                break;

        }
            
    }

end:
    s->inner_state = state;
    return offset;
}

/**
 * Convert an atom name to a string
 */
static const char *name_from_type(unsigned type)
{
    static char buf[5];
    size_t i;
    buf[0] = (type >> 24) & 0xFF;
    buf[1] = (type >> 16) & 0xFF;
    buf[2] = (type >>  8) & 0xFF;
    buf[3] = (type >>  0) & 0xFF;
    buf[4] = '\0';
    for (i=0; i<4; i++) {
        if (!isprint(buf[i]))
            buf[i] = '*';
    }
    return buf;
}

void qtatom_debug_prolog(const struct qtatom *atom)
{
    printf("%u 0x%08x 0x%08x \"%s\" ", 
            atom->indent,
            (unsigned)atom->file_offset,
            (unsigned)atom->length,
            name_from_type(atom->type));

}
void qtatom_debug_epilog(const struct qtatom *atom)
{
    printf("\n");
}
