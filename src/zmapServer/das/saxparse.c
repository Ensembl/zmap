/*  File: saxparse.c
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
 * Description: Uses expat xml parser to parse DAS2 input using the
 *              SAX processing model.
 *              
 * Exported functions: See saxparse.h
 * HISTORY:
 * Last edited: Jul 16 09:39 2004 (edgrif)
 * Created: Thu Mar 18 15:45:59 2004 (edgrif)
 * CVS info:   $Id: saxparse.c,v 1.3 2005-08-11 13:13:50 rds Exp $
 *-------------------------------------------------------------------
 */
#include <config.h>
#include <stdio.h>
#include <expat.h>
#include <glib.h>
#include <ZMap/zmapUtils.h>
#include <saxparse_P.h>


static void start_tag(void *userData, const char *el, const char **attr) ;
static void end_tag(void *userData, const char *el) ;
static void charhndl(void *userData, const XML_Char *s, int len) ;
static void tagFree(gpointer data, gpointer user_data) ;

#if defined DARWIN
/* total hack to get round problem on mac... See note in runconfig!!!!! */
XML_Bool XML_ParserReset(XML_Parser parser, const XML_Char *encoding)
{
  return FALSE ;
}
#endif

SaxParser saxCreateParser(void *user_data)
{
  SaxParser parser ;

  parser = (SaxParser)g_new0(SaxParserStruct, 1) ;

  parser->debug = TRUE ;

  if (!(parser->p = XML_ParserCreate(NULL)))
    {
      zMapLogFatal("%s", "XML_ParserCreate() failed.") ;
    }

  parser->Depth = 0 ;

  parser->tag_stack = g_queue_new() ;

  parser->content = g_string_sized_new(20000) ;		    /* wild guess at size... */

  parser->last_errmsg = NULL ;

  return parser ;
}



gboolean saxParseData(SaxParser parser, void *data, int size)
{
  gboolean result = TRUE ;
  int final ;

  if (size)
    final = 0 ;
  else
    final = 1 ;

  /* try resetting parser..... */
  if (XML_ParserReset(parser->p, NULL) != XML_TRUE)
    {
      zMapLogFatal("%s", "XML_ParserReset() call failed.") ;
    }
  else
    {
      /* When the parser is reset it loses just about everything so reset the lot. */
      XML_SetElementHandler(parser->p, start_tag, end_tag);
      XML_SetCharacterDataHandler(parser->p, charhndl) ;
      XML_SetUserData(parser->p, (void *)parser) ;


      if (!XML_Parse(parser->p, data, size, final))
	{
	  if (parser->last_errmsg)
	    g_free(parser->last_errmsg) ;

	  parser->last_errmsg = g_strdup_printf("saxparse - Parse error (line %d): %s\n",
						XML_GetCurrentLineNumber(parser->p),
						XML_ErrorString(XML_GetErrorCode(parser->p))) ;
	  result = FALSE ;
	}
    }


  return result ;
}


void saxDestroyParser(SaxParser parser)
{
  XML_ParserFree(parser->p) ;

  /* Deallocate tag data and then free the queue. */
  if (!g_queue_is_empty(parser->tag_stack))
    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* alpha version of gtk does not have this....so do it by steam...sigh.... */
      g_queue_foreach(parser->tag_stack, tagFree, parser) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      gpointer dummy ;

      while ((dummy = g_queue_pop_head(parser->tag_stack)))
	{
	  g_free(dummy) ;
	}

    }
  g_queue_free(parser->tag_stack) ;

  g_string_free(parser->content, TRUE) ;		    /* TRUE => free string data as well. */

  if (parser->last_errmsg)
    g_free(parser->last_errmsg) ;

  g_free(parser) ;

  return ;
}


static void tagFree(gpointer data, gpointer user_data_unused)
{
  SaxParser parser = (SaxParser)user_data_unused ;

  g_free(data) ;

  return ;
}



/* Set handlers for start and end tags. Attributes are passed to the start handler
 * as a pointer to a vector of char pointers. Each attribute seen in a start (or empty)
 * tag occupies 2 consecutive places in this vector: the attribute name followed by the
 * attribute value. These pairs are terminated by a null pointer. */

static void start_tag(void *userData, const char *el, const char **attr)
{
  SaxParser parser = (SaxParser)userData ;
  SaxTag current_tag ;
  int i;
  

  if (parser->debug)
    {
      for (i = 0; i < parser->Depth; i++)
	printf("  ");
      
      printf("<%s>", el) ;

      if (attr[0])
	{
	  printf("  -  ") ;

	  for (i = 0; attr[i]; i += 2)
	    {
	      printf(" %s='%s'", attr[i], attr[i + 1]);
	    }
	}

      printf("\n");
    }


  /* Push the current tag on to our stack of tags. */
  current_tag = g_new0(SaxTagStruct, 1) ;
  current_tag->element_name = (char *)el ;
  current_tag->attributes = (char **)attr ;
  g_queue_push_head(parser->tag_stack, current_tag) ;

  parser->Depth++;

  return ;
}


static void end_tag(void *userData, const char *el)
{
  SaxParser parser = (SaxParser)userData ;
  int i ;
  gpointer dummy ;
  SaxTag current_tag ;

  parser->Depth-- ;

  dummy = g_queue_pop_head(parser->tag_stack) ;		    /* Now get rid of head element. */

  current_tag = (SaxTag)dummy ;

  /* If there is content between the start/end tags process it. */
  if (parser->content->len)
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      gchar *tag = (gchar *)g_queue_peek_head(parser->tag_stack) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      if (parser->debug)
	{
	  for (i = 0; i < parser->Depth; i++)
	    printf("  ");

	  printf("%s content -  %s\n", current_tag->element_name, parser->content->str) ;
	}


      /* Here is where we need to do some of the (most of the ?) gff processing.... */


      /* reset once processed..... */
      parser->content = g_string_set_size(parser->content, 0) ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  dummy = g_queue_pop_head(parser->tag_stack) ;		    /* Now get rid of head element. */

  current_tag = (SaxTag)dummy ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  g_free(dummy) ;


  if (parser->debug)
    {
      for (i = 0; i < parser->Depth; i++)
	printf("  ") ;

      printf("</%s>\n", el) ;
    }

  return ;
}



/* Set a text handler. The string your handler receives is NOT zero terminated.
 * You have to use the length argument to deal with the end of the string.
 * A single block of contiguous text free of markup may still result in a
 * sequence of calls to this handler. In other words, if you're searching for a pattern
 * in the text, it may be split across calls to this handler. 
 */

static void charhndl(void *userData, const XML_Char *s, int len)
{
  int i ;
  SaxParser parser = (SaxParser)userData ;


  /* N.B. we _only_ get away with this because XML_Char == char at the moment, this will need
   * to be fixed in the future..... */
  parser->content = g_string_append_len(parser->content, s, len) ;


  return ;
}



