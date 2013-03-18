/*  File: AceConnProto.h
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
 * Description: Intent is for this to be a one-time general header
 *              for all C/C++ code that communicates with the acedb
 *              socket server. The header defines the format of messages
 *              for the the client/server protocol. The header is a model
 *              for implementations in other languages.
 *              
 * HISTORY:
 * Last edited: Sep 28 12:44 2007 (edgrif)
 * Created: Thu Mar  7 12:01:05 2002 (edgrif)
 * CVS info:   $Id: AceConnProto.h,v 1.1.1.1 2007-10-03 13:28:00 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_ACECONNPROTO_H
#define DEF_ACECONNPROTO_H


/* I REALLY WANT THIS TO BE USED IN wsocket/ CODE IN THE END SO THAT WE JUST */
/* ONE HEADER FOR ACEDB SOCKET STUFF....                                     */
/* But that means that this should ONLY have the bare minimum about the      */
/* protocol.....the client struct stuff should not be in here...             */



/* It's assumed in the protocol that ints are 4 bytes, this will stop the    */
/* code from compiling, but we also do a run time check (although I can't    */
/* imagine that a 32-bit binary can really be run with ints that aren't 4    */
/* bytes...).                                                                */
/* n.b. solaris and mac seem unable to cope with this test, I don't know why.*/
/*                                                                           */

/* I can't get this to work....sigh...*/
#ifdef NEVER_EVER_DEFINED
/* #if (UINT_MAX != 4294967295U) */
#if (UINT_MAX != 0xffffffff)
#error "WARNING !! You cannot compile this program, it requires 4 byte integers and this machine does not have them."
#endif
#endif




/* Must be set in header so server can detect clients byte order.            */
#define ACECONN_BYTE_ORDER 0x12345678

/* We strings rather than enums, easier for others to interface to...        */
/* No ACESERV_MSGnnn string should be longer than                            */
/*                             (ACECONN_MSGTYPE_BUFLEN - 1)                  */
/* you must not lightly change this length, this is the size clients will    */
/* be expecting, to change it means changing all clients.                    */
/*                                                                           */
enum {ACECONN_MSGTYPE_BUFLEN = 30} ;

/* Client only, its messages are either normal requests (commands) or are    */
/* ace data that needs to be parsed in.                                      */
#define ACECONN_MSGREQ    "ACESERV_MSGREQ"
#define ACECONN_MSGDATA   "ACESERV_MSGDATA"

/* Server only, it may just be sending or a reply or it may be sending an    */
/* instruction, such as "operation refused".                                 */
#define ACECONN_MSGOK     "ACESERV_MSGOK"
#define ACECONN_MSGENCORE "ACESERV_MSGENCORE"
#define ACECONN_MSGFAIL   "ACESERV_MSGFAIL"
#define ACECONN_MSGKILL   "ACESERV_MSGKILL"


/* The message header struct, this has to be packed into the the first       */
/* ACECONN_HEADER_BYTES of a message sent between client & server.           */
/* You can use these enums to help you in writing code to put bytes into the */
/* message.                                                                  */

/* Number of fields in the header that may need to be byte swopped.          */
enum {ACECONN_HEADER_SWAPFIELDS = 5} ;

/* Because we want a CONSTANT size for the buffer, we define this very       */
/* tediously...we have 5 ints of 4 bytes each and a char buffer to hold the  */
/* message type which is ACECONN_MSGTYPE_MAXLEN long.                        */
enum {ACECONN_HEADER_BYTES = ((ACECONN_HEADER_SWAPFIELDS * 4) + ACECONN_MSGTYPE_BUFLEN)} ;

typedef struct
{
  int byte_swap ;					    /* Set to ACECONN_BYTE_ORDER */
  int length ;						    /* Length of data being sent in bytes. */
  int server_version ;					    /* Server sets this. */
  int client_id ;					    /* Unique client identifier, set by
							       server. */
  int max_bytes ;					    /* Maximum request size, set by server. */
  char msg_type[ACECONN_MSGTYPE_BUFLEN] ;		    /* See the msgs defined above. */
} AceConnHeaderRec, *AceConnHeader ;


/* On connection to the server, the client must send this text.              */
#define ACECONN_CLIENT_HELLO "bonjour"

/* The server will reply with this string when password verification is      */
/* complete.                                                                 */
#define ACECONN_SERVER_HELLO "et bonjour a vous"

/* Server can send a reply in slices, it will return "encore" when this is   */
/* true and client must reply with "encore" to get further slices.           */
#define ACECONN_SERVER_CLIENT_SLICE "encore"

/* To disconnect the server must sent this text.                             */
#define ACECONN_CLIENT_GOODBYE "quit"

#endif /* !defined DEF_ACECONNPROTO_H */

