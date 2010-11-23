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
#include "sxmlc.h"

/* --- XMLNode methods --- */

/*
 Add 'node' to given '*array' of '*len_array' elements.
 '*len_array' is the number of elements in '*array' after its reallocation.
 Return false for memory error.
 */
static int _add_node(XMLNode*** array, int* len_array, XMLNode* node)
{
	XMLNode** pt = (XMLNode**)realloc(*array, (*len_array+1) * sizeof(XMLNode*));
	
	if (pt == NULL) return -1;
	
	pt[*len_array] = node;
	*array = pt;
	(*len_array)++;
	
	return *len_array - 1;
}

void XMLNode_init(XMLNode* node)
{
	if (node == NULL) return;
	
	node->tag = NULL;
	node->text = NULL;
	
	node->attributes = NULL;
	node->n_attributes = 0;
	
	node->children = NULL;
	node->n_children = 0;
	
	node->tag_type = TAG_NONE;
	node->active = true;
}

XMLNode* XMLNode_alloc(int n)
{
	int i;
	XMLNode* p;
	
	if (n <= 0) return NULL;
	
	p = (XMLNode*)malloc(n * sizeof(XMLNode));
	for (i = 0; i < n; i++)
		XMLNode_init(&p[i]);
	
	return p;
}

void XMLNode_free(XMLNode* node)
{
	int i;

	if (node == NULL) return;
	
	if (node->attributes != NULL) {
		for (i = 0; i < node->n_attributes; i++) {
			if (node->attributes[i].name != NULL) free(node->attributes[i].name);
			if (node->attributes[i].value != NULL) free(node->attributes[i].value);
		}
		free(node->attributes);
		node->attributes = NULL;
	}
	node->n_attributes = 0;
	
	if (node->children != NULL) {
		for (i = 0; i < node->n_children; i++)
			if (node->children[i] != NULL) {
				XMLNode_free(node->children[i]);
			}
		free(node->children);
		node->children = NULL;
	}
	node->n_children = 0;
	
	if (node->tag != NULL) {
		free(node->tag);
		node->tag = NULL;
	}
	if (node->text != NULL) {
		free(node->text);
		node->text = NULL;
	}
	
	node->tag_type = TAG_NONE;
}

int XMLNode_copy(XMLNode* dst, XMLNode* src, int copy_children)
{
	int i;
	
	if (dst == NULL) return false;
	
	XMLNode_free(dst); /* 'dst' is freed first */
	
	/* NULL 'src' resets 'dst' */
	if (src == NULL) return true;
	
	/* Tag */
	if (src->tag != NULL) {
		dst->tag = (char*)malloc(strlen(src->tag)+1);
		if (dst->tag == NULL) goto copy_err;
		strcpy(dst->tag, src->tag);
	}

	/* Text */
	if (dst->text != NULL) {
		dst->text = (char*)malloc(strlen(src->text)+1);
		if (dst->text == NULL) goto copy_err;
		strcpy(dst->text, src->text);
	}

	/* Attributes */
	if (src->n_attributes > 0) {
		dst->attributes = (XMLAttribute*)malloc(src->n_attributes * sizeof(XMLAttribute));
		if (dst->attributes== NULL) goto copy_err;
		dst->n_attributes = src->n_attributes;
		for (i = 0; i < src->n_attributes; i++) {
			dst->attributes[i].name = (char*)malloc(strlen(src->attributes[i].name)+1);
			dst->attributes[i].value = (char*)malloc(strlen(src->attributes[i].value)+1);
			if (dst->attributes[i].name == NULL || dst->attributes[i].value == NULL) goto copy_err;
		
			strcpy(dst->attributes[i].name, src->attributes[i].name);
			strcpy(dst->attributes[i].value, src->attributes[i].value);
		}
	}

	dst->tag_type = src->tag_type;
	dst->father = src->father;
	dst->user = src->user;
	dst->active = src->active;
	
	/* Copy children if required */
	if (copy_children) {
		dst->children = (XMLNode**)malloc(src->n_children * sizeof(XMLNode*));
		if (dst->children == NULL) goto copy_err;
		dst->n_children = src->n_children;
		for (i = 0; i < src->n_children; i++) {
			if (!XMLNode_copy(dst->children[i], src->children[i], true)) goto copy_err;
		}
	}
	
	return true;
	
copy_err:
	XMLNode_free(dst);
	
	return false;
}

void XMLNode_set_active(XMLNode* node, int active)
{
	if (node == NULL) return;

	node->active = active;
}

int XMLNode_set_tag(XMLNode* node, char* tag)
{
	if (node == NULL || tag == NULL) return false;
	
	if (node->tag != NULL) free(node->tag);
	node->tag = strcpy_alloc(tag);
	if (node->tag == NULL) return false;
	
	return true;
}

int XMLNode_set_comment(XMLNode* node, char* comment)
{
	if (!XMLNode_set_tag(node, comment)) return false;
	
	node->tag_type = TAG_COMMENT;
	
	return true;
}

int XMLNode_set_attribute(XMLNode* node, char* attr_name, char* attr_value)
{
	XMLAttribute* pt;
	int i;
	
	if (node == NULL || attr_name == NULL || attr_name[0] == 0) return -1;
	
	i = XMLNode_search_attribute(node, attr_name, 0);
	if (i >= 0) {
		pt = node->attributes;
		if (pt[i].value != NULL) free(pt[i].value);
		pt[i].value = (char*)malloc(strlen(attr_value)+1);
		if (pt[i].value != NULL) strcpy(pt[i].value, attr_value);
	}
	else {
		i = node->n_attributes;
		pt = (XMLAttribute*)realloc(node->attributes, (i+1) * sizeof(XMLAttribute));
		if (pt == NULL) return 0;

		pt[i].name = (char*)malloc(strlen(attr_name)+1);
		pt[i].value = (char*)malloc(strlen(attr_value)+1);
		if (pt[i].name != NULL && pt[i].value != NULL) {
			strcpy(pt[i].name, attr_name);
			strcpy(pt[i].value, attr_value);
			node->attributes = pt;
			node->n_attributes = i + 1;
		}
		else {
			node->attributes = (XMLAttribute*)realloc(pt, i * sizeof(XMLAttribute)); /* Frees memory, cannot fail hopefully! */
			return -1;
		}
	}

	return node->n_attributes;
}

int XMLNode_search_attribute(XMLNode* node, char* attr_name, int isearch)
{
	int i;
	
	if (node == NULL || attr_name == NULL || attr_name[0] == 0 || isearch < 0 || isearch > node->n_attributes) return -1;
	
	for (i = isearch; i < node->n_attributes; i++)
		if (node->attributes[i].active && !strcmp(node->attributes[i].name, attr_name)) return i;
	
	return -1;
}

int XMLNode_remove_attribute(XMLNode* node, int i_attr)
{
	if (node == NULL || i_attr < 0 || i_attr >= node->n_attributes) return -1;
	
	/* Free attribute fields first */
	if (node->attributes[i_attr].name != NULL) free(node->attributes[i_attr].name);
	if (node->attributes[i_attr].value != NULL) free(node->attributes[i_attr].value);
	
	memmove(&node->attributes[i_attr], &node->attributes[i_attr+1], (node->n_attributes - i_attr - 1) * sizeof(XMLAttribute));
	node->attributes = (XMLAttribute*)realloc(node->attributes, --(node->n_attributes) * sizeof(XMLAttribute)); /* Frees memory */
	
	return node->n_attributes;
}

int XMLNode_set_text(XMLNode* node, char* text)
{
	if (node == NULL) return false;

	if (text == NULL) { /* We want to remove it => free node text */
		if (node->text != NULL) {
			free(node->text);
			node->text = NULL;
		}

		return true;
	}

	/* No text is defined yet => allocate it */
	if (node->text == NULL) {
		node->text = (char*)malloc(strlen(text)+1);
		if (node->text == NULL) return false;
	}
	else {
		char* p = (char*)realloc(node->text, strlen(text)+1);
		if (p == NULL) return false;
		node->text = p;
	}

	strcpy(node->text, text);

	return true;
}

int XMLNode_add_child(XMLNode* node, XMLNode* child)
{
	int n;
	
	if (node == NULL || child == NULL) return false;
	
	n = node->n_children;
	return _add_node(&node->children, &node->n_children, child) >= 0 ? true : false;
}

int XMLNode_search_child(XMLNode* node, char* tag, int isearch)
{
	int i;
	
	if (node == NULL || tag == NULL || tag[0] == 0 || isearch < 0 || isearch > node->n_children) return -1;
	
	for (i = isearch; i < node->n_children; i++)
		if (node->children[i]->active && !strcmp(node->children[i]->tag, tag)) return i;
	
	return -1;
}

int XMLNode_remove_child(XMLNode* node, int ichild)
{
	if (node == NULL || ichild < 0 || ichild >= node->n_children) return -1;
	
	/* Free node first */
	XMLNode_free(node->children[ichild]);
	
	memmove(&node->children[ichild], &node->children[ichild+1], (node->n_children - ichild - 1) * sizeof(XMLNode*));
	node->children = (XMLNode**)realloc(node->children, --(node->n_children) * sizeof(XMLNode*)); /* Frees memory */
	
	return node->n_children;
}

int XMLNode_equal(XMLNode* node1, XMLNode* node2)
{
	int i, j;

	if (node1 == node2) return true;

	if (node1 == NULL || node2 == NULL) return false;

	if (strcmp(node1->tag, node2->tag)) return false;

	/* Test all attributes from 'node1' */
	for (i = 0; i < node1->n_attributes; i++) {
		if (!node1->attributes[i].active) continue;
		j = XMLNode_search_attribute(node2, node1->attributes[i].name, 0);
		if (j < 0) return false;
		if (strcmp(node1->attributes[i].name, node2->attributes[j].name)) return false;
	}

	/* Test other attributes from 'node2' that might not be in 'node1' */
	for (i = 0; i < node2->n_attributes; i++) {
		if (!node2->attributes[i].active) continue;
		j = XMLNode_search_attribute(node1, node2->attributes[i].name, 0);
		if (j < 0) return false;
		if (strcmp(node2->attributes[i].name, node1->attributes[j].name)) return false;
	}

	return true;
}

XMLNode* XMLNode_next_sibling(XMLNode* node)
{
	int i;
	XMLNode* father;

	if (node == NULL || node->father == NULL) return NULL;

	father = node->father;
	for (i = 0; i < father->n_children && father->children[i] != node; i++) ;
	i++; /* father->children[i] is now 'node' next sibling */

	return i < father->n_children ? father->children[i] : NULL;
}

static XMLNode* _XMLNode_next(XMLNode* node, int in_children)
{
	XMLNode* node2;

	if (node == NULL) return NULL;

	/* Check first child */
	if (in_children && node->n_children > 0) return node->children[0];

	/* Check next sibling */
	if ((node2 = XMLNode_next_sibling(node)) != NULL) return node2;

	/* Check next uncle */
	return _XMLNode_next(node->father, false);
}

XMLNode* XMLNode_next(XMLNode* node)
{
	return _XMLNode_next(node, true);
}

/* --- XMLDoc methods --- */

void XMLDoc_init(XMLDoc* doc)
{
	doc->nodes = NULL;
	doc->n_nodes = 0;
	doc->i_root = -1;
}

void XMLDoc_free(XMLDoc* doc)
{
	int i;
	
	for (i = 0; i < doc->n_nodes; i++)
		XMLNode_free(doc->nodes[i]);
	free(doc->nodes);
	XMLDoc_init(doc);
}

int XMLDoc_set_root(XMLDoc* doc, int i_root)
{
	if (doc == NULL || i_root < 0 || i_root >= doc->n_nodes) return false;
	
	doc->i_root = i_root;
	
	return true;
}

int XMLDoc_add_node(XMLDoc* doc, XMLNode* node, int tag_type)
{
	if (doc == NULL || node == NULL) return false;
	
	if (tag_type == TAG_NONE) tag_type = node->tag_type;
	node->tag_type = tag_type;
	if (_add_node(&doc->nodes, &doc->n_nodes, node) >= 0) {
		if (tag_type == TAG_FATHER) doc->i_root = doc->n_nodes - 1;
		return doc->n_nodes;
	}
	else
		return -1;
}

/*
 Helper functions to print formatting before a new tag.
 Returns the new number of characters in the line.
 */
static int count_new_char_line(char* str, int nb_char_tab, int cur_sz_line)
{
	for (; *str; str++) {
		if (*str == '\n') cur_sz_line = 0;
		else if (*str == '\t') cur_sz_line += nb_char_tab;
		else cur_sz_line++;
	}
	
	return cur_sz_line;
}
static int _print_formatting(FILE* f, char* tag_sep, char* child_sep, int nb_char_tab, int depth, int cur_sz_line)
{
	int i;
	
	if (tag_sep) {
		fprintf(f, tag_sep);
		cur_sz_line = count_new_char_line(tag_sep, nb_char_tab, cur_sz_line);
	}
	if (child_sep) {
		for (i = 0; i < depth; i++) {
			fprintf(f, child_sep);
			cur_sz_line = count_new_char_line(child_sep, nb_char_tab, cur_sz_line);
		}
	}
	
	return cur_sz_line;
}

void XMLNode_print(XMLNode* node, FILE* f, char* tag_sep, char* child_sep, int sz_line, int nb_char_tab, int depth)
{
	static int cur_sz_line = 0; /* How many characters are on the current line */
	int i;
	
	if (node == NULL || f == NULL || !node->active) return;
	
	if (nb_char_tab <= 0) nb_char_tab = 1;
	
	/* Print formatting */
	cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
	
	if (node->tag_type == TAG_COMMENT) {
		fprintf(f, "<!--%s-->", node->tag);
		/*cur_sz_line += strlen(node->tag) + 7;*/
		return;
	}
	else if (node->tag_type == TAG_PROLOG) {
		fprintf(f, "<?%s?>", node->tag);
		/*cur_sz_line += strlen(node->tag) + 4;*/
		return;
	}
	else if (node->tag_type == TAG_CDATA) {
		fprintf(f, "<![CDATA[%s]]/>", node->tag);
		/*cur_sz_line += strlen(node->tag) + 13;*/
		return;
	}
	else {
		/* Print tag name */
		fprintf(f, "<%s", node->tag);
		cur_sz_line += strlen(node->tag) + 1;
	}
	
	/* Print attributes */
	for (i = 0; i < node->n_attributes; i++) {
		if (!node->attributes[i].active) continue;
		cur_sz_line += strlen(node->attributes[i].name) + strlen(node->attributes[i].value) + 3;
		if (sz_line > 0 && cur_sz_line > sz_line) {
			cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
			/* Add extra separator, as if new line was a child of the previous one */
			if (child_sep) {
				fprintf(f, child_sep);
				cur_sz_line = count_new_char_line(child_sep, nb_char_tab, cur_sz_line);
			}
		}
		/* Attribute name */
		fprintf(f, " %s=", node->attributes[i].name);
		
		/* Attribute value */
		fputc('"', f);
		cur_sz_line += fprintHTML(f, node->attributes[i].value);
		fputc('"', f);
	}
	
	/* End the tag if there are no children and no text */
	if (node->n_children == 0 && (node->text == NULL || node->text[0] == 0)) {
		fprintf(f, "/>");
		/*cur_sz_line += 2;*/
		return;
	}
	else {
		fputc('>', f);
	}
	if (node->text != NULL && node->text[0]) fprintHTML(f, node->text);
	cur_sz_line++;
	
	/* Recursively print children */
	for (i = 0; i < node->n_children; i++)
		XMLNode_print(node->children[i], f, tag_sep, child_sep, sz_line, nb_char_tab, depth+1);
	
	/* Print tag end after children */
		/* Print formatting */
	if (node->n_children > 0)
		cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
	fprintf(f, "</%s>", node->tag);
	/*cur_sz_line += strlen(node->tag) + 3;*/
}

void XMLDoc_print(XMLDoc* doc, FILE* f, char* tag_sep, char* child_sep, int sz_line, int nb_char_tab)
{
	int i;
	
	if (doc == NULL || f == NULL) return;
	
	for (i = 0; i < doc->n_nodes; i++)
		XMLNode_print(doc->nodes[i], f, tag_sep, child_sep, sz_line, nb_char_tab, 0);
}

/* --- */

int parse_XML_attribute(char* str, XMLAttribute* xmlattr)
{
	char *p;
	int i, n0, n1, remQ = 0;
	int ret = 1;
	
	if (str == NULL || xmlattr == NULL) return 0;
	
	/* Search for the '=' */
	/* 'n0' is where the attribute name stops, 'n1' is where the attribute value starts */
	for (n0 = 0; str[n0] && str[n0] != '=' && !isspace(str[n0]); n0++) ; /* Search for '=' or a space */
	for (n1 = n0; str[n1] && isspace(str[n1]); n1++) ; /* Search for something not a space */
	if (str[n1] != '=') return 0; /* '=' not found: malformed string */
	for (n1++; str[n1] && isspace(str[n1]); n1++) ; /* Search for something not a space */
	if (str[n1] == '"') remQ = 1; /* Remove quotes */
	
	xmlattr->name = (char*)malloc(n0+1);
	xmlattr->value = (char*)malloc(strlen(str) - n1 - remQ);
	xmlattr->active = true;
	if (xmlattr->name != NULL && xmlattr->value != NULL) {
		/* Copy name */
		for (i = 0; i < n0; i++) xmlattr->name[i] = str[i];
		xmlattr->name[i] = 0;
		/* Copy value (p starts after the quote (if any) and stops at the end of 'str'
		  (skipping the quote if any, hence the '*(p+remQ)') */
		for (i = 0, p = str + n1 + remQ; *(p+remQ); i++, p++)
			xmlattr->value[i] = *p;
		xmlattr->value[i] = 0;
		html2str(xmlattr->value); /* Convert HTML escape sequences */
		if (remQ && *p != '"') ret = 2; /* Quote at the beginning but not at the end */
	}
	else ret = 0;
	
	if (ret == 0) {
		if (xmlattr->name != NULL) free(xmlattr->name);
		if (xmlattr->value != NULL) free(xmlattr->value);
	}
	
	return ret;
}

/*
 Reads a string that is supposed to be an xml tag like '<tag (attribName="attribValue")* [/]>' or '</tag>'.
 Fills the 'xmlnode' structure with the tag name and its attributes.
 Returns 0 if an error occurred (malformed 'str' or memory). 'TAG_*' when string is recognized.
 */
static int _parse_XML_1string(char* str, XMLNode* xmlnode)
{
	char *p, c;
	XMLAttribute* pt;
	int n, nn, len, tag_end = 0;
	
	if (str == NULL || xmlnode == NULL) return 0;
	len = strlen(str);
	
	/* Check for malformed string */
	if (str[0] != '<' || str[len-1] != '>') return 0;
	
	/* Prolog tag */
	if (str[1] == '?') {
		if (str[len-2] == '?') len--; /* If 'str' ends with "?>" (which should be the case), do not copy the trailing '?' */
		xmlnode->tag = (char*)malloc(len-3); /* 'len' - "<?" and ">" */
		if (xmlnode->tag == NULL) return 0;
		strncpy(xmlnode->tag, str+2, len-4);
		xmlnode->tag[len-4] = 0;
		xmlnode->tag_type = TAG_PROLOG;
		return TAG_PROLOG;
	}
	
	/* Comment or CDATA tag */
	if (str[1] == '!') {
		/* Comment */
		if (!strncmp(str, "<!--", 4)) {
			if (strncmp(str+len-3, "-->", 3)) return TAG_PARTIAL_COMMENT; /* There probably is a '>' inside the comment (typically when commenting tags) */
			xmlnode->tag = (char*)malloc(len-6); /* 'len' - "<!--" and "-->" + '\0' */
			if (xmlnode->tag == NULL) return 0;
			strncpy(xmlnode->tag, str+4, len-7);
			xmlnode->tag[len-7] = 0;
			xmlnode->tag_type = TAG_COMMENT;
			return TAG_COMMENT;
		}

		/* CDATA */
		if (!strncmp(str, "<![CDATA[", 9)) {
			if (strncmp(str+len-4, "]]/>", 4)) return TAG_PARTIAL_CDATA; /* There probably is a '>' inside the CDATA */
			xmlnode->tag = (char*)malloc(len-12); /* 'len' - "<![CDATA[" and "]]/>" + '\0' */
			if (xmlnode->tag == NULL) return 0;
			strncpy(xmlnode->tag, str+9, len-13);
			xmlnode->tag[len-13] = 0;
			xmlnode->tag_type = TAG_CDATA;
			return TAG_CDATA;
		}
	}
	
	if (str[1] == '/') tag_end = 1;
	
	/* tag starts at index 1 (or 2 if tag end) and ends at the first space or '/>' */
	for (n = 1 + tag_end; str[n] && str[n] != '>' && str[n] != '/' && !isspace(str[n]); n++) ;
	xmlnode->tag = (char*)malloc(n - tag_end);
	if (xmlnode->tag == NULL) return 0;
	strncpy(xmlnode->tag, str+1+tag_end, n-1-tag_end);
	xmlnode->tag[n-1-tag_end] = 0;
	if (tag_end) {
		xmlnode->tag_type = TAG_END;
		return TAG_END;
	}
	
	/* Here, 'n' is the position of the first space after tag name */
	while (n < len) {
		/* Skips spaces */
		while (isspace(str[n])) n++;
		
		/* Check for XML end ('>' or '/>') */
		if (str[n] == '>') { /* Tag with children */
			xmlnode->tag_type = TAG_FATHER;
			return TAG_FATHER;
		}
		if (!strcmp(str+n, "/>")) { /* Tag without children */
			xmlnode->tag_type = TAG_SELF;
			return TAG_SELF;
		}
		
		/* New attribute found */
		p = strchr(str+n, '=');
		if (p == NULL) goto parse_err;
		pt = (XMLAttribute*)realloc(xmlnode->attributes, ++(xmlnode->n_attributes) * sizeof(XMLAttribute));
		if (pt == NULL) {
			xmlnode->n_attributes--; /* Reallocation failed */
			goto parse_err;
		}
		xmlnode->attributes = pt;
		while (*p && isspace(*++p)) ; /* Skip spaces */
		if (*p == '"') { /* Attribute value starts with a '"', look for next one, ignoring protected ones with '\"' */
			for (nn = p-str+1; str[nn] && str[nn] != '"'; nn++) {
				if (str[nn] == '\\') nn++;
			}
			nn++;
		}
		else { /* Attribute value stops at first space or end of XML string */
			for (nn = p-str+1; str[nn] && !isspace(str[nn]) && str[nn] != '/' && str[nn] != '>'; nn++) ; /* Go to the end of the attribute value */
		}
		
		/* Here 'str[nn]' is either a '"' or a space */
		/* the attribute definition ('attrName="attr val"') is between 'str[n]' and 'str[nn]' */
		c = str[nn]; /* Backup character */
		str[nn] = 0; /* End string to call 'parse_XML_attribute' */
		if (!parse_XML_attribute(str+n, &xmlnode->attributes[xmlnode->n_attributes - 1])) goto parse_err;
		str[nn] = c;
		
		n = nn;
	}
	
	fprintf(stderr, "\nWE SHOULD NOT BE HERE!\n[%s]\n\n", str);
	
parse_err:
	XMLNode_free(xmlnode);
	return 0;
}

int XMLDoc_parse_file_DOM(char* filename, XMLDoc* doc)
{
	FILE* f;
	char *line, *txt_end, *p;
	XMLNode *node, *father;
	int ret, nline, sz, n0, ncr, tag_type;
	
	if (doc == NULL || filename == NULL || filename[0] == 0) return false;
	
	XMLDoc_init(doc);
	strncpy(doc->filename, filename, sizeof(doc->filename));
	f = fopen(filename, "rt");
	if (f == NULL) return false;
	ret = true;
	nline = 1; /* Line counter, starts at 1 */
	sz = 0; /* 'line' buffer size */
	father = NULL; /* Where to add children nodes to (initialized to 'doc->root' at the beginning, when NULL) */
	while ((n0 = read_line_alloc(f, &line, &sz, 0, 0, '>', true, '\n', &ncr)) != 0) {
		for (p = line; *p && isspace(*p); p++) ; /* Checks if text is only spaces */
		if (*p == 0) break;
		node = (XMLNode*)malloc(sizeof(XMLNode));
		if (node == NULL) {
			ret = false;
			break;
		}
		XMLNode_init(node);
		nline += ncr;
/*printf("%4d: %s\n", nline, line);*/
		
		/* Get text for 'father' (i.e. what is before '<') */
		txt_end = strchr(line, '<');
		if (txt_end == NULL) { /* Missing tag start */
			fprintf(stderr, "%s:%d: ERROR: Unexpected tag end '>', without matching '<'!\n", filename, nline);
			ret = false;
			break;
		}
		/* First part of 'line' (before '<') is to be added to 'father->text' */
		*txt_end = 0; /* Makes 'line' be the text for 'father' */
		for (p = line; *p && isspace(*p); p++) ; /* Checks if text is only spaces */
		if (*p && father == NULL) { /* If there is text and no father (i.e. root node has not been reached yet) */
			fprintf(stderr, "%s:%d: ERROR: Text before root node!\n", filename, nline);
			ret = false;
			break;
		}
		if (*p) { /* Text is not only spaces */
			/* 'father' is not NULL here. If 'text' is NULL, allocate it. If not, add 'line' to 'father->text' */
			n0 = (father->text == NULL ? 0 : strlen(father->text));
			p = (char*)realloc(father->text, strlen(line)+1 + n0);
			if (p == NULL) {
				ret = false;
				break;
			}
			father->text = p;
			strcpy(father->text + n0, line);
		}
		*txt_end = '<'; /* Restores tag start */
		tag_type = _parse_XML_1string(txt_end, node);
		if (tag_type == 0) { /* Error */
			ret = false;
			break;
		}
		else if (tag_type == TAG_END) { /* Look upwards which node to finish */
			if (strcmp(node->tag, father->tag)) { /* Should be the father's tag */
				fprintf(stderr, "%s:%d: ERROR: Unexpected tag end </%s>, waiting for <%s>!\n", filename, nline, node->tag, father->tag);
				ret = false;
				break;
			}
			father = father->father; /* Next tag read is to be added to father */
			free(node);
		}
		else { /* Add 'node' to 'father' children */
			/* If the line looks like a comment (or CDATA) but is not properly finished, loop until we find the end. */
			while (tag_type == TAG_PARTIAL_COMMENT || tag_type == TAG_PARTIAL_CDATA) {
				n0 = read_line_alloc(f, &line, &sz, n0, 0, '>', true, '\n', &ncr); /* Go on reading the file from current position until next '>' */
				if (!n0) {
					ret = false;
					break;
				}
				nline += ncr;
				tag_type = _parse_XML_1string(txt_end, node);
			}
			if (ret == false) break;
			if (_add_node(father == NULL ? &doc->nodes : &father->children, father == NULL ? &doc->n_nodes : &father->n_children, node) < 0) {
				ret = false;
				break;
			}
			node->father = father;
			/* If 'node' is a father, next nodes are to be added to it. It also becomes the root node if none existed. */
			if (tag_type == TAG_FATHER) {
				father = node;
				if (doc->i_root < 0) doc->i_root = doc->n_nodes - 1;
			} /* else 'node->tag_type' is 'TAG_SELF' or comment, next tag is to be added to current father */
		}
		if (ret == false || feof(f)) break;
	}
	free(line);
	fclose(f);
	if (!ret) XMLDoc_free(doc);
	
	return ret;
}

int XMLDoc_parse_file_SAX(char* filename, SAX_Callbacks* sax, void* user)
{
	FILE* f;
	char *line, *txt_end, *p;
	XMLNode node;
	int ret, exit, nline, sz, n0, ncr, tag_type;
	
	if (sax == NULL || filename == NULL || filename[0] == 0) return false;
	
	f = fopen(filename, "rt");
	if (f == NULL) return false;
	ret = true;
	exit = false;
	nline = 1; /* Line counter, starts at 1 */
	sz = 0; /* 'line' buffer size */
	XMLNode_init(&node);
	while ((n0 = read_line_alloc(f, &line, &sz, 0, 0, '>', true, '\n', &ncr)) != 0) {
		XMLNode_free(&node);
		for (p = line; *p && isspace(*p); p++) ; /* Checks if text is only spaces */
		if (*p == 0) break;
		nline += ncr;
/*printf("%4d: %s\n", nline, line);*/
		
		/* Get text for 'father' (i.e. what is before '<') */
		txt_end = strchr(line, '<');
		if (txt_end == NULL) { /* Missing tag start */
			fprintf(stderr, "%s:%d: ERROR: Unexpected tag end '>', without matching '<'!\n", filename, nline);
			ret = false;
			break;
		}
		/* First part of 'line' (before '<') is to be added to 'father->text' */
		*txt_end = 0; /* Makes 'line' be the text for 'father' */
		if (sax->new_text != NULL) {
			exit = !sax->new_text(line, user);
			if (exit) break;
		}
		*txt_end = '<'; /* Restores tag start */
		tag_type = _parse_XML_1string(txt_end, &node);
		if (tag_type == 0) {
			ret = false;
			break;
		}
		else if (tag_type == TAG_END && sax->end_node != NULL) {
			exit = !sax->end_node(&node, user);
			if (exit) break;
		}
		else { /* Add 'node' to 'father' children */
			/* If the line looks like a comment (or CDATA) but is not properly finished, loop until we find the end. */
			while (tag_type == TAG_PARTIAL_COMMENT || tag_type == TAG_PARTIAL_CDATA) {
				n0 = read_line_alloc(f, &line, &sz, n0, 0, '>', true, '\n', &ncr); /* Go on reading the file from current position until next '>' */
				if (!n0) {
					ret = false;
					break;
				}
				nline += ncr;
				tag_type = _parse_XML_1string(txt_end, &node);
			}
			if (ret == false) break;
			if (sax->start_node != NULL) exit = !sax->start_node(&node, user);
			if (exit) break;
			if (node.tag_type != TAG_FATHER && sax->end_node != NULL) exit = !sax->end_node(&node, user);
			if (exit) break;
		}
		if (exit == true || ret == false || feof(f)) break;
	}
	free(line);
	fclose(f);
	XMLNode_free(&node);
	
	return ret;
}

int XMLDoc_parse_file(char* filename, XMLDoc* doc)
{
	return XMLDoc_parse_file_DOM(filename, doc);
}
