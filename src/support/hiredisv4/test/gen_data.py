write_file = open("data.txt","w")
pair = ""
for i in range(1,1000001):
	key = 'hello'+str(i)+' '
	value = 'world'+str(i)+' '
	pair = pair+key+value
	if i%50 == 0:
		write_file.write(pair+'\n')
		pair = ""
