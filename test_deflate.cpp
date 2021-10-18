#include "igzip_wrapper.h"
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

#define READ_BUF_ONCE 1024 * 1024
#define BUFFER_SIZE 1ll << 35

void load(FILE *fp, unsigned char *dst, size_t *length) {
  size_t read;
  for (*length = 0; *length < BUFFER_SIZE; *length += read) {
    read = fread(dst + (*length), 1, READ_BUF_ONCE, fp);
    if (read == 0) {
      break;
    } else if (read == -1) {
      std::cerr << "error to read compared file" << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  unsigned char *src = (unsigned char *)malloc(BUFFER_SIZE);
  if (src == NULL) {
    log_print(ERROR, "Cannot malloc check buffer\n");
    exit(-1);
  }

  FILE *src_fp = fopen(argv[1], "rb");
  if (src_fp == NULL) {
    log_print(ERROR, "Cannot open source file\n");
    exit(-1);
  }

  size_t src_len = 0, decompress_len = 0;

  load(src_fp, src, &src_len);

  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();

  // max compress level is 3, max thread nums is 8
  compress_file(src, src_len, argv[2], 3, 8);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::cout
      << "Compression elapse :"
      << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()
      << "s" << std::endl;

  unsigned char *decompress = (unsigned char *)malloc(BUFFER_SIZE);
  if (decompress == NULL) {
    log_print(ERROR, "Cannot malloc decompression buffer\n");
    exit(-1);
  }

  decompress_file(argv[2], decompress, &decompress_len);

  assert(src_len == decompress_len);
  assert(memcmp(src, decompress, src_len) == 0);

  std::cout << "Passed!" << std::endl;

  return 0;
}