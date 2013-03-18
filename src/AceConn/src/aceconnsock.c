/*  File: aceconnsock.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2002
 *-------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 *-------------------------------------------------------------------
 * Description: Contains all the code that needs to know about sockets
 *              for the Ace-Conn package.
 * Exported functions: See aceconn_.h
 * HISTORY:
 * Last edited: Jun 12 16:40 2006 (edgrif)
 * Created: Thu Mar  7 11:12:59 2002 (edgrif)
 * CVS info:   $Id: aceconnsock.c,v 1.1.1.1 2007-10-03 13:28:00 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include <sys/ioctl.h>
#include <arpa/inet.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <global.h>					    /* MD5 */
#include <md5.h>					    /* MD5 */

#include <AceConnProto.h>				    /* defines server msg protocol. */
#include <aceconn_.h>



/* Messages:                                                                 */
/*                                                                           */
typedef struct
{
  /* header handling */
  AceConnHeaderRec proto_hdr ;
  gboolean done_header ;
  char header_buf[ACECONN_HEADER_BYTES] ;
  int hBytesRequested ;
  int hBytesPending ;

  /* message handling */
  gboolean done_message ;
  char *message ;
  GString *encore_message ;
  int mBytesRequested ;
  int mBytesPending ;

} AceConnMsgRec ;


/* Messages are in one of these states while being sent, this is because even*/
/* without non-blocking I/O you can't guarantee sending the message in one   */
/* go if it is very big.                                                     */
typedef enum {ACECONNMSG_WAIT, ACECONNMSG_DONE, ACECONNMSG_ERROR} AceConnMsgState ;


/* Sockets can be accessed for READ or WRITE, not both currently.            */
typedef enum {ACECONNACC_READ, ACECONNACC_WRITE} AceConnSockAccess ;


/* The MD5 code returns an array of unsigned char of size 16, the value 16   */
/* has no symbolic constant in md5.h so I define one, plus the size of the   */
/* string required to hold the hexadecimal string version of the array.      */
/*                                                                           */
enum {MD5_HASHLEN = 16, MD5_HEX_HASHLEN = ((16 * 2) + 1)} ;


static AceConnStatus accessSocket(AceConnection connection, AceConnSockAccess access) ;
static AceConnStatus aceconnSocketRead(AceConnection connection) ;
static AceConnMsgState readSocket(int fd, char *readbuf, int *bytes_pending_inout,
				  char **errmsg_out) ;
static AceConnStatus aceconnSocketWrite(AceConnection connection) ;
static AceConnMsgState writeSocket(int fd, char *writebuf, int *bytes_pending_inout,
				   char **errmsg_out) ;

static AceConnMsg messageCreate(void) ;
static void messageSetForRead(AceConnMsg msg, gboolean encore) ;
static void messageSetForWrite(AceConnMsg msg, char *message) ;
static void messageGetReply(AceConnMsg msg, void **reply, int *reply_len) ;
static gboolean messageIsEncore(AceConnMsg msg_in) ;
static void messageGetEncore(AceConnMsg msg_in) ;
static gboolean messageDone(AceConnMsg msg_in) ;
static void messageDestroy(AceConnMsg msg) ;

static void buf2hdr(char *buf, AceConnHeader hdr) ;
static void hdr2buf(AceConnHeader hdr, char *buf) ;
static void setMsgType(char buffer[], char *msgType) ;
static gboolean testMsgType(char buffer[], char *msgType) ;

static AceConnStatus selectSocket(AceConnection connection, AceConnSockAccess access) ;
static AceConnStatus resetBlockingSocket(AceConnection connection, int *oldflags_out, int *newflags) ;
static AceConnStatus shutdownSocket(AceConnection connection) ;

static char *hashAndHexStrings(char *strings[], int num_strings) ;
static char *makeHash(char *userid, char *passwd) ;
static char *convertMD5toHexStr(unsigned char digest[]) ;

/*                                                                           */
/* External package routines.                                                */
/*                                                                           */


/* Set up a socket connection to the server.                                 */
/*                                                                           */
AceConnStatus aceconnServerOpen(AceConnection connection)
{ 
  AceConnStatus status = ACECONN_OK ;
  int sock ;
  struct sockaddr_in sname ;
  struct hostent *hp ;

  /* create a socket for writing/reading */
  if (status == ACECONN_OK)
    {
      sock = socket(AF_INET, SOCK_STREAM, 0) ;
      if (sock < 0)
	{
	  status = ACECONN_NOSOCK ;
	  aceconnSetErrMsg(connection, ACECONN_NOSOCK, "Failed to create socket: %d",
			   g_strerror(errno)) ;
	}
      else
	connection->socket = sock ;
    }

  if (status == ACECONN_OK)
    {
      /* create absolute socket name */
      memset((void *)&sname, 0, sizeof(sname)) ;
      sname.sin_family = AF_INET ;
      sname.sin_port = htons(connection->port) ;

      hp = gethostbyname(connection->host) ;
      if (!hp)
	{
	  status = ACECONN_UNKNOWNHOST ;
	  aceconnSetErrMsg(connection, ACECONN_UNKNOWNHOST, "Unknown host: %d", connection->host) ;
	}
    }


  /* OK, connect to the socket.                                              */
  if (status == ACECONN_OK)
    {
      int tmp_errno ;
      int tmp_rc ;

      memcpy((void*)&sname.sin_addr, (void*)hp->h_addr, hp->h_length) ;

      errno = 0 ;
      if ((tmp_rc = connect(sock, (struct sockaddr *)&sname, sizeof(sname))) == 0)
	connection->state = ACECONNSTATE_OPEN ;
      else
	{
	  tmp_errno = errno ;

	  status = ACECONN_NOCONNECT ;
	  aceconnSetErrMsg(connection, ACECONN_NOCONNECT, "Could not connect to socket: %s",
			   g_strerror(tmp_errno)) ;
	}
    }

  return status ;
}

/* Send an initial message to the client, we will then get a reply which     */
/* contains a 'nonce' or key with which we must encode the users userid and  */
/* password. Then get the users userid/password and do the encryption and    */
/* send it back to the server for verification. We should then get another   */
/* reply from the server to OK this.                                         */
/* If this routine fails then we exit.                                       */
/*                                                                           */
AceConnStatus aceconnServerHandshake(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;
  char *reply = NULL ;
  int reply_len = 0 ;
  enum {HASH_STRING_NUM = 2} ;
  char *hash_strings[HASH_STRING_NUM] ;
  char *hash_nonce ;
  char *request = NULL ;

  /* This is the first time we need to send a message to the server so       */
  /* create the message handling struct.                                     */
  connection->msg = messageCreate() ;

  /* We need to be able to add stuff to our error message to say we failed   */
  /* in handshake....                                                        */

  /* Say an initial hello to the server, the server returns a nonce string   */
  /* for us to hash our password hash with.                                  */
  if (status == ACECONN_OK)
    {
      status = aceconnServerRequest(connection, ACECONN_CLIENT_HELLO,
				    (void *)&reply, &reply_len) ;
      if (status == ACECONN_OK)
	{
	  /* WE SHOULD CHECK THAT THE SERVER ONLY RETURNED ONE WORD HERE....     */
	  /* where is the unix func to do that ??                                */

	  /* Create a hash of our userid and passwd and then hash this with the  */
	  /* nonce and convert to a hex string to pass back to server.           */
	  connection->passwd_hash = makeHash(connection->userid, connection->passwd) ;
	  hash_strings[0] = connection->passwd_hash ;
	  hash_strings[1] = reply ;
	  hash_nonce = hashAndHexStrings(hash_strings, HASH_STRING_NUM) ;
      
	  /* Now make a string containing the userid and hash to send back.  */
	  request = g_strjoin(" ", connection->userid, hash_nonce, NULL) ;

	  g_free(reply) ;
	}
    }

  /* Send our nonce-hashed hash back, if all is ok the server will reply     */
  /* with its standard message.                                              */
  if (status == ACECONN_OK)
    {
      status = aceconnServerRequest(connection, request, (void *)&reply, &reply_len) ;
      if (status == ACECONN_OK)
	{
	  if (strcmp(reply, ACECONN_SERVER_HELLO) != 0)
	    {
	      status = ACECONN_HANDSHAKE ;
	      aceconnSetErrMsg(connection, ACECONN_HANDSHAKE, "Server reply in handshake incorrect, "
			       "it should have said: \"%s\"  but actually said: \"%s\"",
			       ACECONN_SERVER_HELLO, reply) ;
	    }
	  else
	    {
	      /* Return the userid and passwd hash for later reuse.                      */
	      connection->nonce_hash = hash_nonce ;
	      connection->state = ACECONNSTATE_READY ;
	    }

	  g_free(reply) ;
	}
    }

  g_free(request) ;

  return status ;
}


/* Send a request to the server and get the reply.                           */
/*                                                                           */
AceConnStatus aceconnServerRequest(AceConnection connection,
				   char *request, void **reply, int *reply_len)
{
  AceConnStatus status = ACECONN_OK ;
  gboolean encore = FALSE, finished = FALSE ;


  while (!finished)
    {
      if (status == ACECONN_OK)
	{
	  messageSetForWrite(connection->msg, request) ;

	  status = accessSocket(connection, ACECONNACC_WRITE) ;
	  if (status != ACECONN_OK)
	    finished = TRUE ;
	}
      
      if (status == ACECONN_OK)
	{
	  messageSetForRead(connection->msg, encore) ;

	  status = accessSocket(connection, ACECONNACC_READ) ;
	  if (status != ACECONN_OK)
	    finished = TRUE ;
	  else
	    {
	      if (messageIsEncore(connection->msg))
		{
		  encore = TRUE ;
		  request = ACECONN_SERVER_CLIENT_SLICE ;
		}
	      else
		{
		  finished = TRUE ;
		}

	      if (encore)
		messageGetEncore(connection->msg) ;

	      if (finished)
		messageGetReply(connection->msg, reply, reply_len) ;
	    }
	}
    }

  return status ;
}




AceConnStatus aceconnServerClose(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;
  char *reply = NULL ;
  int reply_len = 0 ;

  if (connection->state == ACECONNSTATE_OPEN
      || connection->state == ACECONNSTATE_READY)
    {
      if (connection->state == ACECONNSTATE_READY)
	{
	  status = aceconnServerRequest(connection, ACECONN_CLIENT_GOODBYE,
					(void *)&reply, &reply_len) ;
	}

      /* Always close the socket.                                                */
      status =  shutdownSocket(connection) ;
    }

  messageDestroy(connection->msg) ;
  connection->msg = NULL ;
  connection->state = ACECONNSTATE_NONE ;

  return status ;
}



/*                                                                           */
/* Internal routines.                                                        */
/*                                                                           */


/* Read or write a message to a socket as a more or less atomic operation,   */
/* but allowing timeout via the use of select() to block for a specified time*/
/* if we have to wait on the socket.                                         */
/*                                                                           */
static AceConnStatus accessSocket(AceConnection connection, AceConnSockAccess access)
{
  AceConnStatus status = ACECONN_OK ;
  gboolean finished ;

  finished = FALSE ;
  while (!finished)
    {
      /* Is the socket ready ?                                               */
      if ((status = selectSocket(connection, access)) != ACECONN_OK)
	finished = TRUE ;

      /* If so, process the rest of the message.                             */
      if (!finished)
	{
	  if (access == ACECONNACC_WRITE)
	    status = aceconnSocketWrite(connection) ;
	  else
	    status = aceconnSocketRead(connection) ;

	  /* We only finish if there is a problem (e.g. timeout) OR the msg  */
	  /* has been successfully processed.                                */
	  if (status != ACECONN_OK
	      || (status == ACECONN_OK && (messageDone(connection->msg))))
	    finished = TRUE ;
	}
    }

  return status ;
}


/**************************************************************/


static AceConnStatus aceconnSocketRead(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;
  AceConnMsgState state = ACECONNMSG_ERROR ;
  int sock_flags ;
  char *errmsg = NULL ;
  int fd = connection->socket ;
  AceConnMsgRec *req = (AceConnMsgRec *)connection->msg ;


  /* We can't just do a recv with MSG_DONTWAIT and MSG_PEEK because          */
  /* the former is not supported on many systems yet. So instead we make the */
  /* whole socket non-blocking, do the recv and then reset the socket.       */
  if (status == ACECONN_OK)
    {
      status = resetBlockingSocket(connection, &sock_flags, NULL) ;
    }

  if (status == ACECONN_OK)
    {
      /* Try to read the header, probably do this in one go.                     */
      if (req->done_header == FALSE)
	{
	  /* First time for this message so allocate a buffer.                   */
	  if (!req->hBytesPending)
	    {
	      req->hBytesPending = req->hBytesRequested = ACECONN_HEADER_BYTES ;
	    }

	  state = readSocket(fd, req->header_buf + (req->hBytesRequested - req->hBytesPending),
			     &(req->hBytesPending), &errmsg) ;

	  if (state == ACECONNMSG_DONE)
	    {
	      buf2hdr(req->header_buf, &(req->proto_hdr)) ;
	      req->done_header = TRUE ;
	    }
	  else if (state == ACECONNMSG_ERROR)
	    {
	      status = ACECONN_READERROR ;
	      aceconnSetErrMsg(connection, ACECONN_READERROR,
			       "Error reading message header from socket: %s", errmsg) ;
	      g_free(errmsg) ;
	    }
	}
    }

  /* We have a header so try to read buffer (this may take several calls to
   * this function), or it may take none if there is no message content which.
   * can happen if the user as set "quiet" mode on and this is a reply to a command
   * which has no output. */
  if (status == ACECONN_OK)
    {
      if (req->done_header == TRUE)
	{
	  if (req->proto_hdr.length == 0)
	    {
	      req->done_message =  TRUE ;
	    }
	  else
	    {
	      /* First time for this message so allocate a buffer.                   */
	      if (!req->message)
		{
		  req->message = g_malloc(req->proto_hdr.length) ;
		  req->mBytesRequested = req->mBytesPending = req->proto_hdr.length ;
		}

	      state = readSocket(fd, req->message + req->mBytesRequested - req->mBytesPending,
				 &(req->mBytesPending), &errmsg) ;
	      if (state == ACECONNMSG_DONE)
		{
		  req->done_message = TRUE ;
		}
	      else if (state == ACECONNMSG_ERROR)
		{
		  status = ACECONN_READERROR ;
		  aceconnSetErrMsg(connection, ACECONN_READERROR,
				   "Error reading message body from socket: %s", errmsg) ;
		  g_free(errmsg) ;
		}
	    }
	}
    }


  /* Reset the socket to blocking, try to do this even if we have an error.  */
  /* perhaps not worth it ?                                                  */
  if (status == ACECONN_OK)
    {
      status = resetBlockingSocket(connection, NULL, &sock_flags) ;
    }

  return status ;
}


/* Common read routine for handling reading from a socket and any resulting  */
/* errors. Note that it updates the the number of bytes left to read given   */
/* by bytesPending. If it fails it will give the reason in err_msg_out.      */
/*                                                                           */
/* n.b. should not be called with zero bytes to send, null readbuf etc.      */
/*                                                                           */
static AceConnMsgState readSocket(int fd, char *readbuf, int *bytes_pending_inout,
				  char **errmsg_out)
{
  AceConnMsgState state = ACECONNMSG_ERROR ;
  int bytes ;

  bytes = read(fd, readbuf, *bytes_pending_inout) ;
  if (bytes < 0)
    {
      /* This is potentially not strict enough, we may need to put a load of */
      /* #defines in here to check for SYS_V, POSIX etc. etc. to make sure   */
      /* we check for the correct errno's...aaaggghhh....                    */
      if (errno == EWOULDBLOCK || errno == EINTR || errno == EAGAIN)
	state = ACECONNMSG_WAIT ;
      else
	{
	  state = ACECONNMSG_ERROR ;
	  *errmsg_out = g_strdup_printf("Read error trying to read client data on socket: %s",
					g_strerror(errno)) ;
	}
    }
  else if (bytes == 0)
    {
      /* CHECK THIS ERROR CONDITION IN STEVENS....                           */
      state = ACECONNMSG_ERROR ;
      *errmsg_out = g_strdup("Zero bytes returned from read.") ;
    }
  else
    {
      *bytes_pending_inout -= bytes ;
      
      if (*bytes_pending_inout < 0)
	{
	  state = ACECONNMSG_ERROR ;
	  *errmsg_out = g_strdup("Logic error, read more bytes than number requested.") ;
	}
      else  if (*bytes_pending_inout == 0)
	state = ACECONNMSG_DONE ;
      else
	state = ACECONNMSG_WAIT ;
    }

  return state ;
}



/**************************************************************/

/* Jean I added code here to store any current handler for SIGPIPE, we don't */
/* know if the rest of the application will have installed one. We then      */
/* turn off signal handling for SIGPIPE so that we get EPIPE for a write to  */
/* a broken connection which we handle here. Then we turn back on any        */
/* existing signal handler.                                                  */
static AceConnStatus aceconnSocketWrite(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;
  AceConnMsgState state = ACECONNMSG_ERROR ;
  int sock_flags = -1 ;
  struct sigaction oursigpipe, oldsigpipe ;
  char *errmsg = NULL ;
  int fd = connection->socket ;
  AceConnMsgRec *req = (AceConnMsgRec *)connection->msg ;


  /* writes can deliver a SIGPIPE if the socket has been disconnected, by    */
  /* ignoring we will just receive errno = EPIPE.                            */
  oursigpipe.sa_handler = SIG_IGN ;
  sigemptyset(&oursigpipe.sa_mask) ;
  oursigpipe.sa_flags = 0 ;
  if (sigaction(SIGPIPE, &oursigpipe, &oldsigpipe) < 0)
    {
      status = ACECONN_SIGSET ;
      aceconnSetErrMsg(connection, ACECONN_SIGSET,
		       "Cannot set SIG_IGN for SIGPIPE for socket write operations: %s ",
		       g_strerror(errno)) ;
    }

  /* We can't just do a recv with MSG_DONTWAIT and MSG_PEEK because          */
  /* the former is not supported on many systems yet. So instead we make the */
  /* whole socket non-blocking, do the recv and then reset the socket.       */
  if (status == ACECONN_OK)
    {
      status = resetBlockingSocket(connection, &sock_flags, NULL) ;
    }


  /* OK, Let's try to write the header                                       */

  if (status == ACECONN_OK)
    {
      if (req->done_header == FALSE)
	{
	  /* Do any byte swapping.                                               */
	  hdr2buf(&(req->proto_hdr), req->header_buf) ;

	  /* Now do the writes...                                                */
	  state = writeSocket(fd, req->header_buf + req->hBytesRequested - req->hBytesPending,
			      &(req->hBytesPending), &errmsg) ;
	  if (state == ACECONNMSG_DONE)
	    {
	      req->done_header = TRUE ;
	    }
	  else if (state == ACECONNMSG_ERROR)
	    {
	      status = ACECONN_WRITEERROR ;
	      aceconnSetErrMsg(connection, ACECONN_WRITEERROR,
			       "Error reading message header from socket: %s", errmsg) ;
	      g_free(errmsg) ;
	    }
	}
    }

  /* OK, header written, so try to write buffer.                             */
  if (status == ACECONN_OK)
    {
      if (req->done_header == TRUE)
	{
	  state = writeSocket(fd,
			      req->message + (req->mBytesRequested - req->mBytesPending),
			      &(req->mBytesPending), &errmsg) ;
	  if (state == ACECONNMSG_DONE)
	    {
	      req->done_message = TRUE ;
	    }
	  else if (state == ACECONNMSG_ERROR)
	    {
	      status = ACECONN_WRITEERROR ;
	      aceconnSetErrMsg(connection, ACECONN_WRITEERROR,
			       "Error reading message body from socket: %s", errmsg) ;
	      g_free(errmsg) ;
	    }
	}
    }

  if (status == ACECONN_OK)
    {
      status = resetBlockingSocket(connection, NULL, &sock_flags) ;
    }

  /* Reset the old signal handler, do this whatever our state.               */
  if (sigaction(SIGPIPE, &oldsigpipe, NULL) < 0)
    {
      status = ACECONN_SIGSET ;
      aceconnSetErrMsg(connection, ACECONN_SIGSET,
		       "Cannot reset previous handler for SIGPIPE after "
		       "socket write operations: %s",
		       g_strerror(errno)) ;
    }

  return status ;
}


/* Common write routine for handling writing to a socket and any resulting   */
/* errors. Note that it updates the the number of bytes left to write given  */
/* by bytesPending.                                                          */
static AceConnMsgState writeSocket(int fd, char *writebuf, int *bytes_pending_inout,
				   char **errmsg_out)
{
  AceConnMsgState state = ACECONNMSG_ERROR ;
  int bytes ;

  bytes = write(fd, writebuf, *bytes_pending_inout) ;
  if (bytes < 0)
    {
      if (errno == EINTR || errno == EAGAIN)
	state = ACECONNMSG_WAIT ;
      else
	{
	  state = ACECONNMSG_ERROR ;
	  *errmsg_out = g_strdup_printf("Write error trying to write client data to socket: %s",
					g_strerror(errno)) ;
	}
    }
  else
    {
      *bytes_pending_inout -= bytes ;

      if (*bytes_pending_inout < 0)
	{
	  state = ACECONNMSG_ERROR ;
	  *errmsg_out = g_strdup("Logic error, wrote more bytes than number requested.") ;
	}
      else  if (*bytes_pending_inout == 0)
	state = ACECONNMSG_DONE ;
      else 
	state = ACECONNMSG_WAIT ;
    }

  return state ;
}


/*                                                                           */
/* Message handling code.                                                    */
/*                                                                           */

/* Allocate and initialise to zero.                                          */
/*                                                                           */
static AceConnMsg messageCreate(void)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)g_malloc0(sizeof(AceConnMsgRec)) ;

  msg->proto_hdr.byte_swap = ACECONN_BYTE_ORDER ;		    /* Always the same. */

  return (AceConnMsg)msg ;
}


/* Set up a message to receive data from a socket.                           */
static void messageSetForRead(AceConnMsg msg_in, gboolean encore)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;

  /* Nothing to set in the protocol header.                                  */

  /* Set fields for controlling write of header and message.                 */
  msg->done_header = FALSE ;
  memset(msg->header_buf, 0, ACECONN_HEADER_BYTES) ;
  msg->hBytesRequested =  msg->hBytesPending = ACECONN_HEADER_BYTES ;

  msg->done_message = FALSE ;
  msg->message = NULL ;
  if (!encore && msg->encore_message)
    {
      g_string_free(msg->encore_message, FALSE) ;
      msg->encore_message = NULL ;
    }
  msg->mBytesRequested =  msg->mBytesPending = 0 ;

  return ;
}


/* Set up a message to be written out to a socket.                           */
/*                                                                           */
static void messageSetForWrite(AceConnMsg msg_in, char *message)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;

  /* Set fields in protocol header, may be special "encore" request meaning  */
  /* get next slice.                                                         */
  if (strcmp(message, ACECONN_SERVER_CLIENT_SLICE) == 0)
    setMsgType(msg->proto_hdr.msg_type, ACECONN_MSGENCORE) ;
  else
    setMsgType(msg->proto_hdr.msg_type, ACECONN_MSGREQ) ;
  msg->proto_hdr.length = strlen(message) + 1 ;

  /* Set fields for controlling write of header and message.                 */
  msg->done_header = FALSE ;
  memset(msg->header_buf, 0, ACECONN_HEADER_BYTES) ;
  msg->hBytesRequested = msg->hBytesPending = ACECONN_HEADER_BYTES ;

  msg->done_message = FALSE ;
  msg->message = message ;
  msg->mBytesPending = msg->mBytesRequested = msg->proto_hdr.length ;

  return ;
}

/* Has a message been completely sent ?                                      */
static gboolean messageDone(AceConnMsg msg_in)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;
  gboolean result = FALSE ;

  if (msg->done_header && msg->done_message)
    result = TRUE ;

  return result ;
}


/* Was message an "encore" message ?                                         */
static gboolean messageIsEncore(AceConnMsg msg_in)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;
  gboolean result = FALSE ;

  if (testMsgType(msg->proto_hdr.msg_type, ACECONN_MSGENCORE))
    result = TRUE ;

  return result ;
}

static void messageGetEncore(AceConnMsg msg_in)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;

  if (msg->message != NULL)
    {
      if (msg->encore_message == NULL)
	msg->encore_message = g_string_new(msg->message) ;
      else
	msg->encore_message = g_string_append(msg->encore_message, msg->message) ;
      
      g_free(msg->message) ;
      msg->message = NULL ;
    }

  return ;
}


static void messageGetReply(AceConnMsg msg_in, void **reply, int *reply_len)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;

  /* I don't much like this, we should always be getting from the same buffer, revisit this. */
  if (msg->encore_message)
    {
      *reply = msg->encore_message->str ;
      *reply_len = msg->encore_message->len ;
    }
  else
    {
      *reply = msg->message ;
      *reply_len = msg->mBytesRequested ;
    }

  return ;
}


/* Clean up a message of allocated resources                                 */
/* N.B. we DO NOT free any message buffer, that is always the clients        */
/* responsibility.                                                           */
/*                                                                           */
static void messageDestroy(AceConnMsg msg_in)
{
  AceConnMsgRec *msg = (AceConnMsgRec *)msg_in ;

  if (msg->encore_message)
    g_string_free(msg->encore_message, FALSE) ;

  g_free(msg) ;

  return ;
}




/* Utilities to fiddle with protocol header.                                 */
/*                                                                           */
static void buf2hdr(char *buf, AceConnHeader hdr)
{
  memcpy(&(hdr->byte_swap), buf, 4) ;      buf += 4 ;
  memcpy(&(hdr->length), buf, 4) ;         buf += 4 ;
  memcpy(&(hdr->server_version), buf, 4) ; buf += 4 ;
  memcpy(&(hdr->client_id), buf, 4) ;      buf += 4 ;
  memcpy(&(hdr->max_bytes), buf, 4) ;      buf += 4 ;
  memcpy(&(hdr->msg_type), buf, ACECONN_MSGTYPE_BUFLEN) ;

  return ;
}

static void hdr2buf(AceConnHeader hdr, char *buf)
{
  memcpy(buf, &(hdr->byte_swap), 4) ;      buf += 4 ;
  memcpy(buf, &(hdr->length), 4) ;         buf += 4 ;
  memcpy(buf, &(hdr->server_version), 4) ; buf += 4 ;
  memcpy(buf, &(hdr->client_id), 4) ;      buf += 4 ;
  memcpy(buf, &(hdr->max_bytes), 4) ;      buf += 4 ;
  memcpy(buf, &(hdr->msg_type), ACECONN_MSGTYPE_BUFLEN) ;

  return ;
}


/* These two routines set the type and test it, they could be macros but     */
/* performance is not the problem here.                                      */
/*                                                                           */
/* They cope with caller supplying msgType which itself points to buffer.    */
/*                                                                           */
static void setMsgType(char buffer[], char *msgType)
{
  char *msg_copy = NULL ;

  /* Ugly bug here...what if msgType points to buffer ? We'll do belt and    */
  /* braces and clean the buffer anyway.                                     */
  if (msgType == &(buffer[0]))
    {
      msg_copy = g_strdup(msgType) ;
    }

  /* Reset the message type section of the header to be zeroed so there is   */
  /* no extraneous bumpf if the previous message was longer than this one.   */
  /* This is important for perl and other non-C languages that have to parse */
  /* this bit out of the message buffer.                                     */
  memset(buffer, 0, ACECONN_MSGTYPE_BUFLEN) ;

  if (msg_copy != NULL)
    msgType = msg_copy ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (strcpy(buffer, msgType) == NULL)
    messcrash("copy of message type failed, message was: %s",  msgType) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  strcpy(buffer, msgType) ;

  if (msg_copy != NULL)
    g_free(msg_copy) ;

  return ;
}


static gboolean testMsgType(char buffer[], char *msgType)
{
  gboolean result = FALSE ;

  if (msgType == &(buffer[0]))
    result = TRUE ;
  else if (strcmp(buffer, msgType) == 0)
    result = TRUE ;

  return result ;
}



/*                                                                           */
/*                    Socket utilities                                       */
/*                                                                           */

/* Currently we only wait for READ or WRITE, not both, could alter interface */
/* to allow both quite easily.                                               */
/* On success returns ACECONN_OK                                             */
/*                                                                           */
static AceConnStatus selectSocket(AceConnection connection, AceConnSockAccess access)
{
  AceConnStatus status = ACECONN_OK ;
  int rc ;
  struct timeval tv, *tv_ptr ;
  fd_set readset, writeset ;
  fd_set *readset_ptr = NULL, *writeset_ptr = NULL, *exceptset_ptr = NULL ;
  int maxfd ;

  if (access == ACECONNACC_WRITE)
    {
      FD_ZERO(&writeset) ;
      FD_SET(connection->socket, &writeset) ;
      writeset_ptr = &writeset ;
    }
  else
    {
      FD_ZERO(&readset) ;
      FD_SET(connection->socket, &readset) ;
      readset_ptr = &readset ;
    }

  maxfd = connection->socket + 1 ;			    /* "+ 1" is usual array offset gubbins */

  /* We allow the "never timeout" option, i.e. only return if the socket is  */
  /* ready.                                                                  */
  if (connection->timeout == 0)
    tv_ptr = NULL ;
  else
    {
      tv.tv_sec = connection->timeout ;
      tv.tv_usec = 0 ;
      tv_ptr = &tv ;
    }

  rc = select(maxfd, readset_ptr, writeset_ptr, exceptset_ptr, tv_ptr) ;
  if (rc < 0)
    { 
      status = ACECONN_NOSELECT ;
      aceconnSetErrMsg(connection, ACECONN_NOSELECT,
		       "select() on socket failed: %s", g_strerror(errno)) ;
    }
  else if (rc == 0)
    {
      status = ACECONN_TIMEDOUT ;
      aceconnSetErrMsg(connection, ACECONN_TIMEDOUT, "%s", "select() on socket timed out.") ;
    }
  else
    status = ACECONN_OK ;

  return status ;
}


/* Set/reset non-blocking/blocking on socket.                                */
/* If oldflags_out is non-NULL the current flags will be returned in it.     */
/* If newflags is non-NULL the socket will be set to those flags, otherwise  */
/* the existing socket flags will be OR'd with O_NONBLOCK.                   */
/*                                                                           */
static AceConnStatus resetBlockingSocket(AceConnection connection,
					 int *oldflags_out, int *newflags)
{
  AceConnStatus status = ACECONN_OK ;
  int fd = connection->socket ;
  int flags ;

  if (newflags)
    flags = *newflags ;
  else
    {
      flags = fcntl(fd, F_GETFL, 0) ;
      if (flags < 0)
	{
	  status = ACECONN_NONBLOCK ;
	  aceconnSetErrMsg(connection, ACECONN_NONBLOCK, "Failed to get socket flags: %s",
			   g_strerror(errno)) ;
	}
      else
	{
	  if (oldflags_out)				    /* Return old flags ? */
	    *oldflags_out = flags ;
	  flags |= O_NONBLOCK ;
	}
    }

  if (status == ACECONN_OK)
    {
      flags = fcntl(fd, F_SETFL, flags) ;
      if (flags < 0)
	{
	  status = ACECONN_NONBLOCK ;
	  aceconnSetErrMsg(connection, ACECONN_NONBLOCK, "Failed to set socket flags: %s",
			   g_strerror(errno)) ;
	}
    }

  return status ;
}


/* We ignore ENOTCONN which means that the socket has probably been closed   */
/* by the server already, other errors imply a coding error by us.           */
/*                                                                           */
static AceConnStatus shutdownSocket(AceConnection connection)
{
  AceConnStatus status = ACECONN_OK ;

  if (shutdown(connection->socket, SHUT_RDWR) < 0)
    {
      if (errno != ENOTCONN)
	{
	  status = ACECONN_INTERNAL ;
	  aceconnSetErrMsg(connection, ACECONN_INTERNAL,
			   "Internal code error, invalid option to shutdown call: %s",
			   g_strerror(errno)) ;
	}
    }

   return status ;
}



/* Encryption and hashing code.                                              */
/*                                                                           */
/* We use MD5 for encryption and this can be found in the wmd5 directory.    */
/*                                                                           */
/* Note that MD5 produces as its output a 128 bit value that uniquely        */
/* represents the input strings. We convert this output value into a hex     */
/* string version of the 128 bit value for several reasons:                  */
/*                                                                           */
/*     - the md5 algorithm requires strings as input and we need to use      */
/*       some of the md5 output as input into a new md5 hash.                */
/*     - it makes all handling of the encrypted data much simpler            */
/*     - using hex means that the string we produce consists entirely of the */
/*       digits 0-9 and the letters a-f (i.e. no unprintable chars)          */
/*     - the passwd hash can be kept in a plain text file in the database    */
/*                                                                           */

/* hash the userid and password together and convert into a hex string.      */
/*                                                                           */
static char *makeHash(char *userid, char *passwd)
{
  char *hash = NULL ;
  enum {HASH_STRING_NUM = 2} ;
  char *hash_strings[HASH_STRING_NUM] ;

  hash_strings[0] = userid ;
  hash_strings[1] = passwd ;
  hash = hashAndHexStrings(hash_strings, HASH_STRING_NUM) ;

  return hash ;
}

/* Takes an array of strings, hashes then together using MD5 and then        */
/* produces a hexstring translation of the MD5 output.                       */
/*                                                                           */
static char *hashAndHexStrings(char *strings[], int num_strings)
{
  char *hex_string = NULL ;
  MD5_CTX Md5Ctx ;
  unsigned char digest[MD5_HASHLEN] ;
  int i ;

  MD5Init(&Md5Ctx) ;

  for (i = 0 ; i < num_strings ; i++)
    {
      MD5Update(&Md5Ctx, (unsigned char *)strings[i], strlen(strings[i])) ;
    }

  MD5Final(digest, &Md5Ctx) ;

  hex_string = convertMD5toHexStr(&digest[0]) ;

  return hex_string ;
}


/************************************************************/

/* Takes the array of unsigned char output by the MD5 routines and makes a   */
/* hexadecimal version of it as a null-terminated string.                    */
/*                                                                           */
static char *convertMD5toHexStr(unsigned char digest[])
{
  char *digest_str ;
  int i ;
  char *hex_ptr ;

  digest_str = g_malloc(MD5_HEX_HASHLEN) ;
  for (i = 0, hex_ptr = digest_str ; i < MD5_HASHLEN ; i++, hex_ptr+=2)
    {
      sprintf(hex_ptr, "%02x", digest[i]) ;
    }

  return digest_str ;
}


