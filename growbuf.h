/**
 * Growable Buffer
 *
 * by William R. Fraser, 10/19/2011
 */

#ifndef GROWBUF_H
#define GROWBUF_H

typedef struct _growbuf
{
    void*  buf;
    size_t allocated_size;
    size_t size;
} growbuf;

growbuf* growbuf_create(size_t initial_size);
void     growbuf_free(growbuf* gb);
int      growbuf_append(growbuf* gb, const void* buf, size_t len);
int      growbuf_append_byte(growbuf* gb, char byte);

#define growbuf_contains(g, thing)                                          \
    ({                                                                      \
        bool ret = false;                                                   \
        __typeof__(thing) _thing = (thing);                                 \
        for (size_t i = 0; i < g->size / sizeof(_thing); i++) {             \
            if (((__typeof__(thing)*)(g->buf))[i] == _thing) {              \
                ret = true;                                                 \
                break;                                                      \
            }                                                               \
        }                                                                   \
        ret;                                                                \
    })

#endif //GROWBUF_H
