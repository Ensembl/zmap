/*  File: zmapGFFAttribute.h
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
 * These macro definitions create the attribute names in the following form:
 *
 *     DEFINE_ATTRIBUTE(a, b)    =>    const char * a_b = "b" ;
 *
 * and are used to generate names such as
 *
 *   const char * sAttributeName_ensembl_variation = "ensembl_variation" ;
 *
 * and so on. I prefer not to (ab)use macros in this way, but this saves some
 * typing and reduces the possibility of errors. Also gives a consistent naming
 * convention.
 *
 */
#ifdef ATTRIBUTE_DEFINITIONS
#define DEFINE_ATTRIBUTE(a, b) const char * a##_##b = #b ;
#else
#define DEFINE_ATTRIBUTE(a, b) extern const char * a##_##b ;
#endif

DEFINE_ATTRIBUTE(sAttributeName, Class)
DEFINE_ATTRIBUTE(sAttributeName, percentID)
DEFINE_ATTRIBUTE(sAttributeName, Align)
DEFINE_ATTRIBUTE(sAttributeName, start_not_found)
DEFINE_ATTRIBUTE(sAttributeName, end_not_found)
DEFINE_ATTRIBUTE(sAttributeName, length)
DEFINE_ATTRIBUTE(sAttributeName, Name)
DEFINE_ATTRIBUTE(sAttributeName, Alias)
DEFINE_ATTRIBUTE(sAttributeName, Target)
DEFINE_ATTRIBUTE(sAttributeName, Dbxref)
DEFINE_ATTRIBUTE(sAttributeName, Ontology_term)
DEFINE_ATTRIBUTE(sAttributeName, Is_circular)
DEFINE_ATTRIBUTE(sAttributeName, Parent)
DEFINE_ATTRIBUTE(sAttributeName, url)
DEFINE_ATTRIBUTE(sAttributeName, ensembl_variation)
DEFINE_ATTRIBUTE(sAttributeName, allele_string)
DEFINE_ATTRIBUTE(sAttributeName, Note)
DEFINE_ATTRIBUTE(sAttributeName, Derives_from)
DEFINE_ATTRIBUTE(sAttributeName, ID)
DEFINE_ATTRIBUTE(sAttributeName, sequence)
DEFINE_ATTRIBUTE(sAttributeName, Source)
DEFINE_ATTRIBUTE(sAttributeName, Assembly_source)
DEFINE_ATTRIBUTE(sAttributeName, locus)
DEFINE_ATTRIBUTE(sAttributeName, gaps)
DEFINE_ATTRIBUTE(sAttributeName, Gap)
DEFINE_ATTRIBUTE(sAttributeName, cigar_exonerate)
DEFINE_ATTRIBUTE(sAttributeName, cigar_ensembl)
DEFINE_ATTRIBUTE(sAttributeName, cigar_bam)
DEFINE_ATTRIBUTE(sAttributeName, vulgar_exonerate)
DEFINE_ATTRIBUTE(sAttributeName, Known_name)
DEFINE_ATTRIBUTE(sAttributeName, assembly_path)
DEFINE_ATTRIBUTE(sAttributeName, read_pair_id)

#undef DEFINE_ATTRIBUTE



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
gboolean zMapGFFDestroyAttribute(ZMapGFFAttribute) ;

ZMapGFFAttributeName zMapGFFAttributeGetName(ZMapGFFAttribute) ;
char * zMapGFFAttributeGetNamestring(ZMapGFFAttribute ) ;
char * zMapGFFAttributeGetTempstring(ZMapGFFAttribute) ;
char * zMapGFFAttributeGetString(ZMapGFFAttributeName) ;

gboolean zMapGFFAttributeGetIsMultivalued(ZMapGFFAttributeName) ;
gboolean zMapGFFAttributeIsValid(ZMapGFFAttribute) ;

ZMapGFFAttribute zMapGFFAttributeParse(ZMapGFFParser, const char * const, gboolean) ;
gboolean zMapGFFAttributeRemoveQuotes(ZMapGFFAttribute) ;
ZMapGFFAttribute* zMapGFFAttributeParseList(ZMapGFFParser, const char * const, unsigned int * const, gboolean ) ;
gboolean zMapGFFAttributeDestroyList(ZMapGFFAttribute * , unsigned int) ;

ZMapGFFAttribute zMapGFFAttributeListContains(ZMapGFFAttribute * , unsigned int, const char * const) ;

void attribute_test_example() ;
void attribute_test_parse(ZMapGFFParser, char **, unsigned int) ;

/*
 * Attribute parsing functions. Similar to previous implementation, but broken up
 * into simpler components.
 */
gboolean zMapAttParseAlias(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseAlign(ZMapGFFAttribute, int * const, int * const, ZMapStrand * const);
gboolean zMapAttParseAssemblySource(ZMapGFFAttribute, char ** const, char ** const) ;
gboolean zMapAttParseAssemblyPath(ZMapGFFAttribute, char ** const, ZMapStrand * const , int * const, GArray ** const, char*) ;
gboolean zMapAttParseAnyTwoStrings(ZMapGFFAttribute, char ** const, char ** const) ;
gboolean zMapAttParseClass(ZMapGFFAttribute, ZMapHomolType *const );
gboolean zMapAttParseCigarExonerate(ZMapGFFAttribute , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCigarEnsembl(ZMapGFFAttribute , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCigarBam(ZMapGFFAttribute , GArray ** const , ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseCDSStartNotFound(ZMapGFFAttribute, gboolean * const, int * const ) ;
gboolean zMapAttParseCDSEndNotFound(ZMapGFFAttribute, gboolean * const ) ;
gboolean zMapAttParseDerives_from(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseDbxref(ZMapGFFAttribute, char ** const, char ** const ) ;
gboolean zMapAttParseGaps(ZMapGFFAttribute, GArray ** const, ZMapStrand, ZMapStrand ) ;
gboolean zMapAttParseID(ZMapGFFAttribute, GQuark * const ) ;
gboolean zMapAttParseIs_circular(ZMapGFFAttribute, gboolean * const ) ;
gboolean zMapAttParseKnownName(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseLocus(ZMapGFFAttribute, GQuark * const ) ;
gboolean zMapAttParseLength(ZMapGFFAttribute, int* const) ;
gboolean zMapAttParseName(ZMapGFFAttribute, char ** const) ;
gboolean zMapAttParseNameV2(ZMapGFFAttribute, GQuark *const, char ** const, char ** const) ;
gboolean zMapAttParseNote(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseOntology_term(ZMapGFFAttribute, char ** const, char ** const ) ;
gboolean zMapAttParseParent(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParsePID(ZMapGFFAttribute, double *const);
gboolean zMapAttParseSequence(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseSource(ZMapGFFAttribute, char ** const) ;
gboolean zMapAttParseTargetV2(ZMapGFFAttribute, char ** const, char ** const ) ;
gboolean zMapAttParseTarget(ZMapGFFAttribute, GQuark * const, int * const , int * const , ZMapStrand * const ) ;
gboolean zMapAttParseURL(ZMapGFFAttribute, char** const ) ;
gboolean zMapAttParseVulgarExonerate(ZMapGFFAttribute , GArray ** const, ZMapStrand , int, int, ZMapStrand, int, int) ;
gboolean zMapAttParseEnsemblVariation(ZMapGFFAttribute, char ** const ) ;
gboolean zMapAttParseAlleleString(ZMapGFFAttribute, char ** const) ;
gboolean zMapAttParseReadPairID(ZMapGFFAttribute, GQuark * const) ;



#endif
