/*  File: zmapGFFFeatureData.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2013: Genome Research Ltd.
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
 *     	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Header file for data structure and associated functions
 * associated with FeatureData class. This is used to represent the data
 * that are parser out of a GFF file in order to construct a feature.
 *
 *-------------------------------------------------------------------
 */


#ifndef ZMAP_GFFFEATUREDATA_P_H
#define ZMAP_GFFFEATUREDATA_P_H


#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <ZMap/zmap.h>
#include <ZMap/zmapUtils.h>
#include <ZMap/zmapUtilsDebug.h>
#include <ZMap/zmapString.h>
#include <ZMap/zmapFeature.h>
#include <ZMap/zmapSO.h>
#include <ZMap/zmapSOParser.h>
#include <ZMap/zmapGFF.h>
#include "zmapGFF3_P.h"
#include "zmapGFFAttribute.h"



/*
 * Structure to store field data used to construct features.
 */
typedef struct ZMapGFFFeatureDataStruct_
  {
    char * sSequence ;
    char * sSource ;
    unsigned int iStart ;
    unsigned int iEnd ;
    double dScore ;
    ZMapStrand cStrand ;
    ZMapPhase cPhase ;
    char *sAttributes ;
    const ZMapGFFAttribute *pAttributes ;
    unsigned int nAttributes ;
    ZMapSOIDData pSOIDData ;

    struct
      {
        unsigned int got_seq : 1 ;           /* sequence           */
        unsigned int got_sou : 1 ;           /* source             */
        unsigned int got_sta : 1 ;           /* start              */
        unsigned int got_end : 1 ;           /* end                */
        unsigned int got_sco : 1 ;           /* score              */
        unsigned int got_str : 1 ;           /* strand             */
        unsigned int got_pha : 1 ;           /* phase              */
        unsigned int got_att : 1 ;           /* attributes string  */
        unsigned int got_pat : 1 ;           /* parsed attributes  */
        unsigned int got_sod : 1 ;           /* so term data       */
    } flags ;


  } ZMapGFFFeatureDataStruct ;



/*
 * Creation and deletion of the objects
 */
ZMapGFFFeatureData zMapGFFFeatureDataCreate() ;
void zMapGFFFeatureDataDestroy(ZMapGFFFeatureData const ) ;

/*
 * Return flag values and data values.
 */
gboolean zMapGFFFeatureDataGetFlagSeq(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagSou(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagSta(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagEnd(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagSco(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagStr(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagPha(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagAtt(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagPat(const ZMapGFFFeatureData const) ;
gboolean zMapGFFFeatureDataGetFlagSod(const ZMapGFFFeatureData const) ;

/*
 * Return various data elements of the object.
 */
char*                      zMapGFFFeatureDataGetSeq(const ZMapGFFFeatureData const) ;
char*                      zMapGFFFeatureDataGetSou(const ZMapGFFFeatureData const) ;
unsigned int               zMapGFFFeatureDataGetSta(const ZMapGFFFeatureData const) ;
unsigned int               zMapGFFFeatureDataGetEnd(const ZMapGFFFeatureData const) ;
double                     zMapGFFFeatureDataGetSco(const ZMapGFFFeatureData const) ;
ZMapStrand                 zMapGFFFeatureDataGetStr(const ZMapGFFFeatureData const) ;
ZMapPhase                  zMapGFFFeatureDataGetPha(const ZMapGFFFeatureData const) ;
char *                     zMapGFFFeatureDataGetAtt(const ZMapGFFFeatureData const) ;
unsigned int               zMapGFFFeatureDataGetNat(const ZMapGFFFeatureData const) ;
const ZMapGFFAttribute *   zMapGFFFeatureDataGetAts(const ZMapGFFFeatureData const) ;
const ZMapSOIDData         zMapGFFFeatureDataGetSod(const ZMapGFFFeatureData const) ;

/*
 * Other manipulation functions.
 */
gboolean zMapGFFFeatureDataSet(ZMapGFFFeatureData const,
                               const char * const,
                               const char * const,
                               unsigned int,
                               unsigned int,
                               gboolean,
                               double,
                               ZMapStrand,
                               ZMapPhase,
                               const char * const,
                               const ZMapGFFAttribute * const,
                               unsigned int,
                               const ZMapSOIDData const) ;
gboolean zMapGFFFeatureDataIsValid(const ZMapGFFFeatureData const ) ;



#endif
