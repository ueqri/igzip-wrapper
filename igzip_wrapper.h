#ifndef _IGZIP_WRAPPER_H_
#define _IGZIP_WRAPPER_H_

#define _FILE_OFFSET_BITS 64
#include <assert.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE (1024 * 1024)

// Config options
#ifndef _IGZIP_IS_INTERACTIVE
#define _IGZIP_IS_INTERACTIVE 1 // interactive prompt by default
#endif
#ifndef _IGZIP_VERBOSE_LEVEL
#define _IGZIP_VERBOSE_LEVEL 0 // quiet level by default
#endif
#ifndef _IGZIP_FILE_FORCE_OVERRITTEN
#define _IGZIP_FILE_FORCE_OVERRITTEN 0 // not overritten by default
#endif

// Error exit codes
#define MALLOC_FAILED -1
#define FILE_OPEN_ERROR -2
#define FILE_READ_ERROR -3
#define FILE_WRITE_ERROR -4

enum log_types { INFORM, WARN, ERROR, VERBOSE };

typedef struct _string_view {
  unsigned char *data;
  size_t offset;
} string_with_head;

/* utilities */
void log_print(int log_type, char *format, ...);
void *malloc_safe(size_t size);
FILE *fopen_safe(const char *file_name, char *mode);
void open_in_file(FILE **in, const char *infile_name);
void open_out_file(FILE **out, const char *outfile_name);
size_t fread_safe(void *buf, size_t word_size, size_t buf_size, FILE *in,
                  const char *file_name);
size_t fwrite_safe(void *buf, size_t word_size, size_t buf_size, FILE *out,
                   const char *file_name);

/* igzip inflate wrapper */
int decompress_file(const char *infile_name, unsigned char *output_string,
                    size_t *output_length);

/* igzip deflate wrapper */
int compress_file(unsigned char *input_string, size_t input_length,
                  const char *outfile_name, int compress_level, int thread_num);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
