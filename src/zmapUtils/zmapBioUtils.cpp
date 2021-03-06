/*  File: zmapBioUtils.c
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
 * Description: Functions to do useful bioinformatics related stuff.
 *
 * Exported functions: See ZMap/zmapUtils.h
 *
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.hpp>

#include <string.h>
#include <glib.h>

#include <ZMap/zmapUtils.hpp>
#include <ZMap/zmapGLibUtils.hpp>





/* 
 *                            External routines
 */


/* Parses chromosome location strings of the following forms:
 * 
 * Ensembl page title string: "13: 32,889,611-32,973,805"
 * 
 * UCSC page location string: "chr21:33,031,597-33,041,570"
 * 
 * and a catch all string: "21:33,031,597-33,041,570"
 * 
 * (numbers will be accepted without ",")
 * 
 * The regex takes any of the above and produces these match strings:
 * 
 *             "21" "33,031,597" and "33,041,570"
 * 
 * Note that we anchor the regex at both ends to ensure we don't allow
 * leading or trailing rubbish.
 * 
 * Note also that the chromosome "number" can also be "X" or "Y" !!
 * 
 * Returns TRUE and the chromosome as a string and the start/end coords as ints
 * if string is parseable, FALSE otherwise, in which case chromosome_out etc
 * are not altered.
 * 
 * Chromosome is returned as string because it could be "X" or "Y" or some variant
 * which often have a suffix letter etc or may given as roman numerals.
 *  */
gboolean zMapUtilsBioParseChromLoc(char *location_str, char **chromosome_out, int *start_out, int *end_out)
{
  gboolean result = FALSE ;
  static const char *regex_str = "^(?:chr)?([\\d]+|X|Y):\\s*([\\d,]+)-([\\d,]+)$" ;
  static GRegex *regex = NULL ;
  GError *error = NULL ;
  gchar **sub_strings ;
  gchar **match ;
  char *chromosome_str = NULL, *start_str = NULL, *end_str = NULL ;

  zMapReturnValIfFail((location_str && *location_str), FALSE) ;


  /* Set up the regex's just the once. */
  if (!regex)
    {
      regex = g_regex_new(regex_str,
                          (GRegexCompileFlags)0,
                          (GRegexMatchFlags)0,
                          &error) ;
    }

  /* Do the split...note that if the regex fails we get our location string
   * back as the first match. Note also that this regex produces an initial
   * empty match then the three matches we want and then another empty match,
   * I don't know why..... */
  sub_strings = g_regex_split_full(regex, location_str, -1, 0, (GRegexMatchFlags)0, -1, &error) ;

  if (strcmp(*sub_strings, location_str) != 0)
    {
      match = sub_strings ;

      while (*match)
        {
          char *curr_match ;

          curr_match = *match ;

          if (curr_match && *curr_match)                    /* skip odd empty matches.... */
            {
              if (!chromosome_str)
                chromosome_str = curr_match ;
              else if (!start_str)
                start_str = curr_match ;
              else if (!end_str)
                end_str = curr_match ;
              else
                break ;                                     /* Ignore any other stuff. */
            }

          match++ ;
        }

      /* If we've got all parts then convert the numbers to ints. */
      if (chromosome_str && start_str && end_str)
        {
          int start, end ;

          /* Remove the "," from the numbers. */
          start_str = zMap_g_remove_char(start_str, ',') ;
          end_str = zMap_g_remove_char(end_str, ',') ;

          /* Check start/end can be converted to ints. */
          if (zMapStr2Int(start_str, &start) && zMapStr2Int(end_str, &end))
            {
              *chromosome_out = g_strdup(chromosome_str) ;
              *start_out = start ;
              *end_out = end ;

              result = TRUE ;
            }
        }
    }

  return result ;
}


/* If chromosome_str is of the form "chr6-18" returns "6" otherwise NULL.
 * 
 * Handles strings of the form:
 * 
 *     "chr6-18"
 *     "chr61-18"
 *     "chr6"
 *     "chrX-11"
 * 
 *  */
gboolean zMapUtilsBioParseChromNumber(char *chromosome_str, char **chromosome_out)
{
  gboolean result = FALSE ;
  static const char *regex_str = "^(?:^chr)([\\d]+|X|Y)(?:-[\\d,]+)?$" ;
  static GRegex *regex = NULL ;
  GError *error = NULL ;
  gchar **sub_strings ;
  gchar **match ;
  char *number_str = NULL ;

  zMapReturnValIfFail((chromosome_str && *chromosome_str && chromosome_out), FALSE) ;


  /* Set up the regex's just the once. */
  if (!regex)
    {
      regex = g_regex_new(regex_str,
                          (GRegexCompileFlags)0,
                          (GRegexMatchFlags)0,
                          &error) ;
    }

  /* Do the split...note that if the regex fails we get our location string
   * back as the first match. Note also that this regex produces an initial
   * empty match then the matche we want and then another empty match,
   * I don't know why..... */
  sub_strings = g_regex_split_full(regex, chromosome_str, -1, 0, (GRegexMatchFlags)0, -1, &error) ;

  if (strcmp(*sub_strings, chromosome_str) != 0)
    {
      match = sub_strings ;

      while (*match)
        {
          char *curr_match ;

          curr_match = *match ;

          if (curr_match && *curr_match)                    /* skip odd empty matches.... */
            {
              number_str = curr_match ;
            }

          match++ ;
        }

      /* Now return the chromosome "number". */
      if (number_str)
        {
          *chromosome_out = g_strdup(number_str) ;
          result = TRUE ;
        }
    }

  return result ;
}

