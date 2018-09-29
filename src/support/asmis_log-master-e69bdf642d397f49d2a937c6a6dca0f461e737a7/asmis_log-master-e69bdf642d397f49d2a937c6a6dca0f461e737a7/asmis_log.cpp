/*####################################################################################*//**
*#  @FILE: netlog.c
*#  DESCRIPTION:Frist build by sunyong@iie.ac.cn
*#  				Reconstruct by zhengchao@iie.ac.cn 20130422 to support mutiple writer.
*#  $Id$
*####################################################################################*/
#include<pthread.h>
#include "asmis_log.h"

#define MAX_PATH_LEN	256
#define MAX_COM_LEN	512

#define LOGCONV_TIME_MIN 1
typedef struct _netlog_manage_space
{
	char	proc_name[MAX_PATH_LEN];
	char	base_path[MAX_PATH_LEN];
	time_t	logmsg_conv_time_n;
	char	logmsg_conv_time_s[MAX_PATH_LEN];
	time_t	userdef_conv_time_n;
	char	userdef_conv_time_s[MAX_PATH_LEN];
	pthread_mutex_t logging_mutex;
}netlog_handle_t;
int ASMIS_LOG_VERSION_20170906=0;
extern time_t g_AV_CurrentTime;
int system_cmd_mkdir(const char* path)
{
	char cmd[MAX_COM_LEN] = { 0 };
	snprintf(cmd, sizeof(cmd),"mkdir -p %s", path);
	return  system(cmd) ;
}
int system_cmd_mv(const char* src_file,const char*dst_file)
{
	char cmd[MAX_COM_LEN] = { 0 };
	snprintf(cmd,sizeof(cmd), "mv %s %s", src_file, dst_file);
	return  system(cmd);


}
/****************************************************************************
*  函数名：netlog_Init
*  函数功能：NetLOG初始化
*  完成工作：创建目录，初始化变量
*  函数备注：需要注意根据操作系统类型设置base_path
*            UNIX下为：/var/log/asmis/outer/ProcName
*            WINDOWS下为：C:\log\asmis\outer\ProcName
*
*
*			 pProcName:			应用程序名称
****************************************************************************/
void* asmis_log_Init(const char *pProcName)
{
	netlog_handle_t* handle=NULL;
	if(NULL == pProcName)
	{
		return NULL;
	}
	handle=(netlog_handle_t*)calloc(sizeof(netlog_handle_t),1);
	pthread_mutex_init(&(handle->logging_mutex), NULL);
	/* 初始化变量 */
	handle->logmsg_conv_time_n	= LOGCONV_TIME_MIN;
	handle->userdef_conv_time_n	= LOGCONV_TIME_MIN;

	/* 创建日志目录 */
	snprintf(handle->proc_name,MAX_PATH_LEN,"%s",pProcName);
	snprintf(handle->base_path,MAX_PATH_LEN, "/var/log/asmis/outer/%s", pProcName);
	

	if(-1 == system_cmd_mkdir(handle->base_path))
	{
		return NULL;
	}

	return handle;
}

/****************************************************************************
*  函数名：  asmis_log_AppVer
*  函数功能：登记程序版本信息
*  完成工作：
*  函数备注：程序启动时，写入版本信息
*     <Time>2009-12-18T09:33:00</Time>
*     <No>20091201.1016</No>
*     <Desc>数据分析程序，以多线程方式运行</Desc>
****************************************************************************/
int asmis_log_AppVer(void* netlog_handle,const char *pVersionTime, const char *pVersionNO, const char *pVersionDesc)
{
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle )
	{
		return(-1);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	sprintf(chPath, "%s/app_version_%02d%02d%02d%02d.xml", handle->base_path, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	sprintf(tmp_chPath, "%s.ing", chPath);
	fp = fopen(tmp_chPath, "wb");
	if(fp == NULL)
	{
		printf("Create %s faile\n", chPath);
		return(-1);
	}

	/* 写入版本信息 */
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n"
		"\t<Version>\n"
		"\t\t<Time>%s</Time>\n"
		"\t\t<No>%s</No>\n"
		"\t\t<Desc>%s</Desc>\n"
		"\t</Version>\n"
		"</Application>\n"
		"</AsmisApp>\n",
		handle->proc_name,
		pVersionTime,
		pVersionNO,
		pVersionDesc);

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath,chPath) )
	{
		return(-1);
	}

	return(0);
}

/****************************************************************************
*  函数名：  asmis_log_RunStart
*  函数功能：登记程序启动信息
*  完成工作：
*  函数备注：程序启动时调用，检测了ContiRun的合法性，未强制返回
*
*     <ContiRun>%d</ContiRun>
****************************************************************************/
int asmis_log_RunStart(void* netlog_handle,int nContiRun)
{
	int ret = 0;
	netlog_handle_t* handle=(netlog_handle_t* )netlog_handle;
	if(NULL == handle)
	{
		return(-1);
	}

	if(nContiRun < 0 || nContiRun > 60)
	{
		ret = -2;
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	sprintf(chPath, "%s/app_runstart_%02d%02d%02d%02d.xml", handle->base_path, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	sprintf(tmp_chPath, "%s.ing", chPath);
	fp = fopen(tmp_chPath, "wb");
	if(fp == NULL)
	{
		printf("Create %s faile\n", chPath);
		return(-1);
	}

	/* 写入启动信息 */
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n"
		"\t<RunMsg>\n"
		"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
		"\t\t<Text>应用程序启动成功！</Text>\n"
		"\t\t<Type>start</Type>\n"
		"\t\t<ContiRun>%d</ContiRun>\n"
		"\t</RunMsg>\n"
		"</Application>\n"
		"</AsmisApp>\n",
		handle->proc_name,
		tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
		nContiRun);

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath, chPath) )
	{
		return(-1);
	}

	return(ret);
}

/****************************************************************************
*  函数名：  asmis_log_RunStop
*  函数功能：登记程序停止信息
*  完成工作：
*  函数备注：程序停止时调用，检测了ContiRun的合法性，未强制返回
*
*     <ContiRun>%d</ContiRun>
****************************************************************************/
int asmis_log_RunStop(void* netlog_handle,int nContiRun)
{
	int ret = 0;
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle)
	{
		return(-1);
	}

	if(nContiRun < 0 || nContiRun > 60)
	{
		ret = -2;
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	snprintf(chPath,sizeof(chPath), "%s/app_runstop_%02d%02d%02d%02d.xml", handle->base_path, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	snprintf(tmp_chPath, sizeof(tmp_chPath),"%s.ing", chPath);
	fp = fopen(tmp_chPath, "wb");
	if(fp == NULL)
	{
		printf("Create %s faile\n", chPath);
		return(-1);
	}

	/* 写入停止信息 */
	fprintf(fp, "<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n"
		"\t<RunMsg>\n"
		"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
		"\t\t<Text>应用程序退出！</Text>\n"
		"\t\t<Type>stop</Type>\n"
		"\t\t<ContiRun>%d</ContiRun>\n"
		"\t</RunMsg>\n"
		"</Application>\n"
		"</AsmisApp>\n",
		handle->proc_name,
		tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
		nContiRun);

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath, chPath) )
	{
		return(-1);
	}
	return(ret);
}

/****************************************************************************
*  函数名：  asmis_log_LogMsg
*  函数功能：程序运行日志信息
*  完成工作：
*  函数备注：检测nAlarmType，强制返回
*      NO编码检测可以以后添加
*    <Time>2009-12-18T09:33:00</Time>
*    <Text>连接数据库1失败！ORA-1031</Text>
*    <No>1.1.1.2.1</No>
*    <Type>alarm</Type>
****************************************************************************/
int asmis_log_LogMsg(void* netlog_handle,const char *pMsg, const char *pNo, int nAlarmType)
{
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle)
	{
		return(-1);
	}

	if(nAlarmType !=ASMIS_KEY&&nAlarmType !=ASMIS_ALARM&& nAlarmType !=ASMIS_OTHER)
	{
		return(-2);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;
	char		tmp_logmsg_path[MAX_PATH_LEN],new_logmsg_path[MAX_PATH_LEN];
	char		current_logmsg_path[MAX_PATH_LEN];
	struct stat	fileinfo;
	if(0!=pthread_mutex_trylock(&(handle->logging_mutex)))
	{
		return -1;
	}
	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	FILE *fp = NULL;

	snprintf(current_logmsg_path,sizeof(current_logmsg_path), "%s/app_logmsg_%s.xml.ing", handle->base_path, handle->logmsg_conv_time_s);
	bzero(&fileinfo, sizeof(struct stat) );
	stat(current_logmsg_path, &fileinfo);

	if( (LOGCONV_TIME_MIN == handle->logmsg_conv_time_n) || (0 == strlen(handle->logmsg_conv_time_s) ) ) /* 非追加，新建文件 */
	{
		if(pMsg == NULL)
		{
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(0);
		}
		sprintf(handle->logmsg_conv_time_s, "%02d%02d%02d%02d", tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
		sprintf(tmp_logmsg_path, "%s/app_logmsg_%s.xml.ing", handle->base_path, handle->logmsg_conv_time_s);

		if(NULL == (fp = fopen(tmp_logmsg_path, "wb") ) )
		{
			handle->logmsg_conv_time_n = LOGCONV_TIME_MIN;

			printf("Create %s failed\n", tmp_logmsg_path);
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(-1);
		}
		else
		{
			handle->logmsg_conv_time_n = i_curtime;

			fprintf(fp,
				"<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
				"<AsmisApp Version=\"1.0\">\n"
				"<Application>\n"
				"\t<ProcName>%s</ProcName>\n",
				handle->proc_name);
		}
	}
	else if(i_curtime - handle->logmsg_conv_time_n > ASMIS_LOGMSG_TIMEOUT ||
		fileinfo.st_size >= ASMIS_LOGMSG_LENGTH) /* 追加文件，超过5分钟，或者文件长度超过40K 关闭旧文件，开始新文件 */
	{
		sprintf(tmp_logmsg_path, "%s/app_logmsg_%s.xml.ing", handle->base_path, handle->logmsg_conv_time_s);

		if(NULL != (fp = fopen(tmp_logmsg_path, "ab") ) )
		{
			fprintf(fp,
				"</Application>\n"
				"</AsmisApp>\n");

			fclose(fp);
			fp = NULL;

			/* 将临时文件名修改为正式文件名 */
			snprintf(new_logmsg_path,sizeof(new_logmsg_path),"%s/app_logmsg_%s.xml",handle->base_path,handle->logmsg_conv_time_s);
			if(-1 == rename(tmp_logmsg_path,new_logmsg_path) )
			{
				handle->logmsg_conv_time_n	= LOGCONV_TIME_MIN;
				pthread_mutex_unlock(&(handle->logging_mutex));
				return(-1);
			}
		}
		if(pMsg == NULL)
		{
			handle->logmsg_conv_time_n	= LOGCONV_TIME_MIN;
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(0);
		}
		snprintf(handle->logmsg_conv_time_s,sizeof(handle->logmsg_conv_time_s), "%02d%02d%02d%02d", tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
		sprintf(tmp_logmsg_path, "%s/app_logmsg_%s.xml.ing", handle->base_path, handle->logmsg_conv_time_s);

		if(NULL == (fp = fopen(tmp_logmsg_path, "wb") ) )
		{
			handle->logmsg_conv_time_n = LOGCONV_TIME_MIN;

			printf("Create %s failed\n", tmp_logmsg_path);
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(-1);
		}
		else
		{
			handle->logmsg_conv_time_n = i_curtime;

			fprintf(fp,
				"<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
				"<AsmisApp Version=\"1.0\">\n"
				"<Application>\n"
				"\t<ProcName>%s</ProcName>\n",
				handle->proc_name);
		}
	}
	else    /* 追加文件 */
	{
		if(pMsg == NULL)
		{
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(0);
		}
		sprintf(tmp_logmsg_path, "%s/app_logmsg_%s.xml.ing", handle->base_path, handle->logmsg_conv_time_s);

		if(NULL == (fp = fopen(tmp_logmsg_path, "ab") ) )
		{
			printf("Open %s failed\n", tmp_logmsg_path);
			pthread_mutex_unlock(&(handle->logging_mutex));
			return(-1);
		}
	}

	switch(nAlarmType)
	{
		case ASMIS_KEY:
			fprintf(fp,
				"\t<LogMsg>\n"
				"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
				"\t\t<Text>%s</Text>\n"
				"\t\t<No>%s</No>\n"
				"\t\t<Type>key</Type>\n"
				"\t</LogMsg>\n",
				tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
				pMsg, pNo);
			break;
		case ASMIS_ALARM:
			fprintf(fp,
				"\t<LogMsg>\n"
				"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
				"\t\t<Text>%s</Text>\n"
				"\t\t<No>%s</No>\n"
				"\t\t<Type>alarm</Type>\n"
				"\t</LogMsg>\n",
				tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
				pMsg, pNo);
			break;
		case ASMIS_OTHER:
			fprintf(fp,
				"\t<LogMsg>\n"
				"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
				"\t\t<Text>%s</Text>\n"
				"\t\t<No>%s</No>\n"
				"\t\t<Type>other</Type>\n"
				"\t</LogMsg>\n",
				tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
				pMsg, pNo);
			break;
		default:
			break;
	}

	fclose(fp);
	pthread_mutex_unlock(&(handle->logging_mutex));
	return(0);
}

/****************************************************************************
*  函数名：  asmis_log_RtdFlow
*  函数功能：程序实时流量统计信息
*  完成工作：
*  函数备注：
*      <Time>2009-12-18T09:25:00</Time>
*      <Duration>5</Duration>
*      <Flow>
*       <ValType>处理</ValType>
*       <BDType>leixing</BDType>
*       <RTTimeSlice>2009-12-18T09:25:00</RTTimeSlice>
*       <RTTSDuration>1</RTTSDuration>
*       <Value>2010</Value>
*      </Flow>
****************************************************************************/
int asmis_log_RtdFlow(void* netlog_handle,time_t nStartTime, int nDuration, struct info_rtd_flow *info, int nFlow)
{
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle)
	{
		return(-1);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	snprintf(chPath,sizeof(chPath), "%s/app_rtdflow_%02d%02d%02d%02d.xml", handle->base_path, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	snprintf(tmp_chPath,sizeof(tmp_chPath), "%s.ing", chPath);

	if(NULL == (fp = fopen(tmp_chPath, "wb") ) )
	{
		printf("Create %s faile\n", chPath);
		return(-1);
	}

	/* 写入实时流量统计信息 */
	localtime_r(&nStartTime, &tm_curtime);
	fprintf(fp,
		"<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n"
		"\t<RTDataFlow>\n"
		"\t\t<!-- 流量值统计时刻 -->\n"
		"\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
		"\t\t<Duration>%d</Duration>\n",
		handle->proc_name,
		tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
		( (nDuration > 0) ? (nDuration) : (1) ) );

	int i;
	for(i = 0; i < nFlow; i++)
	{
		localtime_r(&(info[i].nRTTS), &tm_curtime);
		fprintf(fp,
			"\t\t<Flow>\n"
			"\t\t\t<ValType>%s</ValType>\n"
			"\t\t\t<BDType>%s</BDType>\n"
			"\t\t\t<RTTimeSlice>%d-%02d-%02dT%02d:%02d:%02d</RTTimeSlice>\n"
			"\t\t\t<RTTSDuration>%d</RTTSDuration>\n"
			"\t\t\t<Value>%llu</Value>\n"
			"\t\t</Flow>\n",
			info[i].sValType, info[i].sBDType,
			tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
			( (info[i].nDuration > 0) ? info[i].nDuration : 1), info[i].nValue);
	}

	fprintf(fp,
		"\t</RTDataFlow>\n"
		"</Application>\n"
		"</AsmisApp>\n");

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath,chPath) )
	{
		return(-1);
	}

	return(0);
}

/****************************************************************************
*  函数名：  asmis_log_PortUsed
*  函数功能：登记程序使用端口信息
*  完成工作：
*  函数备注：程序启动监听端口时进行登记，登记nPort个端口
*      检测了PortType的合法性，未强制返回
*
*      <Port>8888</Port>
*      <ProtocolType>6</ProtocolType>
*      <PortType>1</PortType>
*      <PortDesc>接收数据监听端口</PortDesc>
****************************************************************************/
int asmis_log_PortUsed(void* netlog_handle,struct info_port_used *info, int nPort)
{
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	int ret = 0;

	if(NULL == handle )
	{
		return(-1);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	sprintf(chPath, "%s/app_port_used.xml", handle->base_path);
	sprintf(tmp_chPath, "%s.ing", chPath);

	if(NULL == (fp = fopen(tmp_chPath, "wb") ) )
	{
		printf("Create %s faile\n", chPath);
		return(-1);
	}

	/* 写入端口信息 */
	fprintf(fp,
		"<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n"
		"\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n",
		handle->proc_name,
		tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);

	int i;
	for(i = 0; i < nPort; i++)
	{
		fprintf(fp,
			"\t<PortMsg>\n"
			"\t\t<Port>%d</Port>\n"
			"\t\t<ProtocolType>%d</ProtocolType>\n"
			"\t\t<PortType>%d</PortType>\n"
			"\t\t<PortDesc>%s</PortDesc>\n"
			"\t</PortMsg>\n",
			info[i].nPort, info[i].nProtocolType, info[i].nPortType, info[i].sPortDesc);

		if(info[i].nPortType < 1 || info[i].nPortType > 3)
		{
			ret = -2;
		}
	}

	fprintf(fp,
		"</Application>\n"
		"</AsmisApp>\n");

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath, chPath) )
	{
		return(-1);
	}

	return(ret);
}

/****************************************************************************
*  函数名：  asmis_log_HeartBeat
*  函数功能：程序运行心跳信息
*  完成工作：
*  函数备注：和日志文件共用同样的时间节点进行日志切换
****************************************************************************/


/*
 * int asmis_log_HeartBeat(const char *pMsg)
 * {
 * if(NULL == proc_name || 0 == strlen(proc_name))
 * return -1;
 *
 * struct tm tm_curtime;
 * time_t i_curtime;
 * char         tmp_userdef_path[MAX_PATH_LEN];
 *
 * time(&i_curtime);
 * localtime_r(&i_curtime,&tm_curtime);
 *
 * FILE *fp = NULL;
 *
 *
 * asmis_log_LogMsg(NULL,NULL,ASMIS_ALARM);
 *
 * if((LOGCONV_TIME_MIN == userdef_conv_time_n) || (0 == strlen(userdef_conv_time_s))) {	//非追加，新建文件
 * sprintf(userdef_conv_time_s, "%02d%02d%02d%02d", tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
 * sprintf(tmp_userdef_path, "%s/app_userdef_%s.xml.ing", base_path, userdef_conv_time_s);
 *
 * if(NULL == (fp = fopen(tmp_userdef_path, "wb")))
 * {
 * userdef_conv_time_n = LOGCONV_TIME_MIN;
 *
 * printf("Create %s faile\n", tmp_userdef_path);
 * return -1;
 * } else {
 * userdef_conv_time_n = i_curtime;
 *
 * fprintf(fp,
 *      "<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
 *      "<AsmisApp Version=\"1.0\">\n"
 *      "<Application>\n"
 *      "\t<ProcName>%s</ProcName>\n",
 *      proc_name);
 * }
 * } else if(i_curtime - userdef_conv_time_n > ASMIS_LOGMSG_TIMEOUT) {	//追加文件，超过5分钟，关闭旧文件，开始新文件
 * sprintf(tmp_userdef_path, "%s/app_userdef_%s.xml.ing", base_path, userdef_conv_time_s);
 *
 * if(NULL != (fp = fopen(tmp_userdef_path, "ab")))
 * {
 * fprintf(fp,
 *      "</Application>\n"
 *      "</AsmisApp>\n");
 *
 * fclose(fp);
 * fp = NULL;
 *
 * //将临时文件名修改为正式文件名
 * char cmd[MAX_COM_LEN] = {0};
 * sprintf(cmd, "mv %s/app_userdef_%s.xml.ing %s/app_userdef_%s.xml", base_path, userdef_conv_time_s, base_path, userdef_conv_time_s);
 * if(-1 == system(cmd)) {
 *  userdef_conv_time_n = LOGCONV_TIME_MIN;
 *  return -1;
 * }
 * }
 *
 * sprintf(userdef_conv_time_s, "%02d%02d%02d%02d", tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
 * sprintf(tmp_userdef_path, "%s/app_userdef_%s.xml.ing", base_path, userdef_conv_time_s);
 *
 * if(NULL == (fp = fopen(tmp_userdef_path, "wb")))
 * {
 * userdef_conv_time_n = LOGCONV_TIME_MIN;
 *
 * printf("Create %s faile\n", tmp_userdef_path);
 * return -1;
 * } else {
 * userdef_conv_time_n = i_curtime;
 *
 * fprintf(fp,
 *      "<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
 *      "<AsmisApp Version=\"1.0\">\n"
 *      "<Application>\n"
 *      "\t<ProcName>%s</ProcName>\n",
 *      proc_name);
 * }
 * } else {	//追加文件
 * sprintf(tmp_userdef_path, "%s/app_userdef_%s.xml.ing", base_path, userdef_conv_time_s);
 *
 * if(NULL == (fp = fopen(tmp_userdef_path, "ab")))
 * {
 * printf("Open %s faile\n", tmp_userdef_path);
 * return -1;
 * }
 * }
 *
 * fprintf(fp,
 *    "\t<UserMsg>\n"
 *    "\t\t<Time>%d-%02d-%02dT%02d:%02d:%02d</Time>\n"
 *    "\t\t<Text>%s</Text>\n"
 *    "\t\t<Type>HeartBeat</Type>\n"
 *    "\t</UserMsg>\n",
 *    tm_curtime.tm_year+1900, tm_curtime.tm_mon+1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
 *    pMsg);
 * fclose(fp);
 *
 * return 0;
 * }
 */

int asmis_log_HeartBeat(void*netlog_handle,const char *pMsg)
{
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle )
	{
		return(-1);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;
	char		tmp_userdef_path[MAX_PATH_LEN];

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	FILE *fp = NULL;

	snprintf(tmp_userdef_path,sizeof(tmp_userdef_path), "%s/app_heartbeat.txt", handle->base_path);

	if(NULL == (fp = fopen(tmp_userdef_path, "wb") ) )
	{
/*		userdef_conv_time_n = LOGCONV_TIME_MIN; */

		printf("Create %s faile\n", tmp_userdef_path);
		return(-1);
	}
	else
	{
		fprintf(fp, "%d-%02d-%02dT%02d:%02d:%02d", tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	}
	fclose(fp);
	return(0);
}

/****************************************************************************
*  函数名：  asmis_log_Policy
*  函数功能：登记策略更新信息
*  完成工作：
*  函数备注：程序启动或者策略更新时记录
*      检测了PortType的合法性，未强制返回
*
*     <Name>内容放行</Name>
*      <Desc>进行内容放行分析的策略模板内容</Desc>
*      <UpdateTime>2009-12-18T09:33:00</UpdateTime>
*      <CurrPolicy>
*       <Version>987FFECB0B </Version>
*       <TotalCount>35</TotalCount>
*       <NewCount>6</NewCount>
*       <DelCount>1</DelCount>
*       <UpdCount>0</UpdCount>
*       <Size>1050</Size>
*      </CurrPolicy>
****************************************************************************/
int asmis_log_Policy(void * netlog_handle,struct info_policy_update *info, int nPolicy)
{
	int ret = 0;
	netlog_handle_t* handle=(netlog_handle_t*)netlog_handle;
	if(NULL == handle )
	{
		return(-1);
	}

	struct tm	tm_curtime;
	time_t		i_curtime;

	time(&i_curtime);
	localtime_r(&i_curtime, &tm_curtime);

	/* 生成文件名，打开临时文件 */
	char	chPath[MAX_PATH_LEN];
	char	tmp_chPath[MAX_PATH_LEN];
	FILE	*fp = NULL;

	snprintf(chPath,sizeof(chPath), "%s/app_policy_%02d%02d%02d%02d.xml", handle->base_path, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec);
	snprintf(tmp_chPath,sizeof(tmp_chPath), "%s.ing", chPath);

	if(NULL == (fp = fopen(tmp_chPath, "wb") ) )
	{
		printf("Create %s failed\n", chPath);
		return(-1);
	}

	/* 写入端口信息 */
	fprintf(fp,
		"<?xml version=\"1.0\" encoding=\"gb18030\" ?>\n"
		"<AsmisApp Version=\"1.0\">\n"
		"<Application>\n"
		"\t<ProcName>%s</ProcName>\n",
		handle->proc_name);

	int i;
	for(i = 0; i < nPolicy; i++)
	{
		localtime_r(&(info[i].nUpdateTime), &tm_curtime);
		fprintf(fp,
			"\t<PolicyInfo>\n"
			"\t\t<Name>%s</Name>\n"
/*							"\t\t<Desc>%s</Desc>\n" */
			"\t\t<UpdateTime>%d-%02d-%02dT%02d:%02d:%02d</UpdateTime>\n"
			"\t\t<CurrPolicy>\n"
			"\t\t\t<Version>%s</Version>\n"
			"\t\t\t<TotalCount>%d</TotalCount>\n"
			"\t\t\t<NewCount>%d</NewCount>\n"
			"\t\t\t<DelCount>%d</DelCount>\n"
			"\t\t\t<UpdCount>%d</UpdCount>\n"
			"\t\t\t<Size>%d</Size>\n"
			"\t\t</CurrPolicy>\n"
			"\t</PolicyInfo>\n",
/*							info[i].sName, info[i].sDesc, */
			info[i].sName,
			tm_curtime.tm_year + 1900, tm_curtime.tm_mon + 1, tm_curtime.tm_mday, tm_curtime.tm_hour, tm_curtime.tm_min, tm_curtime.tm_sec,
			info[i].sVersion, info[i].nTotal,
			info[i].nNew, info[i].nDelete, info[i].nUpdate, info[i].nSize);
	}

	fprintf(fp,
		"</Application>\n"
		"</AsmisApp>\n");

	fclose(fp);

	/* 将临时文件名修改为正式文件名 */
	if(-1 == rename(tmp_chPath, chPath) )
	{
		return(-1);
	}

	return(ret);
}

/**
 * Moidfy History:
 * $Log
 */
