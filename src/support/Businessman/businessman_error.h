#ifndef _BIZMAN_ERROR_H_INCLUDE_
#define _BIZMAN_ERROR_H_INCLUDE_
#define set_error(s)	set_bizman_error(s,__FILE__,__LINE__,__PRETTY_FUNCTION__)
enum BusinessmanErrorType{
	ERROR_TYPE_USER=-1,
	ERROR_TYPE_NOERROR=0,
	ERROR_TYPE_SOCKET,
	ERROR_TYPE_OUTOFMEM,
	ERROR_TYPE_INVALID_PARAMETER,
	ERROR_TYPE_DEST_UNREACHABLE
};

void set_user_error(char * format, ...);
void set_bizman_error(int error_type,const char*source_file,int code_line_num,const char*function_name);
char * get_error_string();
#endif//_BIZMAN_ERROR_H_INCLUDE_

