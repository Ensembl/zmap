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
 * Last edited: Jan 21 17:59 2004 (edgrif)
 * Created: Thu Jul 24 14:39:06 2003 (edgrif)
 * CVS info:   $Id: zmapConfig_P.h,v 1.1 2004-01-21 18:15:31 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIG_P_H
#define ZMAP_CONFIG_P_H

#include <ZMap/zmapConfig.h>


typedef struct _ZMapConfigStruct
{
  char *config_file ;
} ZMapConfigStruct ;


#endif /* !ZMAP_CONFIG_P_H */
