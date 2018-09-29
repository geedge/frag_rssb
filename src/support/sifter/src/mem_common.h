#ifndef __MEM_COMMON_H__
#define __MEM_COMMON_H__

int strncasecmp_one_word_mesa(const char *str1, size_t len1, const char *str2, size_t len2);
int strcmp_one_word_mesa(const char *s1_lowercase, const char *s1_uppercase, size_t len1, const char *s2, size_t len2);
int strncmp_one_word_mesa(const char *s1_lowercase, const char *s1_uppercase, size_t len1, const char *s2, size_t len2);
int strcmp_one_word_mesa_equal_len(const char *s1_lowercase, const char *s1_uppercase, const char *s2, size_t len);

int strcmp_two_word(char *srcdata,int datalen, const char *first, int flen, const char *second, int slen);

const void *mail_memmem(const void *start, unsigned int s_len, const void *find, unsigned int f_len);
char *mesa_memncasemem(const char *haystack, size_t haystacklen,const char *needle, size_t needlelen);

#endif
