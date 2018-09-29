#ifndef _DYNAMIC_ARRAY_H_
#define _DYNAMIC_ARRAY_H_

#define DYNAMIC_ARRAY_DEBUG		(1)

typedef void * maskey_dynamic_array_handle;

typedef int (* maskey_dynamic_array_init_cb)(void *struct_data);
typedef int (* maskey_dynamic_array_iterate_cb)(int index, void *struct_data, void *user_arg);
typedef int (* maskey_dynamic_array_safe_copy_cb)(int index, void *new_data, void *old_data);

maskey_dynamic_array_handle maskey_dynamic_array_init(int struct_len, int array_init_num, int array_max_num);
maskey_dynamic_array_handle maskey_dynamic_array_init_with_cb(int struct_len, int array_init_num, 
						int array_max_num, maskey_dynamic_array_init_cb init_cb);
void maskey_dynamic_array_destroy(maskey_dynamic_array_handle dah, maskey_dynamic_array_iterate_cb cb, void *);
int maskey_dynamic_array_append(maskey_dynamic_array_handle dah, void *struct_data, int struct_len);
int maskey_dynamic_array_append_distinct(maskey_dynamic_array_handle dah, void *struct_data, int struct_len);
void *maskey_dynamic_array_fetch(maskey_dynamic_array_handle dah, int index);
int maskey_dynamic_array_get_num(maskey_dynamic_array_handle dah);
void *maskey_dynamic_array_get_data(maskey_dynamic_array_handle dah);
void *maskey_dynamic_array_get_last_item(maskey_dynamic_array_handle dah);
int maskey_dynamic_array_iterate(maskey_dynamic_array_handle dah, maskey_dynamic_array_iterate_cb cb, void *user_arg);
maskey_dynamic_array_handle maskey_dynamic_array_quick_copy(maskey_dynamic_array_handle old_dah);

#endif

