/*  File: check_libpfetch.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr 16 10:57 2009 (rds)
 * Created: Tue Mar 31 10:14:55 2009 (rds)
 * CVS info:   $Id: check_libpfetch.h,v 1.2 2009-06-04 09:15:59 rds Exp $
 *-------------------------------------------------------------------
 */


#ifndef CHECK_LIBPFETCH_H
#define CHECK_LIBPFETCH_H

#include <check.h>
#include <libpfetch/libpfetch.h>
#include <tests/zmapCheck.h>

#define PFETCH_SUITE_NAME "Pfetch Suite"

#define PFETCH_READ_SIZE 80

#define CHECK_PFETCH_ISOFORM_SEQ TRUE
#define CHECK_PFETCH_DEBUG       TRUE
#define CHECK_PFETCH_FULL        TRUE


/* Allow for -D for location, port and cookie jar when compiling */
#ifndef CHECK_PFETCH_LOCATION
#define CHECK_PFETCH_LOCATION    "pfetch"
#endif 

#ifndef CHECK_PFETCH_PORT
#define CHECK_PFETCH_PORT        80
#endif 

#ifndef CHECK_PFETCH_COOKIE_JAR
#define CHECK_PFETCH_COOKIE_JAR  "~/.cookie_jar"
#endif 

#ifdef VARIANT_TEST
#define CHECK_PFETCH_SEQUENCE    "Sw:Q9Y6M0-3.1"
#else
#define CHECK_PFETCH_SEQUENCE    "Sw:Q9H3S3.1"
#endif /* VARIANT_TEST */

#define CHECK_PFETCH_NO_MATCH    "No Match"


Suite *zMapCheckPfetchSuite(void);

#endif /* CHECK_LIBPFETCH_H */

