#########################################################################################################################
# Version: 1.1														#
# Date   : 2010-09-17													#
# Author : liuganggang													#
# E-mail : liuganggang@software.ict.ac.cn										#
# Purpose: Check log files to see the problems										#
#															#
# 															#
#########################################################################################################################

LOG="./washout.log"
AV_CHECK_CONF_FILE_NAME="./check.conf"
ct_prog_name=`cat $AV_CHECK_CONF_FILE_NAME | awk '{print $1}'| tr $'\x0d\x0a' $'\x20'`
line=`cat $AV_CHECK_CONF_FILE_NAME | wc -l`


for ((loop=1;loop<= $line;++loop))
do
  cur_prog_name=`echo "$ct_prog_name"|awk '{print $'$loop'}'`
# echo "$cur_prog_name"
  cat $LOG|grep $cur_prog_name>>result1.txt
done
cat result1.txt | awk '{print $4,$8}'|sort > result2.txt




