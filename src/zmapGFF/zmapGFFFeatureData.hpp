/*  File: zmapGFFFeatureData.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
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
 *     Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
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
#include <ZMap/zmap.hpp>
#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapUtilsDebug.hpp>
#include <ZMap/zmapString.hpp>
#include <ZMap/zmapFeature.hpp>
#include <ZMap/zmapSO.hpp>
#include <ZMap/zmapSOParser.hpp>
#include <ZMap/zmapGFF.hpp>
#include <zmapGFF3_P.hpp>
#include <zmapGFFAttribute.hpp>



/*
 * Structure to store field data used to construct features.
 */
typedef struct ZMapGFFFeatureDataStruct_
  {
    char * sSequence ;
    char * sSource ;
    int iStart ;
    int iEnd ;
    double dScore ;
    ZMapStrand cStrand ;
    ZMapPhase cPhase ;
    char *sAttributes ;
    ZMapGFFAttribute *pAttributes ;
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
void zMapGFFFeatureDataDestroy(ZMapGFFFeatureData) ;

/*
 * Return flag values and data values.
 */
gboolean zMapGFFFeatureDataGetFlagSeq(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagSou(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagSta(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagEnd(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagSco(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagStr(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagPha(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagAtt(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagPat(ZMapGFFFeatureData) ;
gboolean zMapGFFFeatureDataGetFlagSod(ZMapGFFFeatureData) ;

/*
 * Set some of the flag values
 */
gboolean zMapGFFFeatureDataSetFlagSco(ZMapGFFFeatureData, gboolean) ;

/*
 * Set some data members of the object.
 */
gboolean zMapGFFFeatureDataSetSeq(ZMapGFFFeatureData, const char * const ) ;
gboolean zMapGFFFeatureDataSetSou(ZMapGFFFeatureData, const char * const ) ;
gboolean zMapGFFFeatureDataSetSta(ZMapGFFFeatureData, int) ;
gboolean zMapGFFFeatureDataSetEnd(ZMapGFFFeatureData, int) ;
gboolean zMapGFFFeatureDataSetSco(ZMapGFFFeatureData, double ) ;
gboolean zMapGFFFeatureDataSetSod(ZMapGFFFeatureData, ZMapSOIDData ) ;
gboolean zMapGFFFeatureDataSetStr(ZMapGFFFeatureData, ZMapStrand) ;
gboolean zMapGFFFeatureDataSetPha(ZMapGFFFeatureData, ZMapPhase) ;

/*
 * Return various data elements of the object.
 */
char*                      zMapGFFFeatureDataGetSeq(ZMapGFFFeatureData) ;
char*                      zMapGFFFeatureDataGetSou(ZMapGFFFeatureData) ;
int                        zMapGFFFeatureDataGetSta(ZMapGFFFeatureData) ;
int                        zMapGFFFeatureDataGetEnd(ZMapGFFFeatureData) ;
double                     zMapGFFFeatureDataGetSco(ZMapGFFFeatureData) ;
ZMapStrand                 zMapGFFFeatureDataGetStr(ZMapGFFFeatureData) ;
ZMapPhase                  zMapGFFFeatureDataGetPha(ZMapGFFFeatureData) ;
char *                     zMapGFFFeatureDataGetAtt(ZMapGFFFeatureData) ;
unsigned int               zMapGFFFeatureDataGetNat(ZMapGFFFeatureData) ;
ZMapGFFAttribute *         zMapGFFFeatureDataGetAts(ZMapGFFFeatureData) ;
const ZMapSOIDData         zMapGFFFeatureDataGetSod(ZMapGFFFeatureData) ;

/*
 * Other manipulation functions.
 */
gboolean zMapGFFFeatureDataSet(ZMapGFFFeatureData,
                               const char * const,
                               const char * const,
                               int,
                               int,
                               gboolean,
                               double,
                               ZMapStrand,
                               ZMapPhase,
                               const char * const,
                               ZMapGFFAttribute *,
                               unsigned int,
                               ZMapSOIDData) ;
gboolean zMapGFFFeatureDataIsValid(ZMapGFFFeatureData) ;
ZMapGFFFeatureData zMapGFFFeatureDataCC(ZMapGFFFeatureData) ;



#endif
