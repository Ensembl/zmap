/*  File: zmapGFFAttribute.h
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
 * Description: Header file for attribute data structure and
 * associated functions. This is used for representing GFF v3
 * (or v2 if required) attributes, and contains associated
 * parsing functions.
 *
 *-------------------------------------------------------------------
 */




#ifndef ZMAP_GFFATTRIBUTE_H
#define ZMAP_GFFATTRIBUTE_H

#include <string.h>
#include <ctype.h>
#include <ZMap/zmapGFF.h>
#include "zmapGFF3_P.h"

/*
 * Names of attributes.
 */
#ifdef ATTRIBUTE_DEFINITIONS
const char * sAttributeName_Class                     = "Class" ;
const char * sAttributeName_percentID                 = "percentID" ;
const char * sAttributeName_Align                     = "Align" ;
const char * sAttributeName_start_not_found           = "start_not_found" ;
const char * sAttributeName_end_not_found             = "end_not_found" ;
const char * sAttributeName_length                    = "length" ;
const char * sAttributeName_Name                      = "Name" ;
const char * sAttributeName_Alias                     = "Alias" ;
const char * sAttributeName_Target                    = "Target" ;
const char * sAttributeName_Dbxref                    = "Dbxref" ;
const char * sAttributeName_Ontology_term             = "Ontology_term" ;
const char * sAttributeName_Is_circular               = "Is_circular" ;
const char * sAttributeName_Parent                    = "Parent" ;
const char * sAttributeName_url                       = "url" ;
const char * sAttributeName_ensembl_variation         = "ensembl_variation" ;
const char * sAttributeName_allele_string             = "allele_string" ;
const char * sAttributeName_Note                      = "Note" ;
const char * sAttributeName_Derives_from              = "Derives_from" ;
const char * sAttributeName_ID                        = "ID" ;
const char * sAttributeName_sequence                  = "sequence" ;
const char * sAttributeName_Source                    = "Source" ;
const char * sAttributeName_Assembly_source           = "Assembly_source" ;
const char * sAttributeName_locus                     = "locus";
const char * sAttributeName_Gaps                      = "Gaps" ;
const char * sAttributeName_Gap                       = "Gap" ;
const char * sAttributeName_cigar_exonerate           = "cigar_exonerate" ;
const char * sAttributeName_cigar_ensembl             = "cigar_ensembl" ;
const char * sAttributeName_cigar_bam                 = "cigar_bam" ;
const char * sAttributeName_vulgar_exonerate          = "vulgar_exonerate" ;
const char * sAttributeName_Known_name                = "Known_name" ;
const char * sAttributeName_assembly_path             = "assembly_path" ;
#else
extern const char * sAttributeName_Class ;
extern const char * sAttributeName_percentID ;
extern const char * sAttributeName_Align ;
extern const char * sAttributeName_start_not_found ;
extern const char * sAttributeName_end_not_found ;
extern const char * sAttributeName_length ;
extern const char * sAttributeName_Name ;
extern const char * sAttributeName_Alias ;
extern const char * sAttributeName_Target ;
extern const char * sAttributeName_Dbxref ;
extern const char * sAttributeName_Ontology_term ;
extern const char * sAttributeName_Is_circular ;
extern const char * sAttributeName_Parent ;
extern const char * sAttributeName_url;
extern const char * sAttributeName_ensembl_variation;
extern const char * sAttributeName_allele_string ;
extern const char * sAttributeName_Note ;
extern const char * sAttributeName_Derives_from ;
extern const char * sAttributeName_ID ;
extern const char * sAttributeName_sequence ;
extern const char * sAttributeName_Source ;
extern const char * sAttributeName_Assembly_source ;
extern const char * sAttributeName_locus ;
extern const char * sAttributeName_Gaps ;
extern const char * sAttributeName_Gap ;
extern const char * sAttributeName_cigar_exonerate ;
extern const char * sAttributeName_cigar_ensembl ;
extern const char * sAttributeName_cigar_bam ;
extern const char * sAttributeName_vulgar_exonerate ;
extern const char * sAttributeName_Known_name ;
extern const char * sAttributeName_assembly_path ;
#endif

/*
 * Structure to store info about attributes. Data are:
 *
 * eName                name from a predefined list (if identified as such)
 * sName                string of the name as found in the data file
 * n*Data               number of data items of given type associated for this attribute
 * nIsMultivalued       can the attribute be multivalued?
 *
 */
typedef struct ZMapGFFAttributeInfoStruct_
  {
    ZMapGFFAttributeName eName ;
    char *sName ;
    gboolean bIsMultivalued ;
  } ZMapGFFAttributeInfoStruct ;


/*
 * Structure for attribute data. Data are:
 *
 * eName                name from a predefined list (if identified as such)
 * sName                string of the name as found in the data file
 * sTemp                data string found with attribute
 * p*Data               data of various types associated with attribute
 *
 */
typedef struct ZMapGFFAttributeStruct_
  {
    ZMapGFFAttributeName eName ;
    char *sName ;
    char *sTemp ;
  } ZMapGFFAttributeStruct ;


ZMapGFFAttribute zMapGFFCreateAttribute(ZMapGFFAttributeName) ;
gboolean zMapGFFDestroyAttribute(ZMapGFFAttribute const) ;

ZMapGFFAttributeName zMapGFFAttributeGetName(const ZMapGFFAttribute const) ;
char * zMapGFFAttributeGetNamestring(const ZMapGFFAttribute const ) ;
char * zMapGFFAttributeGetTempstring(const ZMapGFFAttribute const ) ;
char * zMapGFFAttributeGetString(ZMapGFFAttributeName) ;

gboolean zMapGFFAttributeGetIsMultivalued(ZMapGFFAttributeName) ;
gboolean zMapGFFAttributeIsValid(const ZMapGFFAttribute const ) ;

ZMapGFFAttribute zMapGFFAttributeParse(const ZMapGFFParser const , const char * const, gboolean) ;
gboolean zMapGFFAttributeRemoveQuotes(const ZMapGFFAttribute const) ;
ZMapGFFAttribute* zMapGFFAttributeParseList(const ZMapGFFParser const , const char * const, unsigned int * const, gboolean ) ;
gboolean zMapGFFAttributeDestroyList(ZMapGFFAttribute * const , unsigned int) ;

ZMapGFFAttribute zMapGFFAttributeListContains(const ZMapGFFAttribute *const , unsigned int, const char * const) ;

void attribute_test_example() ;
void attribute_test_parse(ZMapGFFParser, char **, unsigned int) ;

/*
 * Attribute parsing functions. Similar to previous implementation, but broken up
 * into simpler components.
 */
gboolean zMapAttParseAlias(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseAlign(const ZMapGFFAttribute const, int * const, int * const, ZMapStrand * const);
gboolean zMapAttParseAssemblySource(const ZMapGFFAttribute const, char ** const, char ** const) ;
gboolean zMapAttParseAssemblyPath(const ZMapGFFAttribute const, char ** const, ZMapStrand * const , int * const, GArray ** const) ;
gboolean zMapAttParseAnyTwoStrings(const ZMapGFFAttribute const, char ** const, char ** const) ;
gboolean zMapAttParseClass(const ZMapGFFAttribute const, ZMapHomolType *const );
gboolean zMapAttParseCigarExonerate(const ZMapGFFAttribute const , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCigarEnsembl(const ZMapGFFAttribute const , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCigarBam(const ZMapGFFAttribute const , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCDSStartNotFound(const ZMapGFFAttribute const, gboolean * const, int * const ) ;
gboolean zMapAttParseCDSEndNotFound(const ZMapGFFAttribute const, gboolean * const ) ;
gboolean zMapAttParseDerives_from(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseDbxref(const ZMapGFFAttribute const, char ** const, char ** const ) ;
gboolean zMapAttParseGaps(const ZMapGFFAttribute const, GArray ** const, ZMapStrand, ZMapStrand ) ;
gboolean zMapAttParseID(const ZMapGFFAttribute const, GQuark * const ) ;
gboolean zMapAttParseIs_circular(const ZMapGFFAttribute const, gboolean * const ) ;
gboolean zMapAttParseKnownName(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseLocus(const ZMapGFFAttribute const, GQuark * const ) ;
gboolean zMapAttParseLength(const ZMapGFFAttribute const, int* const) ;
gboolean zMapAttParseName(const ZMapGFFAttribute const, char ** const) ;
gboolean zMapAttParseNameV2(const ZMapGFFAttribute const, GQuark *const, char ** const, char ** const) ;
gboolean zMapAttParseNote(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseOntology_term(const ZMapGFFAttribute const, char ** const, char ** const ) ;
gboolean zMapAttParseParent(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParsePID(const ZMapGFFAttribute const, double *const);
gboolean zMapAttParseSequence(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseSource(const ZMapGFFAttribute const, char ** const) ;
gboolean zMapAttParseTargetV2(const ZMapGFFAttribute const, char ** const, char ** const ) ;
gboolean zMapAttParseTarget(const ZMapGFFAttribute const, char ** const, int * const , int * const , ZMapStrand * const ) ;
gboolean zMapAttParseURL(const ZMapGFFAttribute const, char** const ) ;
gboolean zMapAttParseVulgarExonerate(const ZMapGFFAttribute const , GArray ** const, ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseEnsemblVariation(const ZMapGFFAttribute const, char ** const ) ;
gboolean zMapAttParseAlleleString(const ZMapGFFAttribute const, char ** const) ;



#endif
