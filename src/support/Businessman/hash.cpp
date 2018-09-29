#include "hash.h"

/*
 * hash implementation, xh, 2001-07-26
 */
/*Developer Notice:
in _bizman_kernel_hash_add function,hash table MALLOC memory for keys and  RECORD data pointer,
which means need NOT malloc memory for keys before call _bizman_kernel_hash_add. zhengchao
*/


/*
 * allocates memory for table.
 * returns:
 * -1, if memory fails.
 * -2, if parameters not acceptable.
 *  0, success;
 *
 *  size is 2^N, mandatory or not?
 */
int _bizman_kernel_hash_create(hash_tab_t * table, uint size,
	key_comp_t * key_comp, key2index_t * key2index,
	int (* data_expire)(void *), void (* data_free)(void *))
{
	uint s;

	for(s = size; s != 0; ) {
		if(0x01 != s && 0x01 == (0x01 & s)) return -2;
		s = s >> 1;
	}
	table->elems = (hash_elem_t**)malloc(sizeof(hash_elem_t *) * size);
	if(NULL == table->elems)
		return -1;
	memset(table->elems, 0, sizeof(hash_elem_t *) * size);
	table->size = size;
	if(NULL == key_comp) table->key_comp = _bizman_kernel_hash_key_comp;
	else table->key_comp = key_comp;
	if(NULL == key2index) table->key2index = _bizman_kernel_hash_key2index;
	else table->key2index = key2index;
	table->expire = data_expire;
	table->free = data_free;
	table->elem_count = 0;
	table->threshold_lo = 0;
	table->threshold_hi = size;
	table->max_collision=0;
	table->avg_collision=0;
	return 0;
}

/*
 * frees table->elems and chains attached to each element of it.
 * always returns 0.
 */
int _bizman_kernel_hash_destroy(hash_tab_t * table, void (* func)(void *))
{
	int i;
	hash_elem_t * cur;

	for(i = 0; i < (int)table->size; i ++) {
		while(NULL != table->elems[i]) {
			cur = table->elems[i];
			if(NULL != func) func(cur->data);
			else if(NULL != table->free) table->free(cur->data);
			table->elems[i] = (table->elems[i])->next;
			free(cur->key);
			free(cur);
		}
	}
	free(table->elems);
	table->elems = NULL;
	table->size = 0;
	return 0;
}

/*
 * adds data identified by key to table.
 * table->elems always points to the newest added element.
 * no two elements should have a same key, although them may have a same index.
 * returns:
 * -1, if duplicate is found;
 * -2, if memory fails;
 *  0, success.
 */
int _bizman_kernel_hash_add(hash_tab_t * table, uchar * key, uint size, void * data)
{
	uint index;
	hash_elem_t * ptr;

	index = table->key2index(table, key, size);
	for(ptr = table->elems[index]; NULL != ptr; ptr = ptr->next) {
		if(!table->key_comp(ptr->key, ptr->size, key, size))
			return -1;
	}
	if(NULL == (ptr = (hash_elem_t*)malloc(sizeof(hash_elem_t)))) return -2;
	if(NULL == (ptr->key = (unsigned char*)malloc(size))) {
		free(ptr);
		return -2;
	}
	memcpy(ptr->key, key, size);
	ptr->size = size;
	ptr->data = data;
	ptr->prev = NULL;
	ptr->next = table->elems[index];
	if(NULL != table->elems[index]) table->elems[index]->prev = ptr;
	table->elems[index] = ptr;
	table->elem_count ++;
	/*
	 * scan table to find expired items when elem_count reaches threshold.
	 */

	return 0;
}

/*
 * deletes element identified by key from table.
 * returns:
 * -1, if not found;
 *  0, success.
 */
int _bizman_kernel_hash_del(hash_tab_t * table, uchar * key, uint size, void (* func)(void *))
{
	uint index;
	hash_elem_t * ptr;

	index = table->key2index(table, key, size);
	for(ptr = table->elems[index]; NULL != ptr; ptr = ptr->next) {
		if(!table->key_comp(ptr->key, ptr->size, key, size))
			break;
	}
	if(NULL == ptr) return 0;	/* success, maybe? */
	if(NULL != ptr->prev) ptr->prev->next = ptr->next;
	else table->elems[index] = ptr->next;
	if(NULL != ptr->next) ptr->next->prev = ptr->prev;
	if(NULL != func) func(ptr->data);
	else if(NULL != table->free) table->free(ptr->data);
	free(ptr->key);
	free(ptr);
	table->elem_count --;
	return 0;
}

/*
 * returns data associated with key in table;
 * not found, returns NULL.
 */
void * _bizman_kernel_hash_sel(hash_tab_t * table, uchar * key, uint size)
{
	uint index;
	hash_elem_t * ptr;
	uint i=0;
	index = table->key2index(table, key, size);
	for(ptr = table->elems[index]; NULL != ptr; ptr = ptr->next) {
		i++;
		if(!table->key_comp(ptr->key, ptr->size, key, size))
			break;
	}
	if(i>table->max_collision)
	{
		table->max_collision=i;
	}
	if(NULL != ptr) return ptr->data;
	else return NULL;
}

/*
 * returns hash index of key.
 * never fails.
 */
uint _bizman_kernel_hash_key2index(hash_tab_t * table, uchar * key, uint size)
{
	uchar * c;
	uint h, g;
	if(size==4) return *(unsigned int*)key%table->size;
	for (c = key, h = 0; c - key < size; c ++) {
		h = (h << 2) + c[0];
		if(0 != (g = (h & 0xf0000000))) {
			h = h ^ (g >> 24);
			h = h ^ g;
		}
	}
	return h % table->size;
}

int _bizman_kernel_hash_key_comp(uchar * key1, uint size1, uchar * key2, uint size2)
{
	if(size1 != size2) return 1;
	if(size1==4)
	{
		if(*(int*)key1==*(int*)key2)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}
	return memcmp(key1, key2, size1);
}

int _bizman_kernel_hash_iterate(hash_tab_t * table, void (* func)(uchar * key, uint size, void * data))
{
	int i;
	hash_elem_t * ptr;

	if(NULL == func) return 0;
	for(i = 0; i < (int)table->size; i ++) {
		for(ptr = table->elems[i]; NULL != ptr; ptr = ptr->next)
			func(ptr->key, ptr->size, ptr->data);
	}
	return 0;
}

int _bizman_kernel_hash_iterate2(hash_tab_t * table, void (* func)(uchar * key, uint size, void * data, void * u), void * user)
{
	int i;
	hash_elem_t * ptr;

	if(NULL == func) return 0;
	for(i = 0; i < (int)table->size; i ++) {
		for(ptr = table->elems[i]; NULL != ptr; ptr = ptr->next)
			func(ptr->key, ptr->size, ptr->data, user);
	}
	return 0;
}

int _bizman_kernel_hash_expire(hash_tab_t * table)
{
	int i, delta;
	hash_elem_t * ptr, * tmp;

	if(NULL == table->expire) return 0;
	for(i = 0; i < (int)table->size; i ++) {
		ptr = table->elems[i];
		while(NULL != ptr) {
			if(table->expire(ptr->data)) {
				if(NULL != ptr->prev)
					ptr->prev->next = ptr->next;
				else table->elems[i] = ptr->next;
				if(NULL != ptr->next)
					ptr->next->prev = ptr->prev;
				tmp = ptr;
				ptr = ptr->next;
				if(NULL != table->free)
					table->free(tmp->data);
				free(tmp);

				table->elem_count --;
				if(table->elem_count < table->threshold_lo) {
					delta = table->threshold_hi - table->threshold_lo;
					delta = delta << 1;
					if(delta > HASH_THRESHOLD_MAX_DELTA)
						delta = HASH_THRESHOLD_MAX_DELTA;
					table->threshold_hi = table->threshold_lo;
					if(table->threshold_lo > (unsigned int)delta)
						table->threshold_lo -= delta;
					else
						table->threshold_lo = 0;
				}
				continue;
			}
			ptr = ptr->next;
		}
	}
	return 0;
}

