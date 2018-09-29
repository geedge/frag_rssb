#! /bin/sh

LOG_IPLIST=ip.back.lst
LOG_PIDLIST=pidlst.log
LOG_WASHOUT=washout.log
LOG_TEMP=temp.log

LOG_DIR=/home/ictsoft/preproc/log
USER_NAME=root
USER_PASSWD='111111'


for ip in `cat $LOG_IPLIST`; do
	FETCH_CMD="LOG_RESULT=\"backend.$ip.txt\";
	cd $LOG_DIR; 
	for pid in \`cat $LOG_PIDLIST\`;  
	do grep \$pid $LOG_WASHOUT >> $LOG_TEMP; 
	done; 
	awk '{print \$4,\$6,\$8}' $LOG_TEMP | sort > \$LOG_RESULT; 
	rm -f $LOG_TEMP"

	./kscp  $LOG_PIDLIST $ip $LOG_DIR $USER_NAME $USER_PASSWD
	./kr "$FETCH_CMD" $ip $USER_NAME $USER_PASSWD
	./kscpr $ip $LOG_DIR/backend.*.txt . $USER_NAME $USER_PASSWD
done

cat backend.* | sort > backend.txt
wc -l backend.txt


