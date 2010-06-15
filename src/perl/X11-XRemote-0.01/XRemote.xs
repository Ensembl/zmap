/*  Last edited: Jul 19 21:38 2007 (rds) */
/* Hej, Emacs, this is -*- C -*- mode!   */

/* This is  before the perl code  includes as, as Ed and  I have found
   pthreads.h  and gtk.h have  some weird  interaction.  I  found this
   when   compiling    on   alpha   for    perl5.8.0   (alpha-dec_osf-
   thread-multi-ld).  The  symptom was a complaint about  (* leave) in
   gtkbutton.h . This simple move has */

#include <ZMap/zmapXRemote.h>   /* all the ZMap stuff we need */

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

char *ZMAP_X_PROGRAM_G = "X11::XRemote";

typedef struct _X11__XRemote__HandleStruct
{
  ZMapXRemoteObj handle;
  char *response;
} X11__XRemote__HandleStruct, *X11__XRemote__Handle;

/* We need to cache the response, due to the interface design ;(
 * So it's no longer just a handle....
 * typedef ZMapXRemoteObj X11__XRemote__Handle;
 */

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

char *
client_request_name(...)
      CODE:
      RETVAL = ZMAP_CLIENT_REQUEST_ATOM_NAME;
      OUTPUT:
      RETVAL

char *
client_response_name(...)
      CODE:
      RETVAL = ZMAP_CLIENT_RESPONSE_ATOM_NAME;
      OUTPUT:
      RETVAL


MODULE = X11::XRemote           PACKAGE = X11::XRemote::Handle



X11::XRemote::Handle 
init_obj(class)
    char *class
    CODE:
    {        
      X11__XRemote__Handle handle = g_new0(X11__XRemote__HandleStruct, 1);

        /* parameter to zMapXRemoteNew() must be NULL because currently we don't 
         * know how to get the display pointer out of perl/tk so it must be NULL
         * for now. If we find out then need to change zMapXRemoteNew() too. */

      handle->handle = zMapXRemoteNew(NULL) ;
      RETVAL = handle;
    }
    OUTPUT:
      RETVAL

void
DESTROY(self)
     X11::XRemote::Handle self
     CODE:
     {
       zMapXRemoteDestroy(self->handle);
       g_free(self->response);
       g_free(self);
       XSRETURN(1);
     }


int 
initialiseServer(self, app, id)
    X11::XRemote::Handle self
    char *app
    unsigned long id
    CODE:
      RETVAL = zMapXRemoteInitServer(self->handle, id, app, NULL, NULL);
    OUTPUT:
      RETVAL

int 
initialiseClient(self, id)
    X11::XRemote::Handle self
    unsigned long id
    CODE:
      RETVAL = zMapXRemoteInitClient(self->handle, id);
    OUTPUT:
      RETVAL

void
window_id(self, win_id)
     X11::XRemote::Handle self
     unsigned long win_id
     PROTOTYPE: $$
     CODE:
     {
       zMapXRemoteSetWindowID(self->handle, win_id);
       XSRETURN(1);
     }

int
send_command(self, command)
    X11::XRemote::Handle self
    char *command
    CODE:
    {
      int result;
      SV *errsv = NULL;

      if(self->response)
        g_free(self->response);
      self->response = NULL;

      if((result = zMapXRemoteSendRemoteCommand(self->handle, command, &(self->response))) == ZMAPXREMOTE_SENDCOMMAND_SUCCEED)
        {
          int code;
          char *xml_only;
          zMapXRemoteResponseSplit(self->handle, self->response, &code, &xml_only);
          if(0 && (zMapXRemoteResponseIsError(self->handle, self->response)))
            {
              errsv = get_sv("@", TRUE);
              sv_setsv(errsv, newSVpvf("Non 200 response. error code %d.", code));
              croak(Nullch);
            }
        }
      else
        {
          char *response = NULL;

          errsv = get_sv("@", TRUE);

          if((response = zMapXRemoteGetResponse(NULL)))
             sv_setsv(errsv, newSVpv(response, 0));
          else
             sv_setsv(errsv, newSVpvf("Failure sending xremote command. Code = %d.", result));

          croak(Nullch);
        }

      RETVAL = result;
    }                  
    OUTPUT:
      RETVAL

SV *
full_response_text(self)
    X11::XRemote::Handle self
    CODE:
    {
      char *response = "505" ZMAP_XREMOTE_STATUS_CONTENT_DELIMITER
        ZMAP_XREMOTE_ERROR_START "Serious Error!" ZMAP_XREMOTE_ERROR_END;

      if(self && self->response)
        response = self->response;

      RETVAL = newSVpv(response, 0);
    }
    OUTPUT:
      RETVAL

void
setRequestName(self, name)
    X11::XRemote::Handle self
    char *name
    CODE:
    {
      zMapXRemoteSetRequestAtomName(self->handle, name);
      XSRETURN(1);
    }

void
setResponseName(self, name)
    X11::XRemote::Handle self
    char *name
    CODE:
    {
      zMapXRemoteSetResponseAtomName(self->handle, name);
      XSRETURN(1);
    }

char *
request(self)
   X11::XRemote::Handle self
   CODE:
     RETVAL = zMapXRemoteGetRequest(self->handle);
   OUTPUT:
     RETVAL
     
int
reply(self, reply)
   X11::XRemote::Handle self
   char *reply
   CODE:
     RETVAL = zMapXRemoteSetReply(self->handle, reply);
   OUTPUT:
     RETVAL

