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

#endif //GROWBUF_H
