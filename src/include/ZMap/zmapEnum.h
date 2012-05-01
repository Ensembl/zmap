/*  File: zmapEnum.h
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: defines macros allowing a single string/enum definition
 *              to be used to produce enum types, print functions and 
 *              and more.
 *
 *-------------------------------------------------------------------
 */

#ifndef __ZMAP_ENUM_H__
#define __ZMAP_ENUM_H__

/* Altered code originally from
 * http://alioth.debian.org/snippet/detail.php?type=snippet&id=13 */

/* This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                          *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:         *
 *                          *
 * Free Software Foundation      Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330   Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org         *
 *                                                                  *
 *  Copyright  2004-2005  Neil Williams  <linux@codehelp.co.uk>
 *                                                                  *
 *                          */

/* in the main .h (so that it can be re-used). */

/* MACROS have been renamed slightly, addition of func_name arg, and
 * added struct enum_dummy to swallow the semi-colon. G_STMT_START/END
 * uses do{...}while(0) idiom, but that doesn't work here. */

#define ENUM_BODY(name, value, dummy0, dummy1, dummy2)	\
    name value,

#define AS_STRING_CASE(name, value, dummy0, dummy1, dummy2)		\
    case name: { return #name; }

#define FROM_STRING_CASE(name, value, dummy)	 \
    if (strcmp(str, #name) == 0) {       \
        return name;                     \
    }

#define AS_NAME_TEXT(name, dummy0, name_text, dummy1, dummy2) \
    case name: { return name_text; }

#define AS_SHORT_TEXT(name, value, dummy0, short_text, dummy2)		\
    case name: { return short_text; }


#define ENUM2STR(name, dummy, string, dummy0, dummy1)			\
  {name, string},

#define SWALLOW_SEMI_COLON struct enum_dummy



/* Only the ZMAP_xxxxx Functions swallow the semi-colon! */

/* 
 * Define enum type automatically.
 */
#define ZMAP_DEFINE_ENUM(name, list)     \
    typedef enum {                       \
        list(ENUM_BODY)                  \
    } name;                              \
SWALLOW_SEMI_COLON



/* 
 * Defines enum to exact string convertor function automatically.
 */
#define ZMAP_ENUM_AS_EXACT_STRING_DEC(fname, name)   \
    const char* fname(name n);                 \
SWALLOW_SEMI_COLON

#define ZMAP_ENUM_AS_EXACT_STRING_FUNC(fname, name, list) \
    const char* fname(name n) {                \
        switch (n) {                           \
            list(AS_STRING_CASE)               \
            default: return "";                \
        }                                      \
    }                                          \
SWALLOW_SEMI_COLON


/* 
 * Defines enum to name string converter function automatically.
 * name as in style of g_param_spec_TYPE(name, short, blurb, ...)
 */
#define ZMAP_ENUM_AS_NAME_STRING_DEC(fname, type_name) \
 const char *fname(type_name n);                       \
SWALLOW_SEMI_COLON

#define ZMAP_ENUM_AS_NAME_STRING_FUNC(fname, type_name, list) \
const char *fname(type_name n){                               \
  switch(n) {                                                 \
    list(AS_NAME_TEXT)                                        \
    default: return "";                                       \
  }                                                           \
}                                                             \
SWALLOW_SEMI_COLON

/* 
 * Defines enum to short description convertor automatically.
 */
#define ZMAP_ENUM_TO_SHORT_TEXT_DEC(fname, name)   \
    const char* fname(name n);                 \
SWALLOW_SEMI_COLON

#define ZMAP_ENUM_TO_SHORT_TEXT_FUNC(fname, name, list) \
    const char* fname(name n) {                \
        switch (n) {                           \
            list(AS_SHORT_TEXT)               \
            default: return "";                \
        }                                      \
    }                                          \
SWALLOW_SEMI_COLON


/* 
 * Defines exact string to enum convertor function automatically.
 */
#define ZMAP_ENUM_FROM_EXACT_STRING_DEC(fname, name) \
    name fname(const char* str);               \
SWALLOW_SEMI_COLON

#define ZMAP_ENUM_FROM_EXACT_STRING_FUNC(fname, name, list) \
    name fname(const char* str) {              \
        list(FROM_STRING_CASE)                 \
        return 0;                              \
    }                                          \
SWALLOW_SEMI_COLON



/* 
 * Defines given string to enum convertor function automatically.
 */
#define ZMAP_ENUM_FROM_STRING_DEC(FUNCNAME, TYPE) \
    TYPE FUNCNAME(const char* str) ;              \
SWALLOW_SEMI_COLON


#define ZMAP_ENUM_FROM_STRING_FUNC(fname, TYPE, INVALID_VALUE, list, dummy0, dummy1) \
  TYPE fname(const char* str)                  \
  {					       \
  typedef struct {TYPE enum_value ; char *string ;} TYPE##Enum2StrStruct ;		\
                                                     \
    TYPE result = INVALID_VALUE ;              \
    TYPE##Enum2StrStruct values[] =                   \
      {                                        \
	list(ENUM2STR)                         \
	{INVALID_VALUE, NULL}                  \
      } ;				       \
    TYPE##Enum2StrStruct *curr = values ;                   \
                                                     \
    while(curr->string)                        \
      {                                       \
	if (g_ascii_strcasecmp(str, curr->string) == 0) \
	  {                                        \
	    result = curr->enum_value ;            \
	    break ;                               \
	  }                                        \
                                                     \
	curr++ ;                               \
      }                                        \
                                                     \
      return result ;		       \
  }					       \
SWALLOW_SEMI_COLON



#ifdef REQUIRE_NON_TYPEDEF_ENUM_FUNCS

/* Additionally gnc_engine_util.h defines these Non typedef enum
 * versions. Again edited slightly... */

/* These are unused and untested. RDS. */

#define FROM_STRING_CASE_NON_TYPEDEF(name, value)    \
  if (strcmp(str, #name) == 0) { *type = name; }


#define ZMAP_DEFINE_ENUM_NON_TYPEDEF(name, list)     \
    enum {                               \
        list(ENUM_BODY)                  \
    } name;                              \
SWALLOW_SEMI_COLON 
 
#define ZMAP_ENUM_AS_STRING_NON_TYPEDEF_DEC(fname, name)  \
    const char* fname(enum name n);                       \
SWALLOW_SEMI_COLON
 
#define ZMAP_ENUM_AS_STRING_NON_TYPEDEF_FUNC(fname, name, list) \
    const char* fname(enum name n) {                            \
         switch (n)                                             \
           {                                                    \
             list(AS_STRING_CASE)                               \
            default: { return ""; }                             \
           }                                                    \
    }                                                           \
SWALLOW_SEMI_COLON

#define ZMAP_ENUM_FROM_STRING_NON_TYPEDEF_DEC(fname, name) \
   void fname(const char* str, enum name *type);           \
SWALLOW_SEMI_COLON
 
#define ZMAP_ENUM_FROM_STRING_NON_TYPEDEF_FUNC(fname, name, list) \
   void fname(const char* str, enum name *type) {                 \
   if(!type)       { return ; }                                   \
   if(str == NULL) { return ; }                                   \
     list(FROM_STRING_CASE_NON_TYPEDEF)                           \
}                                                                 \
SWALLOW_SEMI_COLON

#endif /* REQUIRE_NON_TYPEDEF_ENUM_FUNCS */


#ifdef __EXAMPLE_USAGE__


#define ZMAP_STYLE_MODAL_LIST(_) \
_(INVALID, = 0)                  \
_(MOODY,)                        \
_(BORED,)                        \
_(TIRED, = 101)

ZMAP_DEFINE_ENUM(ZMapStyleModal, ZMAP_STYLE_MODAL_LIST);

ZMAP_ENUM_AS_STRING_DEC(ZMapStyleModal);
ZMAP_ENUM_FROM_STRING_DEC(ZMapStyleModal);



/* in the individual .h, assign the values and use the DEC declaration macro. */
/* Can be repeated for a different enum if you just change the name 
e.g. ENUM_LIST_B - maintain that name through the other macros.
*/
#define ENUM_LIST_TYPE(_) \
        _(INVALID,) \
        _(ONCE,) \
        _(DAILY,) \
        _(WEEKLY,) /**< Hmmm... This is sort of DAILY[7]... */ \
        _(MONTHLY,) \
        _(MONTH_RELATIVE,) \
        _(COMPOSITE,)

DEFINE_ENUM(FreqType, ENUM_LIST_TYPE) /**< \enum Frequency specification.

For BI_WEEKLY, use weekly[2] 
 SEMI_MONTHLY, use composite 
 YEARLY, monthly[12] */

     AS_STRING_DEC(FreqType, ENUM_LIST_TYPE)
     FROM_STRING_DEC(FreqType, ENUM_LIST_TYPE)

     /* in the .c use the FUNC - just the two lines. */
     FROM_STRING_FUNC(FreqType, ENUM_LIST_TYPE)
     AS_STRING_FUNC(FreqType, ENUM_LIST_TYPE)

     /* now call the asString function with the name as prefix: */
     /*  g_strdup(FreqTypeasString(value));        */
     /*  int = FreqTypefromString(type_string));   */

     /* this example from GnuCash gnome2 branch, FreqSpec.[c|h] and gnc-engine-util.h */
     /* setting the enum value isn't shown above, it can be done using:
#define ENUM_LIST(_)                               \
    _(RED,   = 0)                                  \
    _(GREEN, = 1)                                  \
    _(BLUE,  = 87)
     */

     /* the _() syntax does NOT mean the strings are translated (why would anyone want to do that?) - it is purely for the macro and the _() does not survive into the pre-processed C code. */

     /* more information?
https://lists.gnucash.org/pipermail/gnucash-devel/2005-March/012849.html
     */

#endif /* __EXAMPLE_USAGE__ */



#endif /* __ZMAP_ENUM_H__ */
