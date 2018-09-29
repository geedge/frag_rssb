#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>

#include "librdkafka/rdkafka.h"  /* for Kafka driver */

static int run = 1;
static rd_kafka_t *rk;
rd_kafka_topic_t *rkt;
int partition = RD_KAFKA_PARTITION_UA;
rd_kafka_topic_conf_t *topic_conf;

static void stop (int sig) {
	run = 0;
	fclose(stdin); /* abort fgets() */
}

static void sig_usr1 (int sig) {
	rd_kafka_dump(stdout, rk);
}

int initProducer (char *parameters) {
    int argc = 1;
    char **argv;
    char *para;
    char *delim = " ";
	char *brokers = "localhost:9092";
	char *topic = NULL;
	int opt;
	rd_kafka_conf_t *conf;
	char errstr[512];
	char tmp[16];

	char copyParameters[1024];
	strcpy(copyParameters, parameters);
	para = strtok(parameters, delim);
    argc++;
	while((para = strtok(NULL, delim)) != NULL){
        argc++;
	}
	argv = (char**)malloc(argc*sizeof(char*));
	argc = 0;
	argv[argc] = "initProducer";
	para = strtok(copyParameters, delim);
	argc++;
	argv[argc] = para;
	while((para = strtok(NULL, delim)) != NULL){
        argc++;
        argv[argc] = para;
	}
	argc++;
	/* Kafka configuration */
	conf = rd_kafka_conf_new();
	/* Quick termination */
	snprintf(tmp, sizeof(tmp), "%i", SIGIO);
	rd_kafka_conf_set(conf, "internal.termination.signal", tmp, NULL, 0);
	/* Topic configuration */
	topic_conf = rd_kafka_topic_conf_new();
	while ((opt = getopt(argc, argv, "PCLt:p:b:z:qd:o:eX:As:")) != -1) {
		switch (opt) {
		case 't':
			topic = optarg;
			break;
		case 'p':
			partition = atoi(optarg);
			break;
		case 'b':
			brokers = optarg;
			break;
        default:
			fprintf(stderr, "%% Failed to init producer with error parameters\n");
		}
	}
	if (optind != argc || !topic) {
		exit(1);
	}
	signal(SIGINT, stop);
	signal(SIGUSR1, sig_usr1);
    /* Create Kafka handle */
    if (!(rk = rd_kafka_new(RD_KAFKA_PRODUCER, conf, errstr, sizeof(errstr)))) {
        fprintf(stderr, "%% Failed to create new producer: %s\n", errstr);
        exit(1);
    }
    rd_kafka_set_log_level(rk, LOG_DEBUG);
    /* Add brokers */
    if (rd_kafka_brokers_add(rk, brokers) == 0) {
        fprintf(stderr, "%% No valid brokers specified\n");
        exit(1);
    }
    /* Create topic */
    rkt = rd_kafka_topic_new(rk, topic, topic_conf);
    topic_conf = NULL; /* Now owned by topic */
	return 1;
}

int freeProducer()
{
    /* Destroy topic */
    rd_kafka_topic_destroy(rkt);
    /* Destroy the handle */
    rd_kafka_destroy(rk);
    if (topic_conf)
        rd_kafka_topic_conf_destroy(topic_conf);
	/* Let background threads clean up and terminate cleanly. */
	run = 5;
	while (run-- > 0 && rd_kafka_wait_destroyed(1000) == -1)
		printf("Waiting for librdkafka to decommission\n");
	if (run <= 0)
		rd_kafka_dump(stdout, rk);
    return 1;
}

int main (int argc, char **argv)
{
    char parameter[] = "-t DF-HTTP-KEYWORD-LOG -b 10.10.6.101:9092,10.10.6.102:9092,10.10.6.104:9092";
    char buf[1024];
    initProducer(parameter);
    while (run && fgets(buf, sizeof(buf), stdin)) {
        if(rd_kafka_produce(rkt, partition, RD_KAFKA_MSG_F_COPY, buf, strlen(buf), NULL, 0, NULL) == -1){
            fprintf(stderr, "%% Failed to produce to topic %s partition %i: %s\n", rd_kafka_topic_name(rkt), partition, rd_kafka_err2str(rd_kafka_last_error()));
        }else{
            fprintf(stderr, "%% Sent %zd bytes to topic %s partition %i\n", strlen(buf), rd_kafka_topic_name(rkt), partition);
        }
    }
    freeProducer();
    return 0;
}

