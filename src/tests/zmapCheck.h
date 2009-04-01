/*  File: zmapCheck.h
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
 * Last edited: Mar 30 16:53 2009 (rds)
 * Created: Mon Mar 30 15:40:45 2009 (rds)
 * CVS info:   $Id: zmapCheck.h,v 1.1 2009-04-01 11:56:49 rds Exp $
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_CHECK_H
#define ZMAP_CHECK_H

#include <glib.h>
#include <glib-object.h>
#include <check.h>

#define ZMAP_HARNESS_VERSION "0.001"

#define ZMAP_CHECK_MSG_STRING "blah, blah, blah..."

#define ZMAP_CHECK_OUTPUT_LEVEL CK_VERBOSE
//#define ZMAP_CHECK_OUTPUT_LEVEL CK_NORMAL

typedef Suite * (*ZMapCheckSuiteCreateFunc)(void);

#endif /* ZMAP_CHECK_H */
