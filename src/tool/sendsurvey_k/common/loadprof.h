#ifndef SLIB_LOADPROF_H
#define SLIB_LOADPROF_H

#include <netinet/in.h>

#define MAX_PROF_LINE	1024	/* MAX bytes in a single line */
#define COMMENT_CHAR	'#'	/* comments line prefix */
#define SECTION_PRE	'['	/* section name prefix */
#define SECTION_POST	']'	/* section name postfix */
#define VALUE_SET	'='	/* set key value operator */

#define SEP_ITEM ','
#define SEP_IP_PORT ':'

typedef struct addr4 {
    in_addr_t s_addr;
    in_port_t sin_port;
    in_port_t __align;
} addr4_t;


#define DEV_NAME_MAX 10
typedef struct addr4dev {
    in_addr_t s_addr;
    in_port_t sin_port;
    char dev[DEV_NAME_MAX];
} addr4dev_t;





// Read in specified integer value
//
// Return:
//	>= 0 : success
//	<  0 : error, val is set to default
int profile_read_int(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	int *val,	// [OUT] returned value
	int dval);	// [IN] default value

// Read in specified unsigned integer value
//
// Return:
//	>= 0 : success
//	<  0 : error, val is set to default
int profile_read_uint(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	unsigned int *val,	// [OUT] returned value
	unsigned int dval);	// [IN] default value

// Read in specified string value,
// if value string is too long to return, extra chars truncated.
// prefix/postfix space chars cutted,
// space chars: ' ', '\t'
//
// Return:
//	>= 0 : length of val
//	<  0 : error, str is set to default
int profile_read_string(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	char *str,	// [OUT] returned string
	size_t size,	// [IN] buffer size(bytes)
	const char *dstr);	// [IN] default string



int profile_read_nbyte_hex(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	unsigned char *bytes,
	size_t size);	// [IN] buffer size(bytes)

int profile_read_naddr(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	addr4_t *addrs,	// [OUT] returned string
	size_t size);	// [IN] buffer size(num of addr4_t)

int profile_read_naddrdev(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	addr4dev_t *addrdevs,	// [OUT] returned string
	size_t size);	// [IN] buffer size(num of addr4_t)


int profile_read_nip(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	in_addr_t *ips,	// [OUT] returned string
	size_t size);	// [IN] buffer size(num of in_addr_t)

// Return:
//	>= 0 : num of successfully converted network address.
//	<  0 : error, 
int profile_read_nport(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	in_port_t *ports,	// [OUT] 
	size_t size);// [IN] buffer size
int profile_read_nstring(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	char *strings,	// [OUT] returned string
	size_t string_size,
	size_t string_num);	
#endif /* #ifndef SLIB_LOADPROF_H */
