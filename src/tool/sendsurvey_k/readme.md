### 说明

-	按照K项目的接口，使用unixSocket而非UDP接口发送建议给粗拼装

### 粗拼装配置要求

- ./conf/main.conf中增加配置项目：UnixSocketRecvAddr=/home/mesasoft/frag_rssb/un_recv，按照实际运行环境修改

### 使用方法

程序名 粗拼装接收建议的socket PID service_type configID level logurl

例如：
./sendsurvey /home/mesasoft/frag_rssb/un_recv 11111111111111 37 212 100 192.168.1.1:/home/lishu/test.txt

logurl可以不写
./sendsurvey /home/mesasoft/frag_rssb/un_recv 11111111111111111 0 0 0
