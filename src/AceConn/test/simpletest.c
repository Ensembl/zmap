/*  File: simpletest.c
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
 * Description: Simple test program for socket package allowing a single
 *              command to be sent to the server and then quitting.
 * HISTORY:
 * Last edited: Sep  5 14:46 2003 (edgrif)
 * Created: Mon Mar 11 14:25:09 2002 (edgrif)
 * CVS info:   $Id: simpletest.c,v 1.1.1.1 2007-10-03 13:28:00 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>

#include <getopt.h>
#include <glib.h>
#include <AceConn.h>

/* Defaults, ok for me, not for anyone else....                              */
#define DEF_HOST     "griffin2"
#define DEF_PORT     22222
#define DEF_USERID   "anonymous"
#define DEF_PASSWD   "anyone"
#define DEF_TIMEOUT  120

static void usage(char *err_str) ;



int main(int argc, char **argv)
{
  int rc = EXIT_SUCCESS ;
  guint version, release, update ;
  AceConnStatus status ;
  char *host = DEF_HOST ;
  int port = DEF_PORT ;
  char *userid = DEF_USERID ;
  char *passwd = DEF_PASSWD ;
  int timeout = DEF_TIMEOUT ;
  AceConnection connection = NULL ;
  char *request = "find sequence" ;
  char *reply = NULL ;
  int reply_len ;

  while (1)
    {
      struct option long_options[] =
      {
	{"host",     required_argument, NULL, 'h'},
	{"port",     required_argument, NULL, 'p'},
	{"userid",   required_argument, NULL, 'u'},
	{"passwd",   required_argument, NULL, 'x'},
	{"timeout",  required_argument, NULL, 't'},
	{0, 0, 0, 0}
      } ;
      int c ;
      int option_index = 0 ;				    /* getopt_long stores the option index */
							    /* here. */

      c = getopt_long_only(argc, argv, "t:p:u:x:oif:",
			   long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

	case 'h':
	  host = optarg ;
	  break ;

        case 'p':
	  if (sscanf(optarg, "%d", &port) != 1)
	    {
	      usage(g_strdup_printf("Bad -p option: %s\n", optarg)) ;
	    }
          break;

	case 'u':
	  userid = optarg ;
	  break ;

	case 'x':
	  passwd = optarg ;
	  break ;

        case 't':
	  if (sscanf(optarg, "%d", &timeout) != 1)
	    {
	      usage(g_strdup_printf("Bad -t option: %s\n", optarg)) ;
	    }
          break;

        case '?':
          /* getopt_long prints an error message. */
	  usage(NULL) ;
          break;

        default:
	  usage(g_strdup_printf("Internal logic error, unexpected value returned by"
				"getopt_long_only() call: %c\n", c)) ;
        }

    }


  /* User can enter a single string request to be sent to server.            */
  if (optind < argc)
    {
      request = argv[optind++] ;
      if (optind < argc)
	usage(g_strdup_printf("Excess command line parameter: %s\n", argv[optind])) ;
    }

  /* Show the version.                                                       */
  printf("\nAceConn Header  Version: %d.%d.%d\n",
	 ACECONN_VERSION, ACECONN_RELEASE, ACECONN_UPDATE) ;
  AceConnGetVersion(&version, &release, &update) ;
  printf("AceConn Library Version: %d.%d.%d\n", version, release, update) ;
  if (!ACECONN_CHECK_VERSION(version, release, update))
    usage("AceConn.h and libAceConn.a are incompatible.\n") ;
  else
    printf("\n") ;


  /* Make the connection.                                                    */
  if ((status = AceConnCreate(&connection, host, port, userid, passwd, timeout)) != ACECONN_OK)
    {
      printf("Cannot create initial connection object.\n") ;

      exit(EXIT_FAILURE) ;
    }

  /* Connect to server.                                                      */
  if (status == ACECONN_OK)
    {
      status = AceConnConnect(connection) ;
      if (status != ACECONN_OK)
	{
	  printf("Error connecting: %s\n", AceConnGetErrorMsg(connection, status)) ;
	}
    }

  if (status == ACECONN_OK)
    {
      status = AceConnRequest(connection, request, (void *)(&reply), &reply_len) ; 
      if (status == ACECONN_OK || status == ACECONN_QUIT)
	{
	  printf("Reply length: %d\n", reply_len) ;
	  printf("Reply text: >|%s|<\n", reply) ;
	  g_free(reply) ;
	}
      else
	{
	  printf("Error making request: %s\n", AceConnGetErrorMsg(connection, status)) ;
	}
    }

  if (status == ACECONN_OK || status == ACECONN_QUIT)
    {
      status = AceConnDisconnect(connection) ;
      if (status != ACECONN_OK)
	{
	  printf("Error disconnnecting: %s\n", AceConnGetErrorMsg(connection, status)) ;
	}
    }


  return rc ;
}



static void usage(char *err_str)
{
  if (err_str)
    printf("Error: %s\n", err_str) ;

  printf("\nusage:\n"
	 "\tsimpletest [-h host] [-p number] [-u userid] [-x passwd][-t time_out_secs] {request]\n"
	 "Allows you to send a single request to the Acedb socket server and then quits.\n") ;

  exit(EXIT_FAILURE) ;

  return ;
}
