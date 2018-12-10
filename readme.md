### 简介
frag_rssb音视频后端程序，支持音视频、图片、VOIP、文件摘要的数据接收。
支持K项目与ZX项目。

### 编译
make PROJECT=K MODE=PIC

PROKECT表示项目，可以取值为K，默认为Z

MODE表示业务，可以取值为PIC,VOIP,默认为AV

摘要与PIC共享。

### 端口约定

## AV
- 数据接收端口 BizmanPort=22082
- 发送响应给前端 BizmanAckPort=22084

## PIC与摘要

## VOIP