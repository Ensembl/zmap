/*  File: zmapConfig_P.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * and was written by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Feb 17 12:47 2006 (rds)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapConfig_P.h,v 1.7 2006-02-17 13:02:11 rds Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_P_H
#define ZMAP_CONFIG_P_H

#include <ZMap/zmapConfig.h>



/* A stanza consisting of a name and a set of elements. */
typedef struct _ZMapConfigStanzaStruct
{
  char *name ;
  GList *elements ;
} ZMapConfigStanzaStruct ;

/* A set of stanzas held as a list. Note duplication of stanza name, its actually in all the
 * the stanzas as a set of stanzas must be of the same name under the current scheme.
 * This may need to change of course.... */
typedef struct _ZMapConfigStanzaSetStruct
{
  char *name ;
  GList *stanzas ;
} ZMapConfigStanzaSetStruct ;

/* A configuration, including the configuration file and the list of stanzas derived from
 * that file. */
typedef struct _ZMapConfigStruct
{
  char *config_dir ;
  char *config_file ;

  GList *stanzas ;
} ZMapConfigStruct ;


/* These functions are internal to zmapConfig and should not be used outside of this package. */
gboolean zmapGetUserConfig(ZMapConfig config) ;
gboolean zmapGetComputedConfig(ZMapConfig config, char *buffer) ;
gboolean zmapMakeUserConfig(ZMapConfig config) ;

gboolean zmapGetConfigStanzas(ZMapConfig config,
			      ZMapConfigStanza spec_stanza, ZMapConfigStanzaSet *stanzas_out) ;
void zmapConfigDeleteStanzaSet(ZMapConfigStanzaSet stanzas) ;
ZMapConfigStanza zmapConfigCreateStanza(char *stanza_name) ;
ZMapConfigStanza zmapConfigCopyStanza(ZMapConfigStanza stanza_in) ;
void zmapConfigDestroyStanza(ZMapConfigStanza stanza) ;

ZMapConfigStanzaElement zmapConfigCreateElement(char *name, ZMapConfigElementType type) ;
ZMapConfigStanzaElement zmapConfigCopyElement(ZMapConfigStanzaElement element_in) ;
void zmapConfigCopyElementData(ZMapConfigStanzaElement element_in,
			       ZMapConfigStanzaElement element_out) ;
void zmapConfigDestroyElement(ZMapConfigStanzaElement element) ;


#endif /* !ZMAP_CONFIG_P_H */
