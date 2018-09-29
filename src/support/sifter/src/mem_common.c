#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int strncasecmp_one_word_mesa(const char *str1, size_t len1, const char *str2, size_t len2)
{
	if (len2 != len1 || len1==0)
		return 0;
	
	do {
		if(*str1 - 'a' > 0) //str1小写
		{
			if (*str1 == *str2 || toupper(*str1) == *str2)
			{
				++str1;
				++str2;
				continue;
			}
		}
		else
		{
			if (*str1 == *str2 || tolower(*str1) == *str2)
			{
				++str1;
				++str2;
				continue;
			}
		}
		return 0;
	} while (--len1);
	
	return 1;
}


int strncmp_one_word_mesa(const char *s1_lowercase, const char *s1_uppercase, size_t len1, const char *s2, size_t len2)
{
	unsigned char *s1,*s12;

	if (len2 < len1)
		return 0;
	
	if (s2[len1-1]-'a'>=0)
	{
		s1 = (unsigned char *)s1_lowercase;
		s12= (unsigned char *)s1_uppercase;
	}
	else
	{
		s1 = (unsigned char *)s1_uppercase;
		s12= (unsigned char *)s1_lowercase;
	}
	
	do {
		if (*s1 == *s2 || *s12 == *s2)
		{
			++s1;
			++s12;
			++s2;
			continue;
		}
		return 0;
	} while (--len1);
	
	return 1;
}

/*
* mesa_memicmp2
* FUNCTION
* 	compare s2 with s1, ignorance of case
* INPUT
* 	@s1_lowercase MUST be lowercase
* 	@s1_uppercase MUST be uppercase
* 	@len1 is the length of s1
* 	@s2 is the string you want to be compared
* 	@len2 is the length of s2
* OUTPUT:
* 	0:s1==s2
* 	-1:s1!=s2
*/
int strcmp_one_word_mesa(const char *s1_lowercase, const char *s1_uppercase, size_t len1, const char *s2, size_t len2)
{
	unsigned char *s1,*s12;

	if (len1 != len2)
		return 0;
	
	if (s2[len2-1]-'a'>=0)
	{
		s1 = (unsigned char *)s1_lowercase;
		s12= (unsigned char *)s1_uppercase;
	}
	else
	{
		s1 = (unsigned char *)s1_uppercase;
		s12= (unsigned char *)s1_lowercase;
	}
	
	do {
		if (*s1 == *s2 || *s12 == *s2)
		{
			++s1;
			++s12;
			++s2;
			continue;
		}
		return 0;
	} while (--len1);
	
	return 1;
}

//相对于strcmp_one_word_mesa仅仅去掉了长度相等的比较
int strcmp_one_word_mesa_equal_len(const char *s1_lowercase, const char *s1_uppercase, const char *s2, size_t len)
{
	unsigned char *s1,*s12;

	if (s2[len-1]-'a'>=0)
	{
		s1 = (unsigned char *)s1_lowercase;
		s12= (unsigned char *)s1_uppercase;
	}
	else
	{
		s1 = (unsigned char *)s1_uppercase;
		s12= (unsigned char *)s1_lowercase;
	}
	
	do {
		if (*s1 == *s2 || *s12 == *s2)
		{
			++s1;
			++s12;
			++s2;
			continue;
		}
		return 0;
	} while (--len);
	
	return 1;
}

/*********************************************************************
函数名称：mail_strcmp_two_word
功能简介：判断一个由2个单词组成的字符串是不是目标SMTP命令字符串，含':'，
				忽略目标字符串2个单词及':'之间的空格，如"mail from :"，忽略大小写
输入参数：srcdata：
输出参数：无
返回值： 1：是
	 0：不是
*********************************************************************/
int strcmp_two_word(char *srcdata,int datalen, const char *first, int flen, const char *second, int slen)
{					
	char * tok;
	
	if(datalen < flen+slen+1)
		return 0;
	
	if(strncasecmp(srcdata, first, flen) != 0) 
	{
		return 0;
	}
	
	tok = srcdata + flen;
	while(*tok==' ') 
		tok++;
	
	if(strncasecmp(tok, second, slen) != 0) 
		return 0;

	tok += slen;
	while(*tok==' ') 
		tok++;
	if( *tok != ':' )
		return 0;

	return 1;
}


//查找开始位置
const void *mail_memmem(const void *start, unsigned int s_len, const void *find, unsigned int f_len)
{
     const char *p, *q;
     unsigned int len;
				        
     p = (const char *)start, q = (const char *)find;
     len = 0;
     while((p - (const char *)start + f_len) <= s_len)
	 {
	       while(*p++ == *q++)
		   {
	       		len++;
	       		if(len == f_len)
				    return(p - f_len);
			};
			q = (const char *)find;
			len = 0;
	 };
	 
	 return NULL;
}

/*
* mesa_memncasemem3
* FUNCTION
* 	find needle in haystack, ignorance of case
* INPUT
* 	@needle MUST be lowercase
* 	@needlelen is the length of needle
* OUTPUT
* 	NULL if not found
*/
char *mesa_memncasemem(const char *haystack, size_t haystacklen,const char *needle, size_t needlelen)
{
	unsigned char *psub,*psrc,*p,*pendsrc,*pendsubstr;
	int diff=-32; //A=a-32;
 
	if ((haystack==NULL) || needle==NULL)
		return NULL;
	if (haystacklen<needlelen) 
		return NULL;

	pendsrc = (unsigned char *)haystack + haystacklen - needlelen + 1;
	pendsubstr = (unsigned char *)needle + needlelen;
	for (psrc =(unsigned char *)haystack; psrc != pendsrc; psrc++)
	{
		for (p = psrc,psub = (unsigned char *)needle; psub != pendsubstr; psub++,p++)
		{
			if (*p!=*psub && *p!=*psub + diff)
				break;
		}
		if (psub == pendsubstr)
		{
			return (char *)psrc;
		}
	} 
	return NULL;
}


/*
* FUNCTION
* 	find last needle in haystack, ignorance of case
* INPUT
* 	@needle MUST be lowercase
* 	@needlelen is the length of needle
* OUTPUT
* 	NULL if not found
*/
char *mesa_memncasemem_reverse(const char *haystack, size_t haystacklen,const char *needle, size_t needlelen)
{
	unsigned char *p,*psrc,*psub,*pendsubstr;
	int diff=-32; //A=a-32;
 
	if ((haystack==NULL) || needle==NULL)
		return NULL;
	if (haystacklen<needlelen) 
		return NULL;

	pendsubstr = (unsigned char *)needle + needlelen;
	for (psrc = (unsigned char *)haystack + haystacklen - needlelen; psrc != (unsigned char *)haystack-1; psrc--)
	{
		for (p = psrc,psub = (unsigned char *)needle; psub != pendsubstr; p++,psub++)
		{
			if (*p!=*psub && *p!=*psub + diff)
				break;
		}
		if (psub == pendsubstr)
		{
			return (char *)psrc;
		}
	}
	return NULL;
}

