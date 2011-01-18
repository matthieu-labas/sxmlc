/*
    This file is part of sxmlc.

    sxmlc is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    sxmlc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with sxmlc.  If not, see <http://www.gnu.org/licenses/>.

	Copyright 2010 - Matthieu Labas
*/
#if defined(WIN32) || defined(WIN64)
#pragma warning(disable : 4996)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

#ifdef DBG_MEM
void* __malloc(size_t sz)
{
	void* p = malloc(sz);
	printf("0x%x: MALLOC (%d)\n", p, sz);
	return p;
}
void* __realloc(void* mem, size_t sz)
{
	void* p = realloc(mem, sz);
	printf("0x%x: REALLOC 0x%x (%d)\n", p, mem, sz);
	return p;
}
void __free(void* mem)
{
	printf("0x%x: FREE\n", mem);
	free(mem);
}
char* __strdup(const char* s)
{
	char* p = strdup(s);
	printf("0x%x: STRDUP (%d)\n", p, strlen(s));
	return p;
}
#endif

/* Dictionary of special characters and their HTML equivalent */
static struct _html_special_dict {
	char chr;		/* Original character */
	char* html;		/* Equivalent HTML string */
	int html_len;	/* 'strlen(html)' */
} HTML_SPECIAL_DICT[] = {
	{ '<', "&lt;", 4 },
	{ '>', "&gt;", 4 },
	{ '"', "&quot;", 6 },
	{ '&', "&amp;", 5 },
	{ '\0', NULL, 0 }, /* Terminator */
};

int _bgetc(DataSourceBuffer* ds)
{
	if (ds == NULL || ds->buf[ds->cur_pos] == '\0') return EOF;
	
	return (int)(ds->buf[ds->cur_pos++]);
}

int _beob(DataSourceBuffer* ds)
{

	if (ds == NULL || ds->buf[ds->cur_pos] == '\0') return true;

	return false;
}

int read_line_alloc(void* in, DataSourceType in_type, char** line, int* sz_line, int i0, char from, char to, int keep_fromto, char interest, int* interest_count)
{
	int init_sz = 0;
	char c, *pt;
	int n, ret;
	int (*mgetc)(void* ds) = (in_type == DATA_SOURCE_BUFFER ? (int(*)(void*))_bgetc : (int(*)(void*))fgetc);
	int (*meos)(void* ds) = (in_type == DATA_SOURCE_BUFFER ? (int(*)(void*))_beob : (int(*)(void*))feof);
	
	if (in == NULL || line == NULL) return 0;
	
	if (to == '\0') to = '\n';
	/* Search for character 'from' */
	if (interest_count != NULL) *interest_count = 0;
	while (true) {
		c = (char)mgetc(in);
		if (interest_count != NULL && c == interest) (*interest_count)++;
		/* Reaching EOF before 'to' char is not an error but should trigger 'line' alloc and init to '' */
		/* If 'from' is '\0', we stop here */
		if (c == from || c == CEOF || from == '\0') break;
	}
	
	if (sz_line == NULL) sz_line = &init_sz;
	
	if (*line == NULL || *sz_line == 0) {
		if (*sz_line == 0) *sz_line = MEM_INCR_RLA;
		*line = (char*)__malloc(*sz_line);
		if (*line == NULL) return 0;
	}
	if (i0 < 0) i0 = 0;
	if (i0 > *sz_line) return 0;
	
	n = i0;
	if (c == CEOF) { /* EOF reached before 'to' char => return the empty string */
		(*line)[n] = '\0';
		return meos(in) ? n : 0; /* Error if not EOF */
	}
	if (c != from || keep_fromto)
		(*line)[n++] = c;
	(*line)[n] = '\0';
	ret = 0;
	while (true) {
		c = (char)mgetc(in);
		if (interest_count != NULL && c == interest) (*interest_count)++;
		if (c == CEOF) { /* EOF or error */
			(*line)[n] = '\0';
			ret = meos(in) ? n : 0;
			break;
		}
		else {
			(*line)[n] = c;
			if (c != to || (keep_fromto && to != '\0' && c == to)) n++; /* If we reached the 'to' character and we keep it, we still need to add the extra '\0' */
			if (n >= *sz_line) { /* Too many characters for our line => realloc some more */
				*sz_line += MEM_INCR_RLA;
				pt = (char*)__realloc(*line, *sz_line);
				if (pt == NULL) {
					ret = 0;
					break;
				}
				else
					*line = pt;
			}
			(*line)[n] = '\0'; /* If we reached the 'to' character and we want to strip it, 'n' hasn't changed and 'line[n]' (which is 'to') will be replaced by '\0' */
			if (c == to) {
				ret = n;
				break;
			}
		}
	}
	
#if 0 /* Automatic buffer resize is deactivated */
	/* Resize line to the exact size */
	pt = (char*)__realloc(*line, n+1);
	if (pt != NULL)
		*line = pt;
#endif
	
	return ret;
}

/* --- */

char* strcat_alloc(char** src1, const char* src2)
{
	char* cat;
	int n;

	if (src1 == NULL || *src1 == src2) return NULL; /* Do not concatenate '*src1' with itself */

	/* Concatenate a NULL or empty string */
	if (src2 == NULL || *src2 == '\0') return *src1;

	n = (*src1 == NULL ? 0 : strlen(*src1)) + strlen(src2) + 1;
	cat = (char*)__realloc(*src1, n);
	if (cat == NULL) return NULL;
	if (*src1 == NULL) *cat = '\0';
	*src1 = cat;
	strcat(*src1, src2);

	return *src1;
}

char* strip_spaces(char* str, char repl_sq, char protect)
{
	char *p;
	int i, len;
	
	if (isspace(protect)) return NULL;
	
	/* 'p' to the first non-space */
	for (p = str; *p && isspace(*p); p++) ; /* No need to search for 'protect' as it is not a space */
	len = strlen(str);
	for (i = len-1; isspace(str[i]); i--) ;
	if (str[i] == protect) i++; /* If last non-space is the protection, keep the last space */
	str[i+1] = '\0'; /* New end of string to last non-space */
	
	if (repl_sq == '\0') {
		if (p == str && i == len) return str; /* Nothing to do */
		for (i = 0; (str[i] = *p) != '\0'; i++, p++) ; /* Copy 'p' to 'str' */
		return str;
	}
	
	/* Squeeze all spaces with 'repl_sq' */
	i = 0;
	while (*p) {
		if (isspace(*p)) {
			str[i++] = repl_sq;
			while (isspace(*++p)) ; /* Skips all next spaces */
		}
		else {
			if (*p == protect) p++;
			str[i++] = *p++;
		}
	}
	str[i] = '\0';
	
	return str;
}

char* str_unescape(char* str)
{
	int i, j;

	if (str == NULL) return NULL;

	for (i = j = 0; str[j]; j++) {
		if (str[j] == '\\') j++;
		str[i++] = str[j];
	}

	return str;
}

int split_left_right(char* str, char sep, int* l0, int* l1, int* i_sep, int* r0, int* r1, int ignore_spaces, int ignore_quotes)
{
	int n0, n1, is;
	char quote;

	if (str == NULL) return false;

	if (i_sep != NULL) *i_sep = -1;

	if (!ignore_spaces) ignore_quotes = false; /* No sense of ignore quotes if spaces are to be kept */

	/* Parse left part */

	if (ignore_spaces) {
		for (n0 = 0; str[n0] && isspace(str[n0]); n0++) ; /* Skip head spaces, n0 points to first non-space */
		if (ignore_quotes && isquote(str[n0])) { /* If quote is found, look for next one */
			quote = str[n0++];
			for (n1 = n0; str[n1] && str[n1] != quote; n1++) {
				if (str[n1] == '\\' && str[++n1] == '\0') break; /* Escape character (can be the last) */
			}
			for (is = n1 + 1; str[is] && isspace(str[is]); is++) ; /* '--' not to take quote into account */
		}
		else {
			for (n1 = n0; str[n1] && str[n1] != sep && !isspace(str[n1]); n1++) ; /* Search for separator or a space */
			for (is = n1; str[is] && isspace(str[is]); is++) ;
		}
	}
	else {
		n0 = 0;
		for (n1 = 0; str[n1] && str[n1] != sep; n1++) ; /* Search for separator only */
		if (str[n1] != sep) return false; /* Separator not found: malformed string */
		is = n1;
	}

	/* Here 'n0' is the start of left member, 'n1' is the character after the end of left member */

	if (l0 != NULL) *l0 = n0;
	if (l1 != NULL) *l1 = n1 - 1;
	if (i_sep != NULL) *i_sep = is;
	if (str[is] == '\0' || str[is+1] == '\0') { /* No separator => empty right member */
		if (r0 != NULL) *r0 = is;
		if (r1 != NULL) *r1 = is-1;
		if (i_sep != NULL) *i_sep = (str[is] == '\0' ? -1 : is);
		return true;
	}

	/* Parse right part */

	n0 = is + 1;
	if (ignore_spaces) {
		for (; str[n0] && isspace(str[n0]); n0++) ;
		if (ignore_quotes && isquote(str[n0])) quote = str[n0];
	}

	for (n1 = ++n0; str[n1]; n1++) {
		if (ignore_quotes && str[n1] == quote) break; /* Quote was reached */
		if (str[n1] == '\\' && str[++n1] == '\0') break; /* Escape character (can be the last) */
	}
	if (ignore_quotes && str[n1--] != quote) return false; /* Quote is not the same than earlier, '--' is not to take it into account */
	if (!ignore_spaces)
		while (str[++n1]) ; /* Jump down the end of the string */

	if (r0 != NULL) *r0 = n0;
	if (r1 != NULL) *r1 = n1;

	return true;
}

/* --- */

char* html2str(char* html, char* str)
{
	char *ps, *pd;
	int i;

	if (html == NULL) return NULL;

	if (str == NULL) str = html;
	
	/* Look for '&' and matches it to any of the recognized HTML pattern. */
	/* If found, replaces the '&' by the corresponding char. */
	/* 'p2' is the char to analyze, 'p1' is where to insert it */
	for (pd = str, ps = html; *ps; ps++, pd++) {
		if (*ps != '&') {
			if (pd != ps) *pd = *ps;
			continue;
		}
		
		for (i = 0; HTML_SPECIAL_DICT[i].chr; i++) {
			if (strncmp(ps, HTML_SPECIAL_DICT[i].html, HTML_SPECIAL_DICT[i].html_len)) continue;
			
			*pd = HTML_SPECIAL_DICT[i].chr;
			ps += HTML_SPECIAL_DICT[i].html_len-1;
			break;
		}
		/* If no string was found, simply copy the character */
		if (HTML_SPECIAL_DICT[i].chr == '\0' && pd != ps) *pd = *ps;
	}
	*pd = '\0';
	
	return str;
}

char* str2html(char* str, char* html)
{
	char *ps, *pd;
	int i;

	if (str == NULL || html == NULL) return NULL;

	if (html == str) return NULL; /* Not handled yet */

	for (ps = str, pd = html; *ps; ps++, pd++) {
		for (i = 0; HTML_SPECIAL_DICT[i].chr; i++) {
			if (*ps == HTML_SPECIAL_DICT[i].chr) {
				strcpy(pd, HTML_SPECIAL_DICT[i].html);
				pd += HTML_SPECIAL_DICT[i].html_len - 1;
				break;
			}
		}
		if (HTML_SPECIAL_DICT[i].chr == '\0' && pd != ps) *pd = *ps;
	}
	*pd = '\0';

	return str;
}

int strlen_html(char* str)
{
	int i, j, n;
	
	if (str == NULL) return 0;

	n = 0;
	for (i = 0; str[i]; i++) {
		for (j = 0; HTML_SPECIAL_DICT[j].chr; j++) {
			if (str[i] == HTML_SPECIAL_DICT[j].chr) {
				n += HTML_SPECIAL_DICT[j].html_len;
				break;
			}
		}
		if (HTML_SPECIAL_DICT[j].chr == '\0') n++;
	}

	return n;
}

int fprintHTML(FILE* f, char* str)
{
	char* p;
	int i, n;
	
	for (p = str, n = 0; *p; p++) {
		for (i = 0; HTML_SPECIAL_DICT[i].chr; i++) {
			if (*p != HTML_SPECIAL_DICT[i].chr) continue;
			fprintf(f, HTML_SPECIAL_DICT[i].html);
			n += HTML_SPECIAL_DICT[i].html_len;
			break;
		}
		if (HTML_SPECIAL_DICT[i].chr == '\0') {
			(void)fputc(*p, f);
			n++;
		}
	}
	
	return n;
}

int regstrcmp(char* str, char* pattern)
{
	char *p, *s;

	if (str == NULL && pattern == NULL) return true;

	if (str == NULL || pattern == NULL) return false;

	p = pattern;
	s = str;
	while (true) {
		switch (*p) {
			/* Any character matches, go to next one */
			case '?':
				p++;
				s++;
				break;

			/* Go to next character in pattern and wait until it is found in 'str' */
			case '*':
				for (; *p; p++) { /* Squeeze '**?*??**' to '*' */
					if (*p != '*' && *p != '?') break;
				}
				for (; *s; s++) {
					if (*s == *p) break;
				}
				break;

			/* NULL character on pattern has to be matched by 'str' */
			case 0:
				return *s ? false : true;

			default:
				if (*p == '\\') p++; /* Escape character */
				if (*p++ != *s++) return false; /* Characters do not match */
				break;
		}
	}
}
