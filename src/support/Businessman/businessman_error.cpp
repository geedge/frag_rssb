#include <stdio.h>
#include <stdarg.h>//va_start
#include <string.h>//snprintf memset
#include "businessman_error.h"
const char * error_define_string[]={"no error detected","socket error","out of mem","invalid parameter"};
static int g_error=ERROR_TYPE_NOERROR;
char user_error_string[4096];
char error_location[1024];
char error_output_string[4096];
void set_user_error(char * format, ...)
{
	va_list ap;
	memset(user_error_string,0,sizeof(user_error_string));
	va_start(ap, format);
	vsnprintf(user_error_string, sizeof(user_error_string), format, ap);
	va_end(ap);
	g_error=ERROR_TYPE_USER;
	fprintf(stderr,"Bizman Error:%s\n",user_error_string);
}
void set_unreable(char* fromat, ...)
{
	g_error=ERROR_TYPE_DEST_UNREACHABLE;
}
void set_bizman_error(int error_type,const char*source_file,int code_line_num,const char*function_name){
	g_error=error_type;
	memset(error_location,0,sizeof(error_location));
	snprintf(error_location,sizeof(error_location),"%s %d %s",source_file,code_line_num,function_name);
	fprintf(stderr,"Bizman Error:%s\n",error_location);
}
char * get_error_string(){
	char* result=NULL;
	if(g_error==ERROR_TYPE_USER)
	{
		result= user_error_string;
	}
	else if(g_error==ERROR_TYPE_DEST_UNREACHABLE)
	{
		
	}
	else
	{
		memset(error_output_string,0,sizeof(error_output_string));
		snprintf(error_output_string,sizeof(error_output_string),"bizman error: %s %s",error_define_string[g_error],error_location);
		result= error_output_string;
	}
	g_error=ERROR_TYPE_NOERROR;
	return result;
}

