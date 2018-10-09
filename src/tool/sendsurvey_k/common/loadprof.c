// loadprof.c
//
// general routines to read in profile
//
// CREATED by Guo Le, 2010-12-15


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "loadprof.h"

#ifdef WIN32
#define strncasecmp(x,y,z) memicmp(x,y,z)
#endif


// seek specified section in initialization file,
// file read pointer at beginning of next line
//
// Return:
//	>= 0 : success
//	<  0 : error
static int seek_section(
	FILE *fp,	// [IN] initialization file descriptor
	const char *section)	// [IN] section name in initialization file
{
	char line[MAX_PROF_LINE];
	unsigned int i, j;
	char *pc;

	i = strlen(section);
	while (NULL != fgets(line, MAX_PROF_LINE, fp)) {
		pc = line;
		while (' ' == *pc || '\t' == *pc)
			pc++;	// cut prefixal space chars
		if (SECTION_PRE != *pc)
			continue;
		pc ++;
		j = strlen(pc);
		if (j < (i + 1) || SECTION_POST != pc[i])
			continue;
		if (strncasecmp(section, pc, i))
			continue;
		return 0;
	}
	return -1;
}



// seek specified key in initialization file
//
// Return:
//	>= 0 : success
//	<  0 : error
static int seek_key(
	FILE *fp,	// [IN] initialization file descriptor
	const char *key,	// [IN] key name in initialization file
	char *line)	// [OUT] key value string
{
	unsigned int i, j;
	char *pc;

	i = strlen(key);
	*line = '\0';
	while (NULL != fgets(line, MAX_PROF_LINE, fp)) {
		pc = line;
		while (' ' == *pc || '\t' == *pc)
			pc++;	// cut prefixal space chars
		if (SECTION_PRE == *pc)	// next section
			return -1;
		j = strlen(pc);
		if (j < i)
			continue;
		if (strncasecmp(key, pc, i))
			continue;
		pc += i;
		while (' ' == *pc || '\t' == *pc)
			pc++;
		if (VALUE_SET != *pc)
			continue;
		pc++;
		j = strlen(pc);
		if (j < 1)
			return 0;
		memmove(line, pc, j);
		line[j] = '\0';
		return 1;
	}
	return -1;
}



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
	int dval)	// [IN] default value
{
	char line[MAX_PROF_LINE];
	FILE *fp;

	*val = dval;
	fp = fopen(file, "r");
	if (NULL == fp)
		return -1;
	if (seek_section(fp, section) < 0) {
		fclose(fp);
		return -2;
	}
	if (seek_key(fp, key, line) < 0) {
		fclose(fp);
		return -3;
	}
	*val = strtol(line, NULL, 0);
	return 0;
}



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
	unsigned int dval)	// [IN] default value
{
	char line[MAX_PROF_LINE];
	FILE *fp;

	*val = dval;
	fp = fopen(file, "r");
	if (NULL == fp)
		return -1;
	if (seek_section(fp, section) < 0) {
		fclose(fp);
		return -2;
	}
	if (seek_key(fp, key, line) < 0) {
		fclose(fp);
		return -3;
	}
	*val = strtoul(line, NULL, 0);
	return 0;
}



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
	const char *dstr)	// [IN] default string
{
	char line[MAX_PROF_LINE];
	FILE *fp;
	size_t i;
	char *pc;

	if (0 == size) {
		*str = '\0';
		return 0;	
	}
	i = strlen(dstr);
	if (i > size - 1) {
        return -1;
    }
	strcpy(str, dstr);
	fp = fopen(file, "r");
	if (NULL == fp)
		return -1;
	if (seek_section(fp, section) < 0) {
		fclose(fp);
		return -2;
	}
	if (seek_key(fp, key, line) < 0) {
		fclose(fp);
		return -3;
	}
	fclose(fp);
	pc = line;
	while (' ' == *pc || '\t' == *pc)
		pc++;	// cut prefixal space chars
	i = strlen(pc);
	if (i < 1)
		return 0;
	if ('\n' == pc[i - 1])
		i --;	// cut pending '\n'
	while (' ' == pc[i - 1] || '\t' == pc[i - 1]) {
		i --;	// cut pending space chars
		if (i < 1) {
			*str = '\0';
			return 0;
		}
	}
	if (i > size - 1)
		i = size - 1;	// truncate extra chars
	memcpy(str, pc, i);
	str[i] = '\0';
	return i;
}


int profile_read_nbyte_hex(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	unsigned char *bytes,
	size_t size)	// [IN] buffer size(bytes)
{
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;

	char *p = str;
	uint8_t num;
	size_t count = 0;
	while (0 != *p) {
		if (strncasecmp("0x", p, 2)) break;
		p += 2;
		num = 0;
		while((*p >= '0' && *p <= '9') || 
				(*p >= 'a' && *p <= 'f') || 
				(*p >= 'A' && *p <= 'F')) {
			if (*p >= 'a' && *p  <= 'f')
				num = num * 16 + (*p - 'a' + 10);
			else if (*p >= 'A' && *p <= 'F')
				num = num * 16 + (*p - 'A' + 10);
			else
				num = num * 16 + (*p - '0');
			p++;
		}
		bytes[count++] = num; 
		if (count == size) break;
		while (' ' == *p || '\t' == *p || SEP_ITEM == *p) p++;
	}
	return count;
}


//
// Return:
//	>= 0 : num of successfully converted network address.
//	<  0 : error, 
int profile_read_naddr(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	addr4_t *addrs,	// [OUT] returned string
	size_t size) // [IN] buffer size(num of addr4_t)
{
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;

	char *s, *e;
	char buf[20];
	in_addr_t ip;
	e = str;
	size_t count = 0;
	while ('\0' != *e) {
		s = e;
		while((*e >= '0' && *e <= '9') || '.' == *e) e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		if (inet_pton(AF_INET, buf, &ip) <= 0)
			break;
		addrs[count].s_addr = ip; 

		while (' ' == *e || '\t' == *e || SEP_IP_PORT == *e) e++;
		s = e;
		while(*e >= '0' && *e <= '9') e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		addrs[count].sin_port = htons(atoi(buf));
		if (++count == size)
			break;
		while (' ' == *e || '\t' == *e || SEP_ITEM == *e) e++;
	}
	return count;

}

int profile_read_naddrdev(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	addr4dev_t *addrdevs,	// [OUT] returned string
	size_t size) // [IN] buffer size(num of addr4_t)
{
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;

	char *s, *e;
	char buf[20];
	in_addr_t ip;
	e = str;
	size_t count = 0;
	while ('\0' != *e) {
		s = e;
		while((*e >= '0' && *e <= '9') || '.' == *e) e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		if (inet_pton(AF_INET, buf, &ip) <= 0)
			break;
		addrdevs[count].s_addr = ip; 

		while (' ' == *e || '\t' == *e || SEP_IP_PORT == *e) e++;
		s = e;
		while(*e >= '0' && *e <= '9') e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		addrdevs[count].sin_port = htons(atoi(buf));

        while (' ' == *e || '\t' == *e || SEP_IP_PORT == *e) e++;
		s = e;
		while(!isspace(*e) && SEP_ITEM != *e) e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		strcpy(addrdevs[count].dev, buf);

		if (++count == size)
			break;
		while (' ' == *e || '\t' == *e || SEP_ITEM == *e) e++;
	}
	return count;

}
int profile_read_nstring(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	char *strings,	// [OUT] returned string
	size_t string_size,
	size_t string_num) // [IN] buffer size(num of strigns)
{
	int count=0;
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;
	char* string_begin=str;
	char* string_end=strchr(string_begin,',');
	while(string_end!=NULL&&string_num>count)
	{
		memcpy(strings+count*string_size,string_begin,string_end-string_begin);
		count++;
		string_begin=string_end+1;
		string_end=strchr(string_end+1,',');		
	}
	memcpy(strings+count*string_size,string_begin,strlen(str)-(string_begin-str));
	count++;
	return count;

}

//
// Return:
//	>= 0 : num of successfully converted network address.
//	<  0 : error, 
int profile_read_nip(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	in_addr_t *ips,	// [OUT] returned string
	size_t size)// [IN] buffer size(num of in_addr_t)
	
{
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;

	char *s, *e;
	char buf[20];
	in_addr_t ip;
	e = str;
	size_t count = 0;
	while ('\0' != *e) {
		s = e;
		while((*e >= '0' && *e <= '9') || '.' == *e) e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		if (inet_pton(AF_INET, buf, &ip) <= 0)
			break;
		ips[count] = ip; 

		if (++count == size)
			break;
		while (' ' == *e || '\t' == *e || SEP_ITEM == *e) e++;
	}
	return count;
}

//
// Return:
//	>= 0 : num of successfully converted network address.
//	<  0 : error, 
int profile_read_nport(
	const char *file,	// [IN] initialization file path
	const char *section,	// [IN] section name in initialization file
	const char *key,	// [IN] keyword name in initialization file
	in_port_t *ports,	// [OUT] 
	size_t size)// [IN] buffer size
	
{
	char str[MAX_PROF_LINE]; 
	if (profile_read_string(file, section, key, str, MAX_PROF_LINE, "") < 0)
		return 0;

	char *s, *e;
	char buf[20];
	e = str;
	size_t count = 0;
	while ('\0' != *e) {
		s = e;
		while((*e >= '0' && *e <= '9') || '.' == *e) e++;
		strncpy(buf, s, e-s);
		buf[e-s] = 0;
		ports[count] = htons(atoi(buf));

		if (++count == size)
			break;
		while (' ' == *e || '\t' == *e || SEP_ITEM == *e) e++;
	}
	return count;
}
