/*  File: zmapReadConfig.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2004
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 * This file is part of the ZMap genome database package
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See zmapConfig_P.h
 * HISTORY:
 * Last edited: Apr  6 12:52 2004 (edgrif)
 * Created: Thu Apr  1 14:33:04 2004 (edgrif)
 * CVS info:   $Id: zmapConfigRead.c,v 1.1 2004-04-08 16:40:36 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <zmapConfig_P.h>


/* Stanzas are expected to conform strictly to this format:
 * 
 *    stanza_name
 *    {
 *    keyword = value
 *
 *    [repeated keyword/value pairs]
 *
 *    }
 * 
 *    [repeat stanzas]
 * 
 * Note that white space is irrelevant.
 * 
 */


/* Stanza parsing states. */
typedef enum {STANZA_OUTSIDE,
	      STANZA_BEGIN, STANZA_INSIDE, STANZA_ID, STANZA_VALUE, STANZA_END} ZMapStanzaState ;


static gboolean parsefile(ZMapConfig config, GScanner *scanner) ;


/* Read configuration information from the user configuration file. */
gboolean zmapGetUserConfig(ZMapConfig config)
{
  gboolean result = FALSE ;
  GScanner *scanner ;
  int file_fd ;

  if ((file_fd = open(config->config_file, O_RDONLY, S_IRUSR)) != -1)
    {
      scanner = g_scanner_new(NULL) ;

      g_scanner_input_file(scanner, file_fd) ;

      result = parsefile(config, scanner) ;

      g_scanner_destroy(scanner) ;

      zMapCheck(close(file_fd) == -1, "Could not close config file: %s", config->config_file) ;
    }

  return result ;
}


/* Run through the config file recording all the stanzas we find there.
 * 
 * NOTE that this routine does not validate the contents of the stanzas, only
 * that they are in the correct format. This is a deliberate design decision
 * to keep the config code free from knowledge about the contents of individual
 * resource stanzas.
 * 
 */
static gboolean parsefile(ZMapConfig config, GScanner *scanner)
{
  gboolean result = TRUE ;
  GTokenType token ;
  ZMapStanzaState state ;
  ZMapConfigStanza current_stanza = NULL ;
  ZMapConfigStanzaElement current_element = NULL ;

  state = STANZA_OUTSIDE ;
  while(((token = g_scanner_peek_next_token(scanner)) != G_TOKEN_EOF)
	&& token != G_TOKEN_ERROR)
    {
      GTokenValue current_value ;

      token = g_scanner_get_next_token(scanner) ;

      switch (state)
	{
	case STANZA_OUTSIDE:
	  {
	    if (token == G_TOKEN_IDENTIFIER)
	      {
		current_value = g_scanner_cur_value(scanner) ;

		printf("\nNew Stanza: \"%s\"\n", current_value.v_identifier) ;

		/* Found a new stanza so allocate a struct.... */
		current_stanza = zmapConfigCreateStanza(current_value.v_identifier) ;

		state = STANZA_BEGIN ;
	      }
	    else
	      {
		token = G_TOKEN_ERROR ;
	      }
	    break ;
	  }
	case STANZA_BEGIN:
	  {
	    if (token == G_TOKEN_LEFT_CURLY)
	      {
		state = STANZA_INSIDE ;
	      }
	    else
	      {
		token = G_TOKEN_ERROR ;
	      }
	    break ;
	  }
	case STANZA_INSIDE:
	  {
	    if (token == G_TOKEN_RIGHT_CURLY)
	      {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		state = STANZA_END ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		if (current_stanza->elements == NULL)
		  {
		    /* Should use our logging facilities here...could log into users .ZMap dir... */
		    printf("Empty stanza around line: %d\n", g_scanner_cur_line(scanner)) ;
		    
		    /* Stanza found but is empty so log an error and free it. */
		    zmapConfigDestroyStanza(current_stanza) ;
		  }
		else
		  {
		    /* Add stanza to stanza list, reset current pointer. */
		    config->stanzas = g_list_append(config->stanzas, current_stanza) ;
		    current_stanza = NULL ;
		  }
		
		state = STANZA_OUTSIDE ;

	      }
	    else if (token == G_TOKEN_IDENTIFIER)
	      {
		current_value = g_scanner_cur_value(scanner) ;

		printf("id: %s\n", current_value.v_identifier) ;

		/* Found an element so record it... */
		current_element = zmapConfigCreateElement(current_value.v_identifier,
							  ZMAPCONFIG_INVALID) ;

		state = STANZA_ID ;
	      }
	    else
	      {
		token = G_TOKEN_ERROR ;
	      }
	    break ;
	  }
	case STANZA_ID:
	  {
	    if (token == G_TOKEN_EQUAL_SIGN)
	      {
		state = STANZA_VALUE ;
	      }
	    else
	      {
		token = G_TOKEN_ERROR ;
	      }
	    break ;
	  }
	case STANZA_VALUE:
	  {
	    current_value = g_scanner_cur_value(scanner) ;

	    switch (token)
	      {
	      case G_TOKEN_CHAR:
		{
		  current_element->type = ZMAPCONFIG_BOOL ;
		  if (current_value.v_char == 't')
		    current_element->data.b = TRUE ;
		  else if (current_value.v_char == 'f')
		    current_element->data.b = FALSE ;
		  else
		    token = G_TOKEN_ERROR ;
		  break ;
		}
	      case G_TOKEN_INT:
		{
		  current_element->type = ZMAPCONFIG_INT ;
		  current_element->data.i = current_value.v_int ;
		  break ;
		}
	      case G_TOKEN_FLOAT:
		{
		  current_element->type = ZMAPCONFIG_FLOAT ;
		  current_element->data.f = current_value.v_float ;
		  break ;
		}
	      case G_TOKEN_STRING:
		{
		  current_element->type = ZMAPCONFIG_STRING ;
		  current_element->data.s = g_strdup(current_value.v_string) ;
		  break ;
		}
	      default:
		{
		  token = G_TOKEN_ERROR ;
		  break ;
		}
	      }

	    if (token != G_TOKEN_ERROR)
	      {
		/* Add element to stanza, reset current pointer. */
		current_stanza->elements = g_list_append(current_stanza->elements, current_element) ;
		current_element = NULL ;

		state = STANZA_INSIDE ;
	      }
	    break ;
	  }
	case STANZA_END:
	  {
	    if (current_stanza->elements == NULL)
	      {
		/* Should use our logging facilities here...could log into users .ZMap dir... */
		printf("Empty stanza around line: %d\n", g_scanner_cur_line(scanner)) ;
		
		/* Stanza found but is empty so log an error and free it. */
		zmapConfigDestroyStanza(current_stanza) ;
	      }
	    else
	      {
		/* Add stanza to stanza list, reset current pointer. */
		config->stanzas = g_list_append(config->stanzas, current_stanza) ;
		current_stanza = NULL ;
	      }

	    state = STANZA_OUTSIDE ;
	    break ;
	  }
	default :
	  {
	    zMapCrash("%s", "Code Error: unrecognised state in config file parser") ;
	    break ;
	  }
	}

      if (token == G_TOKEN_ERROR)
	{
	  /* Should use our logging facilities here...could log into users .ZMap dir... */
	  printf("Bad text at line: %d\n", g_scanner_cur_line(scanner)) ;

	  /* Free current element and its resources (has not yet been added to current stanza). */
	  if (current_element)
	    zmapConfigDestroyElement(current_element) ;

	  /* Free stanza and its resources. */
	  if (current_stanza)
	    zmapConfigDestroyStanza(current_stanza) ;

	  result = FALSE ;

	  break ;
	}
    }

  return result ;
}


