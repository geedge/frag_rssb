#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <hiredis-vip/hircluster.h>

#define KEY_LEN 100

char data[2000000][KEY_LEN];

char* genRandom_str(int  len)
{
    int flag, i;
    char *string;
    srand((unsigned)time(NULL));
    if((string = (char *)malloc(len))==NULL)
    {
        printf("malloc error\n");
        return NULL;
    }
    for(i = 0;i < len-1; i++)
    {
        flag  = rand()%3;
        switch(flag)
        {
            case 0:
                string[i] = 'A' + rand()%26;
                break;
            case 1:
                string[i] = 'a' + rand()%26;
                break;
            case 2:
                string[i] = '0' + rand()%26;
                break;
            default:
                string[i] = '.';
                break;
        }
    }
    string[len-1] = '\0';
    return string;
}

int set_data(redisClusterContext *cc){
    int i;
    //char key[KEY_LEN],value[KEY_LEN];
    //strcpy(key,genRandom_str(KEY_LEN));
    //strcpy(value,genRandom_str(KEY_LEN));
    char *key = "156413221";
    char *value = "adsjhtquwtrhjfsdjghdjfkghfudghsdfjkghkjsdfhgjkdsfghufdghsjkdfghdsfjkghdfughsdfgjsdfhsdfkjghdufghsdfgadsjhtquwtrhjfsdjghdjfkghfudghsdfjkghkjsdfhgjkdsfghufdghsjkdfghdsfjkghdfughsdfgjsdfhsdfkjghdufghsdfgadsjhtquwtrhjfsdjghdjfkghfudghsdfjkghkjsdfhgjkdsfghufdghsjkdfghdsfjkghdfughsdfgjsdfhsdfkjghdufghsdfgadsjhtquwtrhjfsdjghdjfkghfudghsdfjkghkjsdfhgjkdsfghufdghsjkdfghdsfjkghdfughsdfgjsdfhsdfkjghdufghsdfgadsjhtquwtrhjfsdjghdjfkghfudghsdfjkghkjsdfhgjkdsfghufdghsjkdfghdsfjkghdfughsdfgjsdfhsdfkjghdufghsdfg";
    //key = genRandom_str(KEY_LEN);
    //value = genRandom_str(KEY_LEN);
    struct timeval start,end;
    gettimeofday(&start,NULL);
    for(i=0;i<1000000;i+=1000)
    {
        //redisReply *reply = NULL;
        
        //redisClusterCommand(cc,"MSET %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s \
                                    %s %s %s %s %s %s %s %s %s %s",\
                                    data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7],data[i+8],data[i+9],\
                                    data[i+10],data[i+21],data[i+12],data[i+13],data[i+14],data[i+15],data[i+16],data[i+17],data[i+18],data[i+19],\
                                    data[i+20],data[i+21],data[i+22],data[i+23],data[i+24],data[i+25],data[i+26],data[i+27],data[i+28],data[i+29],\
                                    data[i+30],data[i+31],data[i+32],data[i+33],data[i+34],data[i+35],data[i+36],data[i+37],data[i+38],data[i+39],\
                                    data[i+40],data[i+41],data[i+42],data[i+43],data[i+44],data[i+45],data[i+46],data[i+47],data[i+48],data[i+49],\
                                    data[i+50],data[i+51],data[i+52],data[i+53],data[i+54],data[i+55],data[i+56],data[i+57],data[i+58],data[i+59],\
                                    data[i+60],data[i+61],data[i+62],data[i+63],data[i+64],data[i+65],data[i+66],data[i+67],data[i+68],data[i+69],\
                                    data[i+70],data[i+71],data[i+72],data[i+73],data[i+74],data[i+75],data[i+76],data[i+77],data[i+78],data[i+79],\
                                    data[i+80],data[i+81],data[i+82],data[i+83],data[i+84],data[i+85],data[i+86],data[i+87],data[i+88],data[i+89],\
                                    data[i+90],data[i+91],data[i+92],data[i+93],data[i+94],data[i+95],data[i+96],data[i+97],data[i+98],data[i+99]);
                                    
        redisClusterAppendCommand(cc,"MSET %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s \
            %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s",\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key,\
            key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,key,value,value,key,value,key,key,value,value,key,key,value,value,key,key,value,value,key);
        //printf("MSET: %s\n",reply->str);
        //freeReplyObject(reply);  
    }
    gettimeofday(&end,NULL);
    double t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);   
    printf("total_time:%f\n",t);  
    redisCLusterReset(cc);
    gettimeofday(&end,NULL);
    t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
    printf("total_time:%f\n",t); 
    return 0;
}

int main()
{
    redisClusterContext *cc;
    const char *addr = "127.0.0.1:7001";

    cc = redisClusterConnect(addr,HIRCLUSTER_FLAG_NULL);
    if(cc == NULL||cc->err){
        printf("connect error : %s\n", cc == NULL?"NULL":cc->errstr);
        redisClusterFree(cc);
        return -1;
    }
    int j;
    //for(j=0;j<1000000;j++)strcpy(data[j],genRandom_str(KEY_LEN));
    //struct timeval start,end;
    //gettimeofday(&start,NULL);
    set_data(cc);
    //gettimeofday(&end,NULL);
    //double t = (end.tv_sec+end.tv_usec/1000000.0) - (start.tv_sec+start.tv_usec/1000000.0);
    //printf("total_time:%f\n",t);
    //redisCLusterReset(cc);
    redisClusterFree(cc);
    return 0;

}
