/*  File: check_ZMapFeature.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
 *  Copyright (c) 2006-2010: Genome Research Ltd.
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
 *      Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *        Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk,
 *     Malcolm Hinsley (Sanger Institute, UK) mh17@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Mar 30 23:02 2009 (rds)
 * Created: Mon Mar 30 14:49:19 2009 (rds)
 * CVS info:   $Id: check_zmapFeature.c,v 1.3 2010-06-14 15:40:13 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <ZMap/zmap.h>






#include <check_zmapFeature.h>

static ZMapFeatureTypeStyle style_G = NULL;
static ZMapFeature feature_G = NULL;


START_TEST(check_zMapFeatureCreate)
{
  ZMapFeature feature = NULL;

  feature = zMapFeatureCreateEmpty();

  if(feature == NULL)
    {
      fail("Creating empty feature failed");
    }
  else
    {
      zMapFeatureDestroy(feature);
    }

  return ;
}
END_TEST


START_TEST(check_zMapFeatureCreateStandard)
{
  ZMapFeature feature = NULL;
  ZMapFeatureTypeStyle style = NULL;
  
  style = zMapStyleCreate("DummyStyle", "Dummy Style for Testing");

  feature = zMapFeatureCreateFromStandardData(ZMAP_CHECK_FEATURE_NAME,
					      ZMAP_CHECK_FEATURE_SEQUENCE,
					      ZMAP_CHECK_FEATURE_ONTOLOGY,
					      ZMAP_CHECK_FEATURE_MODE,
					      style,
					      ZMAP_CHECK_FEATURE_START,
					      ZMAP_CHECK_FEATURE_END,
					      ZMAP_CHECK_FEATURE_HASSCORE,
					      ZMAP_CHECK_FEATURE_SCORE,
					      ZMAP_CHECK_FEATURE_STRAND,
					      ZMAP_CHECK_FEATURE_PHASE);

  if(feature == NULL)
    {
      fail("Creating canned feature failed");
    }

  if(feature->x1 != ZMAP_CHECK_FEATURE_START)
    {
      fail("Start not matched with creation parameters");
    }

  if(feature->x2 != ZMAP_CHECK_FEATURE_END)
    {
      fail("End not matched with creation parameters");
    }
  
  if(feature->strand != ZMAP_CHECK_FEATURE_STRAND)
    {
      fail("Strand not matched with creation parameters");
    }

  if(feature->phase != ZMAP_CHECK_FEATURE_PHASE)
    {
      fail("Phase not matched with creation parameters");
    }

  if(style)
    zMapStyleDestroy(style);
  
  if(feature)
    zMapFeatureDestroy(feature);

  return ;
}
END_TEST


START_TEST(check_zmapFeatureProperties)
{
  GQuark locus;

  if(!feature_G){ fail("!"); }

  locus = g_quark_from_string("locus");

  if(!zMapFeatureAddLocus(feature_G, locus))
    {
      fail("Adding Locus Failed");
    }

  if(feature_G->locus_id != locus)
    {
      fail("Locus ID doesn't match");
    }

  return ;
}
END_TEST


START_TEST(check_zmapFeatureCopy)
{
  ZMapFeature feature;

  if(!feature_G){ fail("!"); }

  feature = (ZMapFeature)zMapFeatureAnyCopy((ZMapFeatureAny)feature_G);

  if(!feature)
    {
      fail("Copy failed");
    }

  if(feature->unique_id != feature_G->unique_id)
    {
      fail("Copied feature unique ids don't match");
    }
  
  if(feature->original_id != feature_G->original_id)
    {
      fail("Copied feature original ids don't match");	
    }

  if(feature->type != feature_G->type)
    {
      fail("Copied feature types don't match");
    }
  
  if(feature->ontology != feature_G->ontology)
    {
      fail("Copied feature ontologies don't match");
    }

  if(feature->style_id != feature_G->style_id)
    {
      fail("Copied style ids don't match");
    }

  if(feature->x1 != feature_G->x1 ||
     feature->x2 != feature_G->x2)
    {
      fail("Copied feature starts/ends don't match");
    }
  
  if(feature->strand != feature_G->strand ||
     feature->phase  != feature_G->phase)
    {
      fail("Copied feature strands/phases don't match");
    }

  return ;
}
END_TEST

static void setup_feature(void)
{
  style_G = zMapStyleCreate("Global-Style", "Global Style for Testing");

  feature_G = zMapFeatureCreateFromStandardData(ZMAP_CHECK_FEATURE_NAME,
						ZMAP_CHECK_FEATURE_SEQUENCE,
						ZMAP_CHECK_FEATURE_ONTOLOGY,
						ZMAP_CHECK_FEATURE_MODE,
						style_G,
						ZMAP_CHECK_FEATURE_START,
						ZMAP_CHECK_FEATURE_END,
						ZMAP_CHECK_FEATURE_HASSCORE,
						ZMAP_CHECK_FEATURE_SCORE,
						ZMAP_CHECK_FEATURE_STRAND,
						ZMAP_CHECK_FEATURE_PHASE);

  return ;
}

static void destroy_feature(void)
{
  zMapStyleDestroy(style_G);
  style_G = NULL;

  zMapFeatureDestroy(feature_G);
  feature_G = NULL;

  return ;
}


Suite *zMapCheckFeatureSuite(void)
{
  Suite *suite = NULL;
  TCase *test_case = NULL;
  
  suite = suite_create(ZMAP_CHECK_FEATURE_SUITE_NAME);

  test_case = tcase_create(ZMAP_CHECK_FEATURE_SUITE_NAME " Tests");

  tcase_add_test(test_case, check_zMapFeatureCreate);

  tcase_add_test(test_case, check_zMapFeatureCreateStandard);

  suite_add_tcase(suite, test_case);

  
  test_case = tcase_create(ZMAP_CHECK_FEATURE_SUITE_NAME " Detailed Tests");

  tcase_add_unchecked_fixture(test_case, setup_feature, destroy_feature);

  tcase_add_test(test_case, check_zmapFeatureProperties);

  tcase_add_test(test_case, check_zmapFeatureCopy);

  suite_add_tcase(suite, test_case);

  return suite;
}

