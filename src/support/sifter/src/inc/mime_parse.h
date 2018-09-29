#ifndef __MIME_PARSE_H__
#define __MIME_PARSE_H__

#define MAX_CHARSET_LEN 32

// 定义转换编码的类型
#define MIME_TRANSENC_UNKNOWN	0
#define MIME_TRANSENC_BASE64	1
#define MIME_TRANSENC_QP		2

typedef struct __mime_result
{
	char *name;
	char *filename;
	char *content;	//数据的内容
	int namelen;	
	int fnamelen;
	int contlen; //数据的长度
	short trans_enc;
	short block_seq; //块的顺序号，新来BOUNDARY时加1，每个块序号唯一
	char cont_charset[MAX_CHARSET_LEN]; //正文内容字符集
	char charset[MAX_CHARSET_LEN]; //name/filename字符集
}mime_result_t;

typedef void * mime_streamparam_t;

typedef int mime_cb_fun_t(mime_streamparam_t stream_param, mime_result_t *mime_result, void *user_arg);

int  mime_header_decode(const char *in, int inl, char *out, int *outl, char* charset, int max_charset_len, void *log_handle);

/*********************************************************************
函数名称：mime_stream_start
功能简介：创建MIME解析句柄并初始化
输入参数：decode_on：是否进行传输编码(BASE64/QP)解码
		log_handle：日志句柄，不可为空
		thread_id：线程ID
返回值：句柄
*********************************************************************/
mime_streamparam_t mime_stream_start(int decode_on, void *log_handle, unsigned int thread_id);

/*********************************************************************
函数名称：mime_stream_feed
功能简介：流式解析MIME
输入参数：data/dlen：输入数据及长度，仅限于输入MIME数据，否则有不可预料的解析错误
		lostlen：前面的包丢失，丢失的长度
		cb：解析出结果后的回调函数，绝不许为空
		userarg：cb函数的自定义参数
输出参数：cb_ret：cb函数的返回值，不可为空
返回值： <0: error;  0: 成功
注意事项：**顶层MIME头部结束之后紧跟的空行也必须输入，便于模块识别头部结束**
*********************************************************************/
int mime_stream_feed(mime_streamparam_t stream_param, const void *data, unsigned int dlen, int lostlen, mime_cb_fun_t *cb, void *user_arg, int *cb_ret);

/*********************************************************************
函数名称：mime_stream_end
功能简介：全部解析完毕，清理内存
*********************************************************************/
void mime_stream_end(mime_streamparam_t stream_param);

/*********************************************************************
函数名称：mime_stream_reset
功能简介：复位解析，在有多个HTTP/邮件等会话时使用，每个会话结束，在进行下一个会话的解析之前复位
*********************************************************************/
int mime_stream_reset(mime_streamparam_t stream_param);

#endif 

