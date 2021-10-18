#include "igzip_wrapper.h"
#include <chrono>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>

#define COMPARE_BLOCK 1024 * 1024
#define READ_BUF_ONCE COMPARE_BLOCK
#define BUFFER_SIZE 1ll << 35

void loadCheckBuffer(FILE *fp, unsigned char *check, size_t *length) {
  size_t read;
  for (*length = 0; *length < BUFFER_SIZE; *length += read) {
    read = fread(check + (*length), 1, READ_BUF_ONCE, fp);
    if (read == 0) {
      break;
    } else if (read == -1) {
      std::cerr << "error to read compared file" << std::endl;
    }
  }
}

int main(int argc, char *argv[]) {
  unsigned char *output = (unsigned char *)malloc(BUFFER_SIZE);
  if (output == NULL) {
    log_print(ERROR, "Cannot malloc output buffer\n");
    exit(-1);
  }
  unsigned char *check = (unsigned char *)malloc(BUFFER_SIZE);
  if (check == NULL) {
    log_print(ERROR, "Cannot malloc check buffer\n");
    exit(-1);
  }
  FILE *fileCheck = fopen(argv[2], "rb");
  if (fileCheck == NULL) {
    log_print(ERROR, "Cannot open check file\n");
    exit(-1);
  }
  size_t checkLength = 0, inflatedLength = -1;

  std::thread readCheckBuffer(&loadCheckBuffer, fileCheck, check, &checkLength);

  std::chrono::steady_clock::time_point begin =
      std::chrono::steady_clock::now();

  decompress_file(argv[1], output, &inflatedLength);

  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

  std::cout
      << "Decompression elapse = "
      << std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()
      << "[s]" << std::endl;

  readCheckBuffer.join();

  assert(checkLength == inflatedLength);
  assert(memcmp(check, output, checkLength) == 0);

  std::cout << "Passed!" << std::endl;

  return 0;
}