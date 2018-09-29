#ifndef _FRAG_REDIS_H
#define _FRAG_REDIS_H


#ifdef __cplusplus
extern "C" {
#endif

int redis_excute_command(int thread_seq, void* logger, redisReply** reply, char* cmmd, int argc, const char **argv, const size_t *argvlen);

#ifdef __cplusplus
}
#endif

#endif
