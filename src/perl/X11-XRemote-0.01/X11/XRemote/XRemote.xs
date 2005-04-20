/*  Last edited: Apr 13 17:25 2005 (rds) */
/* Hej, Emacs, this is -*- C -*- mode!   */


#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <ZMap/zmapXRemote.h>   /* all the ZMap stuff we need */

typedef zMapXRemoteObj X11__XRemote__Handle;

MODULE = X11::XRemote		PACKAGE = X11::XRemote		

char *
format_string(...)
      CODE:
      RETVAL = ZMAP_XREMOTE_REPLY_FORMAT;
      OUTPUT:
      RETVAL

char *
delimiter(...)
      CODE:
      RETVAL = ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER;
      OUTPUT:
      RETVAL

char *
version(...)
      CODE:
      RETVAL = ZMAP_XREMOTE_CURRENT_VERSION;
      OUTPUT:
      RETVAL


MODULE = X11::XRemote           PACKAGE = X11::XRemote::Handle

X11::XRemote::Handle 
init_obj(class)
    char *class
    CODE:
      RETVAL = zMapXRemoteNew();
    OUTPUT:
      RETVAL

int 
initialiseServer(self, app, id)
    X11::XRemote::Handle self
    char *app
    unsigned long id
    CODE:
      RETVAL = zMapXRemoteInitServer(self, id, app, NULL, NULL);
    OUTPUT:
      RETVAL

int 
initialiseClient(self, id)
    X11::XRemote::Handle self
    unsigned long id
    CODE:
      RETVAL = zMapXRemoteInitClient(self, id);
    OUTPUT:
      RETVAL

void
window_id(self, win_id)
     X11::XRemote::Handle self
     unsigned long win_id
     PROTOTYPE: $$
     CODE:
     {
       zMapXRemoteSetWindowID(self, win_id);
       XSRETURN(1);
     }

int
send_command(self, command)
    X11::XRemote::Handle self
    char *command
    CODE:
      RETVAL = zMapXRemoteSendRemoteCommand(self, command);
    OUTPUT:
      RETVAL

char *
full_response_text(self)
    X11::XRemote::Handle self
    CODE:
      RETVAL = zMapXRemoteGetResponse(self);
    OUTPUT:
      RETVAL

void
setRequestName(self, name)
    X11::XRemote::Handle self
    char *name
    CODE:
    {
      zMapXRemoteSetRequestAtomName(self, name);
      XSRETURN(1);
    }

void
setResponseName(self, name)
    X11::XRemote::Handle self
    char *name
    CODE:
    {
      zMapXRemoteSetResponseAtomName(self, name);
      XSRETURN(1);
    }

char *
request(self)
   X11::XRemote::Handle self
   CODE:
     RETVAL = zMapXRemoteGetRequest(self);
   OUTPUT:
     RETVAL
     
int
reply(self, reply)
   X11::XRemote::Handle self
   char *reply
   CODE:
     RETVAL = zMapXRemoteSetReply(self, reply);
   OUTPUT:
     RETVAL

