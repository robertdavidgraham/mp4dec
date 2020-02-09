#ifndef QTATOM_H
#define QTATOM_H
#include <stddef.h>
#include <stdint.h>

enum {
    /* 'ftyp' File type compatibility�identifies the file type and 
     * differentiates it from similar file types, such as MPEG-4 files
     * and JPEG-2000 files. */
    Type_ftyp = 0x66747970,

    /* 'mdat' Movie sample data�media samples such as video frames and
     * groups of audio samples. Usually this data can be interpreted 
     * only by using the movie resource. */
    Type_mdat = 0x6d646174,

    /* 'moov' Movie resource metadata about the movie (number and type 
     * of tracks, location of sample data, and so on). Describes where
     * the movie data can be found and how to interpret it. */
    Type_moov = 0x6D6F6F76,

    /* 'free' Unused space available in file.*/
    Type_free = 0x72666565,

    /* 'skip' Unused space available in file. */
    Type_skip = 0x6b737069,

    /* 'wide' Reserved space�can be overwritten by an extended size field
     * if the following atom exceeds 2^32 bytes, without displacing the
     * contents of the following atom. */
    Type_mvhd = 0x6d766864,

    /* 'pnot' Reference to movie preview data. */
    Type_pnott = 0x6e70746f,

    Type_meta = 0x6d657461,
    Type_hdlr = 0x68646c72,
    Type_keys = 0x6b657973,
    Type_ilst = 0x696c7374,
    Type_dcmd = 0x64636d64,
    Type_tkhd = 0x746b6864,
    Type_elst = 0x656c7374,
    Type_mdhd = 0x6d646864, 
    Type_trak = 0x7472616B,
    Type_wide = 0x77696465,
}; 

struct numbers 
{
    unsigned state;
    unsigned number;
    unsigned index;
};

struct qtatom_hdlr
{
    struct numbers num;
};

struct qtatom_keys
{
    unsigned count;
    unsigned size;
    unsigned index;
    unsigned type;
};

struct qtatom_ilst
{
    struct mp4_state *subitem;

    unsigned size;
    unsigned index;
};

struct mp4_state;

size_t mp4_parse(struct mp4_state *s, const unsigned char *buf, size_t offset, size_t max);


struct qtatom {
    uint64_t file_offset;
    uint64_t length;
    uint64_t remaining;
    uint32_t type;
    unsigned indent;
    unsigned header_length;
    uint8_t state;
};

struct ftyp
{
    unsigned subtype;
    unsigned version;
    unsigned compat;
};

struct qtatom_mvhd
{
    struct numbers num;
    unsigned scale;
};


struct container
{
    struct mp4_state *subitem;
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
        struct qtatom_hdlr hdlr;
        struct qtatom_keys keys;
        struct qtatom_ilst ilst;
        struct ftyp ftyp;
        struct qtatom_mvhd mvhd;
        struct unknown unknown;
        struct container container;
    };
};

/**
 * Parse an atom header.
 */
size_t mp4_parse_atom(struct qtatom *s, const unsigned char *buf, size_t offset, size_t max);
enum {PARSE_ATOM_DONE=8};


void qtatom_debug_prolog(const struct qtatom *atom);
void qtatom_debug_epilog(const struct qtatom *atom);

#define PARSER(name) size_t mp4_parse_##name(struct mp4_state *s, struct qtatom_##name *inner, const unsigned char *buf, size_t offset, size_t max, int is_final)


size_t mp4_parse_numbers(struct mp4_state *s, struct numbers *num, const unsigned char *buf, 
    size_t offset, size_t max, int is_final);

size_t mp4_parse_hdlr(struct mp4_state *s, struct qtatom_hdlr *inner, const unsigned char *buf, 
    size_t offset, size_t max, int is_final);

size_t mp4_parse_mvhd(struct mp4_state *s, struct qtatom_mvhd *inner, const unsigned char *buf, 
    size_t offset, size_t max, int is_final);

PARSER(keys);
PARSER(ilst);



#endif
