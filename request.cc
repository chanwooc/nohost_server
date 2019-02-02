#include "server.h"
#include "request.h"
#include "libmemio.h"
#include "LR_inter.h"
#include "threading.h"
#include "stdint.h"
#include "lockfreeq.h"
#include "lfmpmc.h"
#include <queue.h>
using namespace std;

#define COUNT 1024*1024/4

extern time_check clnt_time[MAX_CLNT];
extern int _req_cnt;
extern int _req_cnt_get;
extern pthread_mutex_t req_cnt_mutex;
extern int req_max;
extern int make_fd, end_fd, total_fd;
pthread_mutex_t req_mutex = PTHREAD_MUTEX_INITIALIZER;
//extern spsc_bounded_queue_t<req_t*> *end_req_Q[QUEUE_NUM];
extern mpmc_bounded_queue_t<req_t*> *end_req_Q[QUEUE_NUM];
//extern std::queue<req_t*> end_req_Q[QUEUE_NUM];
extern pthread_mutex_t end_mutex[QUEUE_NUM];


 
void alloc_dma(req_t *req) {
	req->dmaTag = memio_alloc_dma(req->type_info->type, &(req->value_info->value));
}

void free_dma(req_t *req) {
	memio_free_dma(req->type_info->type, req->dmaTag);
}

req_t* alloc_req() {
	req_t *req;
	req = (req_t*)calloc(1,sizeof(req_t));
	req->type_info = (Type_info*)calloc(1,sizeof(Type_info));
	req->key_info = (Key_info*)calloc(1,sizeof(Key_info));
	req->value_info = (Value_info*)calloc(1,sizeof(Value_info));
#ifndef ENABLE_LIBFTL
	req->value_info->value = (char*)malloc(8192);
#endif
	req->end_req = end_req;

	return req;
}

void free_req(req_t *req) {
#ifdef ENABLE_LIBFTL
	if (req->type_info->type != 1) {
	//	printf("req type : %d, req : %p\n", req->type_info->type, req);
		req->type_info->type = 2;
	}
	if ( req->type_info->type <= 2 ) {
		free_dma(req);
	}
#else
	free(req->value_info->value);
#endif
	free(req->type_info);
	free(req->key_info);
	free(req->value_info);
	free(req);
}
extern threadset processor;
extern MeasureTime nh;
void make_req(req_t *req) {
	static uint64_t key1 = 1;
	static uint64_t key2 = 1;
	static uint64_t total_cnt = 0;
	static int make_req_cnt = 0;
	struct timeval s;
	int flag = 0;
	//printf("key : %d\n",req->key_info->key);
	//printf("make req_cnt : %d\n",++make_req_cnt);
#ifdef SEQ
	if ( req->type_info->type == 2 ) {
		if(key1==1) {
			gettimeofday(&s,NULL);
			printf("first read req :%d.%dsec\n",s.tv_sec,s.tv_usec);
		}else if (key1==COUNT) {
			gettimeofday(&s,NULL);
			printf("last read req :%d.%dsec\n",s.tv_sec,s.tv_usec);
		}
//	printf("asd\n");
		flag = 1;
	}
	pthread_mutex_lock(&req_mutex);
	if(flag) {
		//req->key_info->key = key1;
		if (key1 == 1000) {
			threadset_debug_init(&processor);
			MS(&nh);
		}
		if ( (key1 % 1000000) == 0){ 
			threadset_debug_print(&processor);
			MT(&nh);
			MS(&nh);
		}
//		printf("%d\n", key1);
		key1++;
	//printf("key : %d\n",req->key_info->key);
	}
	else {
		//req->key_info->key = key2;
		if (key2 == 1000) {
			threadset_debug_init(&processor);
			MS(&nh);
		}
		if ( (key2 % 1000000) == 0){ 
			threadset_debug_print(&processor);
			MT(&nh);
			MS(&nh);
		}
		key2++;
	}
	req->key_info->key++;
	pthread_mutex_unlock(&req_mutex);
	//printf("%d %d %d %d %d\n",++total_cnt,req->type_info->type,req->key_info->key,key1-1,key2-1);
	//printf("%d %d\n",req->type_info->type,req->key_info->key);
	//fflush(stdout);
#else
	req->key_info->key++;
#endif
//	pthread_mutex_lock(&req_cnt_mutex);
/*	
	if (req->type_info->type == 2) {
		_req_cnt++;
		printf("req_cnt : %d\n", _req_cnt);
	}	*	
	else { 
		_req_cnt_get++;
		printf("get_req_cnt : %d\n", _req_cnt_get);
	}*/
//	print_time(req->make_time, make_fd);
//	pthread_mutex_unlock(&req_cnt_mutex);

	//	printf("%d\n", req->key_info->key);
//	/*
	//end_req(req);
	//return;
	
//	printf("make req : %p\n", req);
//	while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req));	
/*	
	while(1) {
		pthread_mutex_lock(&end_mutex[req->c->which_queue]);
		if(end_req_Q[req->c->which_queue%2].size() >= 4096) {
			pthread_mutex_unlock(&end_mutex[req->c->which_queue]);
			continue;
		}
		end_req_Q[req->c->which_queue%2].push(req);
		pthread_mutex_unlock(&end_mutex[req->c->which_queue]);
		break;
	}
*/
	//if (req->dmaTag < 0) printf("!!!!!!!!!!!!!!!!!\n");
	//if (req->key_info->key > 5000000) printf("@@@@@@@@@@@@@@@@@");
	
//#ifdef ENABLE_LIBFTL
	if (lr_make_req(req) == -1) {
		err(1, "%s", "make_req");
	}
//#else
//		end_req(req);
//#endif
//	*/		
}

void end_req(req_t *req) {
	char data[8192];
	static int cnt=0;
	struct timeval s,ss,ee;
	unsigned long long int t;
	//	pthread_mutex_lock(req->c->mutex);
	//printf("end_req\n");
	if( req->type_info->type == 2) {
		cnt++;
		if(cnt == COUNT) {
			gettimeofday(&s,NULL);
			printf("last read end req :%d.%dsec\n",s.tv_sec,s.tv_usec);
		}
		//int loop_test=0;
		while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req)){
			//printf("loop!:%d\n",loop_test++);
		};
		//		printf("end get %d\n", req->c->current_items);
		//		printf("end_req p : %p\n", req);
		//		client_write_bulk(req->c, req->value_info->value, 8192);
//	while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req));	
//		_req_cnt--;
		//printf("end_req_key : %d\n", req->key_info->key);
//		while(!end_req_Q->enqueue(req));
#ifdef TIME
		gettimeofday(&ss,NULL);
#endif
//		client_write_bulk(req->c, data, 8192);
#ifdef TIME
		gettimeofday(&ee,NULL);
		t = 1000000 * ( ee.tv_sec - ss.tv_sec) + ee.tv_usec - ss.tv_usec;
		printf("client_write_bulk : %llu\n",t);
#endif
//		req->c->current_items--;
//			client_flush_offset(req->c, 0);
//			client_clear(req->c);
			
//		if (  req->c->output_len >= 800000) {
//		if ( req->c->current_items == 0 || req->c->output_len != 0) {
	//		printf("req : %p, req->c :%p\n",req,req->c);
#ifdef TIME
		printf("output_len : %d\n",req->c->output_len);
		gettimeofday(&ss,NULL);
#endif
//			client_flush_offset(req->c, 0);
//			client_clear(req->c);
#ifdef TIME
		gettimeofday(&ee,NULL);
		t = 1000000 * ( ee.tv_sec - ss.tv_sec) + ee.tv_usec - ss.tv_usec;
		printf("client_flush : %llu\n",t);
#endif
//						printf("flush");
//		}
		
//		free_req(req);
	}	
	else if (req->type_info->type == 1) {
		//printf("not!!!\n");
//		while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req));	

		//req->c->current_items--;	
		//client_write(req->c, "+OK\r\n",5);
		//if (req->c->current_items == 0 ) {
		//	client_flush_offset(req->c, 0);
		//	client_clear(req->c);
		//}
		free_req(req);
	}
	else {
		while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req));	
		/*
		req->c->current_items--;
		if ( req->c->current_items == 0 ) {
			client_write(req->c, "+OK\r\n", 5);
			client_flush_offset(req->c, 0);
			client_clear(req->c);
		}
		*/
//		printf("free_req\n");
//		free_req(req);
	}
//	pthread_mutex_lock(&req_cnt_mutex);
	
	
	/*
	else {
		_req_cnt_get--;
		printf("end_get_req_cnt : %d\n", _req_cnt_get);
	}
	*/
//	print_time(req->end_time, end_fd);
//	print_total_time(req,total_fd);
//	pthread_mutex_unlock(&req_cnt_mutex);

	//	pthread_mutex_unlock(req->c->mutex);
}
