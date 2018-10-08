# CREATED by Guo Le, 08/23/2010
#
CC		=   gcc
PROGRAM = sendsurvey
CFLAGS  = -g -Wall 
INCLUDE = -I. -I./common 
LDFLAGS	= 	-lpthread -Wall 
OBJS 	= backend.o	 
DEPLOY_DIR = /home/ictsoft/fake_backend
BUILD_DIR = ./debug

.PHONY : all clean 
all: $(PROGRAM) 


sendsurvey: sendsurvey.c
	gcc  $(INCLUDE) $(LDFLAGS) -g -Wall sendsurvey.c common.a -o sendsurvey 

$(OBJS): %.o: %.c 
	$(CC) -c $(CFLAGS) $(INCLUDE) $< -o $@


clean:
	$(RM) *.o $(PROGRAM)

localdeploy: $(PROGRAM)
	@echo "- Kill previous process ---------------------------------------------"
	@-killall -vq backend_video backend_audio;
	@echo "---------------------------------------------------------------------"
	@echo "- Check deploy dir --------------------------------------------------"
	@if [ ! -d "$(DEPLOY_DIR)" ]; \
	then echo "DEPLOY_DIR doesn't exist!"; mkdir -p $(DEPLOY_DIR); \
 	else echo "DEPLOY_DIR exist, remove all"; fi
	@-rm -rfv $(DEPLOY_DIR)/*
	@echo "---------------------------------------------------------------------"
	@echo "- Deploy ------------------------------------------------------------"
	@cp -rv $(BUILD_DIR)/* $(DEPLOY_DIR)
	@echo "---------------------------------------------------------------------"
	@echo "- Run ---------------------------------------------------------------"
	@cd $(DEPLOY_DIR); ./r2;
	@ps au | grep -e "backend" | grep -v grep 
