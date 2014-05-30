/*  File: zmapDataSource_P.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2014: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *         Rob Clack (Sanger Institute, UK) rnc@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description:
 * Exported functions:
 *-------------------------------------------------------------------
 */

#ifndef DATA_SOURCE_P_H
#define DATA_SOURCE_P_H


#include <zmapDataSource.h>
#include <stdint.h>

/*
 * HTS header. ATM we have different locations for 32 and 64 bit systems.
 */
/* Intel only version...
#if __x86_64__
#endif
*/



#if UINTPTR_MAX == 0xffffffff
#include "/nfs/vertres01/user/jm18/lucid-i686/samtools-exp-rc/include/htslib/hts.h"
#include "/nfs/vertres01/user/jm18/lucid-i686/samtools-exp-rc/include/htslib/sam.h"
#elif UINTPTR_MAX == 0xffffffffffffffff
#include "/software/vertres/bin-external/samtools-0.2.0-rc7/include/htslib/hts.h"
#include "/software/vertres/bin-external/samtools-0.2.0-rc7/include/htslib/sam.h"
#endif



/*
 *  Full declarations of concrete types.
 */
typedef struct ZMapDataSourceGIOStruct_
  {
    ZMapDataSourceType type ;
    GIOChannel *io_channel ;
  } ZMapDataSourceGIOStruct  ;

typedef struct ZMapDataSourceHTSFileStruct_
  {
    ZMapDataSourceType type ;
    htsFile *hts_file ;
  } ZMapDataSourceHTSFileStruct ;




#endif
