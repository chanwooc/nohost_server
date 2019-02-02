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
#include "request.h"
#include "LR_inter.h"
#include "lockfreeq.h"
using namespace std;

#define THREAD_NUM 2
#define PORT_NUM 4

time_check clnt_time[MAX_CLNT];
spsc_bounded_queue_t<client*> *clnt_q;

const char *ERR_INCOMPLETE = "incomplete";
const char *ERR_QUIT = "quit";

typedef struct thread_arg {
	int listen_sd;
	int thread_num;
}thread_arg;

pthread_mutex_t fd_mutex = PTHREAD_MUTEX_INITIALIZER;


char* get_buffer(client* c) {
	return c->buf + c->buf_idx + c->buf_len;
}

int on_read(client* c) {
	int nread;
	char *buffer = get_buffer(c);
	nread = recv(c->fd, buffer, BUFSIZE, 0);
	if(c->time_valid) {
		TIME_END(&(clnt_time[c->fd].recv_end));
		TIME_TOTAL(clnt_time[c->fd].send_start, clnt_time[c->fd].recv_end, clnt_time[c->fd].send_recv);
	}
	TIME_START(&(clnt_time[c->fd].recv_start));
	if (nread < 0)
	{
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
		return TRUE;
	}

	//printf("nread : %d\n", nread);	
	c->buf_len += nread;

	bool keep_alive = client_exec_commands(c);
	TIME_END(&(clnt_time[c->fd].exec_end));
	TIME_TOTAL(clnt_time[c->fd].recv_start, clnt_time[c->fd].exec_end, clnt_time[c->fd].recv_exec);
	if( (c->output_valid) && (c->buf_idx + c->buf_len == 0) ) {
		//printf("flush!!\n");
		client_flush_offset(c, c->output_offset);
		client_clear(c);
		TIME_START(&(clnt_time[c->fd].send_start));
		c->time_valid = 1;
	}
	c->output_valid = 0;
	if(!keep_alive) {
		return TRUE;
	}

	//while(!clnt_q->enqueue(c));
}

void* worker(void* arg) {
	client *c; 
	/*
	   cpu_set_t cpuset;
	   CPU_ZERO(&cpuset);
	   CPU_SET(1, &cpuset);
	   pthread_t current_thread = pthread_self();
	   pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	 */
	while(1) {
		while(!clnt_q->dequeue(&c));
		client_exec_commands(c);
		if ( (c->output_valid) && (c->buf_idx + c->buf_len == 0) ) {
			client_flush_offset(c, c->output_offset);
			client_clear(c);
			c->time_valid = 1;
		}
		c->output_valid = 0;
	}
}

void* polling_worker(void *arg) {
	int    		len, rc, on = 1;
	int    		new_sd = -1;
	int    		desc_ready, end_server = FALSE, compress_array = FALSE;
	int    		close_conn;
	struct 		sockaddr_in   addr;
	int    		timeout = 3 * 60 * 1000;
	int    		current_size = 0, i, j;
	int 		cnt = 0;
	struct		pollfd fds[MAX_CLNT];
	client*		clients[MAX_CLNT];
	int			nfds = 1;
	int 		listen_sd = *((int*)arg);
	memset(fds, 0, sizeof(fds));
	/*
	   cpu_set_t cpuset;
	   CPU_ZERO(&cpuset);
	   CPU_SET(thread_num, &cpuset);
	   pthread_t current_thread = pthread_self();
	   pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
	 */
	fds[0].fd = listen_sd;
	fds[0].events = POLLIN;
	clients[0] = NULL;

	do {
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

			if (fds[i].fd == listen_sd) {
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

					int w = 1024 * 1024;

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

					fds[nfds].fd = new_sd;
					fds[nfds].events = POLLIN;
					clients[nfds] = client_new();
					clients[nfds]->fd = new_sd;
					nfds++;

				} while (new_sd != -1);
			}
			else {
				close_conn = FALSE;

				do {
					close_conn = on_read(clients[i]);
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

	for (i = 0; i < nfds; i++) {
		if (fds[i].fd >= 0) {
			close(fds[i].fd);
			client_close(clients[i]);
		}
	}
}
/*
void* excute_polling_worker(void *arg) {
	int start_sd = *((int*)arg);
//hile(
i
*/

int* init_socket(int num) {
	int 		rc, on = 1;
	int 		*listen_sd; 
	int 		i;
	struct 		sockaddr_in   addr;
	int    		timeout;
	struct 		pollfd fds[MAX_CLNT];
	int 		port = 8888;

	listen_sd = (int*)malloc(num * sizeof(int));

	for (i = 0; i < num; i++) {
		listen_sd[i] = socket(AF_INET, SOCK_STREAM, 0);

		if (listen_sd[i] < 0) {
			perror("socket() failed");
			exit(-1);
		}

		rc = setsockopt(listen_sd[i], SOL_SOCKET,  SO_REUSEADDR,
				(char *)&on, sizeof(on));
		if (rc < 0)
		{
			perror("setsockopt() failed");
			close(listen_sd[i]);
			exit(-1);
		}

		int w = 1024 * 1024;
		//int w = 2048;

		if (setsockopt(listen_sd[i], SOL_SOCKET, SO_SNDBUF, (char *)&w, sizeof (w))) {
			printf("SNDBUF failed\n");
			close(listen_sd[i]);
			exit(1);
		}

		if (setsockopt(listen_sd[i], SOL_SOCKET, SO_RCVBUF, (char *)&w, sizeof (w))) {
			printf("SNDBUF failed\n");
			close(listen_sd[i]);
			exit(1);
		}

		/*************************************************************/
		/* Set socket to be nonblocking. All of the sockets for    */
		/* the incoming connections will also be nonblocking since  */
		/* they will inherit that state from the listening socket.   */
		/*************************************************************/
		rc = ioctl(listen_sd[i], FIONBIO, (char *)&on);
		if (rc < 0)
		{
			perror("ioctl() failed");
			close(listen_sd[i]);
			exit(-1);
		}

		int fl = fcntl(listen_sd[i], F_GETFL);
		fcntl(listen_sd[i], F_SETFL, fl | O_NONBLOCK);

		/*************************************************************/
		/* Bind the socket                                           */
		/*************************************************************/
		memset(&addr, 0, sizeof(addr));
		addr.sin_family      = AF_INET;
		addr.sin_addr.s_addr = htonl(INADDR_ANY);
		//addr.sin_addr.s_addr = inet_addr("10.0.0.5");
		addr.sin_port        = htons(port+i);

		rc = bind(listen_sd[i],
				(struct sockaddr *)&addr, sizeof(addr));
		if (rc < 0)
		{
			perror("bind() failed");
			close(listen_sd[i]);
			exit(-1);
		}

		/*************************************************************/
		/* Set the listen back log                                   */
		/*************************************************************/
		rc = listen(listen_sd[i], 32);
		if (rc < 0)
		{
			perror("listen() failed");
			close(listen_sd[i]);
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

	return listen_sd;

}

int main (int argc, char *argv[])
{
	int    		len, rc, on = 1;
	int 		num = PORT_NUM;
	int 		*listen_sd, i;
	pthread_t 	t_id[num];
//	lr_inter_init();	

	listen_sd = init_socket(num);

	for(i = 0; i < num; i++) {
		pthread_create(&t_id[i], NULL, polling_worker, (void*)(&listen_sd[i]));
	}

	for(i =0; i < num; i++) {
		pthread_join(t_id[i],NULL);
	}


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

