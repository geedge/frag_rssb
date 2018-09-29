
ssh 10.60.1.15 "cd code/debug; ./stop" 
scp -r debug/* 10.60.1.15:code/debug
ssh 10.60.1.15 "cd code/debug; ./restart" 
