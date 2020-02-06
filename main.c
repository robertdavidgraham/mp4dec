#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>

enum {
    Type_ftyp = 0x66747970,
    Type_mdat = 0x6d646174,
    Type_mvhd = 0x6d766864,
    Type_dcmd = 0x64636d64,
    Type_tkhd = 0x746b6864,
    Type_elst = 0x656c7374,
    Type_mdhd = 0x6d646864, 
    Type_moov = 0x6D6F6F76,
    Type_trak = 0x7472616B,
    Type_wide = 0x77696465,
};

struct qtatom {
    uint64_t file_offset;
    uint64_t length;
    uint64_t remaining;
    uint32_t type;
    uint8_t state;
};

struct ftyp
{
    unsigned subtype;
    unsigned version;
    unsigned compat;
};

struct unknown
{
    unsigned bytes_printed;
};


struct mp4_state {
    unsigned x;
    struct qtatom atom;


    unsigned inner_state;
    union {
        struct ftyp ftyp;
        struct unknown unknown;
    };
};


size_t mp4_parse_ftyp(struct mp4_state *s, struct ftyp *ftyp, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    unsigned state = s->inner_state;

    while (offset < max) {
        unsigned char c = buf[offset++];
        switch (state) {
        case 0:
            memset(ftyp, 0, sizeof(*ftyp));
            ftyp->subtype = c;
            state++;
            break;
        case 1:
        case 2:
        case 3:
            ftyp->subtype <<= 8;
            ftyp->subtype |= c;
            state++;
            if (state == 4) {
                printf(" brand=\"%c%c%c%c\"",
                    (ftyp->subtype>>24) & 0xFF,
                    (ftyp->subtype>>16) & 0xFF,
                    (ftyp->subtype>> 8) & 0xFF,
                    (ftyp->subtype>> 0) & 0xFF
                    );
            }
            break;
        case 4:
        case 5:
        case 6:
        case 7:
            ftyp->version <<= 8;
            ftyp->version |= c;
            state++;
            if (state == 8)
                printf(" v=0x%x", ftyp->version);
            break;
        case 8:
        case 9:
        case 10:
        case 11:
            ftyp->compat <<= 8;
            ftyp->compat |= c;
            state++;
            if (state == 12) {
                printf(" \"%c%c%c%c\"",
                    (ftyp->compat>>24) & 0xFF,
                    (ftyp->compat>>16) & 0xFF,
                    (ftyp->compat>> 8) & 0xFF,
                    (ftyp->compat>> 0) & 0xFF
                    );
                state = 8;
                ftyp->compat = 0;
            }
            break;

        default:
            if (state < 32) {
                printf(" %02x", c);
            }
            break;
        }        
    }

    if (is_final)
        printf("-\n");
    s->inner_state = state;
    return offset;
}


size_t mp4_parse_atom(struct qtatom *s, const unsigned char *buf, size_t offset, size_t max)
{
    while (offset < max) {

        switch (s->state) {
        case 0: case 1: case 2: case 3:
            s->length <<= 8;
            s->length |= buf[offset++];
            s->state++;
            if (s->state == 4) {
                if (s->length == 0) {
                    s->length = 0xFFFFFFFFffffffffULL;
                }
            }
            break;
        case 4: case 5: case 6: case 7:
            s->type <<= 8;
            s->type |= buf[offset++];
            s->state++;
            if (s->state == 8) {
                if (s->length == 1) {
                    s->length = 0;
                    s->state = 16;
                } else if (s->length < 8) {
                    s->state = 9;
                } else {
                    s->remaining = s->length - 8;
                }
            }
            break;
        case 8:
            return offset;
        case 9: /* error */
            offset = max;
            break;
        case 16: case 17: case 18: case 19:
        case 20: case 21: case 22: case 23:
            s->length <<= 8;
            s->length |= buf[offset++];
            s->state++;
            if (s->state == 24) {
                s->remaining = s->length - 16;
                if (s->length < 16)
                    s->state = 9;
                else {
                    s->state = 8;
                }
            }
            break;
        default:
            offset = max;
            assert(!"possible");
            break;
        }

    }

    return offset;
}

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

/**
 * The default parser
 */
size_t mp4_parse_unknown(struct mp4_state *s, struct unknown *unknown, const unsigned char *buf, size_t offset, size_t max, int is_final)
{
    while (offset < max) {
        switch (s->inner_state) {
            case 0:
                printf("0x%08x 0x%08x \"%s\" ", 
                    (unsigned)s->atom.file_offset,
                    (unsigned)s->atom.length,
                    name_from_type(s->atom.type));
                s->inner_state++;
                unknown->bytes_printed = 0;
                break;
            case 1:
                if (unknown->bytes_printed++ < 16)
                    printf(" %02x", buf[offset]);
                offset++;
                break;
        }
    }

    if (is_final)
        printf("+\n");
    return offset;
}

/**
 * This parses the first-level of the file.
 */
void mp4_parse(struct mp4_state *s, const unsigned char *buf, size_t offset, size_t max)
{
    
    while (offset < max) {
        size_t sub_length;
        int is_final;
    
        /* Parse the atom header */
        offset = mp4_parse_atom(&s->atom, buf, offset, max);
        if (offset >= max)
            break;

        /* Calculate the length of the inner part. This can be no
         * bigger than the current remaining bytes of the current
         * atom or the current remaining bytes in this buffer. */
        sub_length = max - offset;
        if (sub_length > s->atom.remaining)
            sub_length = s->atom.remaining;
        is_final = (sub_length == s->atom.remaining);

        /* Parse the inner section */
        switch (s->atom.type) {
        default:
            mp4_parse_unknown(s, &s->unknown, buf, offset, offset + sub_length, is_final);
            break;
        case Type_ftyp:
            mp4_parse_ftyp(s, &s->ftyp, buf, offset, offset + sub_length, is_final);
            break;
        }
        offset += sub_length;
        s->atom.remaining -= sub_length;
        if (offset >= max)
            break;
        assert(s->atom.remaining == 0);
                
        /* start processing the next atom */
        s->atom.file_offset += s->atom.length;
        s->atom.length = 0;
        s->atom.state = 0;
        s->inner_state = 0;
    }
}


int main(int argc, char *argv[])
{
    const char *filename = argv[1];
    FILE *fp;
    struct mp4_state s = {0};
    uint64_t total_read = 0;

    if (argc <= 0) {
        printf("Usage:\n mp4-dev <filename>\n");
        return 1;
    }

    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[-] %s: %s\n", filename, strerror(errno));
        return 1;
    }

    for (;;) {
        size_t bytes_read;
        unsigned char buf[65536];

        bytes_read = fread(buf, 1, sizeof(buf), fp);
        if (bytes_read == 0)
            break;
        
        mp4_parse(&s, buf, 0, bytes_read);
        total_read += bytes_read;
    }

    printf("=\n");
    printf("total = %llu\n", (unsigned long long)total_read);
    return 0;
}
