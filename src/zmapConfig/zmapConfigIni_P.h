/*  File: zmapConfigIni_P.h
 *  Author: malcolm hinsley (mh17@sanger.ac.uk)
 *  Copyright (c) 2006: Genome Research Ltd.
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *      Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Created: 2009-12-09 13:10:58 (mgh)
 * CVS info:   $Id: zmapConfigIni_P.h,v 1.1 2009-12-14 12:20:02 mh17 Exp $
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_CONFIGINI_P_H
#define ZMAP_CONFIGINI_P_H

#define FILE_COUNT 5
#define IMPORTANT_COUNT 2

typedef struct _ZMapConfigIniStruct
{
  //  ZMapMagic magic;
  GKeyFile *buffer_key_file;
  GKeyFile *extra_key_file;
  GKeyFile *user_key_file;
  GKeyFile *zmap_key_file;
  GKeyFile *sys_key_file;

  GError *buffer_key_error;
  GError *extra_key_error;
  GError *user_key_error;
  GError *zmap_key_error;
  GError *sys_key_error;

  unsigned int unsaved_alterations : 1;

} ZMapConfigIniStruct;




#endif /* !ZMAP_CONFIGINI_P_H */
