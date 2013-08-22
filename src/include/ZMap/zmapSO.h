/*  File: zmapSO.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: Functions, structs etc. for a Sequence Ontology (SO)
 *              package.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_SO_H
#define ZMAP_SO_H

#include <ZMap/zmapFeature.h>


/* Calling functions will need to know SO accession strings so put them here in one place. */
#define SO_ACC_CNV           "SO:0001019"
#define SO_ACC_DELETION      "SO:0000159"
#define SO_ACC_INSERTION     "SO:0000667"
#define SO_ACC_POLYA_JUNC    "SO:0001430"
#define SO_ACC_POLYA_SEQ     "SO:0000610"
#define SO_ACC_POLYA_SIGNAL  "SO:0000551"
#define SO_ACC_POLYA_SITE    "SO:0000553"
#define SO_ACC_SEQ_ALT       "SO:0001059"
#define SO_ACC_SNP           "SO:0000694"
#define SO_ACC_MUTATION      "SO:0000109"
#define SO_ACC_SUBSTITUTION  "SO:1000002"
#define SO_ACC_TRANSCRIPT    "SO:0000673"




char *zMapSOAcc2Term(GQuark SO_acc) ;
GQuark zMapSOAcc2TermID(GQuark SO_accession) ;
GQuark zMapSOVariation2SO(char *variation_str) ;
GQuark zMapSOFeature2SO(ZMapFeature feature) ;

#endif /* ZMAP_SO_H */
