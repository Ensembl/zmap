/*  File: check_zmapConfigLoader.c
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
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr  3 16:30 2009 (rds)
 * Created: Fri Apr  3 10:47:20 2009 (rds)
 * CVS info:   $Id: check_zmapConfigLoader.c,v 1.2 2010-03-04 15:10:56 mh17 Exp $
 *-------------------------------------------------------------------
 */

#include <check_zmapConfigLoader.h>


START_TEST(check_zMapConfigIniContextProvide)
{
  ZMapConfigIniContext context = NULL;

  context = zMapConfigIniContextProvide();

  if(!context)
    {
      fail("Failed getting context");
    }

  return ;
}
END_TEST


START_TEST(check_zMapConfigIniContextGetSources)
{
  ZMapConfigIniContext context = NULL;
  GList *sources = NULL;

  context = zMapConfigIniContextProvide();

  sources = zMapConfigIniContextGetSources(context);

  if(!sources)
    {
      fail("No sources found");
    }

  if(g_list_length(sources) != 2)
    {
      fail("Expected 2 sources");
    }

  return ;
}
END_TEST

START_TEST(check_zMapConfigIniContextGetString)
{
  ZMapConfigIniContext context = NULL;
  char *value = NULL;

  context = zMapConfigIniContextProvide();

  if(!zMapConfigIniContextGetString(context, 
				    ZMAPSTANZA_APP_CONFIG, ZMAPSTANZA_APP_CONFIG,
				    ZMAPSTANZA_APP_SEQUENCE, &value))
    {
      fail("Unable to find key '" ZMAPSTANZA_APP_SEQUENCE "' in stanza '" ZMAPSTANZA_APP_CONFIG "'");
    }

  

  return ;
}
END_TEST



static void setup_config(void)
{

  if(!zMapConfigDirCreate(CONFIG_DIR, CONFIG_FILE))
    {
      g_print("Using directory '" CONFIG_DIR "' and file '" CONFIG_FILE "'.\n");
      fail("Attempt to set config dir and file failed");
    }

  return ;
}

static void destroy_config(void)
{
  return ;
}


Suite *zMapCheckConfigLoader(void)
{
  Suite *suite = NULL;
  TCase *test_case = NULL;

  suite     = suite_create(ZMAP_CHECK_CONFIG_SUITE_NAME);

  test_case = tcase_create(ZMAP_CHECK_CONFIG_SUITE_NAME " Tests");

  tcase_add_unchecked_fixture(test_case, setup_config, destroy_config);

  tcase_add_test(test_case, check_zMapConfigIniContextProvide);

  tcase_add_test(test_case, check_zMapConfigIniContextGetSources);

  tcase_add_test(test_case, check_zMapConfigIniContextGetString);

  suite_add_tcase(suite, test_case);

  return suite;
}

