#include "qtatom.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static void
_print_mac_time(const char *prefix, unsigned number)
{
    if (number < 2082844800) {
        printf("%s%u", prefix, number);
    } else {
        time_t timestamp = number - 2082844800;
        struct tm *tm = gmtime(&timestamp);
        if (tm == NULL) {
            printf("%s%u", prefix, number);
        } else {
            char str[128];
            strftime(str, sizeof(str), "%Y-%m-%d" /*"" %H:%M:%S"*/, tm);
            printf("%s%s", prefix, str);
        }
    }
}

size_t mp4_parse_mvhd(struct mp4_state *s, struct qtatom_mvhd *inner, const unsigned char *buf, 
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
            case 2: /* creation time */
                _print_mac_time(" created=", number);
                break;
            case 3:
                //_print_mac_time(" modified=", number);
                break;
            case 4:
                inner->scale = number;
                //printf(" scale=%u", number);
                break;
            case 5:
                printf(" duration=%.2f", (double)number/(double)inner->scale);
                break;
            case 6:
            case 7:
            case 10:
            case 14:
            case 18:
                break;
            case 25:
                printf(" next=%u", number);
                break;
            default:
                if (number != 0)
                    printf(" %u=0x%08x", inner->num.index, number);
                break;
        }
    }

    if (is_final) {
        qtatom_debug_epilog(&s->atom);
    }
    return offset;
}
