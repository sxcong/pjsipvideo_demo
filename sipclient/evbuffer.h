/**
  @file evbuffer.h
  @brief the prev declartion for the libevent's evbuffer.

*/
#ifndef ___EVBUFFER_H_
#define ___EVBUFFER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>

#define __func__ __FUNCTION__

typedef unsigned char u_char;

#define EVBUFFER_LENGTH(x)	(x)->off
#define EVBUFFER_DATA(x)	(x)->buffer

struct evbuffer {
	u_char *buffer;
	u_char *orig_buffer;

	size_t misalign;
	size_t totallen;
	size_t off;

	void (*cb)(struct evbuffer *, size_t, size_t, void *);
	void *cbarg;
};

/**
  Allocate storage for a new evbuffer.

  @return a pointer to a newly allocated evbuffer struct, or NULL if an error
          occurred
 */
struct evbuffer *evbuffer_new(void);


/**
  Deallocate storage for an evbuffer.

  @param pointer to the evbuffer to be freed
 */
void evbuffer_free(struct evbuffer *);


/**
  Expands the available space in an event buffer.

  Expands the available space in the event buffer to at least datlen

  @param buf the event buffer to be expanded
  @param datlen the new minimum length requirement
  @return 0 if successful, or -1 if an error occurred
*/
int evbuffer_expand(struct evbuffer *, size_t);


/**
  Append data to the end of an evbuffer.

  @param buf the event buffer to be appended to
  @param data pointer to the beginning of the data buffer
  @param datlen the number of bytes to be copied from the data buffer
 */
int evbuffer_add(struct evbuffer *, const void *, size_t);



/**
  Read data from an event buffer and drain the bytes read.

  @param buf the event buffer to be read from
  @param data the destination buffer to store the result
  @param datlen the maximum size of the destination buffer
  @return the number of bytes read
 */
int evbuffer_remove(struct evbuffer *, void *, size_t);


/**
 * Read a single line from an event buffer.
 *
 * Reads a line terminated by either '\r\n', '\n\r' or '\r' or '\n'.
 * The returned buffer needs to be freed by the caller.
 *
 * @param buffer the evbuffer to read from
 * @return pointer to a single line, or NULL if an error occurred
 */
char *evbuffer_readline(struct evbuffer *);


/**
  Move data from one evbuffer into another evbuffer.

  This is a destructive add.  The data from one buffer moves into
  the other buffer. The destination buffer is expanded as needed.

  @param outbuf the output buffer
  @param inbuf the input buffer
  @return 0 if successful, or -1 if an error occurred
 */
int evbuffer_add_buffer(struct evbuffer *, struct evbuffer *);


/**
  Append a formatted string to the end of an evbuffer.

  @param buf the evbuffer that will be appended to
  @param fmt a format string
  @param ... arguments that will be passed to printf(3)
  @return The number of bytes added if successful, or -1 if an error occurred.
 */
int evbuffer_add_printf(struct evbuffer *, const char *fmt, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
;


/**
  Append a va_list formatted string to the end of an evbuffer.

  @param buf the evbuffer that will be appended to
  @param fmt a format string
  @param ap a varargs va_list argument array that will be passed to vprintf(3)
  @return The number of bytes added if successful, or -1 if an error occurred.
 */
int evbuffer_add_vprintf(struct evbuffer *, const char *fmt, va_list ap);


/**
  Remove a specified number of bytes data from the beginning of an evbuffer.

  @param buf the evbuffer to be drained
  @param len the number of bytes to drain from the beginning of the buffer
  @return 0 if successful, or -1 if an error occurred
 */
void evbuffer_drain(struct evbuffer *, size_t);


/**
  Write the contents of an evbuffer to a file descriptor.

  The evbuffer will be drained after the bytes have been successfully written.

  @param buffer the evbuffer to be written and drained
  @param fd the file descriptor to be written to
  @return the number of bytes written, or -1 if an error occurred
  @see evbuffer_read()
 */
int evbuffer_write(struct evbuffer *, int);


/**
  Read from a file descriptor and store the result in an evbuffer.

  @param buf the evbuffer to store the result
  @param fd the file descriptor to read from
  @param howmuch the number of bytes to be read
  @return the number of bytes read, or -1 if an error occurred
  @see evbuffer_write()
 */
int evbuffer_read(struct evbuffer *, int, int);


/**
  Find a string within an evbuffer.

  @param buffer the evbuffer to be searched
  @param what the string to be searched for
  @param len the length of the search string
  @return a pointer to the beginning of the search string, or NULL if the search failed.
 */
u_char *evbuffer_find(struct evbuffer *, const u_char *, size_t);

/**
  Set a callback to invoke when the evbuffer is modified.

  @param buffer the evbuffer to be monitored
  @param cb the callback function to invoke when the evbuffer is modified
  @param cbarg an argument to be provided to the callback function
 */
void evbuffer_setcb(struct evbuffer *, void (*)(struct evbuffer *, size_t, size_t, void *), void *);

#ifdef __cplusplus
}
#endif

#endif /* ___EVBUFFER_H_ */