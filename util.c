#include "igzip_wrapper.h"

#ifdef __cplusplus
extern "C"
{
#endif

void log_print(int log_type, char *format, ...)
{
	va_list args;
	va_start(args, format);

	switch (log_type) {
	case INFORM:
		vfprintf(stdout, format, args);
		break;
	case WARN:
		if ((VERBOSE_LEVEL) > 1)
			vfprintf(stderr, format, args);
		break;
	case ERROR:
		if ((VERBOSE_LEVEL) > 0)
			vfprintf(stderr, format, args);
		break;
	case VERBOSE:
		if ((VERBOSE_LEVEL) > 2)
			vfprintf(stderr, format, args);
		break;
	}

	va_end(args);
}

void *malloc_safe(size_t size)
{
	void *ptr = NULL;
	if (size == 0)
		return ptr;

	ptr = malloc(size);
	if (ptr == NULL) {
		log_print(ERROR, "igzip: Failed to allocate memory\n");
		exit(MALLOC_FAILED);
	}

	return ptr;
}

FILE *fopen_safe(char *file_name, char *mode)
{
	FILE *file;
	int answer = 0, tmp;

	/* Assumes write mode always starts with w */
	if (mode[0] == 'w') {
		if (access(file_name, F_OK) == 0) {
			log_print(WARN, "igzip: %s already exists;", file_name);
			if (IS_INTERACTIVE) {
				log_print(WARN, " do you wish to overwrite (y/n)?");
				answer = getchar();

				tmp = answer;
				while (tmp != '\n' && tmp != EOF)
					tmp = getchar();

				if (answer != 'y' && answer != 'Y') {
					log_print(WARN, "       not overwritten\n");
					return NULL;
				}
			} else if (!(FILE_FORCE_OVERRITTEN)) {
				log_print(WARN, "       not overwritten\n");
				return NULL;
			}

			if (access(file_name, W_OK) != 0) {
				log_print(ERROR, "igzip: %s: Permission denied\n", file_name);
				return NULL;
			}
		}
	}

	/* Assumes read mode always starts with r */
	if (mode[0] == 'r') {
		if (access(file_name, F_OK) != 0) {
			log_print(ERROR, "igzip: %s does not exist\n", file_name);
			return NULL;
		}

		if (access(file_name, R_OK) != 0) {
			log_print(ERROR, "igzip: %s: Permission denied\n", file_name);
			return NULL;
		}
	}

	file = fopen(file_name, mode);
	if (!file) {
		log_print(ERROR, "igzip: Failed to open %s\n", file_name);
		return NULL;
	}

	return file;
}

void open_in_file(FILE ** in, char *infile_name)
{
	*in = NULL;
	if (infile_name == NULL)
		*in = stdin;
	else
		*in = fopen_safe(infile_name, "rb");
}

void open_out_file(FILE ** out, char *outfile_name)
{
	*out = NULL;
  // Pass NULL outfile name to use stdout for compression output
	if (outfile_name == NULL)
		*out = stdout;
	else if (outfile_name != NULL)
		*out = fopen_safe(outfile_name, "wb");
	else if (!isatty(fileno(stdout)) || (FILE_FORCE_OVERRITTEN))
		*out = stdout;
	else {
		log_print(WARN, "igzip: No output location. Use -c to output to terminal\n");
		exit(FILE_OPEN_ERROR);
	}
}

size_t fread_safe(void *buf, size_t word_size, size_t buf_size, FILE * in, char *file_name)
{
	size_t read_size;
	read_size = fread(buf, word_size, buf_size, in);
	if (ferror(in)) {
		log_print(ERROR, "igzip: Error encountered while reading file %s\n",
			  file_name);
		exit(FILE_READ_ERROR);
	}
	return read_size;
}

size_t fwrite_safe(void *buf, size_t word_size, size_t buf_size, FILE * out, char *file_name)
{
	size_t write_size;
	write_size = fwrite(buf, word_size, buf_size, out);
	if (ferror(out)) {
		log_print(ERROR, "igzip: Error encountered while writing to file %s\n",
			  file_name);
		exit(FILE_WRITE_ERROR);
	}
	return write_size;
}

#ifdef __cplusplus
} // extern "C"
#endif
