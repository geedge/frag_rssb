#include "asmis_log.h"

int main()
{
	int ret = 0;
	void*netlog_handle=NULL;
	netlog_handle = asmis_log_Init("testmain");
	ret = asmis_log_AppVer(netlog_handle,"2009-12-18T09:33:00", "20120203.1", "version11");
	
	struct info_port_used	info[4];
	info[0].nPort = 100;
	info[0].nProtocolType = 7;
	info[0].nPortType = 1;
	strcpy(info[0].sPortDesc, "test1 100,7,1");
	
	info[1].nPort = 200;
	info[1].nProtocolType = 7;
	info[1].nPortType = 1;
	strcpy(info[1].sPortDesc, "test2 200,7,1");
	
	info[2].nPort = 300;
	info[2].nProtocolType = 16;
	info[2].nPortType = 2;
	strcpy(info[2].sPortDesc, "test3 300,16,2");

	info[3].nPort = 400;
	info[3].nProtocolType = 16;
	info[3].nPortType = 3;
	strcpy(info[3].sPortDesc, "test4 400,16,3");
	
	ret = asmis_log_PortUsed(netlog_handle,info, 4);
	
	ret = asmis_log_RunStart(netlog_handle,0);
	
	int nloop = 100;
	char msg[256];

	struct info_rtd_flow rtdf[4];
	strcpy(rtdf[2].sValType, "处理");
	time(&(rtdf[2].nRTTS));
	rtdf[2].nDuration = 1;
	rtdf[2].nValue = 200;

	struct info_policy_update policy[4];
	strcpy(policy[0].sName, "policy1");
	strcpy(policy[0].sDesc, "desc of policy1");
	strcpy(policy[0].sVersion, "AAAAA1");
	policy[0].nTotal = 1000;
	policy[0].nNew = 10;
	policy[0].nDelete = 20;
	policy[0].nUpdate = 30;
	policy[0].nSize = 40000;

	strcpy(policy[1].sName, "policy2");
	strcpy(policy[1].sDesc, "desc of policy2");
	time(&(policy[1].nUpdateTime));
	strcpy(policy[1].sVersion, "AAAAA2");
	policy[1].nTotal = 2000;
	policy[1].nNew = 20;
	policy[1].nDelete = 30;
	policy[1].nUpdate = 40;
	policy[1].nSize = 80000;

	strcpy(policy[2].sName, "policy3");
	strcpy(policy[2].sDesc, "desc of policy3");
	strcpy(policy[2].sVersion, "AAAAA3");
	policy[2].nTotal = 3000;
	policy[2].nNew = 30;
	policy[2].nDelete = 40;
	policy[2].nUpdate = 50;
	policy[2].nSize = 160000;

	while(nloop--) {
		sleep(10);
		if(nloop % 22 == 0) {
			time(&(policy[0].nUpdateTime));
			time(&(policy[2].nUpdateTime));
			ret = asmis_log_Policy(netlog_handle,policy, 3);
		} else if(nloop % 11 == 0) {
			time(&(policy[0].nUpdateTime));
			ret = asmis_log_Policy(netlog_handle,policy, 1);
		} else if(nloop % 7 == 0) {
			sprintf(msg, "database is down! and nloop is %d", nloop);
			ret = asmis_log_LogMsg(netlog_handle,msg, "1.1.1.1.2.1", ASMIS_ALARM);
		} else if(nloop % 5 == 0) {
			sprintf(msg, "can't open file! and nloop is %d", nloop);
			ret = asmis_log_LogMsg(netlog_handle,msg, "1.3.2.1.2.1", ASMIS_ALARM);
		} else if(nloop % 3 == 0) {
			strcpy(rtdf[0].sValType, "输入");
			time(&(rtdf[0].nRTTS));
			rtdf[0].nDuration = 1;
			rtdf[0].nValue = 2100;

			strcpy(rtdf[1].sValType, "输出");
			time(&(rtdf[1].nRTTS));
			rtdf[1].nDuration = 1;
			rtdf[1].nValue = 1100;

			time_t tt1;
			time(&tt1);
			asmis_log_RtdFlow(netlog_handle,tt1, 5, rtdf, 2);
		} else if(nloop % 2 == 0) {
			time_t tt2;
			time(&tt2);
			asmis_log_RtdFlow(netlog_handle,tt2, 5, rtdf, 3);
		}
		
		asmis_log_HeartBeat(netlog_handle,"test heartbeat");
	}

	ret = asmis_log_RunStop(netlog_handle,0);

	return ret;
}

