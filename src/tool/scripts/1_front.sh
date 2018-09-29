#! /bin/sh

LOG_SENDBACK="sendback.log"
LOG_PIDLIST="pidlst.log"
LOG_SUM="sum.log"
#KEYWORD1=192.168.1.101
#KEYWORD2=10.52.210.142 
KEYWORD1=124.234.244.160
KEYWORD2=208.166.50.105
LOG_TEMP="temp.log"
LOG_RESULT="front.txt"

echo "Merge $LOG_SENDBACK ..."
cat /home/guole/Prog/SendBackData_* > $LOG_SENDBACK

echo "Generate $LOG_PIDLIST ..."
cat /home/guole/Prog/find_prog.log  | grep $KEYWORD1 | grep $KEYWORD2 | awk '{print $4}' |sort -u > $LOG_PIDLIST

echo "Generate $LOG_SUM ..."
./sum $LOG_SENDBACK > $LOG_SUM

echo "Generate $LOG_RESULT ..."
for pid in `cat $LOG_PIDLIST`; do
   grep $pid $LOG_SUM >> $LOG_TEMP 
done
cat $LOG_TEMP | sort > $LOG_RESULT
rm -f $LOG_TEMP
wc -l $LOG_RESULT







