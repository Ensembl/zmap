/*  Last edited: Apr 13 15:01 2004 (rnc) */
/*  file: stringbucket.h
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
 *-------------------------------------------------------------------
 * Zmap is free software; you can redistribute it and/or
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
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk and
 *	Simon Kelley (Sanger Institute, UK) srk@sanger.ac.uk
 */

#include <../acedb/acedb.h>

/* Stringbucket:

Given a string returns a copy which is persistant until the
bucket is destroyed.

Has the property that calling str2p twice on the same string yields
the same pointer.
*/

#ifndef STRINGBUCKET_H
#define STRINGBUCKET_H

struct sbucket;

typedef struct sbucket StringBucket; 

StringBucket *sbCreate(STORE_HANDLE handle);
void sbDestroy(StringBucket *b);
char *str2p(char *string, StringBucket *b);

#endif
