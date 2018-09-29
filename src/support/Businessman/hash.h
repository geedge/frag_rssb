#ifndef _LIB_HASH_H_INCLUDED_
#define _LIB_HASH_H_INCLUDED_
#ifdef __cplusplus
extern "C"
{
#endif
/*
 * general purpose hash table implementation.
 *
 * xiang hong
 * 2002-07-28
 */


#include <stdlib.h>
#include <string.h>

#ifndef uchar
#define uchar	unsigned char
#endif
#ifndef uint
#define uint	unsigned int
#endif

#define HASH_THRESHOLD_MAX		65536
#define HASH_THRESHOLD_MIN		0
#define HASH_THRESHOLD_MAX_DELTA	32768
#define HASH_THRESHOLD_MIN_DELTA	16

struct __hash_tab;

/*
 * hash key compare function prototype, see _bizman_kernel_hash_key_comp().
 */
typedef int key_comp_t(uchar * key1, uint size1, uchar * key2, uint size2);

/*
 * hash key->index computing function prototype, see _bizman_kernel_hash_key2index().
 */
typedef uint key2index_t(struct __hash_tab * tab, uchar * key, uint size);

/*
 * hash element structure.
 *
 * key: a copy of key;
 * size: size of key;
 * data: pointer to attached data;
 * prev, next: pointer to prev and next item;
 */
typedef struct struct_hash_elem {
	uchar * key;
	uint size;
	void * data;
	struct struct_hash_elem * prev;
	struct struct_hash_elem * next;
} hash_elem_t;

/*
 * hash structure.
 *
 * size: number of slots;
 * key_comp: pointer to the function that compares 2 keys;
 * key2index: pointer to the function that computes index from key;
 * expire: pointer to the function that checks if attached data is expired;
 * free: pointer to the function that cleans up attached data;
 * elems: array of slots;
 * elem_count: number of items;
 * threshold_lo, threshold_hi: low and high threshold; when elem_count reaches
 * 	one of them, an iteration will be launched to clean up expired items;
 */
typedef struct __hash_tab {
	uint size;
	key_comp_t * key_comp;
	key2index_t * key2index;
	int (* expire)(void * data);
	void (* free)(void * data);
	hash_elem_t ** elems;
	uint elem_count;
	uint threshold_lo;
	uint threshold_hi;
	uint max_collision;
	uint avg_collision;
} hash_tab_t;

/*
 * name: _bizman_kernel_hash_create
 * functionality: allocats memory for hash slots, and fill in hash structure;
 * param:
 * 	table: caller allocated hash table structure, gets filled in;
 * 	size: how big do you want the table to be;
 * 	key_comp, key2index, data_expire, data_free: function pointers, can be
 * 		NULL;
 * returns:
 * 	0, success;
 * 	-1, memory failure;
 * 	-2, parameter not acceptable;
 */
int _bizman_kernel_hash_create(hash_tab_t * table, uint size,
	key_comp_t * key_comp, key2index_t * key2index,
	int (* data_expire)(void *), void (* data_free)(void *));

/*
 * name: _bizman_kernel_hash_destroy
 * functionality: cleans up hash structure, frees memory occupied;
 * param:
 * 	table: who is the victim;
 * 	func: callback function to clean up data attached to hash items;
 * returns:
 * 	always returns 0;
 */
int _bizman_kernel_hash_destroy(hash_tab_t * table, void (* func)(void *));

/*
 * name: _bizman_kernel_hash_add
 * functionality: adds item to table, call _bizman_kernel_hash_expire() if elem_count gets
 * 	bigger than threshold_hi, and adjust threshold;
 * param:
 * 	table: to which table do you want to add;
 * 	key: what is the label;
 * 	size: how long is the label;
 * 	data: what data do you want to attach;
 * returns:
 * 	0, success;
 * 	-1, duplicates found and can't add this one;
 * 	-2, memory failure;
 */
int _bizman_kernel_hash_add(hash_tab_t * table, uchar * key, uint size, void * data);

/*
 * name: _bizman_kernel_hash_del
 * functionality: deletes item from table, call _bizman_kernel_hash_expire() if elem_count
 * 	gets smaller than threshold_lo, and adjust threshold;
 * param:
 * 	table: from which table do you want to delete;
 * 	key: what is the label;
 * 	size: how long is the label;
 * 	func: callback function to clean up data attached to hash items;
 * returns:
 * 	0, success;
 * 	-1, no such thing;
 */
int _bizman_kernel_hash_del(hash_tab_t * table, uchar * key, uint size, void (* func)(void *));

/*
 * name: _bizman_kernel_hash_sel
 * functionality: selects item from table;
 * param:
 * 	table: from which table do you want to select;
 * 	key: what is the label;
 * 	size: how long is the label;
 * returns:
 * 	pointer to attached data;
 * 	NULL if not found; (thus be careful if you are attaching NULL data on
 * 		purpose.)
 */
void * _bizman_kernel_hash_sel(hash_tab_t * table, uchar * key, uint size);

/*
 * name: _bizman_kernel_hash_iterate
 * functionality: iterates each hash item;
 * params:
 * 	table: what table is to be iterated;
 * 	func: what do you want to do to each attached data item;
 * returns:
 * 	always 0;
 */
int _bizman_kernel_hash_iterate(hash_tab_t * table, void (* func)(uchar * key, uint size, void * data));
int _bizman_kernel_hash_iterate2(hash_tab_t * table, void (* func)(uchar * key, uint size, void * data, void * u), void * user);

/*
 * name: _bizman_kernel_hash_expire
 * functionality: iterates each item and deletes those that are expired;
 * params:
 * 	table: what table do you want to check;
 * returns:
 * 	always 0;
 */
int _bizman_kernel_hash_expire(hash_tab_t * table);

/*
 * name: _bizman_kernel_hash_key2index
 * functionality: computes index from key, default key2index implementation
 * 	if previous _bizman_kernel_hash_create() call did not provide such a function;
 * params:
 * 	table: with which table are you dealing, we want this param be present
 * 		because we may need size of table to compute the key;
 * 	key: what is the label;
 * 	size: how long is the label;
 * returns:
 * 	index in given table for given key, never fails;
 */
uint _bizman_kernel_hash_key2index(hash_tab_t * table, uchar * key, uint size);

/*
 * name: hash_comp
 * functionality: compares two key;
 * params:
 * 	key1, key2: what are the labels;
 * 	size1, size2: how long are them;
 * returns:
 * 	0, if two keys are identical; (just a simple memcmp() );
 * 	1, no.
 */
int _bizman_kernel_hash_key_comp(uchar * key1, uint size1, uchar * key2, uint size2);
#ifdef __cplusplus
}
#endif

#endif	/* _LIB_HASH_H_INCLUDED_ */


