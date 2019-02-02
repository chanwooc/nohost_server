#ifndef SERVER_H
#define SERVER_H

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <netinet/tcp.h>
#include <fcntl.h>

#define SERVER_PORT     12345
#define MAX_CLNT         2000
#define BUFSIZE 		1024 * 1024
//#define BUFSIZE 		36 * 8237 * 4

#define OUTPUT_BUFSIZE  1024* 1024*100
#define QSIZE            2048
#define TRUE                1
#define FALSE               0
#define QUEUE_NUM 1
//#define TIME_START(x) gettimeofday(x,NULL)
//#define TIME_END(x)   gettimeofday(x,NULL)
//#define TIME_TOTAL(x,y,z) z += 1000000 * (y.tv_sec - x.tv_sec) + (y.tv_usec - x.tv_usec)
#define TIME_START(x) ;
#define TIME_END(x)   ;
#define TIME_TOTAL(x,y,z) ;

//extern rocksdb::DB* db;
extern bool nosync;
extern int nprocs;

extern const char *ERR_INCOMPLETE;
extern const char *ERR_QUIT;

void log(char c, const char *format, ...);

int stringmatchlen(const char *pattern, int patternLen,
        const char *string, int stringLen, int nocase);
int pattern_limits(const char *pattern, int patternLen, 
		char **start, int *startLen, char **end, int *endLen);
void flushdb();
int remove_directory(const char *path, int remove_parent);

// atoul returns a positive integer. invalid or negative integers return -1.
int atop(const char* str, int len);

typedef const char *error;

typedef struct client_t {
//	uv_tcp_t tcp;	// should always be the first element.
//	uv_write_t req;
//	uv_work_t worker;
//	uv_stream_t *server;
	int must_close;
	char *buf;
	int buf_cap;
	int buf_len;
	int buf_idx;
	const char **args;
	int args_cap;
	int args_len;
	int *args_size;
	char *tmp_err;
	char *output;
	int output_len;
	int output_cap;
	int output_offset;
	bool output_valid;
	int current_items;
	pthread_mutex_t *mutex;
	int fd;
	int time_valid;
	int multi;
	int which_queue;
} client;

typedef struct time_check {
	struct timeval recv_start;
	struct timeval exec_end;
	struct timeval make_req_start;
	struct timeval end_req_end;
	struct timeval send_start;
	struct timeval recv_end;
	struct timeval clnt_start;
	struct timeval clnt_end;
	double recv_exec;
	double make_end;
	double send_recv;
	double clnt_total;
	double total;
}time_check;

client *client_new();
void client_free(client *c);
void client_close(client *c);

void client_write(client *c, const char *data, int n);
void client_clear(client *c);
void client_write_byte(client *c, char b);
void client_write_bulk(client *c, const char *data, int n);
void client_write_multibulk(client *c, int n);
void client_write_int(client *c, int n);
void client_write_error(client *c, error err);
void client_flush_offset(client *c, int offset);
void client_flush(client *c);
void client_print_args(client *c);
error client_err_expected_got(client *c, char c1, char c2);
error client_err_unknown_command(client *c, const char *name, int count);

bool client_exec_commands(client *c);


error exec_command(client *c);


#endif // SERVER_H
