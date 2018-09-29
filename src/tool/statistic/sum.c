#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// brief Prog info. 
typedef struct prog {
    struct prog *prev;      // used for hash collision 
    struct prog *next;
    uint64_t prog_id;    
    uint64_t len; 
    uint64_t pkt_num;
} prog_t; 

// hash table bucket
typedef struct htable_bucket{
    prog_t *prev, *next;             // used for hash collision 
} htable_bucket_t; // sizeof = 16

typedef struct htable {
    htable_bucket_t *buckets;
    uint32_t size;             // size of hash table
} htable_t;


htable_t *htable_init(int hash_size)
{
	if (hash_size < 1) {
		fprintf(stderr, "invalid parameters");
		return NULL;
	}
	static htable_t s_ht;

	htable_t *ht = &s_ht; 

	ht->size = hash_size; 
	ht->buckets = (htable_bucket_t*)calloc(ht->size, sizeof(htable_bucket_t));
	if (NULL == ht->buckets) {
		fprintf(stderr, "calloc error");
		return NULL;
	}
	unsigned int idx;
	for (idx = 0; idx < ht->size; idx++) {
		ht->buckets[idx].prev = (prog_t*)&ht->buckets[idx];
		ht->buckets[idx].next = (prog_t*)&ht->buckets[idx];
	}

	return ht;
}

void htable_clear(htable_t *ht)
{
	if (NULL == ht)
		return;
	if (ht->buckets)
		free(ht->buckets);
}

inline uint32_t htable_key2index(htable_t *ht, uint64_t prog_id)
{
	return prog_id % ht->size ;
}

prog_t *htable_search (htable_t *ht, uint64_t prog_id)	
{
	// search in hash table 
	htable_bucket_t *htb = ht->buckets + htable_key2index(ht, prog_id);
	prog_t *p = (prog_t*)htb->next;
	while (p != (prog_t*)htb) {
		if(p->prog_id == prog_id)
			return p;
		p = p->next;
	}
	return NULL;
}

prog_t *htable_insert(htable_t *ht, uint64_t prog_id)
{
	htable_bucket_t *htb = ht->buckets + htable_key2index(ht, prog_id);

	prog_t *p = (prog_t*)calloc(1, sizeof(prog_t));
	if (NULL == p) {
		fprintf(stderr, "calloc error");
		return NULL;
	}
	p->next = htb->next;
	p->prev = (prog_t*)htb;
	htb->next->prev = p;
	htb->next = p;


	p->prog_id = prog_id;
	p->len = 0;
	p->pkt_num = 0;

	return p;
}
void htable_traverse (htable_t *ht)	
{
	// search in hash table 
	uint32_t i;
	
	for (i=0; i<ht->size; i++) {
		htable_bucket_t *htb = ht->buckets + i;
		prog_t *p = (prog_t*)htb->next;
		while (p != (prog_t*)htb) {
			printf("%lu %lu %lu\n", p->prog_id, p->pkt_num, p->len);
			p = p->next;
		}
	}
}
#define MAX_LINE 255

int main(int argc, char **argv) 
{

	if (argc < 2) {
		fprintf(stderr, "Usage: %s inputfile", argv[0]);
		exit(1);
	}
	htable_t *ht = htable_init(10);

	uint32_t count = 0;
	FILE *fp = fopen(argv[1], "r");
	if (!fp) {
		exit(1);
	}
	char line[MAX_LINE] = {0};
	while (NULL != fgets(line, MAX_LINE, fp)) {
		uint64_t prog_id = 0;
		uint64_t len = 0;

		char *entry;

		if (NULL != (entry = strstr(line+2, "2011-01-07"))) {
			//fprintf(stderr, "%s\n", line);
			continue;
		}

		char *s = strstr(line, "PID:");
		if (!s)
			continue;
		s += 4;
		while(*s >= '0' && *s <= '9')  {
			prog_id = prog_id * 10 + *s - '0';
			s++;
		}

		s = strstr(line, "len:");
		if (!s)
			continue;
		s += 4;
		while(*s >= '0' && *s <= '9')  {
			len = len * 10 + *s - '0';
			s++;
		}

		//printf("found %lu %lu\n", prog_id, len);
		prog_t *p = htable_search(ht, prog_id);	
		if (!p)
			p = htable_insert(ht, prog_id);

		p->len += len;
		p->pkt_num ++;
		count++;
	}
	fclose(fp);

	htable_traverse(ht);

	return 0;
}

