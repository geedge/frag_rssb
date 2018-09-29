#ifndef _FRAG_SEND_H
#define _FRAG_SEND_H

/*通过sokcet接口发送本机*/
#define FRAG_SEND_SOCKET  0x01
/*多源通过bizman发送给其他frag_rssb*/
#define FRAG_SEND_BIZMAN  0x02
/*异常，数据丢失*/
#define FRAG_SEND_DROP	  0x03

/*多源聚合的地址*/
#define MEDIA_SEND_DST_LOCAL		0x00  /*本机*/
#define MEDIA_SEND_DST_OTHER		0x01  /*其它粗拼装*/

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
