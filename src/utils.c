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
#pragma warning(disable : 4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

/* Dictionnary of special characters and their HTML equivalent */
struct _html_special_dict {
	char chr;		/* Original character */
	char* html;		/* Equivalent HTML string */
	int html_len;	/* 'strlen(html)' */
} HTML_SPECIAL_DICT[] = {
	{ '<', "&lt;", 4 },
	{ '>', "&gt;", 4 },
	{ '"', "&quot;", 6 },
	{ '&', "&amp;", 5 },
	{ 0, NULL, 0 }, /* Terminator */
};

#if 0
int read_line(FILE* f, char* line, int sz_line)
{
	char c;
	int n = 0;
	
	if (f == NULL || line == NULL || sz_line <= 0) return 0;
	
	while (1) {
		c = fgetc(f);
		switch (c) {
			case EOF:
				line[n] = 0;
				return feof(f) ? 1 : 0;
				
			case '\n':
				line[n] = 0;
				return 1;
				
			default:
				line[n++] = c;
				line[n] = 0;
				if (n >= sz_line) return 2;
		}
	}
}
#endif

int read_line_alloc(FILE* f, char** line, int* sz_line, int i0, char from, char to, int keep_fromto, char interest, int* interest_count)
{
	int init_sz = 0;
	char c, *pt;
	int n, ret;
	
	if (f == NULL || line == NULL) return 0;
	
	if (to == 0) to = '\n';
	/* Search for character 'from' */
	if (interest_count != NULL) *interest_count = 0;
	while (1) {
		c = fgetc(f);
		if (interest_count != NULL && c == interest) (*interest_count)++;
		/* Reaching EOF before 'to' char is not an error but should trigger 'line' alloc and init to '' */
		/* If 'from' is '\0', we stop here */
		if (c == from || c == EOF || from == 0) break;
	}
	
	if (sz_line == NULL) sz_line = &init_sz;
	
	if (*line == NULL || *sz_line == 0) {
		if (*sz_line == 0) *sz_line = MEM_INCR_RLA;
		*line = (char*)malloc(*sz_line);
		if (*line == NULL) return 0;
	}
	if (i0 < 0) i0 = 0;
	if (i0 > *sz_line) return 0;
	
	n = i0;
	if (c == EOF) { /* EOF reached before 'to' char => return the empty string */
		(*line)[n] = 0;
		return feof(f) ? n : 0; /* Error if not EOF */
	}
	if (c != from || keep_fromto)
		(*line)[n++] = c;
	(*line)[n] = 0;
	ret = 0;
	while (1) {
		c = fgetc(f);
		if (interest_count != NULL && c == interest) (*interest_count)++;
		if (c == EOF) { /* EOF or error */
			(*line)[n] = 0;
			ret = feof(f) ? n : 0;
			break;
		}
		else {
			(*line)[n] = c;
			if (c != to || keep_fromto && to != 0 && c == to) n++; /* If we reached the 'to' character and we keep it, we still need to add the extra '\0' */
			if (n >= *sz_line) { /* Too many characters for our line => realloc some more */
				*sz_line += MEM_INCR_RLA;
				pt = (char*)realloc(*line, *sz_line);
				if (pt == NULL) {
					ret = 0;
					break;
				}
				else
					*line = pt;
			}
			(*line)[n] = 0; /* If we reached the 'to' character and we want to strip it, 'n' hasn't changed and 'line[n]' (which is 'to') will be replaced by '\0' */
			if (c == to) {
				ret = n;
				break;
			}
		}
	}
	
#if 0 /* Automatic buffer resize is deactivated */
	/* Resize line to the exact size */
	pt = (char*)realloc(*line, n+1);
	if (pt != NULL)
		*line = pt;
#endif
	
	return ret;
}

/* --- */

char* strcpy_alloc(char* src)
{
	char* p;

	if (src == NULL) return NULL;

	p = (char*)malloc(strlen(src)+1);
	if (p == NULL) return NULL;

	strcpy(p, src);

	return p;
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
	str[i+1] = 0; /* New end of string to last non-space */
	
	if (repl_sq == 0) {
		if (p == str && i == len) return str; /* Nothing to do */
		for (i = 0; (str[i] = *p) != 0; i++, p++) ; /* Copy 'p' to 'str' */
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
	str[i] = 0;
	
	return str;
}

/* --- */

char* html2str(char* str)
{
	char *p1, *p2;
	int i;
	
	/* Look for '&' and matches it to any of the recognized HTML pattern. */
	/* If found, replaces the '&' by the corresponding char. */
	/* 'p2' is the char to analyze, 'p1' is where to insert it */
	for (p1 = p2 = str; *p2; p1++, p2++) {
		if (*p2 != '&') {
			if (p1 != p2) *p1 = *p2;
			continue;
		}
		
		for (i = 0; HTML_SPECIAL_DICT[i].chr; i++) {
			if (strncmp(p2, HTML_SPECIAL_DICT[i].html, HTML_SPECIAL_DICT[i].html_len)) continue;
			
			*p1 = HTML_SPECIAL_DICT[i].chr;
			p2 += HTML_SPECIAL_DICT[i].html_len-1;
			break;
		}
		/* If no string was found, simply copy the character */
		if (HTML_SPECIAL_DICT[i].chr == 0 && p1 != p2) *p1 = *p2;
	}
	*p1 = 0;
	
	return str;
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
		if (HTML_SPECIAL_DICT[i].chr == 0) {
			fputc(*p, f);
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

	//if (!strcmp(str, pattern)) return true;
	p = pattern;
	s = str;
	while (*p) {
		switch (*p) {
			/* Any character matches, go to next one */
			case '?':
				p++;
				s++;
				break;

			/* Go to next character in pattern and wait until it is found in 'str' */
			case '*':
				p++;
				/* TODO: What if *p == '?' or '*' ??? */
				for (; *s; s++) {
					if (*s == *p) break;
				}
				break;

			default:
				if (*p == '\\') p++; /* Escape character */
				if (*p++ != *s++) return false; /* Characters do not match */
				break;
		}
	}

	return true;
}
