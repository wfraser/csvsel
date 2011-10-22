/**
 * Growable Buffer
 *
 * by William R. Fraser, 10/19/2011
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "growbuf.h"

/**
 * Allocation granularity setting.
 *
 * Up to this size, the growbuf grows by doubling in size each time.
 * After this size, it grows by adding a multiple of this amount.
 */
#define GROWBUF_ALLOC_GRANULARITY 4096

/**
 * Create a growbuf
 *
 * Args:
 *  initial_size    - initial size of the buffer.
 *
 * Returns:
 *  pointer to initialized growbuf struct.
 */
growbuf* growbuf_create(size_t initial_size)
{
    growbuf* gb = (growbuf*)malloc(sizeof(growbuf));
    if (NULL == gb) {
        return NULL;
    }

    gb->buf = (void*)malloc(initial_size);
    if (NULL == gb->buf) {
        free(gb);
        return NULL;
    }

    gb->allocated_size = initial_size;
    gb->size = 0;

    memset(gb->buf, 0, initial_size);

    return gb;
}

/**
 * Free a growbuf.
 * Frees the growbuf structure itself, as well as the internal buffer.
 *
 * Args:
 *  gb  - growbuf to free
 */
void growbuf_free(growbuf* gb)
{
    if (NULL != gb) {
        if (NULL != gb->buf) {
            free(gb->buf);
            gb->size = 0;
            gb->allocated_size = 0;
        }
        free(gb);
    }
}

/**
 * Append to a growbuf, growing it if necessary.
 *
 * Args:
 *  gb  - growbuf to append to
 *  buf - buffer with data to append
 *  len - how many bytes of the buffer to append
 *
 * Returns:
 *  -1 * an errno.h error number. 0 on success.
 */
int growbuf_append(growbuf* gb, const void* buf, size_t len)
{
    if (NULL == gb) {
        return -EINVAL;
    }

    if (gb->size + len > gb->allocated_size) {

        size_t newsize;
        void*  newbuf;

        if (gb->size + len < GROWBUF_ALLOC_GRANULARITY) {

            newsize = gb->size;
            while (newsize < gb->size + len) {
                newsize *= 2;
            }

        }
        else {

            newsize = (((gb->size + len) / GROWBUF_ALLOC_GRANULARITY) + 1) * GROWBUF_ALLOC_GRANULARITY;

        }

        newbuf = realloc(gb->buf, newsize);
        if (NULL == newbuf) {
            return -ENOMEM;
        }

        gb->buf = newbuf;
        gb->allocated_size = newsize;
            
    }

    memcpy(gb->buf + gb->size, buf, len);

    gb->size += len;

    return 0;
}
