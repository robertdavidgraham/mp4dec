#define _CRT_SECURE_NO_WARNINGS
#include "qtatom.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>






size_t mp4_parse_ftyp(struct mp4_state *s, struct ftyp *ftyp, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{
    unsigned state = s->inner_state;

    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
    }

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

    if (is_final) {
        qtatom_debug_epilog(&s->atom);
    }
    s->inner_state = state;
    return offset;
}

size_t mp4_parse_container(struct mp4_state *s, struct container *moov, const unsigned char *buf, 
    size_t offset, size_t max, int is_final)
{

    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
        qtatom_debug_epilog(&s->atom);
        moov->subitem = malloc(sizeof(moov->subitem[0]));
        memset(moov->subitem, 0, sizeof(moov->subitem[0]));
        moov->subitem->atom.indent = s->atom.indent + 1;
        moov->subitem->atom.file_offset = s->atom.file_offset + s->atom.header_length;
        s->inner_state++;
    }

    mp4_parse(moov->subitem, buf, offset, max);
    
    if (is_final)
        free(moov->subitem);
    return offset;
}

size_t mp4_parse_atom(struct qtatom *s, const unsigned char *buf, size_t offset, size_t max)
{
    while (offset < max) {

        switch (s->state) {
        case 0: case 1: case 2: case 3:
            s->header_length = 0;
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
                    s->header_length = 8;
                }
            }
            break;
        case 8:
            /* This is where parsing ends for the header */
            assert(s->header_length == 8 || s->header_length == 16);
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
                    s->header_length = 16;
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


/**
 * The default parser
 */
size_t mp4_parse_unknown(struct mp4_state *s, struct unknown *unknown, const unsigned char *buf, size_t offset, size_t max, int is_final)
{
    if (s->inner_state == 0) {
        qtatom_debug_prolog(&s->atom);
    }

    while (offset < max) {
        switch (s->inner_state) {
            case 0:
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

    if (is_final) {
        qtatom_debug_epilog(&s->atom);
    }
    return offset;
}


/**
 * This parses the first-level of the file.
 */
size_t mp4_parse(struct mp4_state *s, const unsigned char *buf, size_t offset, size_t max)
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
            sub_length = (size_t)s->atom.remaining;
        is_final = (sub_length == s->atom.remaining);

        /* Parse the inner section */
        switch (s->atom.type) {
        default:
            mp4_parse_unknown(s, &s->unknown, buf, offset, offset + sub_length, is_final);
            break;
        case Type_ftyp:
            mp4_parse_ftyp(s, &s->ftyp, buf, offset, offset + sub_length, is_final);
            break;
        case Type_mvhd:
            mp4_parse_mvhd(s, &s->mvhd, buf, offset, offset + sub_length, is_final);
            break;
        case Type_hdlr:
            mp4_parse_hdlr(s, &s->hdlr, buf, offset, offset + sub_length, is_final);
            break;
        case Type_keys:
            mp4_parse_keys(s, &s->keys, buf, offset, offset + sub_length, is_final);
            break;
        case Type_ilst:
            mp4_parse_ilst(s, &s->ilst, buf, offset, offset + sub_length, is_final);
            break;
        case Type_moov:
        case Type_trak:
        case Type_meta:
            mp4_parse_container(s, &s->container, buf, offset, offset + sub_length, is_final);
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
    return offset;
}


int parse_file(const char *filename)
{
    FILE *fp;
    struct mp4_state s = {0};
    uint64_t total_read = 0;

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

    printf("total = %llu\n", (unsigned long long)total_read);

    fclose(fp);

    return 0;
}
struct configuration
{
    char **files;
    unsigned filter;
};

int command_line_parm(int argc, char *argv[], int i, struct configuration *cfg)
{
    if (strcmp(argv[i], "--files") == 0) {
        FILE *fp;
        if (i + 1 >= argc || (argv[i+1][0] == '-' && argv[i][1] != '\0')) {
            fprintf(stderr, "[-] --files: expected filename\n");
            exit(1);
        }
        if (strcmp(argv[i+1], "-") == 0)
            fp = stdin;
        else {
            char line[1024];
            fp = fopen(argv[i+1], "rb");
            if (fp == NULL) {
                perror(argv[i+1]);
                exit(1);
            }
            while (fgets(line, sizeof(line), fp)) {
                while (line[0] && isspace(line[0]))
                    memmove(line, line+1, strlen(line));
                while (line[0] && isspace(line[strlen(line)-1]))
                    line[strlen(line)-1] = '\0';
                parse_file(line);
            }
            fclose(fp);
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    int i;
    struct configuration cfg = {0};

    if (argc <= 0) {
        printf("Usage:\n mp4dec <filename>\n");
        return 1;
    }

    for (i=1; i<argc; i++) {
        if (argv[i][0] == '-')
            i += command_line_parm(argc, argv, i, &cfg);
        else
            parse_file(argv[i]);
    }
    return 0;
}
