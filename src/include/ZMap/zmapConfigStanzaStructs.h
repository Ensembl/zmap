/*  File: zmapConfigStanzaStructs.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * HISTORY:
 * Last edited: Nov 13 08:58 2008 (edgrif)
 * Created: Tue Aug 26 12:38:28 2008 (rds)
 * CVS info:   $Id: zmapConfigStanzaStructs.h,v 1.4 2010-03-04 15:14:54 mh17 Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAPCONFIGSTANZASTRUCTS_H
#define ZMAPCONFIGSTANZASTRUCTS_H


/* We should convert this to use the same calls/mechanism as the keyvalue stuff below. */
typedef struct _ZMapConfigSourceStruct
{
  char *url ;
  char *version ;
  char *featuresets, *navigatorsets ;
  char *styles_list, *stylesfile ;
  char *format ;
  int timeout ;
  gboolean writeback, sequence ;
} ZMapConfigSourceStruct, *ZMapConfigSource;


typedef enum {ZMAPCONF_INVALID, ZMAPCONF_BOOLEAN, ZMAPCONF_INT, ZMAPCONF_DOUBLE,
	      ZMAPCONF_STR,  ZMAPCONF_STR_ARRAY} ZMapKeyValueType ;
typedef enum {ZMAPCONV_INVALID, ZMAPCONV_NONE, ZMAPCONV_STR2ENUM, ZMAPCONV_STR2COLOUR} ZMapKeyValueConv  ;

typedef int (*ZMapConfStr2EnumFunc)(const char *str) ;

typedef struct ZMapKeyValueStructID
{
  char *name ;
  gboolean has_value ;

  ZMapKeyValueType type ;
  union
  {
    gboolean b ;
    int i ;
    double d ;
    char *str ;
    char **str_array ;
  } data ;

  ZMapKeyValueConv conv_type ;
  union
  {
    ZMapConfStr2EnumFunc str2enum ;
  } conv_func ;

} ZMapKeyValueStruct, *ZMapKeyValue ;


#endif /* ZMAPCONFIGSTANZASTRUCTS_H */
