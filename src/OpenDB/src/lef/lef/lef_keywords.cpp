// *****************************************************************************
// *****************************************************************************
// Copyright 2012 - 2016, Cadence Design Systems
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
//  $Author: dell $
//  $Revision: #1 $
//  $Date: 2017/06/06 $
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "lefiDefs.hpp"
#include "lefiDebug.hpp"
#include "lefrReader.hpp"
#include "lefrData.hpp"
#include "lefrCallBacks.hpp"
#include "lefrSettings.hpp"
#include "crypt.hpp"
#include "lex.h"

#ifdef WIN32
#   include <direct.h>
#else // not WIN32 
#   include <unistd.h>
#endif // WIN32

#include "lefrData.hpp"

using namespace std;

BEGIN_LEFDEF_PARSER_NAMESPACE

#include "lef.tab.h"

extern YYSTYPE lefyylval;


inline string 
strip_case(const char *str)
{
    string result(str);

    if (lefData->namesCaseSensitive) {
        return result;
    };

    for (string::iterator p = result.begin(); result.end() != p; ++p) {
        *p = toupper(*p);
    }

    return result;
}


inline
int lefGetKeyword(const char* name, int *result) 
{ 
    map<const char*, int, lefCompareCStrings>::iterator search = lefSettings->Keyword_set.find(name);
    if ( search != lefSettings->Keyword_set.end()) {
        *result = search->second;
        return TRUE;
    }

    return FALSE;
}


inline int
lefGetStringDefine(const char* name, const char** value)
{
    map<string, string, lefCompareStrings>::iterator search = lefData->defines_set.find(strip_case(name));

    if ( search != lefData->defines_set.end()) {
        *value = search->second.c_str();
        return TRUE;
    }
    return FALSE;
}


inline int
lefGetIntDefine(const char* name, int* value)
{
    map<string, int, lefCompareStrings>::iterator search = lefData->defineb_set.find(strip_case(name));

    if ( search != lefData->defineb_set.end()) {
        *value = search->second;
        return TRUE;
    }
    return FALSE;
}


inline int
lefGetDoubleDefine(const char* name, double* value)
{
    map<string, double, lefCompareStrings>::iterator search = lefData->define_set.find(strip_case(name));

    if ( search != lefData->define_set.end()) {
        *value = search->second;
        return TRUE;
    }
    return FALSE;
}


inline int
lefGetAlias(const char* name, const char** value)
{
    map<string, string, lefCompareStrings>::iterator search = lefData->alias_set.find(strip_case(name));

    if ( search != lefData->alias_set.end()) {
        *value = search->second.c_str();
        return TRUE;
    }
    return FALSE;
}


#define yyparse    lefyyparse
#define yylex    lefyylex
#define yyerror    lefyyerror
#define yylval    lefyylval
#define yychar    lefyychar
#define yydebug    lefyydebug
#define yynerrs    lefyynerrs

// lef.cpph starts here.

// variable to count number of warnings 

// 12/08/1999 -- Wanda da Rosa 
// file pointer to the lefRWarning.log 

extern char *lef_kywd(int num);

// User defined if log file should be in append from the previous run 

// User defined if property string value should be process 

// Varible from lex.cpph to keep track of invalid nonEnglish character 
// in lef file 

void
lefReloadBuffer()
{
    int nb;

    nb = 0;

    if (lefData->first_buffer) {
        lefData->first_buffer = 0;
        if (lefSettings->ReadFunction) {
            if ((nb = (*lefSettings->ReadFunction)(lefData->lefrFile, lefData->current_buffer, 4)) != 4) {
                lefData->next = NULL;
                return;
            }
        } else {
            if ((nb = fread(lefData->current_buffer, 1, 4, lefData->lefrFile)) != 4) {
                lefData->next = NULL;
                return;
            }
        }
        lefData->encrypted = encIsEncrypted((unsigned char*) lefData->current_buffer);
    }

    if (lefData->encrypted) {
        int i;
        int c;

        if (lefSettings->ReadEncrypted) {
            // is encrypted file and user has set the enable flag to read one 
            for (i = 0; i < IN_BUF_SIZE; i++) {
                if ((c = encFgetc(lefData->lefrFile)) == EOF) {
                    break;
                }
                lefData->current_buffer[i] = c;
            }
            nb = i;
        } else {      // an encrypted file, but user does not allow to read one 
            printf("File is an encrypted file, reader is not set to read one.\n");
            return;
        }
    } else if (nb == 0) {
        if (lefSettings->ReadFunction)
            nb = (*lefSettings->ReadFunction)(lefData->lefrFile, lefData->current_buffer, IN_BUF_SIZE);
        else
            // This is a normal file so just read some bytes. 
            nb = fread(lefData->current_buffer, 1, IN_BUF_SIZE, lefData->lefrFile);
    }

    if (nb <= 0) {
        lefData->next = NULL;
    } else {
        lefData->next = lefData->current_buffer;
        lefData->last = lefData->current_buffer + nb - 1;
    }
}

int
lefGetc()
{
    if (lefData->input_level >= 0) {        // Token has been getting from 
        const char *ch, *s;
        s = ch = lefData->current_stack[lefData->input_level];
        lefData->current_stack[lefData->input_level] = ++s;
        return *ch;
    }

    // Remove '\r' symbols from Windows streams.
    for (;;) {
        if (lefData->next > lefData->last)
            lefReloadBuffer();
        if (lefData->next == NULL)
            return EOF;

        int ch = *lefData->next++;

        if (ch != '\r')
            return ch;
    }
}

void
UNlefGetc(char ch)
{
    if ((lefData->next <= lefData->current_buffer) || (lefData->input_level > 0)) {
        lefError(1111, "UNlefGetc: buffer access violation.");
    } else {
        *(--lefData->next) = ch;
    }
}


// The following two variables are for communicating with the parser
/* Return a copy of the string allocated from the lefData->ring buffer.
 * We will keep several strings in the buffer and just reuse them.
 * This could cause problems if we need to use more strings than we
 * have in the buffer.
 */
static char *
ringCopy(const char *string)
{
    int len = strlen(string) + 1;
    if (++lefData->ringPlace >= RING_SIZE)
        lefData->ringPlace = 0;
    if (len > lefData->ringSizes[lefData->ringPlace]) {
        lefData->ring[lefData->ringPlace] = (char*) lefRealloc(lefData->ring[lefData->ringPlace], len);
        lefData->ringSizes[lefData->ringPlace] = len;
    }
    strcpy(lefData->ring[lefData->ringPlace], string);
    return lefData->ring[lefData->ringPlace];
}

char *
qStrCopy(char *string)
{
    int     len = strlen(string) + 3;
    char    *retStr;

    retStr = (char*) lefMalloc(len);
    sprintf(retStr, "\"%s\"", string);
    return retStr;
}


/* NOTE: we don't allocate these tables until they are used.  The reason
 * we don't allocate at the beginning of the program is that we don't know
 * at that point if we should be case sensitive or not. */

void
lefAddStringDefine(const char *token, const char *str)
{    
    string tmpStr((lefData->lefDefIf == TRUE) ? "" : "\"");

    tmpStr += str;

    lefData->defines_set[strip_case(token)] = tmpStr;
    lefData->lefDefIf = FALSE;
    lefData->inDefine = 0;
}


void
lefAddBooleanDefine(const char  *token, int val)
{
    lefData->defineb_set[strip_case(token)] = val;
}


void
lefAddNumDefine(const char  *token, double val)
{
    lefData->define_set[strip_case(token)] = val;
}


static int
GetTokenFromStack(char *s)
{
    const char    *ch;                  // utility variable 
    char          *prS = NULL;          // pointing to the previous char or s 
    char          *save = s;            // for debug printing 

    while (lefData->input_level >= 0) {
        for (ch = lefData->current_stack[lefData->input_level]; *ch != 0; ch++)    // skip white space 
            if (*ch != ' ' && *ch != '\t' && (lefData->lefNlToken || *ch != '\n'))
                break;
        // did we find anything?  If not, decrement level and try again 
        if (*ch == 0)
            lefData->input_level--;
        else if (*ch == '\n') {
            *s++ = *ch;
            *s = 0;
            if (lefData->lefDebug[11])
                printf("Stack[%d] Newline token\n", lefData->input_level);
            return TRUE;
        } else {        // we found something 
            for (; ; ch++) {
                if (*ch == ' ' || *ch == '\t' || *ch == '\n' || *ch == 0) {
                    /* 10/10/2000 - Wanda da Rosa, pcr 341032
                    ** Take out the lefData->last '"', the 1st will be skip later
                    */
                    if (*prS == '"') {
                        *prS = '\0';
                    } else
                        *s++ = '\0';
                    lefData->current_stack[lefData->input_level] = ch;
                    if (lefData->lefDebug[11])
                        printf("Stack[%d]: <%s>, dm=%d\n",
                               lefData->input_level, save, lefData->lefDumbMode);
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
    return FALSE;        // if we get here, we ran out of input levels 
}


// Increment current position of buffer pointer. 
// Double buffer size if curPos is out of boundary.
static inline void  
IncCurPos(char **curPos, char **buffer, int *bufferSize)
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


inline static void 
print_nlines(int lineNum)
{
    // call the callback line number function if it is set 
    if (lefSettings->LineNumberFunction &&
        (lineNum % lefSettings->DeltaNumberLines) == 0)
        lefSettings->LineNumberFunction(lineNum);
}


static int
GetToken(char **buffer, int *bufferSize)
{
    char *s = *buffer;
    int ch;

    lefData->lef_ntokens++;
    lefData->lefInvalidChar = 0;

    if (lefData->input_level >= 0) {            // if we are expanding an alias 
        if (GetTokenFromStack(s))    // try to get a token from it 
            return TRUE;                // if we get one, return it 
    }                                 // but if not, continue 

    // skip blanks and count lines 
    while ((ch = lefGetc()) != EOF) {
        // check if the file is encrypted and user allows to read 
        if (lefData->encrypted && !lefSettings->ReadEncrypted)
            ch = EOF;
        if (ch == '\n') {
            print_nlines(++lefData->lef_nlines);           
        }
        if (ch != ' ' && ch != '\t' && (lefData->lefNlToken || ch != '\n'))
            break;
    }

    if (ch == EOF)
        return FALSE;

    if (ch == '\n') {
        *s = ch;
        IncCurPos(&s, buffer, bufferSize);

        *s = '\0';
        if (lefData->lefDebug[11])
            printf("Newline token\n");
        return TRUE;
    }

    // now get the token 
    if (ch == '"') {
        do {
            /* 5/6/2008 - CCR 556818
            ** Check if the ch is a valid ascii character 0 =< ch < 128
            ** If not write out an error
            */
            /* 8/7/2008 - CCR 586175
            ** Some files may not end with \n or \0 or EOF as the lefData->last character
            ** The parser allows this char instead of error out
            */
            if ((ch < -1) || (ch > 127)) {
                lefData->lefInvalidChar = 1;
            }

            /* 8/22/2000 - Wanda da Rosa, pcr 333334
            ** save the previous char to allow backslash quote within quote
            */
            if (!lefSettings->DisPropStrProcess) {
                // 3/4/2008 - CCR 523879 - convert \\ to \, \" to ", \x to x 
                if (ch == '\\') {      // got a \, save the lefData->next char only 
                    ch = lefGetc();

                    if ((ch == '\n') || (ch == EOF)) {
                        *s = '\0';
                        lefError(6015, "Unexpected end of the LEF file.");
                        lefData->hasFatalError = 1;
                        return FALSE;
                    }
                }
            }

            // 5/5/2004 - pcr 704784
            // If name, or quote string is longer than current buffer size 
            // increase the buffer.                 
            *s = ch;
            IncCurPos(&s, buffer, bufferSize);

            ch = lefGetc();

            // 7/23/2003 - pcr 606558 - do not allow \n in a string instead 
            // of ; 
            if (ch == '\n') {
                print_nlines(++lefData->lef_nlines);
                // 2/2/2007 - PCR 909714, allow string to go more than 1 line 
                //            continue to parse 
            }

            if (ch == EOF) {
                *s = '\0';
                lefError(6015, "Unexpected end of the LEF file.");
                lefData->hasFatalError = 1;
                return FALSE;
            }
        } while (ch != '"');
        *s = '\0';
        /* 10/31/2006 - pcr 926068
        ** When it reaches to here, chances are it reaches the end ".
        ** Check if there is a space following the "
        */
        if (ch == '"') {
            ch = lefGetc();
            if (ch != ' ' && ch != EOF) {
                UNlefGetc(ch);
                lefData->spaceMissing = 1;
                return FALSE;
            }
            UNlefGetc(ch);
        }
        return TRUE;
    }

    if (lefData->namesCaseSensitive) {
        for (; ; ch = lefGetc()) {
            /* 5/6/2008 - CCR 556818
            ** Check if the ch is a valid ascii character 0 =< ch < 128
            ** If not write out an error
            */
            if ((ch < -1) || (ch > 127)) {
                lefData->lefInvalidChar = 1;
            }

            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
                break;

            *s = ch;
            IncCurPos(&s, buffer, bufferSize);
        }
    } else if (lefSettings->ShiftCase) { // we are case insensitive, use a different loop 
        for (; ; ch = lefGetc()) {
            /* 5/6/2008 - CCR 556818
            ** Check if the ch is a valid ascii character 0 =< ch < 128
            ** If not write out an error
            */
            if ((ch < -1) || (ch > 127)) {
                lefData->lefInvalidChar = 1;
            }

            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
                break;
                
            *s = (ch >= 'a' && ch <= 'z')? (ch - 'a' + 'A') : ch;
            IncCurPos(&s, buffer, bufferSize);
        }
    } else {
        for (; ; ch = lefGetc()) {
            /* 5/6/2008 - CCR 556818
            ** Check if the ch is a valid ascii character 0 =< ch < 128
            ** If not write out an error
            */
            if ((ch < -1) || (ch > 127)) {
                lefData->lefInvalidChar = 1;
            }

            if (ch == ' ' || ch == '\t' || ch == '\n' || ch == EOF)
                break;
   
            *s = ch;
            IncCurPos(&s, buffer, bufferSize);
        }
    }

    // If we got this far, the lefData->last char was whitespace 
    *s = '\0';
    if (ch != EOF)   // shouldn't ungetc an EOF 
        UNlefGetc(ch);
    return TRUE;
}

// creates an upper case copy of an array 
void
lefuc_array(char    *source,
            char    *dest)
{
    for (; *source != 0; )
        *dest++ = toupper(*source++);
    *dest = 0;
} 

void
lefStoreAlias()
{
    string     so_far; // contains alias contents as we build it 

    int tokenSize = 10240;
    char *aname = (char*)malloc(tokenSize);

    GetToken(&aname, &tokenSize);

    char *line = (char*) malloc(tokenSize);

    GetToken(&line, &tokenSize);     // should be "=" 
    
    char        *uc_line = (char*)malloc(tokenSize);

    if (strcmp(line, "=") != 0)
        lefError(1000, "Expecting '='");

    /* now keep getting lines till we get one that contains &ENDALIAS */
    for(char *p = NULL; p == NULL;){
        int i;
        char *s = line;
        for(i=0;i<tokenSize-1;i++) {
            int ch = lefGetc();
            if (ch == EOF) {
                lefError(1001, "End of file in &ALIAS");
                return;
            }

            *s++ = ch;
            
            if (ch == '\n') {
                print_nlines(++lefData->lef_nlines);             
                break;
            }
        }

        *s = '\0';
        lefuc_array(line, uc_line);             // make upper case copy 
        p = strstr(uc_line, "&ENDALIAS");       // look for END_ALIAS 
        
        if (p != NULL)                          // if we find it 
            *(line + (p - uc_line)) = 0;         // remove it from the line 

        so_far += line;
    }

    char *dup = (char*)malloc(strlen(so_far.c_str()) + 1);

    strcpy(dup, so_far.c_str());
    lefData->alias_set[strip_case(aname)] = dup;


    free(aname);
    free(line);
    free(uc_line);
}

int lefamper_lookup(char *token); // forward reference to this routine 

/* The main routine called by the YACC parser to get the lefData->next token.
 *    Returns 0 if no more tokens are available.
 *    Returns an integer for keywords (see the yacc_defines.h for values)
 *    Returns the character itself for punctuation
 *    Returns NUMBER for numeric looking tokens
 *    Returns T_STRING for anything else
 * If the global "lefData->lefDumbMode" is > 0, it reads the lefData->next token in "dumb mode".
 * In this case, it does not do keyword lookup, or attempt to read a token
 * as a number; if the token is not punctuation, it's a T_STRING.  Each token
 * read decrements lefData->lefDumbMode, so you can instruct the the lexer to read the
 * lefData->next N tokens in dumb mode by setting "lefData->lefDumbMode" to that value.
 *
 * Newlines are in general silently ignored.  If the global lefData->lefNlToken is
 * true, however, they are returned as the token K_NL.
 */
extern int lefsublex();

int
yylex()
{
    int v = lefsublex();

    if (lefData->lefDebug[13]) {
        if (v == 0) {
            printf("yylex NIL\n");
        } else if (v < 256) {
            printf("yylex char %c\n", v);
        } else if (v == QSTRING) {
            printf("yylex quoted string '%s'\n", yylval.string);
        } else if (v == T_STRING) {
            printf("yylex string '%s'\n", yylval.string);
        } else if (v == NUMBER) {
            printf("yylex number %f\n", yylval.dval);
        } else {
            printf("yylex keyword %s\n", lef_kywd(v));
        }
    }

    // At 5.6, lefData->doneLib is always true, since "END LIBRARY" is optional 
    if ((v == 0) && (!lefData->doneLib)) {
        if (!lefData->spaceMissing) {
            lefError(1002, "Incomplete lef file.");
            lefData->hasFatalError = 1;
        }

        return (-1);
    }

    return v;
}

int
lefsublex()
{

    char    fc;
    char    *outStr;

    strcpy(lefData->pv_token, lefData->current_token);   // save the previous token 

    /* First, we eat all the things the parser should be unaware of.
     * This includes:
     * a) Comments
     * b) &alias definitions
     * c) &alias expansions
     */

    for (; ; ) {
        if (!GetToken(&lefData->current_token, &lefData->tokenSize))    // get a raw token 
            return 0;

        // Token size can change. Do preventive re-alloc. 
        lefData->uc_token = (char*) realloc(lefData->uc_token, lefData->tokenSize);
        lefData->pv_token = (char*) realloc(lefData->pv_token, lefData->tokenSize);

        fc = lefData->current_token[0];

        /* lefData->first, check for comments or &alias statements.  Comments we
         * ignore, and &alias statements are eaten and recorded by the lexer.
         */
        if (fc == lefSettings->CommentChar) {
            for (fc = lefGetc(); ; fc = lefGetc()) {// so skip to the end of line 
                if (fc == EOF)
                    return 0;
                if (fc == '\n') {
                    print_nlines(++lefData->lef_nlines);
                    break;
                }
            }
        } else if (fc == '&') {
            // begins with &.  If &alias, read contents and 
            // store them.  Otherwise it's a define, or a macro use. 
            const char *cptr;
            lefuc_array(lefData->current_token, lefData->uc_token);
            if (strcmp(lefData->uc_token, "&ALIAS") == 0)
                lefStoreAlias();    // read and store the alias 
            else if (strncmp(lefData->uc_token, "&DEFINE", 7) == 0) {
                lefData->inDefine = 1;       // it is a define statement 
                break;
            } else if (lefGetAlias(lefData->current_token, &cptr))
                lefData->current_stack[++lefData->input_level] = cptr;
            else if (lefGetStringDefine(lefData->current_token, &cptr) && !lefData->inDefine)
                lefData->current_stack[++lefData->input_level] = cptr;
            else
                break;    // begins with &, but not an &alias defn. or use. 
        } else
            break;    // does not begin with CommentChar or '&' 
    }

    if (lefData->lefInvalidChar) {
        outStr = (char*) lefMalloc(500 + strlen(lefData->current_token));
        sprintf(outStr, "Invalid characters found in \'%s\'.\nThese characters might have created by character types other than English.",
                lefData->current_token);
        lefError(1008, outStr);
        lefFree(outStr);
        return 0;
    }

    if (lefData->ge56almostDone && (strcmp(lefData->current_token, "END") == 0)) {
        // Library has BEGINEXT and also end with END LIBRARY 
        // Use END LIBRARY to indicate the end of the library 
        lefData->ge56almostDone = 0;
    }

    if ((lefData->doneLib && lefData->versionNum < 5.6) || // END LIBRARY is passed for pre 5.6 
        (lefData->ge56almostDone && (strcmp(lefData->current_token, "END"))) || // after EXT, not 
        // follow by END 
        (lefData->ge56done)) {                     // END LIBRARY is passed for >= 5.6 
        fc = EOF;
        lefInfo(3000, "There are still data after the END LIBRARY");
        return 0;
    }

    if (fc == '\"') {
        yylval.string = ringCopy(&(lefData->current_token[1]));
        return QSTRING;
    }

    // at this point we've read a token 
    // printf("Token is %s\n", token); 
    lefData->lefDumbMode--;
    lefData->lefNoNum--;
    if (isdigit(fc) || fc == '.' || (fc == '-' && lefData->current_token[1] != '\0')) {
        char *ch;
        yylval.dval = strtod(lefData->current_token, &ch);
        if (lefData->lefNoNum < 0 && *ch == '\0') {    // did we use the whole string? 
                return NUMBER;
        } else {  // failed integer conversion, try floating point 
                yylval.string = ringCopy(lefData->current_token);  // NO, it's a string 
                return T_STRING;
            }
        }

    // 5/17/2004 - Special checking for nondefaultrule 
    if (lefData->lefNdRule && (strcmp(lefData->current_token, "END") != 0)) {
        if (strcmp(lefData->current_token, lefData->ndName) == 0) {
            yylval.string = ringCopy(lefData->current_token);  // a nd rule name 
            return T_STRING;
        } else {
            // Can be NONDEFAULTRULE END without name, this case, string 
            // should be a reserve word  or name is incorrect 
            // lefData->first check if it is a reserve word 
            lefData->lefDumbMode = -1;
        }
    }
    // if we are dumb mode, all we return is punctuation and strings & numbers
    // until we see the lefData->next '+' or ';' token 
    if (lefData->lefDumbMode >= 0) {
        if (lefData->current_token[1] == '\0' && (fc == '(' || fc == ')' || fc == '+' || fc == ';' || fc == '*')) {
            if (fc == ';' || fc == '+')
                lefData->lefDumbMode = 0;
            return (int) fc;
        }
        if (lefData->lefNewIsKeyword && strcmp(lefData->current_token, "NEW") == 0) {
            return K_NEW; // even in dumb mode, we must see the NEW token 
        }
        yylval.string = ringCopy(lefData->current_token);
        // 5/17/2004 - Special checking for nondefaultrule 
        if (lefData->lefNdRule) {
            if (strcmp(lefData->current_token, lefData->ndName) == 0)
                return T_STRING;
            else {
                // Can be NONDEFAULTRULE END without name, this case, string 
                // should be a reserve word  or name is incorrect 
                // lefData->first check if it is a reserve word 
            }
        }
        return T_STRING;
    }

    // if we get here we are in smart mode.  Parse token 
    if (isalpha(fc) || fc == '&' || fc == '_') {
        int     result;
        char    *ch, *uch;

        for (ch = lefData->current_token, uch = lefData->uc_token; *ch != '\0'; ch++)
            *uch++ = toupper(*ch);
        *uch = '\0';

        lefData->Hist_text.resize(0);

        if (lefGetKeyword(lefData->uc_token, &result)) {
            if (K_HISTORY == result) {  // history - get up to ';' 
                int c;
                int prev;
                prev = ' ';
                for (; ; ) {
                    c = lefGetc();

                    if (c == EOF) {
                        lefError(6015, "Unexpected end of the LEF file.");
                        lefData->hasFatalError = 1;
                        break;
                    }

                    if (c == ';' &&
                        (prev == ' ' || prev == '\t' || prev == '\n'))
                        break;
                    if (c == '\n') {
                        // call the callback line number function if it is set 
                        print_nlines(++lefData->lef_nlines);
                    }
                    prev = c;
                    lefData->Hist_text.push_back(c);
                }
                lefData->Hist_text.push_back('\0');
            } else if (K_BEGINEXT == result) { // extension, get up to end 
                int cc;
                int foundTag = 0;
                int notEmpTag = 0;
                int begQuote = 0;
                // First make sure there is a name after BEGINEXT within quote 
                // BEGINEXT "name" 
                for (cc = lefGetc(); ; cc = lefGetc()) {
                    if (cc == EOF)
                        break;   // lef file may not have END LIB 
                    if (cc == '\n') {
                        if (!foundTag) {
                            lefError(1003, "tag is missing for BEGINEXT");
                            break;
                        }
                    } else {
                        // Make sure the tag is quoted 
                        lefData->Hist_text.push_back(cc);
                        if (cc != ' ') {
                            if (cc == '\"') {   // found a quote 
                                if (!begQuote)
                                    begQuote = 1;
                                else if (notEmpTag) {
                                    foundTag = 1;
                                    break;      // Found the quoted tag 
                                } else {
                                    lefError(1004, "Tag for BEGINEXT is empty");
                                    break;
                                }
                            } else if (!begQuote) {   // anything but a quote 
                                lefError(1005, "\" is missing in tag");
                                break;
                            } else             // anything but a quote and there 
                                notEmpTag = 1;   // is already a quote 
                        }
                    }
                }
                if (foundTag) {
                    // We have handle with the tag, just read the rest until 
                    // ENDEXT 
                    begQuote = 0;
                    for (cc = lefGetc(); ; cc = lefGetc()) {
                        if (cc == EOF)
                            break;   // lef file may not have END LIB 
                        if (cc == '\n') {
                            // call the callback line number function if it is set 
                            print_nlines(++lefData->lef_nlines);
                        } else if (cc == '\"') {
                            if (!begQuote)
                                begQuote = 1;
                            else
                                begQuote = 0;
                        }
                        lefData->Hist_text.push_back(cc);     
                        int histTextSize = lefData->Hist_text.size();

                        if (histTextSize >= 6 && memcmp(&lefData->Hist_text[histTextSize - 6 ], "ENDEXT", 6) == 0) { 
                            if (begQuote)
                                lefError(1006, "Ending \" is missing");
                            break;
                        } else if (histTextSize >= 11 && memcmp(&lefData->Hist_text[histTextSize - 11 ], "END LIBRARY", 11) == 0) {
                            lefError(1007, "ENDEXT is missing");
                            return 1;
                        }
                    }
                }
                lefData->Hist_text.push_back('\0');
            }
            return result;        // YES, return its value 
        } else {  // we don't have a keyword.  
            if (fc == '&')
                return lefamper_lookup(lefData->current_token);
            yylval.string = ringCopy(lefData->current_token);  // NO, it's a string 
            return T_STRING;
        }
    } else {  // it should be a punctuation character 
        if (lefData->current_token[1] != '\0') {
            if (strcmp(lefData->current_token, ">=") == 0)
                return K_GE;
            if (strcmp(lefData->current_token, "<=") == 0)
                return K_LE;
            if (strcmp(lefData->current_token, "<>") == 0)
                return K_NE;
            if (lefData->current_token[0] == ';') { // we got ';TOKEN' which is not allowed by
                //';' cannot be attached to other tokens.
                lefError(1009, "Symbol ';' should be separated by space(s).");
                return 0;
                // strcpy(saved_token, &token[1]); // the standard syntax, but 
                // stack[++lefData->input_level] = saved_token; // C3 and GE support. 
            } else if (lefData->current_token[0] == '_') {// name starts with _, return as T_STRING 
                yylval.string = ringCopy(lefData->current_token);
                return T_STRING;
            } else {
                lefError(6016, "Odd punctuation found.");
                lefData->hasFatalError = 1;
                return 0;
            }
        } else if (strlen(lefData->current_token) > 2
                   || strlen(lefData->current_token) == 0) {
            lefError(6016, "Odd punctuation found.");
            lefData->hasFatalError = 1;
            return 0;
        }
        return (int) lefData->current_token[0];
    }
}

/* We have found a token beginning with '&'.  If it has been previously
   defined, substitute the definition.  Otherwise return it. */
int
lefamper_lookup(char *tkn)
{
    double        dptr;
    int           result;
    const char    *cptr;

    // printf("Amper_lookup: %s\n", tkn); 

    // &define returns a number 
    if (lefGetDoubleDefine(tkn, &dptr)) {
        yylval.dval = dptr;
        return NUMBER;
    }
    // &defineb returns TRUE or FALSE, encoded as K_TRUE or K_FALSE 
    if (lefGetIntDefine(tkn, &result))
        return result;
    // &defines returns a T_STRING 
    if (lefGetStringDefine(tkn, &cptr)) {
        if (lefGetKeyword(cptr, &result))
            return result;
        yylval.string = ringCopy(cptr);
        return (cptr[0] == '\"' ? QSTRING : T_STRING);
    }
    // if none of the above, just return the token. 
    yylval.string = ringCopy(tkn);
    return T_STRING;
}

void
lefError(int        msgNum,
         const char *s)
{
    char        *str;
    const char  *curToken = isgraph(lefData->current_token[0]) ? lefData->current_token
                                                               : "<unprintable>";
    const char  *pvToken = isgraph(lefData->pv_token[0]) ? lefData->pv_token
                                                         : "<unprintable>";
    int         len = strlen(curToken) - 1;
    int         pvLen = strlen(pvToken) - 1;

    if (lefData->hasFatalError) 
        return;
    if ((lefSettings->TotalMsgLimit > 0) && (lefData->lefErrMsgPrinted >= lefSettings->TotalMsgLimit))
        return;
    if (lefSettings->MsgLimit[msgNum] > 0) {
        if (lefData->msgLimit[0][msgNum] >= lefSettings->MsgLimit[msgNum]) // over the limit 
            return;
        lefData->msgLimit[0][msgNum] = lefData->msgLimit[0][msgNum] + 1;
    }

    // PCR 690679, probably missing space before a ';' 
    if (strcmp(s, "parse error") == 0) {
        if ((len > 1) && (lefData->current_token[len] == ';')) {
            str = (char*) lefMalloc(len + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s>, space is missing before <;>\n",
                    msgNum, s, lefData->lefrFileName, lefData->lef_nlines, curToken);
        } else if ((pvLen > 1) && (lefData->pv_token[pvLen] == ';')) {
            str = (char*) lefMalloc(pvLen + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s>, space is missing before <;>\n",
                    msgNum, s, lefData->lefrFileName, lefData->lef_nlines - 1, pvToken);
        } else if ((lefData->current_token[0] == '"') && (lefData->spaceMissing)) {
            // most likely space is missing after the end " 
            str = (char*) lefMalloc(len + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s\">, space is missing between the closing \" of the string and ;.\n",
                    1010, s, lefData->lefrFileName, lefData->lef_nlines, curToken);
            lefData->spaceMissing = 0;
        } else {
            str = (char*) lefMalloc(len + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): Lef parser has encountered an error in file %s at line %d, on token %s.\nProblem can be syntax error on the lef file or an invalid parameter name.\nDouble check the syntax on the lef file with the LEFDEF Reference Manual.\n",
                    msgNum, lefData->lefrFileName, lefData->lef_nlines, curToken);
        }
    } else if (strcmp(s, "syntax error") == 0) {  // linux machines 
        if ((len > 1) && (lefData->current_token[len] == ';')) {
            str = (char*) lefMalloc(len + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s>, space is missing before <;>\n",
                    msgNum, s, lefData->lefrFileName, lefData->lef_nlines, curToken);
        } else if ((pvLen > 1) && (lefData->pv_token[pvLen] == ';')) {
            str = (char*) lefMalloc(pvLen + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s>, space is missing before <;>\n",
                    msgNum, s, lefData->lefrFileName, lefData->lef_nlines - 1, pvToken);
        } else if ((lefData->current_token[0] == '"') && (lefData->spaceMissing)) {
            // most likely space is missing after the end " 
            str = (char*) lefMalloc(len + strlen(s) + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): %s, see file %s at line %d\nLast token was <%s\">, space is missing between the closing \" of the string and ;.\n",
                    1011, s, lefData->lefrFileName, lefData->lef_nlines, curToken);
            lefData->spaceMissing = 0;
        } else {
            str = (char*) lefMalloc(len + strlen(lefData->lefrFileName) + 350);
            sprintf(str, "ERROR (LEFPARS-%d): Lef parser has encountered an error in file %s at line %d, on token %s.\nProblem can be syntax error on the lef file or an invalid parameter name.\nDouble check the syntax on the lef file with the LEFDEF Reference Manual.\n",
                    msgNum, lefData->lefrFileName, lefData->lef_nlines, curToken);
        }
    } else {
        str = (char*) lefMalloc(len + strlen(s) + strlen(lefData->lefrFileName) + 350);
        sprintf(str, "ERROR (LEFPARS-%d): %s Error in file %s at line %d, on token %s.\n",
                msgNum, s, lefData->lefrFileName, lefData->lef_nlines, curToken);
    }
    fflush(stdout);
    lefiError(1, msgNum, str);
    free(str);
    lefData->lefErrMsgPrinted++;
    // Not really error, error numbers between 1300 & 1499, those errors 
    // are not from lef.y or the parser 
    if ((msgNum < 1300) || (msgNum > 1499))
        lefData->lef_errors++;
}

// yyerror is called by bison.simple, 5 locations will call this function 
void
yyerror(const char *s)
{

    lefError(lefData->msgCnt++, s);
}

// All info starts with 3000 
// All info within lefInfo starts with 3500 
void
lefInfo(int         msgNum,
        const char  *s)
{
    int disableStatus = lefSettings->suppresMsg(msgNum);

    if (disableStatus == 1) {
        char msgStr[60];
        sprintf(msgStr, "Message (LEFPARS-%d) has been suppressed from output.", msgNum);
        lefWarning(2502, msgStr);
        return;
    } else if (disableStatus == 2) {
        return;
    }

    if ((lefSettings->TotalMsgLimit > 0) && (lefData->lefInfoMsgPrinted >= lefSettings->TotalMsgLimit))
        return;
    if (lefSettings->MsgLimit[msgNum] > 0) {
        if (lefData->msgLimit[0][msgNum] >= lefSettings->MsgLimit[msgNum]) { // over the limit 
            char msgStr[100];
            if (lefData->msgLimit[1][msgNum]) // already printed out warning 
                return;
            lefData->msgLimit[1][msgNum] = 1;
            sprintf(msgStr,
                    "Message (LEFPARS-%d) has exceeded the message display limit of %d",
                    msgNum, lefSettings->MsgLimit[msgNum]);
            lefWarning(2503, msgStr);
            return;
        }
        lefData->msgLimit[0][msgNum] = lefData->msgLimit[0][msgNum] + 1;
    }
    lefData->lefInfoMsgPrinted++;

    if (lefSettings->WarningLogFunction) {
        char *str = (char*) lefMalloc(strlen(lefData->current_token) + strlen(s) + strlen(lefData->lefrFileName)
                                      + 350);
        sprintf(str, "INFO (LEFPARS-%d): %s See file %s at line %d.\n",
                msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
        (*lefSettings->WarningLogFunction)(str);
        free(str);
    } else if (lefData->lefrLog) {
        fprintf(lefData->lefrLog, "INFO (LEFPARS-%d): %s See file %s at line %d\n",
                msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
    } else {
        if (!lefData->hasOpenedLogFile) {
            if ((lefData->lefrLog = fopen("lefRWarning.log", "w")) == 0) {
                printf("WARNING (LEFPARS-3500): Unable to open the file lefRWarning.log in %s.\n",
                       getcwd(NULL, 64));
                printf("Info messages will not be printed.\n");
            } else {
                lefData->hasOpenedLogFile = 1;
                fprintf(lefData->lefrLog, "Info from file: %s\n\n", lefData->lefrFileName);
                fprintf(lefData->lefrLog, "INFO (LEFPARS-%d): %s See file %s at line %d\n",
                        msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
            }
        } else {
            if ((lefData->lefrLog = fopen("lefRWarning.log", "a")) == 0) {
                printf("WARNING (LEFPARS-3500): Unable to open the file lefRWarning.log in %s.\n",
                       getcwd(NULL, 64));
                printf("Info messages will not be printed.\n");
            } else {
                fprintf(lefData->lefrLog, "\nInfo from file: %s\n\n", lefData->lefrFileName);
                fprintf(lefData->lefrLog, "INFO (LEFPARS-%d): %s See file %s at line %d\n",
                        msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
            }
        }
    }
}

// All warning starts with 2000 
// All warning within lefWarning starts with 2500 
void
lefWarning(int          msgNum,
           const char   *s)
{
    if (lefSettings->dAllMsgs)   // all messages are suppressed 
        return;

    if ((msgNum != 2502) && (msgNum != 2503)) {
        int disableStatus = lefSettings->suppresMsg(msgNum);

        if (disableStatus == 1) {
            char msgStr[60];
            sprintf(msgStr, "Message (LEFPARS-%d) has been suppressed from output.", msgNum);
            lefWarning(2502, msgStr);
            return;
        } else if (disableStatus == 2) {
            return;
        }        
    }

    if ((lefSettings->TotalMsgLimit > 0) && (lefData->lefWarnMsgPrinted >= lefSettings->TotalMsgLimit))
        return;
    if (lefSettings->MsgLimit[msgNum] > 0) {
        if (lefData->msgLimit[0][msgNum] >= lefSettings->MsgLimit[msgNum]) { // over the limit 
            char msgStr[100];
            if (lefData->msgLimit[1][msgNum]) // already printed out warning 
                return;
            lefData->msgLimit[1][msgNum] = 1;
            sprintf(msgStr,
                    "Message (LEFPARS-%d) has exceeded the message display limit of %d",
                    msgNum, lefSettings->MsgLimit[msgNum]);
            lefWarning(2503, msgStr);
            return;
        }
        lefData->msgLimit[0][msgNum] = lefData->msgLimit[0][msgNum] + 1;
    }
    lefData->lefWarnMsgPrinted++;

    if (lefSettings->WarningLogFunction) {
        char *str = (char*) lefMalloc(strlen(lefData->current_token) + strlen(s) + strlen(lefData->lefrFileName)
                                      + 350);
        sprintf(str, "WARNING (LEFPARS-%d): %s See file %s at line %d.\n",
                msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
        (*lefSettings->WarningLogFunction)(str);
        free(str);
    } else if (lefData->lefrLog) {
        fprintf(lefData->lefrLog, "WARNING (LEFPARS-%d): %s See file %s at line %d\n",
                msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
    } else {
        if (!lefData->hasOpenedLogFile) {
            if ((lefData->lefrLog = fopen("lefRWarning.log", "w")) == 0) {
                printf("WARNING (LEFPARS-2500): Unable to open the file lefRWarning.log in %s.\n",
                       getcwd(NULL, 64));
                printf("Warning messages will not be printed.\n");
            } else {
                lefData->hasOpenedLogFile = 1;
                fprintf(lefData->lefrLog, "Warnings from file: %s\n\n", lefData->lefrFileName);
                fprintf(lefData->lefrLog, "WARNING (LEFPARS-%d): %s See file %s at line %d\n",
                        msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
            }
        } else {
            if ((lefData->lefrLog = fopen("lefRWarning.log", "a")) == 0) {
                printf("WARNING (LEFPARS-2501): Unable to open the file lefRWarning.log in %s.\n",
                       getcwd(NULL, 64));
                printf("Warning messages will not be printed.\n");
            } else {
                fprintf(lefData->lefrLog, "\nWarnings from file: %s\n\n", lefData->lefrFileName);
                fprintf(lefData->lefrLog, "WARNING (LEFPARS-%d): %s See file %s at line %d\n",
                        msgNum, s, lefData->lefrFileName, lefData->lef_nlines);
            }
        }
    }
    lefData->lef_warnings++;
}

void *
lefMalloc(size_t lef_size)
{
    void *mallocVar;

    if (lefSettings && lefSettings->MallocFunction)
        return (*lefSettings->MallocFunction)(lef_size);
    else {
        mallocVar = (void*) malloc(lef_size);
        if (!mallocVar) {
            fprintf(stderr, "ERROR (LEFPARS-1009): Not enough memory, stop parsing!\n");
            exit(1);
        }
        return mallocVar;
    }
}

void *
lefRealloc(void *name,
           size_t  lef_size)
{
    if (lefSettings->ReallocFunction)
        return (*lefSettings->ReallocFunction)(name, lef_size);
    else
        return (void*) realloc(name, lef_size);
}

void
lefFree(void *name)
{
    if (lefSettings && lefSettings->FreeFunction)
        (*lefSettings->FreeFunction)(name);
    else
        free(name);
}

char *
lefaddr(const char *in)
{
    return (char*) in;
}

void
lefSetNonDefault(const char *nd_name)
{
    lefData->ndName = (char*)malloc(strlen(nd_name)+1);
    strcpy(lefData->ndName, nd_name);
}

void
lefUnsetNonDefault()
{
    lefData->lefNdRule = 0;
    free(lefData->ndName);
}

char *
lef_kywd(int num)
{
    char *a;
    switch (num) {
    case K_HISTORY:
        a = lefaddr("HISTORY");
        break;
    case K_ABUT:
        a = lefaddr("ABUT");
        break;
    case K_ABUTMENT:
        a = lefaddr("ABUTMENT");
        break;
    case K_ACTIVE:
        a = lefaddr("ACTIVE");
        break;
    case K_ANALOG:
        a = lefaddr("ANALOG");
        break;
    case K_ANTENNAAREAFACTOR:
        a = lefaddr("ANTENNAAREAFACTOR");
        break;
    case K_ANTENNALENGTHFACTOR:
        a = lefaddr("ANTENNALENGTHFACTOR");
        break;
    case K_ARRAY:
        a = lefaddr("ARRAY");
        break;
    case K_BLOCK:
        a = lefaddr("BLOCK");
        break;
    case K_BOTTOMLEFT:
        a = lefaddr("BOTTOMLEFT");
        break;
    case K_BOTTOMRIGHT:
        a = lefaddr("BOTTOMRIGHT");
        break;
    case K_BUFFER:
        a = lefaddr("BUFFER");
        break;
    case K_BY:
        a = lefaddr("BY");
        break;
    case K_CAPACITANCE:
        a = lefaddr("CAPACITANCE");
        break;
    case K_CAPMULTIPLIER:
        a = lefaddr("CAPMULTIPLIER");
        break;
    case K_CLASS:
        a = lefaddr("CLASS");
        break;
    case K_CLOCK:
        a = lefaddr("CLOCK");
        break;
    case K_COLUMNMAJOR:
        a = lefaddr("COLUMNMAJOR");
        break;
    case K_CORE:
        a = lefaddr("CORE");
        break;
    case K_CORNER:
        a = lefaddr("CORNER");
        break;
    case K_COVER:
        a = lefaddr("COVER");
        break;
    case K_CPERSQDIST:
        a = lefaddr("CPERSQDIST");
        break;
    case K_CURRENT:
        a = lefaddr("CURRENT");
        break;
    case K_CURRENTDEN:
        a = lefaddr("CURRENTDEN");
        break;
    case K_CURRENTSOURCE:
        a = lefaddr("CURRENTSOURCE");
        break;
    case K_CUT:
        a = lefaddr("CUT");
        break;
    case K_DEFAULT:
        a = lefaddr("DEFAULT");
        break;
    case K_DATABASE:
        a = lefaddr("DATABASE");
        break;
    case K_DIELECTRIC:
        a = lefaddr("DIELECTRIC");
        break;
    case K_DIRECTION:
        a = lefaddr("DIRECTION");
        break;
    case K_DO:
        a = lefaddr("DO");
        break;
    case K_EDGECAPACITANCE:
        a = lefaddr("EDGECAPACITANCE");
        break;
    case K_EEQ:
        a = lefaddr("EEQ");
        break;
    case K_END:
        a = lefaddr("END");
        break;
    case K_ENDCAP:
        a = lefaddr("ENDCAP");
        break;
    case K_FALL:
        a = lefaddr("FALL");
        break;
    case K_FALLCS:
        a = lefaddr("FALLCS");
        break;
    case K_FALLT0:
        a = lefaddr("FALLT0");
        break;
    case K_FALLSATT1:
        a = lefaddr("FALLSATT1");
        break;
    case K_FALLRS:
        a = lefaddr("FALLRS");
        break;
    case K_FALLSATCUR:
        a = lefaddr("FALLSATCUR");
        break;
    case K_FALLTHRESH:
        a = lefaddr("FALLTHRESH");
        break;
    case K_FEEDTHRU:
        a = lefaddr("FEEDTHRU");
        break;
    case K_FIXED:
        a = lefaddr("FIXED");
        break;
    case K_FOREIGN:
        a = lefaddr("FOREIGN");
        break;
    case K_FROMPIN:
        a = lefaddr("FROMPIN");
        break;
    case K_FUNCTION:
        a = lefaddr("FUNCTION");
        break;
    case K_GENERATE:
        a = lefaddr("GENERATE");
        break;
    case K_GENERATOR:
        a = lefaddr("GENERATOR");
        break;
    case K_GROUND:
        a = lefaddr("GROUND");
        break;
    case K_HEIGHT:
        a = lefaddr("HEIGHT");
        break;
    case K_HORIZONTAL:
        a = lefaddr("HORIZONTAL");
        break;
    case K_INOUT:
        a = lefaddr("INOUT");
        break;
    case K_INPUT:
        a = lefaddr("INPUT");
        break;
    case K_INPUTNOISEMARGIN:
        a = lefaddr("INPUTNOISEMARGIN");
        break;
    case K_COMPONENTPIN:
        a = lefaddr("COMPONENTPIN");
        break;
    case K_INTRINSIC:
        a = lefaddr("INTRINSIC");
        break;
    case K_INVERT:
        a = lefaddr("INVERT");
        break;
    case K_INVERTER:
        a = lefaddr("INVERTER");
        break;
    case K_IRDROP:
        a = lefaddr("IRDROP");
        break;
    case K_ITERATE:
        a = lefaddr("ITERATE");
        break;
    case K_IV_TABLES:
        a = lefaddr("IV_TABLES");
        break;
    case K_LAYER:
        a = lefaddr("LAYER");
        break;
    case K_LEAKAGE:
        a = lefaddr("LEAKAGE");
        break;
    case K_LEQ:
        a = lefaddr("LEQ");
        break;
    case K_LIBRARY:
        a = lefaddr("LIBRARY");
        break;
    case K_MACRO:
        a = lefaddr("MACRO");
        break;
    case K_MATCH:
        a = lefaddr("MATCH");
        break;
    case K_MAXDELAY:
        a = lefaddr("MAXDELAY");
        break;
    case K_MAXLOAD:
        a = lefaddr("MAXLOAD");
        break;
    case K_METALOVERHANG:
        a = lefaddr("METALOVERHANG");
        break;
    case K_MILLIAMPS:
        a = lefaddr("MILLIAMPS");
        break;
    case K_MILLIWATTS:
        a = lefaddr("MILLIWATTS");
        break;
    case K_MINFEATURE:
        a = lefaddr("MINFEATURE");
        break;
    case K_MUSTJOIN:
        a = lefaddr("MUSTJOIN");
        break;
    case K_NAMEMAPSTRING:
        a = lefaddr("NAMEMAPSTRING");
        break;
    case K_NAMESCASESENSITIVE:
        a = lefaddr("NAMESCASESENSITIVE");
        break;
    case K_NANOSECONDS:
        a = lefaddr("NANOSECONDS");
        break;
    case K_NETS:
        a = lefaddr("NETS");
        break;
    case K_NEW:
        a = lefaddr("NEW");
        break;
    case K_NONDEFAULTRULE:
        a = lefaddr("NONDEFAULTRULE");
        break;
    case K_NONINVERT:
        a = lefaddr("NONINVERT");
        break;
    case K_NONUNATE:
        a = lefaddr("NONUNATE");
        break;
    case K_NOWIREEXTENSIONATPIN:
        a = lefaddr("NOWIREEXTENSIONATPIN");
        break;
    case K_OBS:
        a = lefaddr("OBS");
        break;
    case K_OHMS:
        a = lefaddr("OHMS");
        break;
    case K_OFFSET:
        a = lefaddr("OFFSET");
        break;
    case K_ORIENTATION:
        a = lefaddr("ORIENTATION");
        break;
    case K_ORIGIN:
        a = lefaddr("ORIGIN");
        break;
    case K_OUTPUT:
        a = lefaddr("OUTPUT");
        break;
    case K_OUTPUTNOISEMARGIN:
        a = lefaddr("OUTPUTNOISEMARGIN");
        break;
    case K_OUTPUTRESISTANCE:
        a = lefaddr("OUTPUTRESISTANCE");
        break;
    case K_OVERHANG:
        a = lefaddr("OVERHANG");
        break;
    case K_OVERLAP:
        a = lefaddr("OVERLAP");
        break;
    case K_OFF:
        a = lefaddr("OFF");
        break;
    case K_ON:
        a = lefaddr("ON");
        break;
    case K_OVERLAPS:
        a = lefaddr("OVERLAPS");
        break;
    case K_PAD:
        a = lefaddr("PAD");
        break;
    case K_PATH:
        a = lefaddr("PATH");
        break;
    case K_PATTERN:
        a = lefaddr("PATTERN");
        break;
    case K_PICOFARADS:
        a = lefaddr("PICOFARADS");
        break;
    case K_PIN:
        a = lefaddr("PIN");
        break;
    case K_PITCH:
        a = lefaddr("PITCH");
        break;
    case K_PLACED:
        a = lefaddr("PLACED");
        break;
    case K_POLYGON:
        a = lefaddr("POLYGON");
        break;
    case K_PORT:
        a = lefaddr("PORT");
        break;
    case K_POST:
        a = lefaddr("POST");
        break;
    case K_POWER:
        a = lefaddr("POWER");
        break;
    case K_PRE:
        a = lefaddr("PRE");
        break;
    case K_PULLDOWNRES:
        a = lefaddr("PULLDOWNRES");
        break;
    case K_PWL:
        a = lefaddr("PWL");
        break;
    case K_RECT:
        a = lefaddr("RECT");
        break;
    case K_RESISTANCE:
        a = lefaddr("RESISTANCE");
        break;
    case K_RESISTIVE:
        a = lefaddr("RESISTIVE");
        break;
    case K_RING:
        a = lefaddr("RING");
        break;
    case K_RISE:
        a = lefaddr("RISE");
        break;
    case K_RISECS:
        a = lefaddr("RISECS");
        break;
    case K_RISERS:
        a = lefaddr("RISERS");
        break;
    case K_RISESATCUR:
        a = lefaddr("RISESATCUR");
        break;
    case K_RISETHRESH:
        a = lefaddr("RISETHRESH");
        break;
    case K_RISESATT1:
        a = lefaddr("RISESATT1");
        break;
    case K_RISET0:
        a = lefaddr("RISET0");
        break;
    case K_RISEVOLTAGETHRESHOLD:
        a = lefaddr("RISEVOLTAGETHRESHOLD");
        break;
    case K_FALLVOLTAGETHRESHOLD:
        a = lefaddr("FALLVOLTAGETHRESHOLD");
        break;
    case K_ROUTING:
        a = lefaddr("ROUTING");
        break;
    case K_ROWMAJOR:
        a = lefaddr("ROWMAJOR");
        break;
    case K_RPERSQ:
        a = lefaddr("RPERSQ");
        break;
    case K_SAMENET:
        a = lefaddr("SAMENET");
        break;
    case K_SCANUSE:
        a = lefaddr("SCANUSE");
        break;
    case K_SHAPE:
        a = lefaddr("SHAPE");
        break;
    case K_SHRINKAGE:
        a = lefaddr("SHRINKAGE");
        break;
    case K_SIGNAL:
        a = lefaddr("SIGNAL");
        break;
    case K_SITE:
        a = lefaddr("SITE");
        break;
    case K_SIZE:
        a = lefaddr("SIZE");
        break;
    case K_SOURCE:
        a = lefaddr("SOURCE");
        break;
    case K_SPACER:
        a = lefaddr("SPACER");
        break;
    case K_SPACING:
        a = lefaddr("SPACING");
        break;
    case K_SPECIALNETS:
        a = lefaddr("SPECIALNETS");
        break;
    case K_STACK:
        a = lefaddr("STACK");
        break;
    case K_START:
        a = lefaddr("START");
        break;
    case K_STEP:
        a = lefaddr("STEP");
        break;
    case K_STOP:
        a = lefaddr("STOP");
        break;
    case K_STRUCTURE:
        a = lefaddr("STRUCTURE");
        break;
    case K_SYMMETRY:
        a = lefaddr("SYMMETRY");
        break;
    case K_TABLE:
        a = lefaddr("TABLE");
        break;
    case K_THICKNESS:
        a = lefaddr("THICKNESS");
        break;
    case K_TIEHIGH:
        a = lefaddr("TIEHIGH");
        break;
    case K_TIELOW:
        a = lefaddr("TIELOW");
        break;
    case K_TIEOFFR:
        a = lefaddr("TIEOFFR");
        break;
    case K_TIME:
        a = lefaddr("TIME");
        break;
    case K_TIMING:
        a = lefaddr("TIMING");
        break;
    case K_TO:
        a = lefaddr("TO");
        break;
    case K_TOPIN:
        a = lefaddr("TOPIN");
        break;
    case K_TOPLEFT:
        a = lefaddr("TOPLEFT");
        break;
    case K_TOPRIGHT:
        a = lefaddr("TOPRIGHT");
        break;
    case K_TOPOFSTACKONLY:
        a = lefaddr("TOPOFSTACKONLY");
        break;
    case K_TRISTATE:
        a = lefaddr("TRISTATE");
        break;
    case K_TYPE:
        a = lefaddr("TYPE");
        break;
    case K_UNATENESS:
        a = lefaddr("UNATENESS");
        break;
    case K_UNITS:
        a = lefaddr("UNITS");
        break;
    case K_USE:
        a = lefaddr("USE");
        break;
    case K_VARIABLE:
        a = lefaddr("VARIABLE");
        break;
    case K_VERTICAL:
        a = lefaddr("VERTICAL");
        break;
    case K_VHI:
        a = lefaddr("VHI");
        break;
    case K_VIA:
        a = lefaddr("VIA");
        break;
    case K_VIARULE:
        a = lefaddr("VIARULE");
        break;
    case K_VLO:
        a = lefaddr("VLO");
        break;
    case K_VOLTAGE:
        a = lefaddr("VOLTAGE");
        break;
    case K_VOLTS:
        a = lefaddr("VOLTS");
        break;
    case K_WIDTH:
        a = lefaddr("WIDTH");
        break;
    case K_WIREEXTENSION:
        a = lefaddr("WIREEXTENSION");
        break;
    case K_X:
        a = lefaddr("X");
        break;
    case K_Y:
        a = lefaddr("Y");
        break;
    case K_R90:
        a = lefaddr("R90");
        break;
    case T_STRING:
        a = lefaddr("T_STRING");
        break;
    case QSTRING:
        a = lefaddr("QSTRING");
        break;
    case NUMBER:
        a = lefaddr("NUMBER");
        break;
    case K_N:
        a = lefaddr("N");
        break;
    case K_S:
        a = lefaddr("S");
        break;
    case K_E:
        a = lefaddr("E");
        break;
    case K_W:
        a = lefaddr("W");
        break;
    case K_FN:
        a = lefaddr("FN");
        break;
    case K_FS:
        a = lefaddr("FS");
        break;
    case K_FE:
        a = lefaddr("FE");
        break;
    case K_FW:
        a = lefaddr("FW");
        break;
    case K_USER:
        a = lefaddr("USER");
        break;
    case K_MASTERSLICE:
        a = lefaddr("MASTERSLICE");
        break;
    case K_ENDMACRO:
        a = lefaddr("ENDMACRO");
        break;
    case K_ENDMACROPIN:
        a = lefaddr("ENDMACROPIN");
        break;
    case K_ENDVIARULE:
        a = lefaddr("ENDVIARULE");
        break;
    case K_ENDVIA:
        a = lefaddr("ENDVIA");
        break;
    case K_ENDLAYER:
        a = lefaddr("ENDLAYER");
        break;
    case K_ENDSITE:
        a = lefaddr("ENDSITE");
        break;
    case K_CANPLACE:
        a = lefaddr("CANPLACE");
        break;
    case K_CANNOTOCCUPY:
        a = lefaddr("CANNOTOCCUPY");
        break;
    case K_TRACKS:
        a = lefaddr("TRACKS");
        break;
    case K_FLOORPLAN:
        a = lefaddr("FLOORPLAN");
        break;
    case K_GCELLGRID:
        a = lefaddr("GCELLGRID");
        break;
    case K_DEFAULTCAP:
        a = lefaddr("DEFAULTCAP");
        break;
    case K_MINPINS:
        a = lefaddr("MINPINS");
        break;
    case K_WIRECAP:
        a = lefaddr("WIRECAP");
        break;
    case K_STABLE:
        a = lefaddr("STABLE");
        break;
    case K_SETUP:
        a = lefaddr("SETUP");
        break;
    case K_HOLD:
        a = lefaddr("HOLD");
        break;
    case K_DEFINE:
        a = lefaddr("DEFINE");
        break;
    case K_DEFINES:
        a = lefaddr("DEFINES");
        break;
    case K_DEFINEB:
        a = lefaddr("DEFINEB");
        break;
    case K_IF:
        a = lefaddr("IF");
        break;
    case K_THEN:
        a = lefaddr("THEN");
        break;
    case K_ELSE:
        a = lefaddr("ELSE");
        break;
    case K_FALSE:
        a = lefaddr("FALSE");
        break;
    case K_TRUE:
        a = lefaddr("TRUE");
        break;
    case K_EQ:
        a = lefaddr("EQ");
        break;
    case K_NE:
        a = lefaddr("NE");
        break;
    case K_LE:
        a = lefaddr("LE");
        break;
    case K_LT:
        a = lefaddr("LT");
        break;
    case K_GE:
        a = lefaddr("GE");
        break;
    case K_GT:
        a = lefaddr("GT");
        break;
    case K_OR:
        a = lefaddr("OR");
        break;
    case K_AND:
        a = lefaddr("AND");
        break;
    case K_NOT:
        a = lefaddr("NOT");
        break;
    case K_DELAY:
        a = lefaddr("DELAY");
        break;
    case K_TABLEDIMENSION:
        a = lefaddr("TABLEDIMENSION");
        break;
    case K_TABLEAXIS:
        a = lefaddr("TABLEAXIS");
        break;
    case K_TABLEENTRIES:
        a = lefaddr("TABLEENTRIES");
        break;
    case K_TRANSITIONTIME:
        a = lefaddr("TRANSITIONTIME");
        break;
    case K_EXTENSION:
        a = lefaddr("EXTENSION");
        break;
    case K_PROPDEF:
        a = lefaddr("PROPDEF");
        break;
    case K_STRING:
        a = lefaddr("STRING");
        break;
    case K_INTEGER:
        a = lefaddr("INTEGER");
        break;
    case K_REAL:
        a = lefaddr("REAL");
        break;
    case K_RANGE:
        a = lefaddr("RANGE");
        break;
    case K_PROPERTY:
        a = lefaddr("PROPERTY");
        break;
    case K_VIRTUAL:
        a = lefaddr("VIRTUAL");
        break;
    case K_BUSBITCHARS:
        a = lefaddr("BUSBITCHARS");
        break;
    case K_VERSION:
        a = lefaddr("VERSION");
        break;
    case K_BEGINEXT:
        a = lefaddr("BEGINEXT");
        break;
    case K_ENDEXT:
        a = lefaddr("ENDEXT");
        break;
    case K_UNIVERSALNOISEMARGIN:
        a = lefaddr("UNIVERSALNOISEMARGIN");
        break;
    case K_EDGERATETHRESHOLD1:
        a = lefaddr("EDGERATETHRESHOLD1");
        break;
    case K_CORRECTIONTABLE:
        a = lefaddr("CORRECTIONTABLE");
        break;
    case K_EDGERATESCALEFACTOR:
        a = lefaddr("EDGERATESCALEFACTOR");
        break;
    case K_EDGERATETHRESHOLD2:
        a = lefaddr("EDGERATETHRESHOLD2");
        break;
    case K_VICTIMNOISE:
        a = lefaddr("VICTIMNOISE");
        break;
    case K_NOISETABLE:
        a = lefaddr("NOISETABLE");
        break;
    case K_EDGERATE:
        a = lefaddr("EDGERATE");
        break;
    case K_VICTIMLENGTH:
        a = lefaddr("VICTIMLENGTH");
        break;
    case K_CORRECTIONFACTOR:
        a = lefaddr("CORRECTIONFACTOR");
        break;
    case K_OUTPUTPINANTENNASIZE:
        a = lefaddr("OUTPUTPINANTENNASIZE");
        break;
    case K_INPUTPINANTENNASIZE:
        a = lefaddr("INPUTPINANTENNASIZE");
        break;
    case K_INOUTPINANTENNASIZE:
        a = lefaddr("INOUTPINANTENNASIZE");
        break;
    case K_TAPERRULE:
        a = lefaddr("TAPERRULE");
        break;
    case K_DIVIDERCHAR:
        a = lefaddr("DIVIDERCHAR");
        break;
    case K_ANTENNASIZE:
        a = lefaddr("ANTENNASIZE");
        break;
    case K_ANTENNAMETALAREA:
        a = lefaddr("ANTENNAMETALAREA");
        break;
    case K_ANTENNAMETALLENGTH:
        a = lefaddr("ANTENNAMETALLENGTH");
        break;
    case K_RISESLEWLIMIT:
        a = lefaddr("RISESLEWLIMIT");
        break;
    case K_FALLSLEWLIMIT:
        a = lefaddr("FALLSLEWLIMIT");
        break;
    case K_MESSAGE:
        a = lefaddr("MESSAGE");
        break;
    case K_CREATEFILE:
        a = lefaddr("CREATEFILE");
        break;
    case K_OPENFILE:
        a = lefaddr("OPENFILE");
        break;
    case K_CLOSEFILE:
        a = lefaddr("CLOSEFILE");
        break;
    case K_WARNING:
        a = lefaddr("WARNING");
        break;
    case K_ERROR:
        a = lefaddr("ERROR");
        break;
    case K_FATALERROR:
        a = lefaddr("FATALERROR");
        break;
    case IF:
        a = lefaddr("IF");
        break;
    case LNOT:
        a = lefaddr("LNOT");
        break;
    case UMINUS:
        a = lefaddr("UMINUS");
        break;

    default:
        a = lefaddr("bogus");
    }
    return a;
}

END_LEFDEF_PARSER_NAMESPACE
