/*  File: utils.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2012: Genome Research Ltd.
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
 *-------------------------------------------------------------------
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>             /* EXIT_FAILURE & EXIT_SUCCESS */
#include <math.h>
#include <time.h>
#include <gtk/gtk.h>

#define CANVAS_ORIGIN_X (0.0)
#define CANVAS_ORIGIN_Y (0.0)

#define CANVAS_WIDTH  (100.0)
#define CANVAS_HEIGHT (1300.0)

#define COLUMN_WIDTH (20.0)
#define COLUMN_SPACE (2.0)

#define ITEMS_PER_COLUMN (2)
#define ITEM_MAX_HEIGHT (1260.0)

void demoUtilsExit(int exit_rc);

#endif /* UTILS_H */
