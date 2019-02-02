#include "server.h"
#include <pthread.h>

pthread_mutex_t flush_mtx = PTHREAD_MUTEX_INITIALIZER;

client *client_new(){
	client *c = (client*)calloc(1, sizeof(client));
	if (!c){
		err(1, "malloc");
	}
	c->mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	c->buf = (char*)malloc(BUFSIZE);
	c->output = (char*)malloc(OUTPUT_BUFSIZE);
	pthread_mutex_init(c->mutex, NULL); 
	printf("clnt new\n");
	return c;
}

void client_free(client *c){
	if (!c){
		return;
	}
	if (c->buf){
		free(c->buf);
	}
	if (c->args){
		free(c->args);
	}
	if (c->args_size){
		free(c->args_size);
	}
	if (c->output){
		free(c->output);
	}
	if (c->tmp_err){
		free(c->tmp_err);
	}
	if (c->mutex) {
		pthread_mutex_destroy(c->mutex);
	}
	free(c);
}


void client_close(client *c){
	client_free(c);
}

inline void client_output_require(client *c, size_t siz){
	/*
	if (c->output_cap < siz){
		while (c->output_cap < siz){
			if (c->output_cap == 0){
				c->output_cap = 1;
			}else{
				c->output_cap *= 2;
			}
		}
		c->output = (char*)realloc(c->output, c->output_cap);
		if (!c->output){
			err(1, "malloc");
		}
	}
	*/
}
void client_write(client *c, const char *data, int n){
	client_output_require(c, c->output_len+n);
	memcpy(c->output+c->output_len, data, n);	
	c->output_len+=n;
}

void client_write_data(client *c, const char *data, int n){
	client_output_require(c, c->output_len+n);
	memcpy(c->output+c->output_len, data, n);	
	c->output_len+=n;
}

void client_clear(client *c){
	c->output_len = 0;
	c->output_offset = 0;
}

void client_write_byte(client *c, char b){
	client_output_require(c, c->output_len+1);
	c->output[c->output_len++] = b;
}

void client_write_bulk(client *c, const char *data, int n){
	char h[32];
	//static int bulk_cnt = 0;
	//bulk_cnt += 8201;
	//printf("bulk_cnt:%d\n",bulk_cnt);
	sprintf(h, "$%d\r\n", n);
//	char *t = "\r\n";
//	struct iovec iov[3];
//	ssize_t nwritten;
/*
	iov[0].iov_base = h;
	iov[0].iov_len = 7;
	iov[1].iov_base = data;
	iov[1].iov_len = 8192;
	iov[2].iov_base = data;
	iov[2].iov_len = 2;

	nwritten = writev(STDOUT_FILENO, iov, 3);
	*/
	client_write(c, h, strlen(h));
	client_write_data(c, data, n);
	client_write_byte(c, '\r');
	client_write_byte(c, '\n');
}

void client_write_multibulk(client *c, int n){
	char h[32];
	sprintf(h, "*%d\r\n", n);
	client_write(c, h, strlen(h));
}

void client_write_int(client *c, int n){
	char h[32];
	sprintf(h, ":%d\r\n", n);
	client_write(c, h, strlen(h));
}

void client_write_error(client *c, error err){
	client_write(c, "-ERR ", 5);
	client_write(c, err, strlen(err));
	client_write_byte(c, '\r');
	client_write_byte(c, '\n');
}


void client_flush_offset(client *c, int offset){
	//static int total_size = 0;
	if (c->output_len-offset <= 0){
		return;
	}
	ssize_t ret;
	size_t sent = 0;
	size_t size = c->output_len - offset;
	//pthread_mutex_lock(&flush_mtx);
	//total_size += size;
//	printf("%d\n%s\n",c->output_len,c->output);
	/*
	uv_buf_t buf = {0};
	buf.base = c->output+offset;
	buf.len = c->output_len-offset;
	uv_write(&c->req, (uv_stream_t *)&c->tcp, &buf, 1, NULL);
	*/
#ifdef TIME
	struct timeval start, end;
	gettimeofday(&start,NULL);
#endif
	while ( sent < size ) {
		ret = write(c->fd, c->output + offset, c->output_len - offset);
		if (ret < 0 && errno != EWOULDBLOCK) {
			perror("flush_offset");
			exit(1);
		}
		//else if (errno == EWOULDBLOCK) 
			//printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!%d\n",ret);
		//else printf("write:%d\n",ret);
		sent += ret;
		offset += ret;
	}
#ifdef TIME
	gettimeofday(&end,NULL);
	long long unsigned int total = end.tv_usec - start.tv_usec;
	printf("flush : %llu\n", total);
#endif
	c->output_len = 0;
	//printf("total_size:%d\n", total_size);
	//pthread_mutex_unlock(&flush_mtx);
}


void client_flush(client *c){
	client_flush_offset(c, 0);
}

void client_err_alloc(client *c, int n){
	if (c->tmp_err){
		free(c->tmp_err);
	}
	c->tmp_err = (char*)malloc(n);
	if (!c->tmp_err){
		err(1, "malloc");
	}
	memset(c->tmp_err, 0, n);
}

error client_err_expected_got(client *c, char c1, char c2){
	client_err_alloc(c, 64);
	sprintf(c->tmp_err, "Protocol error: expected '%c', got '%c'", c1, c2);
	return c->tmp_err;
}

error client_err_unknown_command(client *c, const char *name, int count){
	client_err_alloc(c, count+64);
	c->tmp_err[0] = 0;
	strcat(c->tmp_err, "unknown command '");
	strncat(c->tmp_err, name, count);
	strcat(c->tmp_err, "'");
	return c->tmp_err;
}
void client_append_arg(client *c, const char *data, int nbyte){
	if (c->args_cap==c->args_len){
		if (c->args_cap==0){
			c->args_cap=1;
		}else{
			c->args_cap*=2;
		}
		c->args = (const char**)realloc(c->args, c->args_cap*sizeof(const char *));
	printf("alloc!!!\n");
		if (!c->args){
			err(1, "malloc");
		}
		c->args_size = (int*)realloc(c->args_size, c->args_cap*sizeof(int));
		if (!c->args_size){
			err(1, "malloc");
		}
	}
	c->args[c->args_len] = data;
	c->args_size[c->args_len] = nbyte;
	c->args_len++;
}

error client_parse_telnet_command(client *c){
	size_t i = c->buf_idx;
	size_t z = c->buf_len+c->buf_idx;
	if (i >= z){
		return ERR_INCOMPLETE;
	}
	c->args_len = 0;
	size_t s = i;
	bool first = true;
	for (;i<z;i++){
		if (c->buf[i]=='\'' || c->buf[i]=='\"'){
			if (!first){
				return "Protocol error: unbalanced quotes in request";
			}
			char b = c->buf[i];
			i++;
			s = i;
			for (;i<z;i++){
				if (c->buf[i] == b){
					if (i+1>=z||c->buf[i+1]==' '||c->buf[i+1]=='\r'||c->buf[i+1]=='\n'){
						client_append_arg(c, c->buf+s, i-s);
						i--;
					}else{
						return "Protocol error: unbalanced quotes in request";
					}
					break;
				}
			}
			i++;
			continue;
		}
		if (c->buf[i] == '\n'){
			if (!first){
				size_t e;
				if (i>s && c->buf[i-1] == '\r'){
					e = i-1;
				}else{
					e = i;
				}
				client_append_arg(c, c->buf+s, e-s);
			}
			i++;
			c->buf_len -= i-c->buf_idx;
			if (c->buf_len == 0){
				c->buf_idx = 0;
			}else{
				c->buf_idx = i;
			}
			return NULL;
		}
		if (c->buf[i] == ' '){
			if (!first){
				client_append_arg(c, c->buf+s, i-s);
				first = true;
			}
		}else{
			if (first){
				s = i;
				first = false;
			}
		}
	}
	return ERR_INCOMPLETE;
}

error client_read_command(client *c){
	c->args_len = 0;
	size_t i = c->buf_idx;
	size_t z = c->buf_idx+c->buf_len;
	int args_len, bulk_flag = 0, ok_flag = 0;
//	printf("first buf_idx : %d buf_len : %d\n", c->buf_idx, c->buf_len);
	if (i >= z){
		return ERR_INCOMPLETE;
	}
	if ( (c->buf[i] != '*') && (c->buf[i] != '$') && (c->buf[i] != '+')) {
		//printf("first buf_idx : %d buf_len : %d\n%s", c->buf_idx, c->buf_len, &c->buf[c->buf_idx]);
		return client_parse_telnet_command(c);
	}
	if (c->buf[i] == '$') {
		args_len = 1;
		bulk_flag = 1;
	}
	else if (c->buf[i] == '+') {
		i++;
		if (i >=z) {
			return ERR_INCOMPLETE;
		}
		if (c->buf_len < 5) {
			return ERR_INCOMPLETE;
		}
		char str[5];
		//memcpy(str, &c->buf[i],4);
		if (strncmp(&c->buf[i], "OK\r\n", 4) == 0) {
			client_append_arg(c, "OK", 2);
			c->buf_len -= 5;
			if (c->buf_len == 0) {
				c->buf_idx = 0;
			}
			else {
				c->buf_idx = i+4;
			}
			return NULL;
		}
		else if (strncmp(&c->buf[i], "KO\r\n", 4) == 0) {
			client_append_arg(c, "KO", 2);
			c->buf_len -= 5;
			if (c->buf_len == 0) {
				c->buf_idx = 0;
			}
			else {
				c->buf_idx = i+4;
			}
			return NULL;
		}
	}
	else {
		i++;
		args_len = 0;
		size_t s = i;
		for (;i < z;i++){
			if (c->buf[i]=='\n'){
				if (c->buf[i-1] !='\r'){
					return "Protocol error: invalid multibulk length";
				}
				c->buf[i-1] = 0;
				args_len = atoi(c->buf+s);
				c->buf[i-1] = '\r';
				if (args_len <= 0){
					if (args_len < 0 || i-s != 2){
						return "Protocol error: invalid multibulk length";
					}
				}
				i++;
				break;
			}
		}
	}
	if (i >= z){
		return ERR_INCOMPLETE;
	}
	for (int j=0;j<args_len;j++){
		if (i >= z){
			return ERR_INCOMPLETE;
		}
		if (c->buf[i] != '$'){
			printf("err\n");
			return client_err_expected_got(c, '$', c->buf[i]);
		}
		i++;
		int nsiz = 0;
		size_t s = i;
		for (;i < z;i++){
			if (c->buf[i]=='\n'){
				if (c->buf[i-1] !='\r'){
					return "Protocol error: invalid bulk length";
				}
				c->buf[i-1] = 0;
				nsiz = atoi(c->buf+s);
				c->buf[i-1] = '\r';
				if (nsiz <= 0){
					if (nsiz == -1) {
						client_append_arg(c, "NOT", 3);
						bulk_flag = 0;
						i += nsiz+2;
						break;
					}
					else if (nsiz < 0 || i-s != 2){
						return "Protocol error: invalid bulk length";
					}
				}
				i++;
				if (z-i < nsiz+2){
					return ERR_INCOMPLETE;
				}
				s = i;
				if (c->buf[s+nsiz] != '\r'){
					return "Protocol error: invalid bulk data";
				}
				if (c->buf[s+nsiz+1] != '\n'){
					return "Protocol error: invalid bulk data";
				}
				if (bulk_flag) {
					client_append_arg(c, "BULK", 4);
					bulk_flag = 0;
				}
				client_append_arg(c, c->buf+s, nsiz);
				i += nsiz+2;
				break;
			}
		}
	}
	c->buf_len -= i-c->buf_idx;
	if (c->buf_len == 0){
		c->buf_idx = 0;
	}
	else{
		c->buf_idx = i;
	}
	return NULL;
}

void client_print_args(client *c){
	printf("args[%d]:", c->args_len);
	for (int i=0;i<c->args_len;i++){
		printf(" [");
		for (int j=0;j<c->args_size[i];j++){
			printf("%c", c->args[i][j]);
		}
		printf("]");
	}
	printf("\n");
}

bool client_exec_commands(client *c){
	for (;;){
		error err = client_read_command(c);
		if (err != NULL){
			if ((char*)err == (char*)ERR_INCOMPLETE){
				return true;
			}
			client_write_error(c, err);
			return false;
		}
		//client_print_args(c);
		err = exec_command(c);
		if (err != NULL){
			if (err == ERR_QUIT){
				return false;
			}
			client_write_error(c, err);
			return true;
		}
	}
	return true;
}
