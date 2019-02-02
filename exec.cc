//#include "server.h"
#include "request.h"
#include "hashMap.h"
#include "lfmpmc.h"
using namespace std;

extern map<uint64_t, unsigned int> *g_hashMap;
extern set<unsigned int> *g_hashValues;
extern mpmc_bounded_queue_t<req_t*> *end_req_Q[QUEUE_NUM];

int req_sw = 0;

static bool islstr(client *c, int arg_idx, const char *str){
	int i = 0;
	for (;i<c->args_size[arg_idx];i++){
		if (c->args[arg_idx][i] != str[i] && c->args[arg_idx][i] != str[i]-32){
			return false;
		}
	}
	return !str[i];
}

static bool iscmd(client *c, const char *cmd){
	return islstr(c, 0, cmd);
}

error exec_set(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	static int __cnt = 0;
	char* stop;
	//char real_data[8192];
	if (argc!=3){
		return "wrong number of arguments for 'set' command";
	}	
#ifdef LSMTREE
	req_t *req = alloc_req();
	req->c = c;
	req->key_info->key = (uint64_t)(atoi(argv[1]+4));
	//req->key_info->key = strtoull(argv[1]+4, &stop, 10);
	//req->key_info->key = hashMapInsert(g_hashMap, g_hashValues, req->key_info->key,1);
	req->type_info->type = 1;
#ifdef ENABLE_LIBFTL
	alloc_dma(req);
#endif
	if(argl[2] > 8192) printf("safasdljkwqaeklajxd\n");
	memcpy(req->value_info->value, argv[2], argl[2]);
	//memcpy(req->value_info->value, real_data, 8192);
	//pthread_mutex_lock(c->mutex);
	//c->current_items++;
	//pthread_mutex_unlock(c->mutex);
	make_req(req);
#endif
	//printf("exec_set %d\n",++__cnt);
	//__cnt++;
	//if (__cnt <= 2000000) {
	client_write(c, "+OK\r\n", 5);
	c->output_valid = 1;
	//}
	//while(!end_req_Q[req->c->which_queue%QUEUE_NUM]->enqueue(req));	
	return NULL;
}

error exec_upt(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	static int __cnt = 0;
	if (argc!=3){
		return "wrong number of arguments for 'set' command";
	}	
#ifdef LSMTREE
	req_t *req = alloc_req();
	req->c = c;
	req->key_info->key = (uint64_t)(atoi(argv[1]+4));
	req->type_info->type = 1;
#ifdef ENABLE_LIBFTL
	alloc_dma(req);
#endif
	if(argl[2] > 8192) printf("safasdljkwqaeklajxd\n");
	memcpy(req->value_info->value, argv[2], argl[2]);
	//pthread_mutex_lock(c->mutex);
	//c->current_items++;
	//pthread_mutex_unlock(c->mutex);
	make_req(req);
#endif
	//printf("exec_set %d\n",++__cnt);
	client_write(c, "+KO\r\n", 5);
	c->output_valid = 1;
	return NULL;
}

error exec_get(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	//static int a = 0;
	char* stop;
	//a++;
	if (argc!=2){
		return "wrong number of arguments for 'get' command";
	}	
	//printf("get!! %d", a);
#ifdef LSMTREE
	req_t *req = alloc_req();
	req->key_info->key = (uint64_t)(atoi(argv[1]+4));
	//req->key_info->key = strtoull(argv[1]+4, &stop, 10);
	//req->key_info->key = hashMapInsert(g_hashMap, g_hashValues, req->key_info->key,0);

	req->type_info->type = 2;

	req->c = c;
#ifdef ENABLE_LIBFTL
	alloc_dma(req);
#endif
//	pthread_mutex_lock(c->mutex);	
//	c->current_items++;
//	pthread_mutex_unlock(c->mutex);
	make_req(req);
#endif
	return NULL;
}

error exec_mset(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	int i;

	pthread_mutex_lock(c->mutex);
//	c->current_items += argc / 2;
//	printf("exec mset%d :%d\n",c->fd, c->current_items);
	pthread_mutex_unlock(c->mutex);

	for (i = 1; i < argc; i+=2) {
#ifdef LSMTREE
		req_t *req = alloc_req();
		req->c = c;
		req->key_info->key = (uint64_t)(atoi(argv[i]+4));
		req->type_info->type = 1;
#ifdef ENABLE_LIBFTL
		alloc_dma(req);
#endif
		//memcpy(req->value_info->value, argv[i+1], argl[i+1]);
		make_req(req);
#endif
		//	printf("exec_set\n");
	}
	client_write(c, "+OK\r\n", 5);
	c->output_valid = 1;
	return NULL;
}

error exec_mget(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	int i;
	char msg[10] = {0,};
//	client_print_args(c);
	pthread_mutex_lock(c->mutex);
	c->current_items += argc - 1;
	printf("exec mget%d :%d\n",c->fd, c->current_items);
	pthread_mutex_unlock(c->mutex);
	sprintf(msg, "*%d\r\n",argc-1);
	client_write(c, msg, strlen(msg));

	for (i = 1; i < argc; i++) {
#ifdef LSMTREE
		req_t *req = alloc_req();
		req->key_info->key = (uint64_t)(atoi(argv[i]+4));
		req->type_info->type = 2;
		req->c = c;
#ifdef ENABLE_LIBFTL
		alloc_dma(req);
#endif
		make_req(req);
#endif
	}

	return NULL;
}
/*
error exec_del(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	if (argc!=2){
		return "wrong number of arguments for 'del' command";
	}
	client_write(c, ":0\r\n", 4);
	c->output_valid = 1;
	return NULL;
}
*/
error exec_del(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	static int __cnt = 0;
	if (argc!=2){
		return "wrong number of arguments for 'set' command";
	}	
#ifdef LSMTREE
	req_t *req = alloc_req();
	req->c = c;
	req->key_info->key = (uint64_t)(atoi(argv[1]+4));
	req->type_info->type = 3;
#ifdef ENABLE_LIBFTL
	//alloc_dma(req);
#endif
	//if(argl[2] > 8192) printf("safasdljkwqaeklajxd\n");
	//memcpy(req->value_info->value, argv[2], argl[2]);
	//pthread_mutex_lock(c->mutex);
	//c->current_items++;
	//pthread_mutex_unlock(c->mutex);
	make_req(req);
#endif
	//printf("exec_set %d\n",++__cnt);
	client_write(c, "+KO\r\n", 5);
	c->output_valid = 1;
	return NULL;
}

error exec_quit(client *c){
	client_write(c, "+OK\r\n", 5);
	c->output_valid = 1;
	return ERR_QUIT;
}

error exec_flushdb(client *c){
	if (c->args_len!=1){
		return "wrong number of arguments for 'flushdb' command";
	}
	//flushdb();
	client_write(c, "+OK\r\n", 5);
	c->output_valid = 1;
	return NULL;
}

static error exec_scan_keys(client *c, 
		bool scan,
		const char *pat, int pat_len, 
		int cursor, int count
		){
	if (count < 0){
		count = 10;
	}
	if (cursor < 0){
		cursor = 0;
	}

	char *start = NULL;
	char *end = NULL;
	int start_len = 0;
	int end_len = 0;
	int star = pattern_limits(pat, pat_len, &start, &start_len, &end, &end_len);
	std::string prefix(start, start_len);
	std::string postfix(end, end_len);

	// to avoid double-buffering, prewrite some bytes and then we'll go back 
	// and fill it in with correctness.
	const int filler = 128;
	for (int i=0;i<filler;i++){
		client_write_byte(c, '?');
	}
	int total = 0;
	int i = 0;
	int ncursor = 0;
	/*
	   rocksdb::Iterator* it = db->NewIterator(rocksdb::ReadOptions());
	   if (star){
	   it->SeekToFirst();
	   }else{
	   it->Seek(prefix);
	   }
	   for (; it->Valid(); it->Next()) {
	   rocksdb::Slice key = it->key();
	   if (stringmatchlen(pat, pat_len, key.data(), key.size(), 1)){
	   if (!star){
	   int res = key.compare(postfix);
	   if (res>=0){
	   break;
	   }
	   }
	   if (i >= cursor){
	   if (scan&&total==count){
	   ncursor = i;
	   break;
	   }
	   client_write_bulk(c, key.data(), key.size());
	   total++;	
	   }
	   i++;
	   }
	   }
	   if (start){
	   free(start);
	   }
	   if (end){
	   free(end);
	   }

	   rocksdb::Status s = it->status();
	   if (!s.ok()){
	   err(1, "%s", s.ToString().c_str());	
	   }
	   delete it;
	   */
	// fill in the header and write from offset.
	char nb[filler];
	if (scan){
		char cursor_s[32];
		sprintf(cursor_s, "%d", ncursor);
		sprintf(nb, "*2\r\n$%zu\r\n%s\r\n*%d\r\n", strlen(cursor_s), cursor_s, total);
	}else{
		sprintf(nb, "*%d\r\n", total);
	}
	int nbn = strlen(nb);
	memcpy(c->output+filler-nbn, nb, nbn);
	c->output_offset = filler-nbn;

	return NULL;
}

error exec_keys(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	if (argc!=2){
		return "wrong number of arguments for 'keys' command";
	}
	return exec_scan_keys(c, false, argv[1], argl[1], -1, -1);
}
error exec_scan(client *c){
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;
	if (argc<2){
		return "wrong number of arguments for 'scan' command";
	}
	int cursor = atop(argv[1], argl[1]);
	if (cursor < 0){
		return "invalid cursor";
	}
	int count = -1;
	const char *pat = "*";
	int pat_len = 1;

	for (int i=2;i<argc;i++){
		if (islstr(c, i, "match")){
			i++;
			if (i==argc){
				return "syntax error";
			}
			pat = argv[i];
			pat_len = argl[i];
		}else if (islstr(c, i, "count")){
			i++;
			if (i==argc){
				return "syntax error";
			}
			count = atop(argv[i], argl[i]);
			if (count < 0){
				return "value is not an integer or out of range";
			}
		}else{
			return "syntax error";
		}
	}
	return exec_scan_keys(c, true, pat, pat_len, cursor, count);
}

error exec_ok(client *c) {
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;

	if (argc != 1) {
		return "wrong number of arguments for 'ok' command";
	}
	
	return NULL;
}

error exec_ko(client *c) {
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;

	if (argc != 1) {
		return "wrong number of arguments for 'ok' command";
	}
	
	return NULL;

}

error exec_bulk(client *c) {
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;

	if (argc != 2) {
		return "wrong number of arguments for 'bulk' command";
	}
	
	return NULL;
}

error exec_not(client *c) {
	const char **argv = c->args;
	int *argl = c->args_size;
	int argc = c->args_len;

	if (argc != 1) {
		return "wrong number of arguments for 'not' command";
	}
	
	return NULL;
}

error exec_command(client *c){
	if (c->args_len==0||(c->args_len==1&&c->args_size[0]==0)){
		return NULL;
	}
	if (iscmd(c, "set")){
		return exec_set(c);
	}else if (iscmd(c, "mset")){
		return exec_mset(c);
	}else if (iscmd(c, "get")){
		return exec_get(c);
	}else if (iscmd(c, "mget")){
		return exec_mget(c);
	}else if (iscmd(c, "del")){
		return exec_del(c);
	}else if (iscmd(c, "quit")){
		return exec_quit(c);
	}else if (iscmd(c, "keys")){
		return exec_keys(c);
	}else if (iscmd(c, "scan")){
		return exec_scan(c);
	}else if (iscmd(c, "flushdb")){
		return exec_flushdb(c);
	}else if (iscmd(c, "upt")){
		return exec_upt(c);
	}else if (iscmd(c, "OK")) {
		return exec_ok(c);
	}else if (iscmd(c, "BULK")) {
		return exec_bulk(c);
	}else if (iscmd(c, "NOT")) {
		return exec_not(c);
	}else if (iscmd(c, "KO")) {
		return exec_ko(c);
	}
	return client_err_unknown_command(c, c->args[0], c->args_size[0]);
}

