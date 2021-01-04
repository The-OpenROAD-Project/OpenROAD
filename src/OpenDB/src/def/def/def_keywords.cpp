// *****************************************************************************
// *****************************************************************************
// Copyright 2013 - 2016, Cadence Design Systems
// 
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8. 
// 
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
// 
//        http://www.apache.org/licenses/LICENSE-2.0
// 
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
// 
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
// 
//  $Author: icftcm $
//  $Revision: #2 $
//  $Date: 2017/08/28 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

/*                                                                     */
/*   Revision:                                                         */
/*   03-15-2000 Wanda da Rosa - Add code to support 5.4, add keywords  */
/*                              for PINS + USE, SPECIALNETS + SHAPE    */
/*                              and other keywords                     */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "lex.h"
#include "defiDefs.hpp"
#include "defiDebug.hpp"
#include "defrCallBacks.hpp"
#include "defrData.hpp"
#include "defrSettings.hpp"

#ifdef WIN32
#   include <direct.h>
#else /* not WIN32 */
#   include <unistd.h>
#endif /* WIN32 */


using namespace std;

BEGIN_LEFDEF_PARSER_NAMESPACE

#include "def.tab.h"


int defrData::defGetKeyword(const char* name, int *result) 
{ 
    map<const char*, int, defCompareCStrings>::const_iterator search = settings->Keyword_set.find(name);

    if ( search != settings->Keyword_set.end()) {
        *result = search->second;
        return TRUE;
    }

    return FALSE;
}

int 
defrData::defGetAlias(const string &name, string &result) 
{ 
    map<string, string, defCompareStrings>::iterator search = def_alias_set.find(name);

    if ( search != def_alias_set.end()) {
        result = search->second;
        return TRUE;
    }

    return FALSE;
}

int defrData::defGetDefine(const string &name, string &result) 
{ 
    map<string, string, defCompareStrings>::iterator search = def_defines_set.find(name);

    if ( search != def_defines_set.end()) {
        result = search->second;
        return TRUE;
    }

    return FALSE;
}

// lex.cpph starts here
/* User defined if log file should be in append from the previous run */
/* User defined if property string value should be process */
/* Varible from lex.cpph to keep track of invalid nonEnglish character */
/* in def file */

/************Some simple file reading routines since ungetc() proves ****/
/************to be quite slow, and we don't need multiple chars of pushback */
#ifndef WIN32
#   include <unistd.h>
#endif

void 
defrData::reload_buffer() {
   int nb = 0;

   if (first_buffer) {
      first_buffer = 0;
      if (settings->ReadFunction) {
         if ((nb = (*settings->ReadFunction)(File, buffer, 4)) != 4) {
            next = NULL;
            return;
         }
      } else {
         if ((nb = fread(buffer, 1, 4, File)) != 4) {
            next = NULL;
            return;
         }
      }
   }

   if (nb == 0) {
      if (settings->ReadFunction)
         nb = (*settings->ReadFunction)(File, buffer, IN_BUF_SIZE);
      else
         /* This is a normal file so just read some bytes. */
         nb = fread(buffer, 1, IN_BUF_SIZE, File);
   }
    
   if (nb <= 0) {
      next = NULL;
   } else {
      next = buffer;
      last = buffer + nb - 1;
   }
}   

int 
 defrData::GETC() {
   // Remove '\r' symbols from Windows streams.
    for(;;) {
       if (next > last)
          reload_buffer();
       if(next == NULL)
          return EOF;

       int ch = *next++;

       if (ch != '\r')
           return ch;
    }
}

void 
defrData::UNGETC(char ch) {
    if (next <= buffer) {
        defError(6111, "UNGETC: buffer access violation.");
    } else {
        *(--next) = ch;
    }
}

/* Return a copy of the string allocated from the ring buffer.
 * We will keep several strings in the buffer and just reuse them.
 * This could cause problems if we need to use more strings than we
 * have in the buffer.
 */
char* 
defrData::ringCopy(const char* string) 
{
   int len = strlen(string) + 1;
   if (++(ringPlace) >= RING_SIZE) 
       ringPlace = 0;
   if (len > ringSizes[ringPlace]) {
      free(ring[ringPlace]);
      ring[ringPlace] = (char*)malloc(len);
      ringSizes[ringPlace] = len;
   }
   strcpy(ring[ringPlace], string);
   return ring[ringPlace];
}


int 
defrData::DefGetTokenFromStack(char *s) {
   const char *ch;        /* utility variable */
   char *prS = NULL;          /* pointing to the previous char or s */

   while(input_level >= 0) {
      for(ch=stack[input_level].c_str(); *ch != 0; ch++)    /* skip white space */
         if (*ch != ' ' && *ch != '\t' && (nl_token || *ch != '\n'))
            break;
      /* did we find anything?  If not, decrement level and try again */
      if (*ch == 0)
         input_level--;
      else if (*ch == '\n') {
         *s++ = *ch; 
         *s = 0;
         return TRUE;
      }
      else {            /* we found something */
        for(; ; ch++)  {
           if (*ch == ' ' || *ch == '\t' || *ch == '\n' || *ch == 0) {
              /* 10/10/2000 - Wanda da Rosa, pcr 341032
              ** Take out the last '"', the 1st will be skip later
              */
              if (*prS == '"') {
                 *prS = '\0';
              } else
                 *s++ = '\0';
              stack[input_level] = ch;

              return TRUE;
           }
           /* 10/10/2000 - Wanda da Rosa, pcr 341032
           ** Save the location of the previous s
           */
           prS = s;
           *s++ = *ch;
        }
     }
   }
   return FALSE;    /* if we get here, we ran out of input levels */
}

 void 
defrData::print_lines(long long lines) 
{
    if (lines % settings->defiDeltaNumberLines) {
        return;
    }

    if (settings->ContextLineNumberFunction) {
        settings->ContextLineNumberFunction(session->UserData, (int)lines);
    } else if (settings->ContextLongLineNumberFunction ) {
        settings->ContextLongLineNumberFunction(session->UserData, lines);
    }
    if (settings->LineNumberFunction) {
        settings->LineNumberFunction((int)lines);
    } else if (settings->LongLineNumberFunction ) {
        settings->LongLineNumberFunction(lines);
    }
}

const char * 
defrData::lines2str(long long lines) 
{

#ifdef _WIN32
    sprintf(lineBuffer, "%I64d", lines);
#else
    sprintf(lineBuffer, "%lld", lines);
#endif 

    return lineBuffer;
}


// Increment current position of buffer pointer. 
// Double buffer size if curPos is out of boundary.
void  
defrData::IncCurPos(char **curPos, char **buffer, int *bufferSize)
{
    (*curPos)++;
    if (*curPos - *buffer < *bufferSize) {
        return;
    }

    long offset = *curPos - *buffer;

    *bufferSize *= 2;
    *buffer = (char*) realloc(*buffer, *bufferSize);
    *curPos = *buffer + offset;
}


int
defrData::DefGetToken(char **buf, int *bufferSize)
{
    char *s = *buf;
    int ch;

    ntokens++;
    defInvalidChar = 0;

    if (input_level >= 0){  /* if we are expanding an alias */
       if(DefGetTokenFromStack(s)) /* try to get a token from it */
          return TRUE;               /* if we get one, return it */
    }                                /* but if not, continue */

    /* skip blanks and count lines */
    while ((ch = GETC()) != EOF) {
       if (ch == '\n') {
          print_lines(++nlines);
       }
       if (ch != ' ' && ch != '\t' && (nl_token || ch != '\n'))
          break;
    }

    if (ch == EOF) 
        return FALSE;


    if (ch == '\n') {
       *s = ch; 
       IncCurPos(&s, buf, bufferSize);

       *s = '\0';
       return TRUE;
    }

    /* now get the token */
    if (ch == '"') {
       do {
          /* 5/5/2008 - CCR 556818
          ** Check if the ch is a valid ascii character 0 =< ch < 128
          ** If not write out an error
          */
          /* 8/7/2008 - CCR 586175
          ** Some files may not end with \n or \0 or EOF as the last character
          ** The parser allows this char instead of error out
          */
          if ((ch < -1) || (ch > 127)) {
             defInvalidChar = 1;
          }

          /* 8/22/2000 - Wanda da Rosa, pcr 333334
          ** save the previous char to allow backslash quote within quote
          */
          if (!settings->DisPropStrProcess) {
             /* 3/4/2008 - CCR 523879 - convert \\ to \, \" to ", \x to x */
             if (ch == '\\') {      /* got a \, save the next char only */
                ch = GETC();
                if ((ch == '\n') || (ch == EOF)) { /* senaty check */
                    *s = '\0';
                    return FALSE;
                }
             }
          }

          *s = ch;
          IncCurPos(&s, buf, bufferSize);

          ch = GETC();

          if (ch == EOF) {
             *s = '\0';
             return FALSE;
          }
       } while (ch != '"');

       *s = '\0';
       return TRUE;
    }

    if (names_case_sensitive) {
       for(; ; ch = GETC())  {

          /* 5/5/2008 - CCR 556818
          ** Check if the ch is a valid ascii character 0 =< ch < 128
          ** If not write out an error
          */
          if ((ch < -1) || (ch > 127)) {
             defInvalidChar = 1;
          }

          if (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
             break;

          *s = ch;
          IncCurPos(&s, buf, bufferSize);        
       }
    }
    else { /* we are case insensitive, use a different loop */
       for(; ; ch = GETC())  {

          /* 5/5/2008 - CCR 556818
          ** Check if the ch is a valid ascii character 0 =< ch < 128
          ** If not write out an error
          */
          if ((ch < -1) || (ch > 127)) {
             defInvalidChar = 1;
          }

          if (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
             break;

          *s = (ch >= 'a' && ch <= 'z')? (ch -'a' + 'A') : ch;            
          IncCurPos(&s, buf, bufferSize);
       }
    }
   
    /* If we got this far, the last char was whitespace */
    *s = '\0';
    if (ch != EOF)   /* shouldn't ungetc an EOF */
       UNGETC((char)ch);
    return TRUE;
}

/* creates an upper case copy of an array */
void 
defrData::uc_array(char *source, char *dest)
{
    for(; *source != 0; )
    *dest++ = toupper(*source++);
    *dest = 0;
}


void
defrData::StoreAlias()
{
    int         tokenSize = TOKEN_SIZE;
    char        *aname = (char*)malloc(tokenSize);

    DefGetToken(&aname, &tokenSize);

    char        *line = (char*)malloc(tokenSize);

    DefGetToken(&line, &tokenSize);  

    char        *uc_line = (char*)malloc(tokenSize);

    string so_far;               /* contains alias contents as we build it */

    if (strcmp(line,"=") != 0) {
       defError(6000, "Expecting '='");
       return;
    }

    /* now keep getting lines till we get one that contains &ENDALIAS */
    for(char *p = NULL ;p == NULL;){
        int i;
        char *s = line;
        for(i=0;i<tokenSize-1;i++) {
            int ch = GETC();
            if (ch == EOF) {
                defError(6001, "End of file in &ALIAS");
                return;
            }

            *s++ = ch;
            
            if (ch == '\n') {
                print_lines(++nlines);             
                break;
            }
        }

        *s = '\0';

        uc_array(line, uc_line);             /* make upper case copy */
        p = strstr(uc_line, "&ENDALIAS");    /* look for END_ALIAS */
        if (p != NULL)                       /* if we find it */
            *(line + (p - uc_line)) = 0;     /* remove it from the line */

        so_far += line;
    }

    def_alias_set[aname] = so_far;

    free(aname);
    free(line);
    free(uc_line);
}

/* The main routine called by the YACC parser to get the next token.
 *    Returns 0 if no more tokens are available.
 *    Returns an integer for keywords (see the yacc_defines.h for values)
 *    Returns the character itself for punctuation
 *    Returns NUMBER for numeric looking tokens
 *    Returns T_STRING for anything else
 * If the global "dumb_mode" is > 0, it reads the next token in "dumb mode".
 * In this case, it does not do keyword lookup, or attempt to read a token
 * as a number; if the token is not punctuation, it's a T_STRING.  Each token
 * read decrements dumb_mode, so you can instruct the the lexer to read the
 * next N tokens in dumb mode by setting "dumb_mode" to that value.
 *
 * Newlines are in general silently ignored.  If the global nl_token is
 * true, however, they are returned as the token K_NL.
 */

int 
defrData::defyylex(YYSTYPE *pYylval) {

   int v = sublex(pYylval);
   if (defPrintTokens) {
      if (v == 0) {
         printf("yylex NIL\n");
      } else if (v < 256) {
         printf("yylex char %c\n", v);
      } else if (v == QSTRING) {
         printf("yylex quoted string '%s'\n", pYylval->string);
      } else if (v == T_STRING) {
         printf("yylex string '%s'\n", pYylval->string);
      } else if (v == NUMBER) {
         printf("yylex number %f\n", pYylval->dval);
      } else {
         printf("yylex keyword %s\n", defrData::defkywd(v));
      }
   }

   if ((v == 0) && (!doneDesign)) {
      defError(6002, "Incomplete def file.");
      // Stop printing error messages after the EOF. 
      hasFatalError = 1; 
      return (-1);
   }

   return v;
}

int 
defrData::sublex(YYSTYPE *pYylval)
{
   char fc;
   double numVal;
   char*  outMsg;

   pv_deftoken = (char*)realloc(pv_deftoken, deftokenLength);
   strcpy(pv_deftoken, deftoken);

   /* First, we eat all the things the parser should be unaware of.
    * This includes:
    * a) Comments
    * b) &alias definitions
    * c) &alias expansions
    */
   for(;;) {
      if(!DefGetToken(&deftoken, &deftokenLength)) {    /* get a raw token */
         return 0;
      }
      fc = deftoken[0];
      uc_token = (char*)realloc(uc_token, deftokenLength);

      /* first, check for # comments or &alias statements.  # comments
      we ignore, and &alias statements are eaten and recorded by the
      lexer. */
      if (fc == settings->CommentChar) {
         // The code isn't work in correct way, no way to fix it exits 
         // but keep it for compatibility reasons. 
         int magic_count = -1;
         for(fc = GETC();; fc = GETC()) {/* so skip to the end of line */
            magic_count++;
            if ((magic_count < (int)strlen(magic)) && (fc == magic[magic_count])) {
              if ((int)strlen(magic) == (magic_count + 1)) {
                if (settings->MagicCommentFoundFunction) {
                  settings->MagicCommentFoundFunction();
                }
              }
            }
            if (fc == EOF) return 0;
            if (fc == '\n') {
                print_lines(++nlines);
                break;
            }
         }
      }
      else if (fc == '&') {
         /* begins with &.  If &alias, read contents and */
         /* store them.  Otherwise it's a define, or a macro use. */
         string alias;
         uc_array(deftoken, uc_token);

         if (strcmp(uc_token,"&ALIAS") == 0)
            StoreAlias();    /* read and store the alias */
         else if (defGetAlias(deftoken, alias))
            stack[++input_level] = alias;
         else
            break;    /* begins with &, but not an &alias defn. or use. */
      } else
         break;    /* does not begin with commentChar or '&' */
   }

   if (defInvalidChar) {
      outMsg = (char*)malloc(500 + strlen(deftoken));
      sprintf(outMsg, "Invalid characters found in \'%s\'.\nThese characters might be using the character types other than English.\nCreate characters by specifying valid characters types.",
              deftoken);
      defError(6008, outMsg);
      free(outMsg);
      hasFatalError = 1;
      return 0;
   }

   if (doneDesign) {
      fc = EOF;
      defInfo(8000, "There are still data after the END DESIGN statement");
      return 0;    
   }

   if(fc == '\"') {
      pYylval->string = ringCopy(&(deftoken[1]));

      return QSTRING;
   }

   /* at this point we've read a token */
   /* printf("Token is %s\n", deftoken); */
   
   // Protect the token counting variables form the decrement overflow.
   if (dumb_mode >= 0) {
       dumb_mode--;
   }

   if (no_num >= 0) {
       no_num--;
   }

   if (isdigit(fc) || fc == '.' || (fc == '-' && deftoken[1] != '\0') ) {
      char *ch;
      /* 6/12/2003 - The following switching to use strtol first is a fix */
      /* for FE for performance improvement. */
      /* Adding the flag "parsing_property" is for pcr 594214, need to call */
      /* strtod first to handle double number inside PROPERTY.  Only        */
      /* property has real number (large number) */
      if (!real_num) {
         pYylval->dval = strtol(deftoken, &ch, 10); /* try string to long first */
         if (no_num < 0 && *ch == '\0') { /* did we use the whole string? */
            return NUMBER;
         } else {  /* failed strtol, try double */
            numVal = pYylval->dval = strtod(deftoken, &ch);
            if (no_num < 0 && *ch == '\0') {  /* did we use the whole string? */
               /* check if the integer has exceed the limit */
               if ((numVal >= lVal) && (numVal <= rVal))
                  return NUMBER;    /* YES, it's really a number */
               else {
                  char* str = (char*)malloc(strlen(deftoken)
                               +strlen(session->FileName)+350);
                  sprintf(str,
                    "<Number has exceed the limit for an integer> in %s at line %s\n",
                    session->FileName, lines2str(nlines));
                  fflush(stdout);
                  defiError(1, 0, str);
                  free(str);
                  errors++;
                  return NUMBER;
               }
             } else {
               pYylval->string = ringCopy(deftoken);  /* NO, it's a string */
               return T_STRING;
            }
         }
      } else {  /* handling PROPERTY, do strtod first instead of strtol */
         numVal = pYylval->dval = strtod(deftoken, &ch);
         if (no_num < 0 && *ch == '\0') { /* did we use the whole string? */
            /* check if the integer has exceed the limit */
            if (real_num)    /* this is for PROPERTYDEF with REAL */
               return NUMBER;
            if ((numVal >= lVal) && (numVal <= rVal)) 
               return NUMBER;    /* YES, it's really a number */
            else {
               char* str = (char*)malloc(strlen(deftoken)
                                +strlen(session->FileName)+350);
               sprintf(str,
                 "<Number has exceed the limit for an integer> in %s at line %s\n",
                 session->FileName, lines2str(nlines));
               fflush(stdout);
               defiError(1, 0, str);
               free(str);
               errors++;
               return NUMBER;
            }
         } else {  /* failed integer conversion, try floating point */
            pYylval->dval = strtol(deftoken, &ch, 10);
            if (no_num < 0 && *ch == '\0')  /* did we use the whole string? */
               return NUMBER;
            else {
               pYylval->string = ringCopy(deftoken);  /* NO, it's a string */
               return T_STRING;
            }
         }
      }
   }
      
   /* if we are dumb mode, all we return is punctuation and strings & numbers*/
   /* until we see the next '+' or ';' deftoken */
   if (dumb_mode >= 0) {
      if (deftoken[1]=='\0' && (fc=='('||fc==')'||fc=='+'||fc==';'||fc=='*')){
      
         if (fc == ';' ||  fc == '+') {
            dumb_mode = 0;
            no_num = 0;
         }   
         return (int)fc;
      }
      if (by_is_keyword  && ((strcmp(deftoken,"BY") == 0) ||
          (strcmp(deftoken, "by") == 0))) {
         return K_BY; /* even in dumb mode, we must see the BY deftoken */
      }
      if (do_is_keyword  && ((strcmp(deftoken,"DO") == 0) ||
          (strcmp(deftoken, "do") == 0))) {
         return K_DO; /* even in dumb mode, we must see the DO deftoken */
      }
      if (new_is_keyword  && ((strcmp(deftoken,"NEW") == 0) ||
          (strcmp(deftoken, "new") == 0))) {
         return K_NEW; /* even in dumb mode, we must see the NEW deftoken */
      }
      if (nondef_is_keyword && ((strcmp(deftoken, "NONDEFAULTRULE") == 0) ||
          (strcmp(deftoken, "nondefaultrule") == 0))){
          return K_NONDEFAULTRULE; /* even in dumb mode, we must see the */
                                   /* NONDEFAULTRULE deftoken */
      }
      if (mustjoin_is_keyword && ((strcmp(deftoken, "MUSTJOIN") == 0) ||
          (strcmp(deftoken, "mustjoin") == 0))) {
          return K_MUSTJOIN; /* even in dumb mode, we must see the */
                             /* MUSTJOIN deftoken */
      }
      if (step_is_keyword  && ((strcmp(deftoken,"STEP") == 0) ||
          (strcmp(deftoken, "step") == 0))) {
          return K_STEP;/* even in dumb mode, we must see the STEP deftoken */
      }
      if (fixed_is_keyword  && ((strcmp(deftoken,"FIXED") == 0) ||
          (strcmp(deftoken, "fixed") == 0))) {
         return K_FIXED; /* even in dumb mode, we must see the FIXED deftoken */
      }  
      if (cover_is_keyword  && ((strcmp(deftoken,"COVER") == 0) ||
          (strcmp(deftoken, "cover") == 0))) {
         return K_COVER; /* even in dumb mode, we must see the COVER deftoken */
      }
      if (routed_is_keyword  && ((strcmp(deftoken,"ROUTED") == 0) ||
          (strcmp(deftoken, "rounted") == 0))) {
         return K_ROUTED; /* even in dumb mode, we must see the */
                          /* ROUTED deftoken */
      }
      
      if (virtual_is_keyword && ((strcmp(deftoken, "VIRTUAL") == 0 )
         || (strcmp(deftoken, "virtual") == 0 ))) {
         return K_VIRTUAL;
      }
      
      if (rect_is_keyword && ((strcmp(deftoken, "RECT") == 0 )
         || (strcmp(deftoken, "rect") == 0 ))) {
         return K_RECT;
      }
      
      if (virtual_is_keyword && ((strcmp(deftoken, "MASK") == 0 )
         || (strcmp(deftoken, "mask") == 0 ))) {
         return K_MASK;
      }
      
      if (orient_is_keyword) {
         int result;
         uc_array(deftoken, uc_token);

         if (defGetKeyword(uc_token, &result)) {
            if (K_N == result)
                return K_N;
            if (K_W == result)
                return K_W;
            if (K_S == result)
                return K_S;
            if (K_E == result)
                return K_E;
            if (K_FN == result)
                return K_FN;
            if (K_FW == result)
                return K_FW;
            if (K_FS == result)
                return K_FS;
            if (K_FE == result)
            if (strcmp(deftoken, "FE") == 0)
                return K_FE;
         }
      }
      pYylval->string = ringCopy(deftoken);
      return T_STRING;
   }

   /* if we get here we are in smart mode.  Parse deftoken */
   /* 2/19/2004 - add _ since name can starts with '_' */
   /* 2/23/2004 - add the following characters which a name can starts with */
   /*    ! $ | : , @ ~ = < . ? { ' ^ " */
   if (isalpha(fc) || fc == '&' || fc == '_') {
      int result;

      History_text.resize(0);
      uc_array(deftoken, uc_token);

      if (defGetKeyword(uc_token, &result)) {
         if (K_HISTORY == result) {  /* history - get up to ';' */
            int c;
            int prev;
            prev = ' ';
            while (1) {
               c = GETC();
               
               if (c == EOF) {
                   defError(6015, "Unexpected end of the DEF file.");
                   break;
               }

               if (c == ';' && (prev == ' ' || prev == '\t' || prev == '\n'))
                    break;
               if (c == '\n') {
                    print_lines(++nlines);
               }
               prev = c;
               History_text.push_back(c);
            }
            History_text.push_back('\0');
         } else if (K_BEGINEXT == result) { /* extension, get up to end */
            int cc;
            int foundTag = 0;
            int notEmpTag = 0;
            int begQuote = 0;
            /* First make sure there is a name after BEGINEXT within quote */
            /* BEGINEXT "name" */
            while (1) {
               cc = GETC();
               
               if (cc == EOF) {
                   defError(6015, "Unexpected end of the DEF file.");
                   break;
               }

               if (cc == '\n') {
                  if (!foundTag) {
                     defError(6003, "tag is missing for BEGINEXT");
                     break;
                  }
               } else {

                  History_text.push_back(cc);
                  if (cc != ' ') {
                     if (cc == '\"') {   /* found a quote */
                        if (!begQuote)
                           begQuote = 1;
                        else if (notEmpTag) {
                           foundTag = 1;
                           break;      /* Found the quoted tag */
                        } else {
                           defError(6004, "The BEGINEXT tag is empty. Specify a value for the tag and try again.");
                           break;
                        }
                     } else if (!begQuote) {   /* anything but a quote */
                        defError(6005, "The '\"' is missing within the tag. Specify the '\"' in the tag and then try again.");
                        break;
                     }  else             /* anything but a quote and there */
                        notEmpTag = 1;   /* is already a quote */
                  }
               }
            }
            if (foundTag) {
               /* We have handle with the tag, just read the rest until */
               /* ENDEXT */
               begQuote = 0;
               while (1) {
                  cc = GETC();
               
                  if (cc == EOF) {
                      defError(6015, "Unexpected end of the DEF file.");
                      break;
                  }

                  if (cc == '\n') {
                        print_lines(++nlines);
                  } else if (cc == '\"') {
                     if (!begQuote)
                        begQuote = 1;
                     else
                        begQuote = 0;
                  }

                  History_text.push_back(cc);

                  int histTextSize = History_text.size();

                  if (histTextSize >= 6 && memcmp(&History_text[histTextSize - 6 ], "ENDEXT", 6) == 0) {
                     if (begQuote)
                        defError(6006, "The ending '\"' is missing in the tag. Specify the ending '\"' in the tag and then try again.");
                     break;
                  } else if (histTextSize >= 10 && memcmp(&History_text[histTextSize - 10 ], "END DESIGN", 10) == 0) {
                     defError(6007, "The ENDEXT statement is missing in the DEF file. Include the statement and then try again.");
                     nlines--;
                     break;
                  }
               }
            }
            History_text.push_back('\0');
         }
         return result;        /* YES, return its value */
      } else {  /* we don't have a keyword.  */
         if (fc == '&')
         return amper_lookup(pYylval, deftoken);
         pYylval->string = ringCopy(deftoken);  /* NO, it's a string */
         return T_STRING;
      }
   } else {  /* it should be a punctuation character */
      if (deftoken[1] != '\0') {
         if (strcmp(deftoken, ">=") == 0) return K_GE;
         if (strcmp(deftoken, "<=") == 0) return K_LE;
         if (strcmp(deftoken, "<>") == 0) return K_NE;

         defError(6017, "Odd punctuation found.");
         hasFatalError = 1;
      } else if (strlen(deftoken) > 2
                 || strlen(deftoken) == 0) {
          defError(6017, "Odd punctuation found.");
          hasFatalError = 1;
      }
      return (int)deftoken[0];
   }
}

/* We have found a deftoken beginning with '&'.  If it has been previously
   defined, substitute the definition.  Otherwise return it. */
int 
defrData::amper_lookup(YYSTYPE *pYylval, char *tkn)
{
   string   defValue;

   /* printf("Amper_lookup: %s\n", tkn); */

   /* &defines returns a T_STRING */
   if (defGetDefine(tkn, defValue)) {
      int value;
      if (defGetKeyword(defValue.c_str(), &value))
         return value;
      if (defValue.c_str()[0] == '"')
         pYylval->string = ringCopy(defValue.c_str()+1);
      else
         pYylval->string = ringCopy(defValue.c_str());
      return (defValue.c_str()[0] == '\"' ? QSTRING : T_STRING);
   }
   /* if none of the above, just return the deftoken. */
   pYylval->string = ringCopy(tkn);
   return T_STRING;
}

void 
defrData::defError(int msgNum, const char *s) {
   char* str;
   const char  *curToken = isgraph(deftoken[0]) ? deftoken
                                                              : "<unprintable>";
   const char  *pvToken = isgraph(pv_deftoken[0]) ? pv_deftoken
                                                           : "<unprintable>";
   int len = strlen(curToken)-1;
   int pvLen = strlen(pvToken)-1;

   if (hasFatalError)
       return;
   if ((settings->totalDefMsgLimit > 0) && (defMsgPrinted >= settings->totalDefMsgLimit))
      return;
   if (settings->MsgLimit[msgNum-5000] > 0) {
      if (msgLimit[msgNum-5000] >= settings->MsgLimit[msgNum-5000])
         return;             /* over the limit */
      msgLimit[msgNum-5000] = msgLimit[msgNum-5000] + 1;
   }

   /* PCR 690679, probably missing space before a ';' */
   if (strcmp(s, "parse error") == 0) {
      if ((len > 1) && (deftoken[len] == ';')) {
         str = (char*)malloc(len + strlen(s) + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): %s, file %s at line %s\nLast token was <%s>, space is missing before <;>\n",
              msgNum, s, session->FileName, lines2str(nlines), curToken);
      } else if ((pvLen > 1) && (pv_deftoken[pvLen] == ';')) {
         str = (char*)malloc(pvLen + strlen(s) + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): %s, file %s at line %s\nLast token was <%s>, space is missing before <;>\n",
              msgNum, s, session->FileName, lines2str(nlines-1), pvToken);
      } else {
         str = (char*)malloc(len + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): Def parser has encountered an error in file %s at line %s, on token %s.\nProblem can be syntax error on the def file or an invalid parameter name.\nDouble check the syntax on the def file with the LEFDEF Reference Manual.\n",
              msgNum, session->FileName, lines2str(nlines), curToken);
      }
   } else if (strcmp(s, "syntax error") == 0) {
      if ((len > 1) && (deftoken[len] == ';')) {
         str = (char*)malloc(len + strlen(s) + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): %s, file %s at line %s\nLast token was <%s>, space is missing before <;>\n",
              msgNum, s, session->FileName, lines2str(nlines), curToken);
      } else if ((pvLen > 1) && (pv_deftoken[pvLen] == ';')) {
         str = (char*)malloc(pvLen + strlen(s) + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): %s, file %s at line %s\nLast token was <%s>, space is missing before <;>\n",
              msgNum, s, session->FileName, lines2str(nlines-1), pvToken);
      } else {
         str = (char*)malloc(len + strlen(session->FileName) + 350);
         sprintf(str, "ERROR (DEFPARS-%d): Def parser has encountered an error in file %s at line %s, on token %s.\nProblem can be syntax error on the def file or an invalid parameter name.\nDouble check the syntax on the def file with the LEFDEF Reference Manual.\n",
              msgNum, session->FileName, lines2str(nlines), curToken);
      }
   } else {
      str = (char*)malloc(len + strlen(s) + strlen(session->FileName) + 350);
      sprintf(str, "ERROR (DEFPARS-%d): %s Error in file %s at line %s, on token %s.\nUpdate the def file before parsing the file again.\n",
           msgNum, s, session->FileName, lines2str(nlines), curToken);
   }

   fflush(stdout);
   defiError(1, msgNum, str);
   free(str);
   errors++;
}

/* yydeferror is called by bison.simple */
void 
defrData::defyyerror(const char *s) {
   defError(defMsgCnt++, s);
}

/* All info starts with 8000 */
/* All info within defInfo starts with 8500 */
void 
defrData::defInfo(int msgNum, const char *s) {
   int i;

   for (i = 0; i < settings->nDDMsgs; i++) {  /* check if info has been disable */
      if (settings->disableDMsgs[i] == msgNum)
         return;  /* don't print out any info since msg has been disabled */
   }
   
   if (settings->ContextWarningLogFunction) {
      char* str = (char*)malloc(strlen(deftoken)+strlen(s)
                                   +strlen(session->FileName)+350);
      sprintf(str, "INFO (DEFPARS-%d): %s See file %s at line %s.\n",
              msgNum, s, session->FileName, lines2str(nlines));
      (*settings->ContextWarningLogFunction)(session->UserData, str);
      free(str);
   } else if (settings->WarningLogFunction) {
      char* str = (char*)malloc(strlen(deftoken)+strlen(s)
                                   +strlen(session->FileName)+350);
      sprintf(str, "INFO (DEFPARS-%d): %s See file %s at line %s.\n",
              msgNum, s, session->FileName, lines2str(nlines));
      (*settings->WarningLogFunction)(str);
      free(str);
   } else if (defrLog) {
      fprintf(defrLog, "INFO (DEFPARS-%d): %s See file %s at line %s\n",
              msgNum, s, session->FileName, lines2str(nlines));
   } else {
      if (!hasOpenedDefLogFile) {
         if ((defrLog = fopen("defRWarning.log", "w")) == 0) {
            printf("WARNING(DEFPARS-8500): Unable to open the file defRWarning.log in %s.\n",
            getcwd(NULL, 64));
            printf("Info messages will not be printed.\n");
         } else {
            hasOpenedDefLogFile = 1;
            fprintf(defrLog, "Info from file: %s\n\n", session->FileName);
            fprintf(defrLog, "INFO (DEFPARS-%d): %s See file %s at line %s\n",
                    msgNum, s, session->FileName, lines2str(nlines));
         }
      } else {
         if ((defrLog = fopen("defRWarning.log", "a")) == 0) {
            printf("WARNING (DEFPARS-8500): Unable to open the file defRWarning.log in %s.\n",
            getcwd(NULL, 64));
            printf("Info messages will not be printed.\n");
         } else {
            hasOpenedDefLogFile = 1;
            fprintf(defrLog, "\nInfo from file: %s\n\n", session->FileName);
            fprintf(defrLog, "INFO (DEFPARS-%d): %s See file %s at line %s\n",
                    msgNum, s, session->FileName, lines2str(nlines));
         }
      } 
   }
}

/* All warning starts with 7000 */
/* All warning within defWarning starts with 7500 */
void 
defrData::defWarning(int msgNum, const char *s) {
   int i;

   for (i = 0; i <settings->nDDMsgs; i++) {  /* check if warning has been disable */
      if (settings->disableDMsgs[i] == msgNum)
         return;  /* don't print out any warning since msg has been disabled */
   }

   if (settings->ContextWarningLogFunction) {
      char* str = (char*)malloc(strlen(deftoken)+strlen(s)
                                   +strlen(session->FileName)+350);
      sprintf(str, "WARNING (DEFPARS-%d): %s See file %s at line %s.\n",
              msgNum, s, session->FileName, lines2str(nlines));
      (*settings->ContextWarningLogFunction)(session->UserData, str);
      free(str);
   } else if (settings->WarningLogFunction) {
      char* str = (char*)malloc(strlen(deftoken)+strlen(s)
                                   +strlen(session->FileName)+350);
      sprintf(str, "WARNING (DEFPARS-%d): %s See file %s at line %s.\n",
              msgNum, s, session->FileName, lines2str(nlines));
      (*settings->WarningLogFunction)(str);
      free(str);
   } else if (defrLog) {
      fprintf(defrLog, "WARNING (DEFPARS-%d): %s See file %s at line %s\n",
              msgNum, s, session->FileName, lines2str(nlines));
   } else {
      if (!hasOpenedDefLogFile) {
         if ((defrLog = fopen("defRWarning.log", "w")) == 0) {
            printf("WARNING (DEFPARS-7500): Unable to open the file defRWarning.log in %s.\n",
            getcwd(NULL, 64));
            printf("Warning messages will not be printed.\n");
         } else {
            hasOpenedDefLogFile = 1;
            fprintf(defrLog, "Warnings from file: %s\n\n", session->FileName);
            fprintf(defrLog, "WARNING (DEFPARS-%d): %s See file %s at line %s\n",
                    msgNum, s, session->FileName, lines2str(nlines));
         }
      } else {
         if ((defrLog = fopen("defRWarning.log", "a")) == 0) {
            printf("WARNING (DEFAPRS-7501): Unable to open the file defRWarning.log in %s.\n",
            getcwd(NULL, 64));
            printf("Warning messages will not be printed.\n");
         } else {
            hasOpenedDefLogFile = 1;
            fprintf(defrLog, "\nWarnings from file: %s\n\n", session->FileName);
            fprintf(defrLog, "WARNING (DEFPARS-%d): %s See file %s at line %s\n",
                    msgNum, s, session->FileName, lines2str(nlines));
         }
      } 
   }
   def_warnings++;
}

const char* 
defrData::defkywd(int num)
{
   switch (num) {
      case QSTRING: return  "QSTRING";
      case T_STRING: return  "T_STRING";
      case SITE_PATTERN: return  "SITE_PATTERN";
      case NUMBER: return "NUMBER";
      case K_ALIGN: return "ALIGN";
      case K_AND: return "AND";
      case K_ARRAY: return "ARRAY";
      case K_ASSERTIONS: return "ASSERTIONS";
      case K_BEGINEXT: return "BEGINEXT";
      case K_BOTTOMLEFT: return "BOTTOMLEFT";
      case K_BUSBITCHARS: return "BUSBITCHARS";
      case K_BY: return "BY";
      case K_CANNOTOCCUPY: return "CANNOTOCCUPY";
      case K_CANPLACE: return "CANPLACE";
      case K_CAPACITANCE: return "CAPACITANCE";
      case K_COMMONSCANPINS: return "COMMONSCANPINS";
      case K_COMPONENT: return "COMPONENT";
      case K_COMPONENTPIN: return "COMPONENTPIN";
      case K_COMPS: return "COMPS";
      case K_COMP_GEN: return "COMP_GEN";
      case K_CONSTRAINTS: return "CONSTRAINTS";
      case K_COVER: return "COVER";
      case K_CUTSIZE: return "CUTSIZE";
      case K_CUTSPACING: return "CUTSPACING";
      case K_DEFAULTCAP: return "DEFAULTCAP";
      case K_DEFINE: return "DEFINE";
      case K_DEFINES: return "DEFINES";
      case K_DEFINEB: return "DEFINEB";
      case K_DESIGN: return "DESIGN";
      case K_DESIGNRULEWIDTH: return "DESIGNRULEWIDTH";
      case K_DIAGWIDTH: return "DIAGWIDTH";
      case K_DIEAREA: return "DIEAREA";
      case K_DIFF: return "DIFF";
      case K_DIRECTION: return "DIRECTION";
      case K_DIST: return "DIST";
      case K_DISTANCE: return "DISTANCE";
      case K_DIVIDERCHAR: return "DIVIDERCHAR";
      case K_DO: return "DO";
      case K_DRIVECELL: return "DRIVECELL";
      case K_E: return "E";
      case K_EEQMASTER: return "EEQMASTER";
      case K_ELSE: return "ELSE";
      case K_ENCLOSURE: return "ENCLOSURE";
      case K_END: return "END";
      case K_ENDEXT: return "ENDEXT";
      case K_EQ: return "EQ";
      case K_EQUAL: return "EQUAL";
      case K_ESTCAP: return "ESTCAP";
      case K_FE: return "FE";
      case K_FALL: return "FALL";
      case K_FALLMAX: return "FALLMAX";
      case K_FALLMIN: return "FALLMIN";
      case K_FALSE: return "FALSE";
      case K_FIXED: return "FIXED";
      case K_FLOATING: return "FLOATING";
      case K_FLOORPLAN: return "FLOORPLAN";
      case K_FN: return "FN";
      case K_FOREIGN: return "FOREIGN";
      case K_FPC: return "FPC";
      case K_FROMCLOCKPIN: return "FROMCLOCKPIN";
      case K_FROMCOMPPIN: return "FROMCOMPPIN";
      case K_FROMPIN: return "FROMPIN";
      case K_FROMIOPIN: return "FROMIOPIN";
      case K_FS: return "FS";
      case K_FW: return "FW";
      case K_GCELLGRID: return "GCELLGRID";
      case K_GE: return "GE";
      case K_GT: return "GT";
      case K_GROUND: return "GROUND";
      case K_GROUNDSENSITIVITY: return "GROUNDSENSITIVITY";
      case K_GROUP: return "GROUP";
      case K_GROUPS: return "GROUPS";
      case K_HALO: return "HALO";
      case K_HARDSPACING: return "HARDSPACING";
      case K_HISTORY: return "HISTORY";
      case K_HOLDRISE: return "HOLDRISE";
      case K_HOLDFALL: return "HOLDFALL";
      case K_HORIZONTAL: return "HORIZONTAL";
      case K_IF: return "IF";
      case K_IN: return "IN";
      case K_INTEGER: return "INTEGER";
      case K_IOTIMINGS: return "IOTIMINGS";
      case K_LAYER: return "LAYER";
      case K_LAYERS: return "LAYERS";
      case K_LE: return "LE";
      case K_LT: return "LT";
      case K_MACRO: return "MACRO";
      case K_MASK: return "MASK";
      case K_MAX: return "MAX";
      case K_MAXDIST: return "MAXDIST";
      case K_MAXHALFPERIMETER: return "MAXHALFPERIMETER";
      case K_MAXX: return "MAXX";
      case K_MAXY: return "MAXY";
      case K_MICRONS: return "MICRONS";
      case K_MIN: return "MIN";
      case K_MINCUTS: return "MINCUTS";
      case K_MINPINS: return "MINPINS";
      case K_MUSTJOIN: return "MUSTJOIN";
      case K_N: return "N";
      case K_NAMESCASESENSITIVE: return "NAMESCASESENSITIVE";
      case K_NAMEMAPSTRING: return "NAMEMAPSTRING";
      case K_NE: return "NE";
      case K_NET: return "NET";
      case K_NETEXPR: return "NETEXPR";
      case K_NETLIST: return "NETLIST";
      case K_NETS: return "NETS";
      case K_NEW: return "NEW";
      case K_NONDEFAULTRULE: return "NONDEFAULTRULE";
      case K_NOSHIELD: return "NOSHIELD";
      case K_NOT: return "NOT";
      case K_OFF: return "OFF";
      case K_OFFSET: return "OFFSET";
      case K_ON: return "ON";
      case K_OR: return "OR";
      case K_ORDERED: return "ORDERED";
      case K_ORIGIN: return "ORIGIN";
      case K_ORIGINAL: return "ORIGINAL";
      case K_OUT: return "OUT";
      case K_PARALLEL: return "PARALLEL";
      case K_PARTITIONS: return "PARTITIONS";
      case K_PATH: return "PATH";
      case K_PATTERN: return "PATTERN";
      case K_PATTERNNAME: return "PATTERNNAME";
      case K_PINPROPERTIES: return "PINPROPERTIES";
      case K_PINS: return "PINS";
      case K_PLACED: return "PLACED";
      case K_PIN: return "PIN";
      case K_POLYGON: return "POLYGON";
      case K_PROPERTY: return "PROPERTY";
      case K_PROPERTYDEFINITIONS: return "PROPERTYDEFINITIONS";
      case K_RANGE: return "RANGE";
      case K_REAL: return "REAL";
      case K_RECT: return "RECT";
      case K_REENTRANTPATHS: return "REREENTRANTPATHS";
      case K_REGION: return "REGION";
      case K_REGIONS: return "REGIONS";
      case K_RISE: return "RISE";
      case K_RISEMAX: return "RISEMAX";
      case K_RISEMIN: return "RISEMIN";
      case K_ROUTED: return "ROUTED";
      case K_ROW: return "ROW";
      case K_ROWCOL: return "ROWCOL";
      case K_ROWS: return "ROWS";
      case K_S: return "S";
      case K_SCANCHAINS: return "SCANCHAINS";
      case K_SETUPRISE: return "SETUPRISE";
      case K_SETUPFALL: return "SETUPFALL";
      case K_SHAPE: return "SHAPE";
      case K_SITE: return "SITE";
      case K_SLEWRATE: return "SLEWRATE";
      case K_SNET: return "SNET";
      case K_SNETS: return "SNETS";
      case K_SOURCE: return "SOURCE";
      case K_SOFT: return "SOFT";
      case K_SPACING: return "SPACING";
      case K_SPECIAL: return "SPECIAL";
      case K_START: return "START";
      case K_START_NET: return "START_NET";
      case K_STEP: return "STEP";
      case K_STRING: return "STRING";
      case K_STOP: return "STOP";
      case K_SUBNET: return "SUBNET";
      case K_SUM: return "SUM";
      case K_SUPPLYSENSITIVITY: return "SUPPLYSENSITIVITY";
      case K_STYLE: return "STYLE";
      case K_STYLES: return "STYLES";
      case K_SYNTHESIZED: return "SYNTHESIZED";
      case K_TAPER: return "TAPER";
      case K_TAPERRULE: return "TAPERRULE";
      case K_THEN: return "THEN";
      case K_THRUPIN: return "THRUPIN";
      case K_TIMING: return "TIMING";
      case K_TIMINGDISABLES: return "TIMINGDISABLES";
      case K_TOCLOCKPIN: return "TOCLOCKPIN";
      case K_TOCOMPPIN: return "TOCOMPPIN";
      case K_TOIOPIN: return "TOIOPIN";
      case K_TOPIN: return "TOPIN";
      case K_TOPRIGHT: return "TOPRIGHT";
      case K_TRACKS: return "TRACKS";
      case K_TRUE: return "TRUE";
      case K_TURNOFF: return "TURNOFF";
      case K_VARIABLE: return "VARIABLE";
      case K_VIA: return "VIA";
      case K_VIARULE: return "VIARULE";
      case K_VIAS: return "VIAS";
      case K_VOLTAGE: return "VOLTAGE";
      case K_TECH: return "TECH";
      case K_UNITS: return "UNITS";
      case K_UNPLACED: return "UNPLACED";
      case K_USE: return "USE";
      case K_USER: return "USER";
      case K_VERSION: return "VERSION";
      case K_VIRTUAL: return "VIRTUAL";
      case K_VERTICAL: return "VERTICAL";
      case K_VPIN: return "VPIN";
      case K_W: return "W";
      case K_WIRECAP: return "WIRECAP";
      case K_WEIGHT: return "WEIGHT";
      case K_WIDTH: return "WIDTH";
      case K_WIREDLOGIC: return "WIREDLOGIC";
      case K_WIREEXT: return "WIREEXT";
      case K_XTALK: return "XTALK";
      case K_X: return "X";
      case K_Y: return "Y";
      default: return "bogus";
   }
}

const char* 
defrData::DEFCASE(const char* ch)
{
    return names_case_sensitive ? ch : upperCase(ch);
}

void
defrData::pathIsDone(int  sh, int  reset, int  osNet, int  *needCbk)
{
    if ((callbacks->NetCbk || callbacks->SNetCbk) && settings->AddPathToNet) {
        //PathObj.reverseOrder();
        if (Subnet) {
            // if (sh)
            //    defrSubnet->addShieldPath(defrPath);
            // else 
            Subnet->addWirePath(&PathObj, reset, osNet,
                                         needCbk);
        } else {
            if (sh)
                Net.addShieldPath(&PathObj, reset, osNet, needCbk);
            else
                Net.addWirePath(&PathObj, reset, osNet, needCbk);
        }
    } else if (callbacks->PathCbk) {
        //defrPath->reverseOrder();
        (*callbacks->PathCbk)(defrPathCbkType, &PathObj, session->UserData);
        PathObj.Destroy();
        free((char*) &PathObj);
    }

    PathObj.Init();
}

END_LEFDEF_PARSER_NAMESPACE




