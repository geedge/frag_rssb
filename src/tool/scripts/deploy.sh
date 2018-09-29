#!/bin/sh
BACKEND="\
10.0.6.203"

slkdfj="10.52.210.181 \
10.52.210.182 \
10.52.210.183 \
10.52.210.184 \
10.52.210.185 \
10.52.210.186 \
10.52.210.187 \
10.52.210.188"

DEPLOY_TGZ=preproc.tgz
DEPLOY_DIR=/home/ictsoft/preproc
USER_NAME=root
USER_PASSWD='111111'

for ip in $BACKEND; do
    echo $ip;
    ping -c1 $ip;
	./kr "killall -vq preprocessor r3; mkdir -p $DEPLOY_DIR; rm -rf $DEPLOY_DIR/*" $ip $USER_NAME $USER_PASSWD
	./kscp $DEPLOY_TGZ $ip $DEPLOY_DIR $USER_NAME $USER_PASSWD
	./kr "cd $DEPLOY_DIR; tar -zxvf $DEPLOY_TGZ; rm -f $DEPLOY_TGZ; ./start" $ip $USER_NAME $USER_PASSWD
done

CHECK_RUN_LOG=runstat.log
for ip in $BACKEND; do
    echo $ip;
	./kr "cd $DEPLOY_DIR/log tail -f preproc*; ps aux |grep preproc" $ip $USER_NAME $USER_PASSWD >>$CHECK_RUN_LOG
done





