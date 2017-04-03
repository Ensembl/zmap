/*  File: zmapDNA.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2006-2017: Genome Research Ltd.
 *-------------------------------------------------------------------
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 * 
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *   Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *       Gemma Guest (Sanger Institute, UK) gb10@sanger.ac.uk
 *      Steve Miller (Sanger Institute, UK) sm23@sanger.ac.uk
 *  
 * Description: DNA manipulation functions.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DNA_H
#define ZMAP_DNA_H

#include <glib.h>
#include <ZMap/zmapFeature.hpp>

/*

We use the standard UPAC coding for representing DNA with a single character
per base:

Exactly known bases

A
T  (or U for RNA)
G
C

Double ambiguity

R     AG    ~Y          puRine
Y     CT    ~R          pYrimidine
M     AC    ~K          aMino
K     GT    ~M          Keto
S     CG    ~S          Strong
W     AT    ~w          Weak

Triple ambiguity

H     AGT   ~D          not G
B     CGT   ~V          not A
V     ACG   ~B          not T
D     AGT   ~H          not C

Total ambiguity

N     ACGT  ~N          unkNown

Possible existence of a base

-       0       padding

Note that:
We do NOT use U internally, but just use it for display if tag RNA is set

NCBI sometimes uses X in place of N, but we only use X for peptides

The - padding character is used when no sequencing is available and
therefore means that the exact length is unknown, 300 - might not mean
exactly 300 unkown bases.

In memory, we use one byte per base, the lower 4 bits correspond to
the 4 bases A_, T_, G_, C_. More than one bit is set in the ambiguous cases.
The - padding is represented as zero. In some parts of the code, the upper
4 bits are used as flags.

On disk, we store the dna as 4 bases per byte, if there is no ambiguity, or
2 bases per byte otherwise.
*/


#define A_ 1
#define T_ 2
#define U_ 2
#define G_ 4
#define C_ 8

#define R_ (A_ | G_)
#define Y_ (T_ | C_)
#define M_ (A_ | C_)
#define K_ (G_ | T_)
#define S_ (C_ | G_)
#define W_ (A_ | T_)

#define H_ (A_ | C_ | T_)
#define B_ (G_ | C_ | T_)
#define V_ (G_ | A_ | C_)
#define D_ (G_ | A_ | T_)

#define N_ (A_ | T_ | G_ | C_)
#define X_ N_     // "NCBI sometimes uses X in place of N"  (X should really be for peptides)


void zMapDNAEncodeString(char *cp) ;
gboolean zMapDNACanonical(char *dna) ;
gboolean zMapDNAValidate(char *dna) ;
gboolean zMapDNAFindMatch(char *cp, char *end, char *tp, int maxError, int maxN,
			  char **start_out, char **end_out, char **match_str) ;
GList *zMapDNAFindAllMatches(char *dna, char *query, ZMapStrand strand, int from, int length,
			     int max_errors, int max_Ns, gboolean return_matches) ;
void zMapDNAReverseComplement(char *sequence, int length) ;

#endif /* ZMAP_DNA_H */
