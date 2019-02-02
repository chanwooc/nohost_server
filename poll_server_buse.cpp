#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <errno.h>
#include <unordered_map>
#include <string.h>
#include <unistd.h>
#include <queue>
#include <libmemio.h>
#include "request.h"
#include "LR_inter.h"
#include "lockfreeq.h"
#include "lfmpmc.h"
#include "hashMap.h"
using namespace std;

#define THREAD_NUM 1
#define PORT_NUM 1

time_check clnt_time[MAX_CLNT];
spsc_bounded_queue_t<client*> *clnt_q;

const char *ERR_INCOMPLETE = "incomplete";
const char *ERR_QUIT = "quit";

//char *reply;
int _req_cnt;
int _req_cnt_get;
int req_max;
pthread_mutex_t req_cnt_mutex = PTHREAD_MUTEX_INITIALIZER;
int make_fd;
int end_fd;
int total_fd;
int RWRATE = 5;
//extern queue<end_req_t*> end_req_Q;
//extern spsc_bounded_queue_t<lsmtree_req_t*> *end_req_Q;
//spsc_bounded_queue_t<req_t*> *end_req_Q[QUEUE_NUM];
mpmc_bounded_queue_t<req_t*> *end_req_Q[QUEUE_NUM];
map<uint64_t, unsigned int> *g_hashMap;
set<unsigned int> *g_hashValues;
//queue<req_t*> end_req_Q[QUEUE_NUM];
//pthread_mutex_t end_mutex[QUEUE_NUM];
extern pthread_mutex_t end_req_mutex;

typedef struct thread_arg {
	int listen_sd;
	int thread_num;
}thread_arg;

pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;
int *listen_sds;



char* get_buffer(client* c) {
	return c->buf + c->buf_idx + c->buf_len;
}

void* end_req_q_handler(void* arg) {
	//lsmtree_req_t *r = NULL;
	static int a=0;
	req_t *r = NULL;
	int idx = *((int*)arg);
	struct timeval s,e;
	long long unsigned int t;
	char data[8192] = {0,};
	memset(data,'c',8192);
	bool ret,flag=false;
//	bdbm_llm_req_t *br;
//	bdbm_drv_info_t *bd;
//	end_req_t *end_r;
   	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
   	CPU_SET(3, &cpuset);
	pthread_t current_thread = pthread_self();
//	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
//	nice(-20);
//		gettimeofday(&s,NULL);
	while(1) {
//		pthread_mutex_lock(&end_req_mutex);
//		if (end_req_Q.empty()) {
//			pthread_mutex_unlock(&end_req_mutex);
//			continue;
//		}
//		end_r = end_req_Q.front();


		ret = end_req_Q[idx]->dequeue(&r);
		if ( ret == false ) {
#ifdef TIME
			if (flag == false) {
				gettimeofday(&s,NULL);
				flag = true;
				printf("!!!!!!!!!!!!!!!!!!!!\n");
//		t = 1000000*(e.tv_sec - s.tv_sec) + e.tv_usec - s.tv_usec;
			}
#endif
			continue;
#ifdef TIME
		}else {
			if ( flag == true) {
				gettimeofday(&e,NULL);
				t = 1000000*(e.tv_sec - s.tv_sec) + e.tv_usec - s.tv_usec;
				printf("dequeue finish %llu\n", t);
				flag = false;
			}
#endif
		}
//		printf("%d\n",a++);
		if (r->type_info->type == 2) {
		//printf("req type : %d, req : %p\n", r->type_info->type, r);

//		write(r->c->fd, reply, 8201);
			client_write_bulk(r->c, data, 8192);
//			client_write_bulk(r->c, r->value_info->value, 8192);
//			if (  r->c->output_len >= 1000000) {
				client_flush_offset(r->c, 0);
				client_clear(r->c);
//			client_write_bulk(r->c, r->value_info->value, 8192);
//			if (  r->c->output_len >= 1000000) {

		//				printf("flush");
//			}
		
			free_req(r);
		}
		else if (r->type_info->type == 1) {
			printf("asdasd!!!\n");
			client_write(r->c, "+OK\r\n", 5);
			client_flush_offset(r->c,0);
			client_clear(r->c);
			//free_req(r);
		}
		//else if ( r->type_info->type == 255) {
		else  {
//		printf("req type : %d, req : %p\n", r->type_info->type, r);
			//printf("not!!\n");
			client_write(r->c, "$-1\r\n", 5);
			client_flush_offset(r->c, 0);
			client_clear(r->c);
			free_req(r);
		}

	//		end_req(r);
	}
		/*
		pthread_mutex_lock(&end_mutex[idx]);
		if (end_req_Q[idx].empty()) {
			pthread_mutex_unlock(&end_mutex[idx]);
			continue;
		}
		r = end_req_Q[idx].front();
		end_req_Q[idx].pop();
		pthread_mutex_unlock(&end_mutex[idx]);
		*/

//		printf("mem_llm1\n");
//		r = end_req_Q.front();
//		printf("back %p\n",end_r);
//		end_req_Q.pop();
//		printf("pop\n");
//		pthread_mutex_unlock(&end_req_mutex);
//		br = end_r->br;
//		bd = end_r->bd;
//		r = (lsmtree_req_t*)br->req;
//		printf("end_req_hand br : %p , bd : %p, lsm_req : %p\n",br,bd,r);

		
		//r->end_req(r);
//		end_req(r);

	//	printf("r : %p c : %p\n", r, r->c);
	//	client_flush_offset(r->c,0);
	//	client_clear(r->c);
/*
		client_write_bulk(r->c, data, 8192);
		r->c->current_items--;
		if ( r->c->current_items == 0 || r->c->output_len >= 160000) {
	//		printf("req : %p, req->c :%p\n",req,req->c);
			client_flush_offset(r->c, 0);
			client_clear(r->c);
		//				printf("flush");
		}
			free_req(r);
*/
//		free_req(r);
//		__memio_free_llm_req((memio_t*)bd->private_data,br);
//		printf("mem_llm2\n");
//		free(end_r);
//		printf("mem_llm3\n");
//		gettimeofday(&e,NULL);
//		t = 1000000*(e.tv_sec - s.tv_sec) + e.tv_usec - s.tv_usec;
//		printf("pop t : %llu\n",t);
//		gettimeofday(&s,NULL);
}

int on_read(client* c) {
	int nread;
	char *buffer = get_buffer(c);
	//printf("buf_idx : %d, buf_len : %d BUFSIZE :%d\n", c->buf_idx, c->buf_len, BUFSIZE - c->buf_idx - c->buf_len);
	nread = recv(c->fd, buffer, BUFSIZE - c->buf_idx - c->buf_len, 0);
//	nread = recv(c->fd, buffer, 19800 - c->buf_len,  0);
//	nread = recv(c->fd, buffer, 8237 + 10, 0);
//	nread = recv(c->fd, buffer, 19800, 0);
	if(c->time_valid) {
		TIME_END(&(clnt_time[c->fd].recv_end));
		TIME_TOTAL(clnt_time[c->fd].send_start, clnt_time[c->fd].recv_end, clnt_time[c->fd].send_recv);
	}
	TIME_START(&(clnt_time[c->fd].recv_start));

	//printf("nread : %d\nmsg:%s\n", nread, &buffer[c->buf_idx]);	
	//printf("nread : %d\n", nread);	
//	printf("!!!buf_idx : %d, buf_len : %d BUFSIZE :%d\n", c->buf_idx, c->buf_len, BUFSIZE - c->buf_idx - c->buf_len);
	if (nread < 0)
	{
			perror("  123recv() failed");
		if (errno != EWOULDBLOCK)
		{
			perror("  recv() failed");
			return TRUE;
		}
	}

	/*****************************************************/
	/* Check to see if the connection has been           */
	/* closed by the client                              */
	/*****************************************************/
	if (nread == 0)
	{
		printf("  Connection closed\n");
		if(c->buf_len != 0) printf("!!!!!!!!!!!!\n");
		return TRUE;
	}

	c->buf_len += nread;

	bool keep_alive = client_exec_commands(c);
	if (BUFSIZE - c->buf_idx - c->buf_len < 100000) {
		memcpy(c->buf, &(c->buf[c->buf_idx]), c->buf_len);
		c->buf_idx = 0;
	}
	TIME_END(&(clnt_time[c->fd].exec_end));
	TIME_TOTAL(clnt_time[c->fd].recv_start, clnt_time[c->fd].exec_end, clnt_time[c->fd].recv_exec);
//	if( (c->output_valid) && (c->buf_idx + c->buf_len == 0) ) {
	if( (c->output_valid)  ) {
		//printf("flush!!\n");
		client_flush_offset(c, c->output_offset);
		client_clear(c);
		TIME_START(&(clnt_time[c->fd].send_start));
		c->time_valid = 1;
		c->output_valid = 0;
	}
	if(!keep_alive) {
		printf("why?");
		return TRUE;
	}

	//while(!clnt_q->enqueue(c));
}

void polling_worker(int start_idx, int socks_num) {
	int    		len, rc, on = 1;
	int    		new_sd = -1;
	int    		desc_ready, end_server = FALSE, compress_array = FALSE;
	int    		close_conn;
	struct 		sockaddr_in   addr;
	//int    		timeout = 3 * 60 * 1000;
	int    		timeout = -1;
	int    		current_size = 0, i, j;
	int 		cnt = 0;

	struct		pollfd fds[MAX_CLNT];
	client*		clients[MAX_CLNT];
	int			nfds = socks_num;
	int 		listen_sd;
	int 		isListen_sd = 0;

	memset(fds, 0, sizeof(fds));
	for (i = 0; i < socks_num; i++) {
		fds[i].fd = listen_sds[start_idx+i];
		fds[i].events = POLLIN;
		clients[i] = NULL;
		printf("fds[i] : %d\n",fds[i].fd);
	}


	
	cpu_set_t cpuset;
   	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);
	pthread_t current_thread = pthread_self();
//	pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);   

//	end_server = TRUE;
	do {
		isListen_sd = 0;
		rc = poll(fds, nfds, timeout);

		if (rc < 0) {
			perror("  poll() failed");
			break;
		}

		if (rc == 0) {
			printf("  poll() timed out. End program.\n");
			break;
		}

		current_size = nfds;
		for (i = 0; i < current_size; i++) {
			if (fds[i].revents == 0)
				continue;

			if (fds[i].revents != POLLIN) {
				printf("  Error! revents = %d\n", fds[i].revents);
			}
			for(j = 0; j < socks_num; j++) {
				listen_sd = listen_sds[start_idx+j];
				if (fds[i].fd == listen_sd) {
					isListen_sd = 1;
					printf("  Listening socket is readable\n");

					do {
						new_sd = accept(listen_sd,NULL,NULL);
						if (new_sd < 0) {
							if (errno != EWOULDBLOCK) {
								perror("  accept() failed");
								end_server = TRUE;
							}
							break;
						}
						
						printf("new sd :%d\n",new_sd);
						
						int w =  32*1024 * 1024;

						if (setsockopt(new_sd, SOL_SOCKET, SO_SNDBUF, (char *)&w, sizeof (w))) {
							printf("SNDBUF failed\n");
							close(listen_sd);
							exit(1);
						}

						if (setsockopt(new_sd, SOL_SOCKET, SO_RCVBUF, (char *)&w, sizeof (w))) {
							printf("SNDBUF failed\n");
							close(listen_sd);
							exit(1);
						}

						if (1) {
							int nodelay = 1;
							if (setsockopt(new_sd, IPPROTO_TCP, TCP_NODELAY, (char*)&nodelay,sizeof(nodelay))) {
								printf("NODELAY failed\n");
								close(listen_sd);
								exit(1);
							}
						}
						
						int rc;
						int on = 0;
						/*
						rc = ioctl(new_sd, FIONBIO, (char *)&on);
						if (rc < 0)
						{
							perror("ioctl() failed");
							close(listen_sds[i]);
							exit(-1);
						}
						*/
						int fl = fcntl(new_sd, F_GETFL, 0);
						fl |= O_NONBLOCK;
						printf("fl : %d\n", fl);
						if (fl & O_NONBLOCK) printf("NON!!!!\n");
						fcntl(new_sd, F_SETFL, fl);
						fl = fcntl(new_sd, F_GETFL, 0);
						printf("fl : %d\n", fl);
						if (!(fl & O_NONBLOCK)) printf("BLOCK!!!!\n");
						
						fds[nfds].fd = new_sd;
						fds[nfds].events = POLLIN;
						clients[nfds] = client_new();
						clients[nfds]->fd = new_sd;
						clients[nfds]->which_queue = j;
						printf("which  : %d\n", j);
						nfds++;

					} while (new_sd != -1);
				}
			}
			if (!isListen_sd) {
				isListen_sd = 0;
				close_conn = FALSE;
				struct timeval s,e;
				unsigned long long int t;
				do {
					//printf("poll\n");
#ifdef TIME
					gettimeofday(&s,NULL);
#endif
					close_conn = on_read(clients[i]);
//					int ret = write(fds[i].fd, "hihihi\r\n",8);
//					if(ret < 0 ) perror("asd\n");
#ifdef TIME
					gettimeofday(&e,NULL);
					t = 1000000 * (e.tv_sec - s.tv_sec) + e.tv_usec - s.tv_usec;
					printf("on_read : %llu\n",t);
#endif
					break;
				} while(TRUE);

				if (close_conn) {
					close(fds[i].fd);
					client_close(clients[i]);
					fds[i].fd = -1;
					compress_array = TRUE;
				}
			}
		}

		if (compress_array) {
			compress_array = FALSE;
			for (i = 0; i < nfds; i++) {
				if (fds[i].fd == -1) {
					for (j = i; j < nfds; j++) {
						fds[j].fd = fds[j+1].fd;
						clients[j] = clients[j+1];
					}
					nfds--;
				}
			}
		}

	} while (end_server == FALSE);
	/*
	   for (i = 0; i < nfds; i++) {
	   if (fds[i].fd >= 0) {
	   close(fds[i].fd);
	   client_close(clients[i]);
	   }
	   }
	   */
	//	printf("nfds polling :%d\n\n",nfds);
}



void* excute_polling_worker(void *arg) {
	int i;
	int num = PORT_NUM / THREAD_NUM;
	int start_idx = *((int*)arg);
	printf("start idx : %d, num : %d\n",start_idx, num);
	polling_worker(start_idx, num);

	return NULL;
}

void init_socket(int num) {
	int 		rc, on = 1;
	int 		i;
	struct 		sockaddr_in   addr;
	int    		timeout;
	struct 		pollfd fds[MAX_CLNT];
	int 		port = 7777;

	listen_sds = (int*)malloc(num * sizeof(int));

	for (i = 0; i < num; i++) {
		listen_sds[i] = socket(AF_INET, SOCK_STREAM, 0);

		if (listen_sds[i] < 0) {
			perror("socket() failed");
			exit(-1);
		}

		rc = setsockopt(listen_sds[i], SOL_SOCKET,  SO_REUSEADDR,
				(char *)&on, sizeof(on));
		if (rc < 0)
		{
			perror("setsockopt() failed");
			close(listen_sds[i]);
			exit(-1);
		}

		int w = 1024 * 1024;
		//int w = 2048;

		if (setsockopt(listen_sds[i], SOL_SOCKET, SO_SNDBUF, (char *)&w, sizeof (w))) {
			printf("SNDBUF failed\n");
			close(listen_sds[i]);
			exit(1);
		}

		if (setsockopt(listen_sds[i], SOL_SOCKET, SO_RCVBUF, (char *)&w, sizeof (w))) {
			printf("SNDBUF failed\n");
			close(listen_sds[i]);
			exit(1);
		}

		/*************************************************************/
		/* Set socket to be nonblocking. All of the sockets for    */
		/* the incoming connections will also be nonblocking since  */
		/* they will inherit that state from the listening socket.   */
		/*************************************************************/
		/*
		rc = ioctl(listen_sds[i], FIONBIO, (char *)&on);
		if (rc < 0)
		{
			perror("ioctl() failed");
			close(listen_sds[i]);
			exit(-1);
		}
		*/

		int fl = fcntl(listen_sds[i], F_GETFL);
		fcntl(listen_sds[i], F_SETFL, fl | O_NONBLOCK);
		/*************************************************************/
		/* Bind the socket                                           */
		/*************************************************************/
		memset(&addr, 0, sizeof(addr));
		addr.sin_family      = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		//addr.sin_addr.s_addr = inet_addr("10.0.0.5");
		addr.sin_port        = htons(port+i);

		rc = bind(listen_sds[i],
				(struct sockaddr *)&addr, sizeof(addr));
		if (rc < 0)
		{
			perror("bind() failed");
			close(listen_sds[i]);
			exit(-1);
		}

		/*************************************************************/
		/* Set the listen back log                                   */
		/*************************************************************/
		rc = listen(listen_sds[i], 32);
		printf("sd : %d",listen_sds[i]);
		if (rc < 0)
		{
			perror("listen() failed");
			close(listen_sds[i]);
			exit(-1);
		}

		/*************************************************************/
		/* Initialize the pollfd structure                           */
		/*************************************************************/
		//	memset(fds, 0 , sizeof(fds));

		/*************************************************************/
		/* Set up the initial listening socket                        */
		/*************************************************************/
		//	fds[0].fd = listen_sd;
		//	fds[0].events = POLLIN;
		/*************************************************************/
		/* Initialize the timeout to 3 minutes. If no               */
		/* activity after 3 minutes this program will end.           */
		/* timeout value is based on milliseconds.                   */
		/*************************************************************/
		//	timeout = (3 * 60 * 1000);

	}
}

MeasureTime nh;
int main (int argc, char *argv[])
{
	int    		len, rc, on = 1;
	int 		num = THREAD_NUM;
	int 		port_num = PORT_NUM / THREAD_NUM;
	int 		i, start_idx[num];
	volatile int st = 0;
	pthread_t 	t_id[num];
	pthread_t 	tid[QUEUE_NUM];
	int 		q_idx[QUEUE_NUM];
//	reply = (char*)malloc(8201);
//	sprintf(reply,"$8192\r\n");
//	reply[8190] = '\r';
//	reply[8191] = '\n';
	for(int i=0; i < QUEUE_NUM; i++) {
		q_idx[i] = i;
		end_req_Q[i] = new mpmc_bounded_queue_t<req_t*>(1024 * 4);
		//end_req_Q[i] = new spsc_bounded_queue_t<req_t*>(1024 * 4);
//		pthread_mutex_init(&end_mutex[i],NULL);
		pthread_create(&tid[i], NULL, end_req_q_handler, (void*)(&q_idx[i]));
	}

//#ifdef ENABLE_LIBFTL
//	if(st)
	g_hashMap = new map<uint64_t, unsigned int>();
	g_hashValues = new set<unsigned int>();

	lr_inter_init();
	measure_init(&nh);
//	pthread_create(&tid, NULL, end_req_q_handler, NULL);
//#endif
	_req_cnt=0;
	_req_cnt_get=0;
	req_max = 0;
	init_socket(PORT_NUM);
	
	//make_fd = open("make_fd",O_CREAT|O_TRUNC|O_WRONLY);
	//end_fd = open("end_fd",O_CREAT|O_TRUNC|O_WRONLY);
	//total_fd = open("total_fd",O_CREAT|O_TRUNC|O_WRONLY);

	for(i = 0; i < num; i++) {
		start_idx[i] = i * port_num;
		pthread_create(&t_id[i], NULL, excute_polling_worker, (void*)(&start_idx[i]));
	}

	for(i =0; i < num; i++) {
		pthread_join(t_id[i],NULL);
	}
	for(i=0; i<QUEUE_NUM; i++) 
		pthread_join(tid[i],NULL);


	/*
	   cpu_set_t cpuset;
	   CPU_ZERO(&cpuset);
	   CPU_SET(0, &cpuset);
	   pthread_t current_thread = pthread_self();
	   pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);

	   pthread_t t_id;
	   clnt_q = new spsc_bounded_queue_t<client*>(2048);
	   pthread_create(&t_id, NULL, worker, NULL);
	   pthread_detach(t_id);
	   */	
	//	clients[0] = NULL;
	/*************************************************************/
	/* Loop waiting for incoming connects or for incoming data   */
	/* on any of the connected sockets.                          */
	/*************************************************************/
	return 0;
}

