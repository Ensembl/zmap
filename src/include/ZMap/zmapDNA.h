/*  File: zmapDNA.h
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
 * originated by
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: DNA manipulation functions.
 *
 *-------------------------------------------------------------------
 */
#ifndef ZMAP_DNA_H
#define ZMAP_DNA_H

#include <glib.h>
#include <ZMap/zmapFeature.h>

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
