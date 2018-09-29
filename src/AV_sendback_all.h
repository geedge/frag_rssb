#ifndef _AV_SENDBACK_H
#define _AV_SENDBACK_H

/*魔数*/
#define AV_MAGIC_VALUE				0x8739	

/*消息类型*/
#define AV_TYPE_META 				0x01
#define	AV_TYPE_DATA				0x02
#define AV_TYPE_RESULT 				0x11

/*协议类型*/
#define	AV_PROTOCOL_HTTP			0x01
#define	AV_PROTOCOL_SMTP			0x02
#define	AV_PROTOCOL_POP3			0x03
#define	AV_PROTOCOL_FTP				0x04
#define	AV_PROTOCOL_IMAP			0x05
#define	AV_PROTOCOL_HTTP_STREAM		0x81
#define	AV_PROTOCOL_RTSP_RDT		0x82
#define	AV_PROTOCOL_RTSP_RTP		0x83
#define	AV_PROTOCOL_ED2K			0x85
#define	AV_PROTOCOL_MMS				0x84
#define	AV_PROTOCOL_RTMP			0x86
#define	AV_PROTOCOL_SIP				0x91
#define	AV_PROTOCOL_BT				0x08

/*媒体类型*/
#define FILE_UNKNOWN    			0x00
#define FILE_VIDEO					0x60
#define FILE_WMV					0x61		
#define FILE_MPG					0x62		
#define FILE_FLV					0x63	
#define FILE_RMFF					0x65
#define FILE_AVI					0x66
#define FILE_SWF					0x67
#define FILE_MPG4					0x68
#define FILE_AIFF					0x69
#define FILE_OGG					0x6A
#define FILE_DRC					0x6B
#define FILE_DIRECTSHOW				0x6C
#define FILE_FLIC					0x6D
#define FILE_INDEO					0x6E
#define FILE_MKV					0x6F

#define FILE_AUDIO					0x70
#define FILE_MP3					0x71

#define FILE_OSMF   				0xA1
#define FILE_HLS   					0xA2
#define FILE_IOS   					0xA3
#define FILE_ANDRIOD   				0xA4
#define FILE_APP   					0xA5

#define AUDIO_UNKNOWN				0xB0
#define AUDIO_G711_ULAW				0xB1
#define AUDIO_G711_ALAW				0xB2
#define AUDIO_G722					0xB3
#define AUDIO_G723					0xB4
#define AUDIO_G726_40				0xB5
#define AUDIO_G726_32				0xB6
#define AUDIO_G726_24				0xB7
#define AUDIO_G726_16				0xB8
#define AUDIO_AAL2_G726_40			0xB9
#define AUDIO_AAL2_G726_32			0xBA
#define AUDIO_AAL2_G726_24			0xBB
#define AUDIO_AAL2_G726_16			0xBC
#define AUDIO_G728					0xBD
#define AUDIO_G729D					0xBE
#define AUDIO_G729E					0xBF
#define AUDIO_GSM					0xC0
#define AUDIO_GSM_EFR				0xC1
#define AUDIO_ILBC					0xC2
#define AUDIO_AMR					0xC3
#define AUDIO_AMR_WB				0xC4
#define AUDIO_SILK					0xC5
#define AUDIO_LPC					0xC6
#define AUDIO_LPC1016				0xC7
#define AUDIO_LPC1015				0xC8
#define AUDIO_L16					0xC9
#define AUDIO_SPEEX					0xCA
#define AUDIO_L8					0xCB
#define AUDIO_MPA					0xCC
#define AUDIO_DVI4					0xCD
#define AUDIO_VDVI					0xCE
#define AUDIO_CN					0xCF
#define AUDIO_RED					0xD0
#define AUDIO_QCELP					0xD1
#define AUDIO_EVRC0					0xD2
#define AUDIO_EVRCB0				0xD3
#define AUDIO_G729             		0xD4
#define AUDIO_VIVOX            		0xD5
#define BE_FRAG_TYPE(t)	 			((t>=FILE_OSMF) && (t<=0xA2))
#define BE_APP_TYPE(t)	 			((t>=FILE_IOS) && (t<=0xA5))
#define BE_IMAGE_TYPE(t) 			((t>=FILE_IMAGE) && (t<0x90))
#define BE_VIDEO_TYPE(t) 			((t>=FILE_VIDEO) && (t<0x70))
#define BE_AUDIO_TYPE(t) 			((t>=FILE_AUDIO) && (t<0x80))
#define BE_VOIP_TYPE(t) 			((t>=AUDIO_UNKNOWN) && (t<=AUDIO_VIVOX))


#define FILE_IMAGE					0x80

#define FILE_JPG					0x84
#define FILE_BMP					0x85
#define FILE_GIF					0x86

#define MMS_TYPE					0x90
#define HTTP_STREAM_TYPE  			0x90
#define RTSP_RDT_TYPE				0x92
#define RTSP_RTP_TYPE				0x91

/*业务类型*/
#define SERVICE_FLAG_FD 			0		// FenDu Flag
#define SERVICE_FLAG_JC 			1		// JianCe Flag
// Defination of service type ;
#define SERVICE_IPPORT_WHITE    	0x01	//IP+PORT白名单
#define SERVICE_IPPORT_BLACK    	0x02	//IP+PORT黑名单
#define SERVICE_URL_WHITE     		0x03	//节目地址白名单
#define SERVICE_URL_BLACK     		0x04	//节目地址黑名单
#define SERVICE_HOST_WHITE    		0x05	//节目地址黑名单
#define SERVICE_HOST_BLACK     		0x06	//节目地址黑名单
//add by lqy 20120427
#define SERVICE_FAKE_SERVERIP     	0x07	//虚假服务器节点

// added by cxj 20100811
#define SERVICE_P2P_STATIC_WHITEIP	0x15
#define SERVICE_P2P_BI_WHITEIP		0x16
#define SERVICE_P2P_MI_WHITEIP		0x17

#define	SERVICE_AUDIO_SAMPLE_FD		0x11
#define	SERVICE_VIDEO_LABEL_FD		0x21
#define	SERVICE_SPEEKER_FD			0x22
#define	SERVICE_VIDEO_SAMPLE_FD		0x23
#define	SERVICE_VIDEO_FACE_FD		0x24
#define	SERVICE_AUDIO_SEX_FD		0x25

#define	SERVICE_IMAGE_SAMPLE_FD		0x31
#define	SERVICE_DYN_SUBSTR_FD		0x32


#define SERVICE_P2P_EMULE_SERVER	0x51		//
#define SERVICE_P2P_EMULE_KEY		0x52		//
#define SERVICE_P2P_EMULE_FILEID	0x53		//
#define SERVICE_P2P_EMULE_NODE		0x54		//

#define SERVICE_P2P_BT_INFOHASH		0x41		// BT Infohash Black List
#define SERVICE_P2P_BT_NODE			0x44		// BT Bone Node
#define SERVICE_P2P_BT_SERVICE		0x61		// BT DHT Index Server List

#define SERVICE_P2P_BT_ECLIPSE_IPLIST		0x45
#define SERVICE_P2P_BT_POLLUTION_IPLIST		0x46
#define SERVICE_P2P_EMULE_POLLUTION_IPLIST	0x55

#define SERVICE_P2P_BT_TRACKEIP		0x43		// BT tracke ip 
#define SERVICE_P2P_BT_PEERIP		0x47		// BT peer ip 
#define SERVICE_P2P_EMULE_PEERIP	0x56		// emule peer ip 


//
#define SERVICE_EMULE_SERVER       	0x71	//节目唯一ID白名单
#define SERVICE_EMULE_KEYS       	0x72	//节目唯一ID黑名单
#define SERVICE_EMULE_FHID    		0x73	//emule的hashkey黑名单
#define SERVICE_EMULE_FNAME   		0x74	//emule的hashkey黑名单
#define SERVICE_EMULE_NODE     		0x75	//emule的hashkey黑名单

#define SERVICE_IPPORT_WHITE_JC    	0x81	//IP+PORT白名单
#define SERVICE_IPPORT_BLACK_JC    	0x82	//IP+PORT黑名单
#define SERVICE_URL_WHITE_JC    	0x83	//IP+PORT白名单
#define SERVICE_URL_BLACK_JC    	0x84	//IP+PORT黑名单
#define SERVICE_HOST_WHITE_JC    	0x85	//IP+PORT黑名单
#define SERVICE_HOST_BLACK_JC    	0x86	//IP+PORT黑名单
#define SERVICE_LOAD_ACTION_JC   	0x87	/* liuxueli */
#define SERVICE_USER_REGION_JC    	0x88	/* liuxueli */

#define	SERVICE_AUDIO_SAMPLE_JC		0x91
#define	SERVICE_SPEEKER_JC			0x92
#define	SERVICE_AUDIO_CONT_JC		0x93
#define	SERVICE_AUDIO_SECEN_JC		0x94
#define	SERVICE_AUDIO_LANG_JC		0x97
#define	SERVICE_AUDIO_LANG_FULL_JC	0x98

#define	SERVICE_VIDEO_LOGO_JC		0xA1
#define	SERVICE_VIDEO_SRT_JC		0xA2
#define	SERVICE_VIDEO_SAMPLE_JC		0xA3
#define	SERVICE_VIDEO_FACE_JC		0xA4
#define	SERVICE_VIDEO_SEC_JC		0xA5

#define	SERVICE_IMAGE_SAMPLE_JC		0xB1
#define	SERVICE_IMAGE_KEYWORD_JC	0xB2
#define	SERVICE_IMG_FEEDBACK_IP		0xB3	
#define	SERVICE_IMG_FEEDBACK_SIZE	0xB4	


/*==================================消息头部==============================================*/
typedef struct msg_header_s 
{
	uint16_t  			magic; 
	uint16_t 			m_type;	 	
	uint32_t 			c_len; 	
}msg_header_t;

/*==================================指导消息==============================================*/
typedef struct metainfo_s
{
	char				pid[8];			//ID，由数据获取节点生成
	uint64_t			proglen;		//流文件的总字节长度，为0表示长度未知
	uint32_t			capip;			// 获取节点的IP
	uint8_t				protocol;		// 承载协议类型
	uint8_t				mediatype;		// 可能的媒体类型
	uint16_t			pad2;			// 固定填0
}metainfo_t;


/*==================================流数据消息==============================================*/
typedef struct av_data_s
{
	char				pid[8];			// 流文件的ID，由数据获取节点生成
	uint16_t			frag_seq;		// 片段序号
	int64_t				offset:48;		// 本次数据在整个流文件中的偏移位置
	char*				avs;			// 视音频流数据
}av_data_t;

/*==================================结果消息==================================================*/
typedef struct msg_result_s
{
	char				pid[8];			// 本流文件的ID，由数据获取节点生成
	uint8_t				servicetype;	// 业务类型
	int8_t				level;			// 检测结果置信度， [0, 100]
	uint16_t			pad;			// 预留，必须填0值
	uint32_t			cfgid;			// 命中的配置ID，值为0表示命中未知配置
	char*  				logindex;		//访问现场数据文件的存储路径URL，以"/0"结束
}msg_result_t;

#endif
