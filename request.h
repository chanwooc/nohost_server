#ifndef __REQUEST_H__
#define __REQUEST_H__

#include <stdint.h>
#include <pthread.h>
#include "server.h"
//#define PAGESIZE 8192
/*
typedef struct{
	int fd;
	uint64_t key;
	char *value;
	char type;	// 1: SET 2:GET 3:DEL
	int len;
}req_t;
*/


typedef struct {
	char type;
}Type_info;

typedef struct {
	uint64_t key;
}Key_info;

typedef struct {
	char* value;
	int len;
}Value_info;

typedef struct req_t{
	int fd;
	int dmaTag;
	int org_type;
	client *c;
	Type_info *type_info;
	Key_info *key_info;
	Value_info *value_info;
	void (*end_req)(struct req_t*);
	struct timeval make_time;
	struct timeval end_time;
}req_t;


void alloc_dma(req_t*);
void free_dma(req_t*);
req_t* alloc_req();
void free_req(req_t*);
void make_req(req_t*);
void end_req(req_t*);
void print_time(struct timeval &time, int fd);
void print_total_time(req_t *r, int fd);


#endif
