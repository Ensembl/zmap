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
 * Last edited: Mar 18 16:16 2004 (edgrif)
 * Created: Thu Mar 18 15:45:59 2004 (edgrif)
 * CVS info:   $Id: saxparse.c,v 1.1 2004-03-22 13:45:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <expat.h>
#include <glib.h>
#include <saxparse_P.h>


static void start_tag(void *userData, const char *el, const char **attr) ;
static void end_tag(void *userData, const char *el) ;
static void charhndl(void *userData, const XML_Char *s, int len) ;
static void tagFree(gpointer data, gpointer user_data) ;


SaxParser saxCreateParser(void)
{
  SaxParser parser ;

  parser = (SaxParser)g_new(SaxParserStruct, 1) ;

  parser->p = XML_ParserCreate(NULL) ;
  if (!parser->p)
    {
      fprintf(stderr, "Couldn't allocate memory for parser\n");
      exit(-1);
    }

  XML_SetElementHandler(parser->p, start_tag, end_tag);

  XML_SetCharacterDataHandler(parser->p, charhndl) ;

  XML_SetUserData(parser->p, (void *)parser) ;

  parser->Depth = 0 ;

  parser->tag_stack = g_queue_new() ;

  parser->content = g_string_sized_new(1000) ;		    /* wild guess at size... */

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

  if (!XML_Parse(parser->p, data, size, final))
    {
      parser->last_errmsg = g_strdup_printf("saxparse - Parse error at line %d:\n%s\n",
					    XML_GetCurrentLineNumber(parser->p),
					    XML_ErrorString(XML_GetErrorCode(parser->p))) ;
      result = FALSE ;
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
      /* Need newer version of glib...hopefully to be installed on Alphas etc.... */
      g_queue_foreach(parser->tag_stack, tagFree, parser) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }
  g_queue_free(parser->tag_stack) ;

  g_string_free(parser->content, TRUE) ;		    /* TRUE => free string data as well. */

  g_free(parser->last_errmsg) ;

  g_free(parser) ;

  return ;
}





static void tagFree(gpointer data, gpointer user_data)
{
  SaxParser parser = (SaxParser)user_data ;

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
  int i;

  for (i = 0; i < parser->Depth; i++)
    printf("  ");

  printf("<%s>", el) ;

  if (attr[i])
    printf("  -  ") ;

  for (i = 0; attr[i]; i += 2)
    {
      printf(" %s='%s'", attr[i], attr[i + 1]);
    }

  printf("\n");

  g_queue_push_head(parser->tag_stack, g_strdup(el)) ;

  parser->Depth++;

  return ;
}


static void end_tag(void *userData, const char *el)
{
  SaxParser parser = (SaxParser)userData ;
  int i ;
  gpointer dummy ;

  parser->Depth-- ;

  if (parser->content->len)
    {
      gchar *tag = (gchar *)g_queue_peek_head(parser->tag_stack) ;

      for (i = 0; i < parser->Depth; i++)
	printf("  ");

      printf("content(<%s>)  -  %s\n", tag, parser->content->str) ;

      /* reset once printed..... */
      parser->content = g_string_set_size(parser->content, 0) ;
    }

  dummy = g_queue_pop_head(parser->tag_stack) ;		    /* Now get rid of head element. */
  g_free(dummy) ;

  for (i = 0; i < parser->Depth; i++)
    printf("  ");

  printf("</%s>\n", el);

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



