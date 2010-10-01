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
#ifndef _UTILS_H_
#define _UTILS_H_

/*
 Read a line from file 'f' into buffer 'line' of size 'sz_line' bytes.
 Returns 0 if an error occured.
         1 if the line was read.
		 2 if the line was partially read but the buffer is too small.
 */
int read_line(FILE* f, char* line, int sz_line);

#ifndef MEM_INCR_RLA
#define MEM_INCR_RLA 256 /* Initial buffer size and increment for memory reallocations */
#endif

/*
 Reads a line from file 'f', eventualy (re-)allocating a given buffer 'line'. Characters
 read will be stored in 'line' starting at 'i0' (this allows multiple calls to 'read_line_alloc'
 on the same 'line' buffer without overwriting it at each call).
 'sz_line' is the size of the buffer 'line' if previously allocated. 'line' can point
 to NULL, in which case it will be allocated '*sz_line' bytes. After the function returns,
 '*sz_line' is the actual buffer size. This allows multiple calls to this function using the
 same buffer (without re-allocating/freeing).
 If 'sz_line' is non NULL and non 0, it means that '*line' is a VALID pointer to a location
 of '*sz_line'.
 Searches for character 'from' until character 'to'. If 'from' is 0, starts from
 current position. If 'to' is 0, it is replaced by '\n'.
 If 'keep_fromto' is 0, removes characters 'from' and 'to' from the line.
 If 'interest_count' is not NULL, will receive the count of 'interest' characters while searching
 for 'to' (e.g. use 'interest'='\n' to count lines in file).
 Returns the number of characters in the line or 0 if an error occured.
 'read_line_alloc' uses constant 'MEM_INCR_RLA' to reallocate memory when needed. It is possible
 to override this definition to use another value.
 */
int read_line_alloc(FILE* f, char** line, int* sz_line, int i0, char from, char to, int keep_fromto, char interest, int* interest_count);

/*
 Strip spaces at the beginning and end of 'str', modifying 'str'.
 If 'repl_sq' is not '\0', squeezes spaces to an single character ('repl_sq').
 If not '\0', 'protect' is used to protect spaces from being deleted (usually a backslash).
 Returns the string or NULL if 'protect' is a space (which would not make sense).
 */
char* strip_spaces(char* str, char repl_sq, char protect);

/*
 Replace occurrences of special characters HTML escape sequences (e.g. '&amp;') by its
 character equivalent (e.g. '&').
 Replacement is made in 'str' itself, overwriting it.
 Returns 'str'.
 */
char* html2str(char* str);

/*
 Print 'str' to 'f', transforming special characters into their HTML equivalent.
 Returns the number of output characters.
 */
int fprintHTML(FILE* f, char* str);

#endif
