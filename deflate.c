#include "igzip_wrapper.h"
/* Normally you use isa-l.h instead for external programs */
#include "isa-l/igzip_lib.h"

#if defined (HAVE_THREADS)
# include <pthread.h>
# include "isa-l/crc.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define UNIX 3

int level_size_buf[10] = {
#ifdef ISAL_DEF_LVL0_DEFAULT
	ISAL_DEF_LVL0_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL1_DEFAULT
	ISAL_DEF_LVL1_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL2_DEFAULT
	ISAL_DEF_LVL2_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL3_DEFAULT
	ISAL_DEF_LVL3_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL4_DEFAULT
	ISAL_DEF_LVL4_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL5_DEFAULT
	ISAL_DEF_LVL5_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL6_DEFAULT
	ISAL_DEF_LVL6_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL7_DEFAULT
	ISAL_DEF_LVL7_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL8_DEFAULT
	ISAL_DEF_LVL8_DEFAULT,
#else
	0,
#endif
#ifdef ISAL_DEF_LVL9_DEFAULT
	ISAL_DEF_LVL9_DEFAULT,
#else
	0,
#endif
};

#if defined(HAVE_THREADS)

#define MAX_THREADS 8
#define MAX_JOBQUEUE 16		/* must be a power of 2 */

enum job_status {
	JOB_UNALLOCATED = 0,
	JOB_ALLOCATED,
	JOB_SUCCESS,
	JOB_FAIL
};

struct thread_job {
	uint8_t *next_in;
	uint32_t avail_in;
	uint8_t *next_out;
	uint32_t avail_out;
	uint32_t total_out;
	uint32_t type;
	uint32_t status;
};
struct thread_pool {
	pthread_t threads[MAX_THREADS];
	struct thread_job job[MAX_JOBQUEUE];
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int head;
	int tail;
	int queue;
	int shutdown;
};

// Globals for thread pool
struct thread_pool pool;

static inline int pool_has_space()
{
	return ((pool.head + 1) % MAX_JOBQUEUE) != pool.tail;
}

static inline int pool_has_work()
{
	return (pool.queue != pool.head);
}

int pool_get_work()
{
	assert(pool.queue != pool.head);
	pool.queue = (pool.queue + 1) % MAX_JOBQUEUE;
	return pool.queue;
}

int pool_put_work(struct isal_zstream *stream)
{
	pthread_mutex_lock(&pool.mutex);
	if (!pool_has_space() || pool.shutdown) {
		pthread_mutex_unlock(&pool.mutex);
		return 1;
	}
	int idx = (pool.head + 1) % MAX_JOBQUEUE;
	pool.job[idx].next_in = stream->next_in;
	pool.job[idx].avail_in = stream->avail_in;
	pool.job[idx].next_out = stream->next_out;
	pool.job[idx].avail_out = stream->avail_out;
	pool.job[idx].status = JOB_ALLOCATED;
	pool.job[idx].type = stream->end_of_stream == 0 ? 0 : 1;
	pool.head = idx;
	pthread_cond_signal(&pool.cond);
	pthread_mutex_unlock(&pool.mutex);
	return 0;
}

void *thread_worker(void *arg)
{
	int compress_level = *((int*)arg);
	struct isal_zstream wstream;
	int check;
	int work_idx;
	int level = compress_level;
	int level_size = level_size_buf[level];
	uint8_t *level_buf = (uint8_t *)malloc_safe(level_size);
	log_print(VERBOSE, "Start worker, compress level %d\n", compress_level);

	while (!pool.shutdown) {
		pthread_mutex_lock(&pool.mutex);
		while (!pool_has_work() && !pool.shutdown) {
			pthread_cond_wait(&pool.cond, &pool.mutex);
		}
		if (pool.shutdown) {
			pthread_mutex_unlock(&pool.mutex);
			continue;
		}

		work_idx = pool_get_work();
		pthread_cond_signal(&pool.cond);
		pthread_mutex_unlock(&pool.mutex);

		isal_deflate_stateless_init(&wstream);
		wstream.next_in = pool.job[work_idx].next_in;
		wstream.next_out = pool.job[work_idx].next_out;
		wstream.avail_in = pool.job[work_idx].avail_in;
		wstream.avail_out = pool.job[work_idx].avail_out;
		wstream.end_of_stream = pool.job[work_idx].type;
		wstream.flush = FULL_FLUSH;
		wstream.level = compress_level;
		wstream.level_buf = level_buf;
		wstream.level_buf_size = level_size;

		check = isal_deflate_stateless(&wstream);
		log_print(VERBOSE, "Worker finished job %d, out=%d\n",
			  work_idx, wstream.total_out);

		pool.job[work_idx].total_out = wstream.total_out;
		pool.job[work_idx].status = JOB_SUCCESS + check;	// complete or fail
		if (check)
			break;
	}
	free(level_buf);
	log_print(VERBOSE, "Worker quit\n");
	pthread_exit(NULL);
}

int pool_create(int thread_num_in_total, int *compress_level)
{
	int i;
	int nthreads = thread_num_in_total - 1;
	pool.head = 0;
	pool.tail = 0;
	pool.queue = 0;
	pool.shutdown = 0;
	for (i = 0; i < nthreads; i++)
		pthread_create(&pool.threads[i], NULL, thread_worker, (void*)compress_level);

	log_print(VERBOSE, "Created %d pool threads\n", nthreads);
	return 0;
}

void pool_quit(int thread_num_in_total)
{
	int i;
	pthread_mutex_lock(&pool.mutex);
	pool.shutdown = 1;
	pthread_mutex_unlock(&pool.mutex);
	pthread_cond_broadcast(&pool.cond);
	for (i = 0; i < thread_num_in_total - 1; i++)
		pthread_join(pool.threads[i], NULL);
	log_print(VERBOSE, "Deleted %d pool threads\n", i);
}

#endif // defined(HAVE_THREADS)

inline int ustr_eof( string_with_head *str, size_t full_length) {
  return str->length == full_length;
}

inline size_t ustrncpy( 
  unsigned char *dst, string_with_head *src, size_t num, 
  size_t full_length 
) {
  if (full_length - src->length < num) {
    num = full_length - src->length;
  }
  unsigned char *head = src->data + src->length;
  for (size_t i = 0; i < num; ++ i) {
    dst[i] = *(head + i);
  }
  src->length += num;
  return num;
}

int compress_file(
  unsigned char *input_string,  size_t input_length, 
  char *outfile_name, int compress_level, int thread_num)
{
	FILE *out = NULL;
	unsigned char *inbuf = NULL, *outbuf = NULL, *level_buf = NULL;
	size_t inbuf_size, outbuf_size;
	int level_size = 0;
	struct isal_zstream stream;
	struct isal_gzip_header gz_hdr;
	int ret, success = 0;
  string_with_head input;
  input.data = input_string;
  input.length = 0;
  string_with_head* input_ptr = &input;

	int level = compress_level;

	if (input_string == NULL)
		goto compress_file_cleanup;

	open_out_file(&out, outfile_name);
	if (out == NULL)
		goto compress_file_cleanup;

	inbuf_size = BLOCK_SIZE;
	outbuf_size = BLOCK_SIZE;

#if defined(HAVE_THREADS)
	if (thread_num > 1) {
		if (thread_num > MAX_THREADS) {
			log_print(WARN, "igzip: compression only supports %d threads at most",
							MAX_THREADS);
			thread_num = MAX_THREADS;
		}
		inbuf_size += (BLOCK_SIZE * MAX_JOBQUEUE);
		outbuf_size += (BLOCK_SIZE * MAX_JOBQUEUE * 2);
		pool_create(thread_num, &compress_level);
	}
#endif

	inbuf = (unsigned char *)malloc_safe(inbuf_size);
	outbuf = (unsigned char *)malloc_safe(outbuf_size);
	level_size = level_size_buf[compress_level];
	level_buf = (unsigned char *)malloc_safe(level_size);

	isal_gzip_header_init(&gz_hdr);
  // do not save file name and timestamp in compress
	gz_hdr.os = UNIX;

	isal_deflate_init(&stream);
	stream.avail_in = 0;
	stream.flush = NO_FLUSH;
	stream.level = level;
	stream.level_buf = level_buf;
	stream.level_buf_size = level_size;
	stream.gzip_flag = IGZIP_GZIP_NO_HDR;
	stream.next_out = outbuf;
	stream.avail_out = outbuf_size;

	isal_write_gzip_header(&stream, &gz_hdr);

	if (thread_num > 1) {
#if defined(HAVE_THREADS)
		int q;
		int end_of_stream = 0;
		uint32_t crc = 0;
		uint64_t total_in = 0;

		// Write the header
		fwrite_safe(outbuf, 1, stream.total_out, out, outfile_name);

		do {
			size_t nread;
			size_t inbuf_used = 0;
			size_t outbuf_used = 0;
			uint8_t *iptr = inbuf;
			uint8_t *optr = outbuf;

			for (q = 0; q < MAX_JOBQUEUE - 1; q++) {
				inbuf_used += BLOCK_SIZE;
				outbuf_used += 2 * BLOCK_SIZE;
				if (inbuf_used > inbuf_size || outbuf_used > outbuf_size)
					break;

				nread = ustrncpy(iptr, input_ptr, BLOCK_SIZE, input_length);
				crc = crc32_gzip_refl(crc, iptr, nread);
				end_of_stream = ustr_eof(input_ptr, input_length);
				total_in += nread;
				stream.next_in = iptr;
				stream.next_out = optr;
				stream.avail_in = nread;
				stream.avail_out = 2 * BLOCK_SIZE;
				stream.end_of_stream = end_of_stream;
				ret = pool_put_work(&stream);
				if (ret || end_of_stream)
					break;

				iptr += BLOCK_SIZE;
				optr += 2 * BLOCK_SIZE;
			}

			while (pool.tail != pool.head) {	// Unprocessed jobs
				int t = (pool.tail + 1) % MAX_JOBQUEUE;
				if (pool.job[t].status >= JOB_SUCCESS) {	// Finished next
					if (pool.job[t].status > JOB_SUCCESS) {
						success = 0;
						log_print(ERROR,
							  "igzip: Error encountered while compressing to file %s\n",
							  outfile_name);
						goto compress_file_cleanup;
					}
					fwrite_safe(pool.job[t].next_out, 1,
						    pool.job[t].total_out, out, outfile_name);

					pool.job[t].total_out = 0;
					pool.job[t].status = 0;
					pool.tail = t;
					pthread_cond_broadcast(&pool.cond);
				}
				// Pick up a job while we wait
				pthread_mutex_lock(&pool.mutex);
				if (!pool_has_work()) {
					pthread_mutex_unlock(&pool.mutex);
					continue;
				}

				int work_idx = pool_get_work();
				pthread_cond_signal(&pool.cond);
				pthread_mutex_unlock(&pool.mutex);

				isal_deflate_stateless_init(&stream);
				stream.next_in = pool.job[work_idx].next_in;
				stream.next_out = pool.job[work_idx].next_out;
				stream.avail_in = pool.job[work_idx].avail_in;
				stream.avail_out = pool.job[work_idx].avail_out;
				stream.end_of_stream = pool.job[work_idx].type;
				stream.flush = FULL_FLUSH;
				stream.level = compress_level;
				stream.level_buf = level_buf;
				stream.level_buf_size = level_size;
				int check = isal_deflate_stateless(&stream);
				log_print(VERBOSE, "Self   finished job %d, out=%d\n",
					  work_idx, stream.total_out);
				pool.job[work_idx].total_out = stream.total_out;
				pool.job[work_idx].status = JOB_SUCCESS + check;	// complete or fail
			}
		} while (!end_of_stream);

		// Write gzip trailer
		fwrite_safe(&crc, sizeof(uint32_t), 1, out, outfile_name);
		fwrite_safe(&total_in, sizeof(uint32_t), 1, out, outfile_name);

#else // No compiled threading support but asked for threads > 1
		assert(1);
#endif
	} else {		// Single thread
		do {
			if (stream.avail_in == 0) {
				stream.next_in = inbuf;
				stream.avail_in =
				    ustrncpy(stream.next_in, input_ptr, inbuf_size, input_length);
				stream.end_of_stream = ustr_eof(input_ptr, input_length);
			}

			if (stream.next_out == NULL) {
				stream.next_out = outbuf;
				stream.avail_out = outbuf_size;
			}

			ret = isal_deflate(&stream);

			if (ret != ISAL_DECOMP_OK) {
				log_print(ERROR,
					  "igzip: Error encountered while compressing to file %s\n",
					  outfile_name);
				goto compress_file_cleanup;
			}

			fwrite_safe(outbuf, 1, stream.next_out - outbuf, out, outfile_name);
			stream.next_out = NULL;

		} while (!ustr_eof(input_ptr, input_length) || stream.avail_out == 0);
	}

	success = 1;

#if defined(HAVE_THREADS)
	if (thread_num > 1)
		pool_quit(thread_num);
#endif

compress_file_cleanup:

	if (out != NULL && out != stdout)
		fclose(out);

	return (success == 0);
}

#ifdef __cplusplus
} // extern "C"
#endif
