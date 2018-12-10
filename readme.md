# 1.简介
frag_rssb音视频后端程序，支持音视频、图片、VOIP、文件摘要的数据接收。
支持K项目与ZX项目。

# 2.编译
make PROJECT=K MODE=PIC

PROKECT表示项目，可以取值为K，默认为Z

MODE表示业务，可以取值为PIC,VOIP,默认为AV

摘要与PIC共享。

# 3.端口约定

## K项目

### AV

- 数据接收端口 BizmanPort=22082
- 发送响应给前端 BizmanAckPort=22084
- 数据发送端口 UdpSendPort=33082;33083
- 建议接收端口 CPZMsgPort=22080
- 建议发送端口 QDMsgPort=22080

### PIC

--- 数据接收端口 BizmanPort=22072
--- 发送响应给前端 BizmanAckPort=22084
--- 数据发送端口 UdpSendPort=33072
--- 建议接收端口 CPZMsgPort=22070
--- 建议发送端口 QDMsgPort=22080

### VOIP

--- 数据接收端口 BizmanPort=22092
--- 发送响应给前端 BizmanAckPort=22084
--- 数据发送端口 UdpSendPort=33092
--- 建议接收端口 CPZMsgPort=22090
--- 建议发送端口 QDMsgPort=22080

### 摘要

--- 数据接收端口 BizmanPort=22082
--- 发送响应给前端 BizmanAckPort=22084
--- 数据发送端口 UdpSendPort=33082
--- 建议接收端口 CPZMsgPort=22080
--- 建议发送端口 QDMsgPort=22080



## Z项目

### AV

--- 数据接收端口 BizmanPort=22082
--- 发送响应给前端 BizmanAckPort=22084
--- 建议接收端口 CPZMsgPort=22080
--- 建议发送端口 QDMsgPort=22080

### PIC

--- 数据接收端口 BizmanPort=22082
--- 发送响应给前端 BizmanAckPort=22084
--- 建议接收端口 CPZMsgPort=22080
--- 建议发送端口 QDMsgPort=22080

### VOIP

--- 数据接收端口 BizmanPort=22092
--- 发送响应给前端 BizmanAckPort=22084
--- 建议接收端口 CPZMsgPort=22080
--- 建议发送端口 QDMsgPort=22080
