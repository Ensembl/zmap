/*  Last edited: Oct 17 11:48 2008 (rds) */

typedef struct
{
  int   action_type;
  char *action;
}XRemoteMessageStruct, *XRemoteMessage;


gboolean zMapXRemoteAPIMessageProcess(char           *message_xml_in, 
				      gboolean        full_process, 
				      char          **action_out,
				      XRemoteMessage *message_out);
