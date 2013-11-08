/*  File: zmapGFFHeader.h
 *  Author: Steve Miller (sm23@sanger.ac.uk)
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
 * originated by
 *     	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *
 * Description: Header file for data structure and associated functions
 * for dealing with GFF header. This essentially refers to a collection
 * of directive ("##" line) objects.
 *
 *-------------------------------------------------------------------
 */

#ifndef ZMAP_GFFHEADER_P_H
#define ZMAP_GFFHEADER_P_H

#include <ZMap/zmapGFF.h>
#include "zmapGFF_P.h"
#include "zmapGFF3_P.h"

/*
 * Base `class'' struct for the header data structure.
 */
typedef struct ZMapGFFHeaderStruct_
{
  char *sequence_name ;
  int features_start ;
  int features_end ;

  ZMapGFFHeaderState eState ;

  struct
  {
    unsigned int got_min : 1 ; /* minimal header (ver & sqr)  */
    unsigned int got_ver : 1 ; /* ##gff-version               */
    unsigned int got_dna : 1 ; /* ##DNA                       */
    unsigned int got_sqr : 1 ; /* ##sequence-region           */
    unsigned int got_feo : 1 ; /* ##feature-ontology          */
    unsigned int got_ato : 1 ; /* ##attribute-ontology        */
    unsigned int got_soo : 1 ; /* ##source-ontology           */
    unsigned int got_spe : 1 ; /* ##species                   */
    unsigned int got_gen : 1 ; /* ##genome-build              */
    unsigned int got_fas : 1 ; /* ##FASTA                     */
    unsigned int got_clo : 1 ; /* ###                         */
  } flags ;

  ZMapGFFDirective pDirective[ZMAPGFF_NUMBER_DIR_TYPES] ;

} ZMapGFFHeaderStruct ;



/*
 * Header creation and destruction.
 */
ZMapGFFHeader zMapGFFCreateHeader() ;
void zMapGFFHeaderDestroy(ZMapGFFHeader const ) ;
void zMapGFFHeaderMinimalTest(const ZMapGFFHeader const ) ;


/*
 * Some data utilities
*/
int zMapGFFGetDirectiveIntData(const ZMapGFFHeader const, ZMapGFFDirectiveName, unsigned int) ;
char *zMapGFFGetDirectiveStringData(const ZMapGFFHeader const, ZMapGFFDirectiveName, unsigned int) ;



#endif

