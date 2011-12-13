/*  File: zmapRemoteControl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2010: Genome Research Ltd.
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
 *      Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: External interface to remote control package.
 *
 * HISTORY:
 * Last edited: Dec  1 15:49 2011 (edgrif)
 * Created: Fri Sep 24 14:51:35 2010 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_REMOTE_PROTOCOL_H
#define ZMAP_REMOTE_PROTOCOL_H

#include <gtk/gtk.h>



/* 
 *     Genome Annotation Transfer Protocol (GACP)
 */


/* Protocol name used in header. */
#define GACP_NAME "GACP"

/* Protocol "types", describe what type of message it is. */
#define GACP_REQUEST  "request"
#define GACP_REPLY    "reply"
#define GACP_VERSION  "2.0"

/* Protocol format, used to format the protocol message headers:
 * 
 *              "<GACP version=vv type=[request | reply] serial_number=nnnnn>"
 * 
 *  */
#define GACP_HEADER_FORMAT "<" GACP_NAME " version=" GACP_VERSION " type=%s serial_number=%d>"


#define GACP_CLIPBOARD_DATA_TYPE   "GACP_COMMAND_STR"
#define GACP_CLIPBOARD_DATA_FORMAT 8			    /* Bits per unit. */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* errr...what's this for ??? */
#define ANNOTATION_ICC "ANNOTATION_DATA"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#endif /* ZMAP_REMOTE_PROTOCOL_H */
