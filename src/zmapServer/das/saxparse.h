/*  File: saxparse.h
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
 * HISTORY:
 * Last edited: Jul  2 09:32 2004 (edgrif)
 * Created: Wed Jan 28 16:17:58 2004 (edgrif)
 * CVS info:   $Id: saxparse.h,v 1.2 2004-07-19 10:13:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef SAXPARSE_H
#define SAXPARSE_H

#include <glib.h>

typedef struct SaxParserStructName *SaxParser ;

SaxParser saxCreateParser(void *user_data) ;

gboolean saxParseData(SaxParser parser, void *data, int size) ;

void saxDestroyParser(SaxParser parser) ;

#endif /* !SAXPARSE_H */
