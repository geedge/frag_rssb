#ifndef _AV_SENDBACK_IN_H
#define _AV_SENDBACK_IN_H


#define AV_MEDAI_TYPE_MAXNUM  		128
#define AV_PROTO_MAXNUM  			16

/*协议类型，与前端一致*/
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

/*媒体类型，与前端一致*/
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

#define FILE_IMAGE					0x80

#define FILE_JPG					0x84
#define FILE_BMP					0x85
#define FILE_GIF					0x86

#define MMS_TYPE					0x90
#define HTTP_STREAM_TYPE  			0x90
#define RTSP_RDT_TYPE				0x92
#define RTSP_RTP_TYPE				0x91

/*媒体类型:文件摘要*/
#define FILE_CONTENT					0x03
#define	FILE_DOC						0xF0
#define	FILE_DOCX						0xF1
#define	FILE_XLS						0xF2
#define	FILE_XLSX						0xF3
#define	FILE_PPT						0xF4
#define	FILE_PPTX						0xF5
#define	FILE_PDF						0xF6
#define	FILE_EXE						0xF7
#define	FILE_APK						0xF8

typedef struct msg_map_s 
{
	uint8_t  			nodeA; 
	uint8_t 			nodeB;	 	
}msg_map_t;


#endif
