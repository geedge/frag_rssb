#ifndef _FRAG_JSON_H
#define _FRAG_JSON_H

#include "frag_reassembly_in.h"

#define	TOPIC_EVENT_CREATE	0
#define	TOPIC_EVENT_EXPIRE	1

/*¿çÃ½Ìå»Ø´«JSONÐÅÏ¢µÄtopic_name*/
#define TOPIC_MEDIA_CREATE_JSON                                                                                 "MEDIA_CREATE_INFO"
#define TOPIC_MEDIA_EXPIRE_JSON                                                                                 "MEDIA_EXPIRE_INFO"
#define TOPIC_SURVEY_JSON                                                                                       "SURVEY_INFO"

#define TOPIC_VOIP_CREATE_JSON                                                                                 "VOIP_CREATE_INFO"
#define TOPIC_VOIP_EXPIRE_JSON                                                                                 "VOIP_EXPIRE_INFO"
#define TOPIC_VOIP_SURVEY_JSON                                                                                 "VOIP_SURVEY_INFO"

#ifdef __cplusplus
extern "C" {
#endif

void media_json_report(media_t* mdi, int topic_event);
#ifdef __cplusplus
}
#endif


#endif
