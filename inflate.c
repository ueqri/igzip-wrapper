#include "igzip_wrapper.h"
/* Normally you use isa-l.h instead for external programs */
#include "isa-l/igzip_lib.h"

#ifdef __cplusplus
extern "C" {
#endif

int decompress_file(const char *infile_name, unsigned char *output_string,
                    size_t *output_length) {
  FILE *in = NULL;
  unsigned char *inbuf = NULL, *outbuf = NULL;
  size_t inbuf_size, outbuf_size;
  struct inflate_state state;
  struct isal_gzip_header gz_hdr;
  int ret = 0, success = 0;

  size_t infile_name_len = strlen(infile_name);

  // Allocate mem and setup to hold gzip header info
  open_in_file(&in, infile_name);
  if (in == NULL)
    goto decompress_file_cleanup;

  inbuf_size = BLOCK_SIZE;
  outbuf_size = BLOCK_SIZE;
  inbuf = (unsigned char *)malloc_safe(inbuf_size);
  outbuf = (unsigned char *)malloc_safe(outbuf_size);

  isal_gzip_header_init(&gz_hdr);
  isal_inflate_init(&state);
  state.crc_flag = ISAL_GZIP_NO_HDR_VER;
  state.next_in = inbuf;
  state.avail_in = fread_safe(state.next_in, 1, inbuf_size, in, infile_name);

  // Actually read and save the header info
  ret = isal_read_gzip_header(&state, &gz_hdr);
  if (ret != ISAL_DECOMP_OK) {
    log_print(ERROR, "igzip: Error invalid gzip header found for file %s\n",
              infile_name);
    goto decompress_file_cleanup;
  }

  if (output_string == NULL) {
    log_print(ERROR, "igzip: Inflated string buffer for file %s is null\n",
              infile_name);
    goto decompress_file_cleanup;
  }

  size_t total_inflated = 0;

  // Start reading in compressed data and decompress
  do {
    if (state.avail_in == 0) {
      state.next_in = inbuf;
      state.avail_in =
          fread_safe(state.next_in, 1, inbuf_size, in, infile_name);
    }

    state.next_out = outbuf;
    state.avail_out = outbuf_size;

    ret = isal_inflate(&state);
    if (ret != ISAL_DECOMP_OK) {
      log_print(ERROR, "igzip: Error encountered while decompressing file %s\n",
                infile_name);
      goto decompress_file_cleanup;
    }

    size_t size_out = state.next_out - outbuf;
    if (size_out > 0) {
      memcpy(&output_string[total_inflated], outbuf, size_out);
      total_inflated += size_out;
    }

  } while (state.block_state != ISAL_BLOCK_FINISH // while not done
           && (!feof(in) || state.avail_out == 0) // and work to do
  );

  // Add the following to look for and decode additional concatenated files
  if (!feof(in) && state.avail_in == 0) {
    state.next_in = inbuf;
    state.avail_in = fread_safe(state.next_in, 1, inbuf_size, in, infile_name);
  }

  while (state.avail_in > 0 && state.next_in[0] == 31) {
    // Look for magic numbers for gzip header. Follows the gzread() decision
    // whether to treat as trailing junk
    if (state.avail_in > 1 && state.next_in[1] != 139)
      break;

    isal_inflate_reset(&state);
    state.crc_flag = ISAL_GZIP; // Let isal_inflate() process extra headers
    do {
      if (state.avail_in == 0 && !feof(in)) {
        state.next_in = inbuf;
        state.avail_in =
            fread_safe(state.next_in, 1, inbuf_size, in, infile_name);
      }

      state.next_out = outbuf;
      state.avail_out = outbuf_size;

      ret = isal_inflate(&state);
      if (ret != ISAL_DECOMP_OK) {
        log_print(ERROR,
                  "igzip: Error while decompressing extra concatenated"
                  "gzip files on %s\n",
                  infile_name);
        goto decompress_file_cleanup;
      }

      size_t size_out = state.next_out - outbuf;
      if (size_out > 0) {
        memcpy(&output_string[total_inflated], outbuf, size_out);
        total_inflated += size_out;
      }

    } while (state.block_state != ISAL_BLOCK_FINISH &&
             (!feof(in) || state.avail_out == 0));

    if (!feof(in) && state.avail_in == 0) {
      state.next_in = inbuf;
      state.avail_in =
          fread_safe(state.next_in, 1, inbuf_size, in, infile_name);
    }
  }

  if (state.block_state != ISAL_BLOCK_FINISH)
    log_print(ERROR, "igzip: Error %s does not contain a complete gzip file\n",
              infile_name);
  else
    success = 1;

decompress_file_cleanup:

  if (in != NULL && in != stdin) {
    fclose(in);
  }

  *output_length = total_inflated;
  return (success == 0);
}

#ifdef __cplusplus
} // extern "C"
#endif
