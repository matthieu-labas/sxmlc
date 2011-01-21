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
#include "sxmlc.h"

/*
 Struct defining "special" tags such as "<? ?>" or "<![CDATA[ ]]/>".
 These tags are considered having a start and an end with some data in between that will
 be stored in the 'tag' member of an XMLNode.
 The 'tag_type' member is a constant that is associated to such tag.
 All 'len_*' members are basically the "strlen()" of 'start' and 'end' members.
 */
typedef struct _Tag {
	TagType tag_type;
	char* start;
	int len_start;
	char* end;
	int len_end;
} _TAG;

typedef struct _SpecialTag {
	_TAG *tags;
	int n_tags;
} SPECIAL_TAG;

/*
 List of "special" tags handled by sxmlc.
 NB the "<!DOCTYPE" tag has a special handling because its 'end' changes according
 to its content ('>' or ']>').
 */
static _TAG _spec[] = {
		{ TAG_INSTR, "<?", 2, "?>", 2 },
		{ TAG_COMMENT, "<!--", 4, "-->", 3 },
		{ TAG_CDATA, "<![CDATA[", 9, "]]>", 3 }
};
static int NB_SPECIAL_TAGS = (int)(sizeof(_spec) / sizeof(_TAG)); /* Auto computation of number of special tags */

/*
 User-registered tags.
 */
static SPECIAL_TAG _user_tags = { NULL, 0 };

int XML_register_user_tag(TagType tag_type, char* start, char* end)
{
	_TAG* p;
	int i, n, le;

	if (tag_type < TAG_USER) return -1;

	if (start == NULL || end == NULL || *start != '<') return -1;

	le = strlen(end);
	if (end[le-1] != '>') return -1;

	i = _user_tags.n_tags;
	n = i + 1;
	p = (_TAG*)__realloc(_user_tags.tags, n * sizeof(_TAG));
	if (p == NULL) return false;

	p[i].tag_type = tag_type;
	p[i].start = start;
	p[i].end = end;
	p[i].len_start = strlen(start);
	p[i].len_end = le;
	_user_tags.tags = p;
	_user_tags.n_tags = n;

	return i;
}

int XML_unregister_user_tag(int i_tag)
{
	if (i_tag < 0 || i_tag > _user_tags.n_tags) return -1;

	_user_tags.tags = (_TAG*)__realloc(_user_tags.tags, (_user_tags.n_tags--) * sizeof(_TAG));

	return _user_tags.n_tags;
}

int XML_get_nb_registered_user_tags(void)
{
	return _user_tags.n_tags;
}

int XML_get_registered_user_tag(TagType tag_type)
{
	int i;

	for (i = 0; i < _user_tags.n_tags; i++)
		if (_user_tags.tags[i].tag_type == tag_type) return i;

	return -1;
}

/* --- XMLNode methods --- */

/*
 Add 'node' to given '*children_array' of '*len_array' elements.
 '*len_array' is overwritten with the number of elements in '*children_array' after its reallocation.
 Return the index of the newly added 'node' in '*children_array', or '-1' for memory error.
 */
static int _add_node(XMLNode*** children_array, int* len_array, XMLNode* node)
{
	XMLNode** pt = (XMLNode**)__realloc(*children_array, (*len_array+1) * sizeof(XMLNode*));
	
	if (pt == NULL) return -1;
	
	pt[*len_array] = node;
	*children_array = pt;
	
	return (*len_array)++;
}

int XMLNode_init(XMLNode* node)
{
	if (node == NULL) return false;
	
	if (node->init_value == XML_INIT_DONE) (void)XMLNode_free(node);

	node->tag = NULL;
	node->text = NULL;
	
	node->attributes = NULL;
	node->n_attributes = 0;
	
	node->father = NULL;
	node->children = NULL;
	node->n_children = 0;
	
	node->tag_type = TAG_NONE;
	node->active = true;

	node->init_value = XML_INIT_DONE;

	return true;
}

XMLNode* XMLNode_allocN(int n)
{
	int i;
	XMLNode* p;
	
	if (n <= 0) return NULL;
	
	p = (XMLNode*)__malloc(n * sizeof(XMLNode));
	if (p == NULL) return NULL;

	for (i = 0; i < n; i++)
		(void)XMLNode_init(&p[i]);
	
	return p;
}

XMLNode* XMLNode_dup(const XMLNode* node, int copy_children)
{
	XMLNode* n;

	if (node == NULL) return NULL;

	n = (XMLNode*)__malloc(sizeof(XMLNode));
	if (n == NULL) return NULL;

	XMLNode_init(n);
	if (!XMLNode_copy(n, node, copy_children)) {
		XMLNode_free(n);

		return NULL;
	}

	return n;
}

int XMLNode_free(XMLNode* node)
{
	int i;

	if (node == NULL || node->init_value != XML_INIT_DONE) return false;
	
	if (node->tag != NULL) {
		__free(node->tag);
		node->tag = NULL;
	}
	if (node->text != NULL) {
		__free(node->text);
		node->text = NULL;
	}

	if (node->attributes != NULL) {
		for (i = 0; i < node->n_attributes; i++) {
			if (node->attributes[i].name != NULL) __free(node->attributes[i].name);
			if (node->attributes[i].value != NULL) __free(node->attributes[i].value);
		}
		__free(node->attributes);
		node->attributes = NULL;
	}
	node->n_attributes = 0;
	
	if (node->children != NULL) {
		for (i = 0; i < node->n_children; i++)
			if (node->children[i] != NULL) {
				(void)XMLNode_free(node->children[i]);
				__free(node->children[i]);
			}
		__free(node->children);
		node->children = NULL;
	}
	node->n_children = 0;
	
	node->tag_type = TAG_NONE;

	return true;
}

int XMLNode_copy(XMLNode* dst, const XMLNode* src, int copy_children)
{
	int i;
	
	if (dst == NULL || (src != NULL &&  src->init_value != XML_INIT_DONE)) return false;
	
	(void)XMLNode_free(dst); /* 'dst' is freed first */
	
	/* NULL 'src' resets 'dst' */
	if (src == NULL) return true;
	
	/* Tag */
	if (src->tag != NULL) {
		dst->tag = __strdup(src->tag);
		if (dst->tag == NULL) goto copy_err;
	}

	/* Text */
	if (dst->text != NULL) {
		dst->text = __strdup(src->text);
		if (dst->text == NULL) goto copy_err;
	}

	/* Attributes */
	if (src->n_attributes > 0) {
		dst->attributes = (XMLAttribute*)__malloc(src->n_attributes * sizeof(XMLAttribute));
		if (dst->attributes== NULL) goto copy_err;
		dst->n_attributes = src->n_attributes;
		for (i = 0; i < src->n_attributes; i++) {
			dst->attributes[i].name = __strdup(src->attributes[i].name);
			dst->attributes[i].value = __strdup(src->attributes[i].value);
			if (dst->attributes[i].name == NULL || dst->attributes[i].value == NULL) goto copy_err;
			dst->attributes[i].active = src->attributes[i].active;
		}
	}

	dst->tag_type = src->tag_type;
	dst->father = src->father;
	dst->user = src->user;
	dst->active = src->active;
	
	/* Copy children if required */
	if (copy_children) {
		dst->children = (XMLNode**)__malloc(src->n_children * sizeof(XMLNode*));
		if (dst->children == NULL) goto copy_err;
		dst->n_children = src->n_children;
		for (i = 0; i < src->n_children; i++) {
			if (!XMLNode_copy(dst->children[i], src->children[i], true)) goto copy_err;
		}
	}
	
	return true;
	
copy_err:
	(void)XMLNode_free(dst);
	
	return false;
}

int XMLNode_set_active(XMLNode* node, int active)
{
	if (node == NULL || node->init_value != XML_INIT_DONE) return false;

	node->active = active;

	return true;
}

int XMLNode_set_tag(XMLNode* node, const char* tag)
{
	if (node == NULL || tag == NULL || node->init_value != XML_INIT_DONE) return false;
	
	if (node->tag != NULL) __free(node->tag);
	node->tag = __strdup(tag);
	if (node->tag == NULL) return false;
	
	return true;
}

int XMLNode_set_type(XMLNode* node, const TagType tag_type)
{
	if (node == NULL || node->init_value != XML_INIT_DONE) return false;

	switch (tag_type) {
		case TAG_ERROR:
		case TAG_END:
		case TAG_PARTIAL:
		case TAG_NONE: /* Try to auto-determine tag type according to whether its has children */
			return false;

		default:
			node->tag_type = tag_type;
			return true;
	}
}

int XMLNode_set_attribute(XMLNode* node, const char* attr_name, const char* attr_value)
{
	XMLAttribute* pt;
	int i;
	
	if (node == NULL || attr_name == NULL || attr_name[0] == '\0' || node->init_value != XML_INIT_DONE) return -1;
	
	i = XMLNode_search_attribute(node, attr_name, 0);
	if (i >= 0) {
		pt = node->attributes;
		if (pt[i].value != NULL) __free(pt[i].value);
		pt[i].value = __strdup(attr_value);
		if (pt[i].value == NULL) return -1;
	}
	else {
		i = node->n_attributes;
		pt = (XMLAttribute*)__realloc(node->attributes, (i+1) * sizeof(XMLAttribute));
		if (pt == NULL) return 0;

		pt[i].name = __strdup(attr_name);
		pt[i].value = __strdup(attr_value);
		if (pt[i].name != NULL && pt[i].value != NULL) {
			pt[i].active = true;
			node->attributes = pt;
			node->n_attributes = i + 1;
		}
		else {
			node->attributes = (XMLAttribute*)__realloc(pt, i * sizeof(XMLAttribute)); /* Frees memory, cannot fail hopefully! */
			return -1;
		}
	}

	return node->n_attributes;
}

int XMLNode_search_attribute(const XMLNode* node, const char* attr_name, int i_search)
{
	int i;
	
	if (node == NULL || attr_name == NULL || attr_name[0] == '\0' || i_search < 0 || i_search > node->n_attributes) return -1;
	
	for (i = i_search; i < node->n_attributes; i++)
		if (node->attributes[i].active && !strcmp(node->attributes[i].name, attr_name)) return i;
	
	return -1;
}

int XMLNode_remove_attribute(XMLNode* node, int i_attr)
{
	if (node == NULL || node->init_value != XML_INIT_DONE || i_attr < 0 || i_attr >= node->n_attributes) return -1;
	
	/* Free attribute fields first */
	if (node->attributes[i_attr].name != NULL) __free(node->attributes[i_attr].name);
	if (node->attributes[i_attr].value != NULL) __free(node->attributes[i_attr].value);
	
	memmove(&node->attributes[i_attr], &node->attributes[i_attr+1], (node->n_attributes - i_attr - 1) * sizeof(XMLAttribute));
	node->attributes = (XMLAttribute*)__realloc(node->attributes, --(node->n_attributes) * sizeof(XMLAttribute)); /* Frees memory */
	
	return node->n_attributes;
}

int XMLNode_set_text(XMLNode* node, const char* text)
{
	if (node == NULL || node->init_value != XML_INIT_DONE) return false;

	if (text == NULL) { /* We want to remove it => __free node text */
		if (node->text != NULL) {
			__free(node->text);
			node->text = NULL;
		}

		return true;
	}

	/* No text is defined yet => allocate it */
	if (node->text == NULL) {
		node->text = (char*)__malloc(strlen(text)+1);
		if (node->text == NULL) return false;
	}
	else {
		char* p = (char*)__realloc(node->text, strlen(text)+1);
		if (p == NULL) return false;
		node->text = p;
	}

	strcpy(node->text, text);

	return true;
}

int XMLNode_add_child(XMLNode* node, XMLNode* child)
{
	if (node == NULL || child == NULL || node->init_value != XML_INIT_DONE || child->init_value != XML_INIT_DONE) return false;
	
	if (_add_node(&node->children, &node->n_children, child) >= 0) {
		node->tag_type = TAG_FATHER;
		child->father = node;
		return true;
	}
	else
		return true;
}

int XMLNode_get_children_count(const XMLNode* node)
{
	int i, n;

	if (node == NULL || node->init_value != XML_INIT_DONE) return -1;

	for (i = n = 0; i < node->n_children; i++)
		if (node->children[i]->active) n++;
	
	return n;
}

XMLNode* XMLNode_get_child(const XMLNode* node, int i_child)
{
	int i;
	
	if (node == NULL || node->init_value != XML_INIT_DONE || i_child < 0 || i_child > node->n_children) return NULL;
	
	for (i = 0; i < node->n_children; i++) {
		if (!node->children[i]->active)
			i_child++;
		else if (i == i_child)
			return node->children[i];
	}
	
	return NULL;
}

int XMLNode_remove_child(XMLNode* node, int i_child, int free_child)
{
	int i;

	if (node == NULL || node->init_value != XML_INIT_DONE || i_child < 0 || i_child >= node->n_children) return -1;
	
	/* Lookup 'i_child'th active child */
	for (i = 0; i < node->n_children; i++) {
		if (!node->children[i]->active)
			i_child++;
		else if (i == i_child)
			break;
	}
	if (i >= node->n_children) return -1; /* Children is not found */

	/* Free node first */
	(void)XMLNode_free(node->children[i_child]);
	if (free_child) __free(node->children[i_child]);
	
	memmove(&node->children[i_child], &node->children[i_child+1], (node->n_children - i_child - 1) * sizeof(XMLNode*));
	node->children = (XMLNode**)__realloc(node->children, --(node->n_children) * sizeof(XMLNode*)); /* Frees memory */
	if (node->n_children == 0) node->tag_type = TAG_SELF;
	
	return node->n_children;
}

int XMLNode_equal(const XMLNode* node1, const XMLNode* node2)
{
	int i, j;

	if (node1 == node2) return true;

	if (node1 == NULL || node2 == NULL || node1->init_value != XML_INIT_DONE || node2->init_value != XML_INIT_DONE) return false;

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

XMLNode* XMLNode_next_sibling(const XMLNode* node)
{
	int i;
	XMLNode* father;

	if (node == NULL || node->init_value != XML_INIT_DONE || node->father == NULL) return NULL;

	father = node->father;
	for (i = 0; i < father->n_children && father->children[i] != node; i++) ;
	i++; /* father->children[i] is now 'node' next sibling */

	return i < father->n_children ? father->children[i] : NULL;
}

static XMLNode* _XMLNode_next(const XMLNode* node, int in_children)
{
	XMLNode* node2;

	if (node == NULL || node->init_value != XML_INIT_DONE) return NULL;

	/* Check first child */
	if (in_children && node->n_children > 0) return node->children[0];

	/* Check next sibling */
	if ((node2 = XMLNode_next_sibling(node)) != NULL) return node2;

	/* Check next uncle */
	return _XMLNode_next(node->father, false);
}

XMLNode* XMLNode_next(const XMLNode* node)
{
	return _XMLNode_next(node, true);
}

/* --- XMLDoc methods --- */

int XMLDoc_init(XMLDoc* doc)
{
	if (doc == NULL) return false;

	if (doc->init_value == XML_INIT_DONE) XMLDoc_free(doc);

	doc->filename[0] = '\0';
	doc->nodes = NULL;
	doc->n_nodes = 0;
	doc->i_root = -1;
	doc->init_value = XML_INIT_DONE;

	return true;
}

int XMLDoc_free(XMLDoc* doc)
{
	int i;
	
	if (doc == NULL || doc->init_value != XML_INIT_DONE) return false;

	for (i = 0; i < doc->n_nodes; i++) {
		(void)XMLNode_free(doc->nodes[i]);
		__free(doc->nodes[i]);
	}
	__free(doc->nodes);
	doc->nodes = NULL;
	doc->n_nodes = 0;
	doc->i_root = -1;

	return true;
}

int XMLDoc_set_root(XMLDoc* doc, int i_root)
{
	if (doc == NULL || doc->init_value != XML_INIT_DONE || i_root < 0 || i_root >= doc->n_nodes) return false;
	
	doc->i_root = i_root;
	
	return true;
}

int XMLDoc_add_node(XMLDoc* doc, XMLNode* node)
{
	if (doc == NULL || node == NULL || doc->init_value != XML_INIT_DONE) return false;
	
	if (_add_node(&doc->nodes, &doc->n_nodes, node) < 0) return -1;

	if (node->tag_type == TAG_FATHER) doc->i_root = doc->n_nodes - 1; /* Main root node is the last father node */

	return doc->n_nodes;
}

int XMLDoc_remove_node(XMLDoc* doc, int i_node, int free_node)
{
	if (doc == NULL || doc->init_value != XML_INIT_DONE || i_node < 0 || i_node > doc->n_nodes) return false;

	/* Free node first */
	(void)XMLNode_free(doc->nodes[i_node]);
	if (free_node) __free(doc->nodes[i_node]);
	
	memmove(&doc->nodes[i_node], &doc->nodes[i_node+1], (doc->n_nodes - i_node - 1) * sizeof(XMLNode*));
	doc->nodes = (XMLNode**)__realloc(doc->nodes, --(doc->n_nodes) * sizeof(XMLNode*)); /* Frees memory */

	return true;
}

/*
 Helper functions to print formatting before a new tag.
 Returns the new number of characters in the line.
 */
static int _count_new_char_line(const char* str, int nb_char_tab, int cur_sz_line)
{
	for (; *str; str++) {
		if (*str == '\n') cur_sz_line = 0;
		else if (*str == '\t') cur_sz_line += nb_char_tab;
		else cur_sz_line++;
	}
	
	return cur_sz_line;
}
static int _print_formatting(FILE* f, const char* tag_sep, const char* child_sep, int nb_char_tab, int depth, int cur_sz_line)
{
	int i;
	
	if (tag_sep != NULL) {
		fprintf(f, tag_sep);
		cur_sz_line = _count_new_char_line(tag_sep, nb_char_tab, cur_sz_line);
	}
	if (child_sep != NULL) {
		for (i = 0; i < depth; i++) {
			fprintf(f, child_sep);
			cur_sz_line = _count_new_char_line(child_sep, nb_char_tab, cur_sz_line);
		}
	}
	
	return cur_sz_line;
}

int XMLNode_print(const XMLNode* node, FILE* f, const char* tag_sep, const char* child_sep, int keep_text_spaces, int sz_line, int nb_char_tab, int depth)
{
	static int cur_sz_line = 0; /* How many characters are on the current line */
	int i;
	char* p;
	
	if (node == NULL || f == NULL || !node->active) return false;
	
	if (nb_char_tab <= 0) nb_char_tab = 1;
	
	/* Print formatting */
	if (depth < 0) /* UGLY HACK: 'depth' forced negative on very first line so we don't print an extra 'tag_sep' (usually "\n") */
		depth = 0;
	else
		cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
	
	/* Special handling of DOCTYPE */
	if (node->tag_type == TAG_DOCTYPE) {
		/* Search for an unescaped '[' in the DOCTYPE definition, in which case the end delimiter should be ']>' instead of '>' */
		for (p = strchr(node->tag, '['); p != NULL && *(p-1) == '\\'; p = strchr(p+1, '[')) ;
		fprintf(f, "<!DOCTYPE%s%s>", node->tag, p != NULL ? "]" : "");
		/*cur_sz_line += strlen(node->tag) + 10 + (p != NULL ? 1 : 0);*/
		return true;
	}

	/* Check for special tags first */
	for (i = 0; i < NB_SPECIAL_TAGS; i++) {
		if (node->tag_type == _spec[i].tag_type) {
			fprintf(f, "%s%s%s", _spec[i].start, node->tag, _spec[i].end);
			/*cur_sz_line += strlen(_spec[i].start) + strlen(node->tag) + strlen(_spec[i].end) + 2;*/
			return true;
		}
	}

	/* Check for user tags */
	for (i = 0; i < _user_tags.n_tags; i++) {
		if (node->tag_type == _user_tags.tags[i].tag_type) {
			fprintf(f, "%s%s%s", _user_tags.tags[i].start, node->tag, _user_tags.tags[i].end);
			/*cur_sz_line += strlen(_user_tags.tags[i].start) + strlen(node->tag) + strlen(_user_tags.tags[i].end) + 2;*/
			return true;
		}
	}
	
	/* Print tag name */
	fprintf(f, "<%s", node->tag);
	cur_sz_line += strlen(node->tag) + 1;

	/* Print attributes */
	for (i = 0; i < node->n_attributes; i++) {
		if (!node->attributes[i].active) continue;
		cur_sz_line += strlen(node->attributes[i].name) + strlen(node->attributes[i].value) + 3;
		if (sz_line > 0 && cur_sz_line > sz_line) {
			cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
			/* Add extra separator, as if new line was a child of the previous one */
			if (child_sep) {
				fprintf(f, child_sep);
				cur_sz_line = _count_new_char_line(child_sep, nb_char_tab, cur_sz_line);
			}
		}
		/* Attribute name */
		fprintf(f, " %s=", node->attributes[i].name);
		
		/* Attribute value */
		(void)fputc(XML_DEFAULT_QUOTE, f);
		cur_sz_line += fprintHTML(f, node->attributes[i].value) + 2;
		(void)fputc(XML_DEFAULT_QUOTE, f);
	}
	
	/* End the tag if there are no children and no text */
	if (node->n_children == 0 && (node->text == NULL || node->text[0] == '\0')) {
		fprintf(f, "/>");
		/*cur_sz_line += 2;*/
		return true;
	}
	else {
		(void)fputc('>', f);
		cur_sz_line++;
	}
	if (node->text != NULL && node->text[0]) {
		/* Text has to be printed: check if it is only spaces */
		if (!keep_text_spaces) {
			for (p = node->text; *p && isspace(*p); p++) ; /* 'p' points to first non-space character, or to '\0' if only spaces */
		}
		else
			p = node->text; /* '*p' won't be '\0' */
		if (*p) cur_sz_line += fprintHTML(f, node->text);
	}
	
	/* Recursively print children */
	for (i = 0; i < node->n_children; i++)
		(void)XMLNode_print(node->children[i], f, tag_sep, child_sep, keep_text_spaces, sz_line, nb_char_tab, depth+1);
	
	/* Print tag end after children */
		/* Print formatting */
	if (node->n_children > 0)
		cur_sz_line = _print_formatting(f, tag_sep, child_sep, nb_char_tab, depth, cur_sz_line);
	fprintf(f, "</%s>", node->tag);
	/*cur_sz_line += strlen(node->tag) + 3;*/

	return true;
}

int XMLDoc_print(const XMLDoc* doc, FILE* f, const char* tag_sep, const char* child_sep, int keep_text_spaces, int sz_line, int nb_char_tab)
{
	int i, depth;
	
	if (doc == NULL || f == NULL || doc->init_value != XML_INIT_DONE) return false;
	
	depth = -1; /* UGLY HACK: 'depth' forced negative on very first line so we don't print an extra 'tag_sep' (usually "\n") */
	for (i = 0; i < doc->n_nodes; i++) {
		(void)XMLNode_print(doc->nodes[i], f, tag_sep, child_sep, keep_text_spaces, sz_line, nb_char_tab, depth);
		depth = 0;
	}
	/* TODO: Find something more graceful than 'depth=-1', even though everyone knows I'll probably never will ;) */

	return true;
}

/* --- */

int XML_parse_attribute(const char* str, XMLAttribute* xmlattr)
{
	const char *p;
	int i, n0, n1, remQ = 0;
	int ret = 1;
	char quote;
	
	if (str == NULL || xmlattr == NULL) return 0;
	
	/* Search for the '=' */
	/* 'n0' is where the attribute name stops, 'n1' is where the attribute value starts */
	for (n0 = 0; str[n0] && str[n0] != '=' && !isspace(str[n0]); n0++) ; /* Search for '=' or a space */
	for (n1 = n0; str[n1] && isspace(str[n1]); n1++) ; /* Search for something not a space */
	if (str[n1] != '=') return 0; /* '=' not found: malformed string */
	for (n1++; str[n1] && isspace(str[n1]); n1++) ; /* Search for something not a space */
	if (isquote(str[n1])) { /* Remove quotes */
		quote = str[n1];
		remQ = 1;
	}
	
	xmlattr->name = (char*)__malloc(n0+1);
	xmlattr->value = (char*)__malloc(strlen(str) - n1 - remQ);
	xmlattr->active = true;
	if (xmlattr->name != NULL && xmlattr->value != NULL) {
		/* Copy name */
		strncpy(xmlattr->name, str, n0);
		xmlattr->name[n0] = '\0';
		(void)str_unescape(xmlattr->name);
		/* Copy value (p starts after the quote (if any) and stops at the end of 'str'
		  (skipping the quote if any, hence the '*(p+remQ)') */
		for (i = 0, p = str + n1 + remQ; *(p+remQ); i++, p++)
			xmlattr->value[i] = *p;
		xmlattr->value[i] = '\0';
		(void)html2str(str_unescape(xmlattr->value), NULL); /* Convert HTML escape sequences */
		if (remQ && *p != quote) ret = 2; /* Quote at the beginning but not at the end */
	}
	else ret = 0;
	
	if (ret == 0) {
		if (xmlattr->name != NULL) __free(xmlattr->name);
		if (xmlattr->value != NULL) __free(xmlattr->value);
	}
	
	return ret;
}

static TagType _parse_special_tag(const char* str, int len, _TAG* tag, XMLNode* node)
{
	if (strncmp(str, tag->start, tag->len_start)) return TAG_NONE;

	if (strncmp(str + len - tag->len_end, tag->end, tag->len_end)) return TAG_PARTIAL; /* There probably is a '>' inside the tag */

	node->tag = (char*)__malloc(len - tag->len_start - tag->len_end + 1);
	if (node->tag == NULL) return TAG_NONE;
	strncpy(node->tag, str + tag->len_start, len - tag->len_start - tag->len_end);
	node->tag[len - tag->len_start - tag->len_end] = 0;
	node->tag_type = tag->tag_type;

	return node->tag_type;
}

/*
 Reads a string that is supposed to be an xml tag like '<tag (attribName="attribValue")* [/]>' or '</tag>'.
 Fills the 'xmlnode' structure with the tag name and its attributes.
 Returns 0 if an error occurred (malformed 'str' or memory). 'TAG_*' when string is recognized.
 */
TagType XML_parse_1string(char* str, XMLNode* xmlnode)
{
	char *p, c;
	XMLAttribute* pt;
	int n, nn, len, tag_end = 0;
	
	if (str == NULL || xmlnode == NULL) return TAG_ERROR;
	len = strlen(str);
	
	/* Check for malformed string */
	if (str[0] != '<' || str[len-1] != '>') return TAG_ERROR;

	for (nn = 0; nn < NB_SPECIAL_TAGS; nn++) {
		n = (int)_parse_special_tag(str, len, &_spec[nn], xmlnode);
		switch (n) {
			case TAG_NONE:	break;				/* Nothing found => do nothing */
			default:		return (TagType)n;	/* Tag found => return it */
		}
	}

	/* "<!DOCTYPE" requires a special handling because it can end with "]>" instead of ">" if a '[' is found inside */
	if (str[1] == '!') {
		/* DOCTYPE */
		if (!strncmp(str, "<!DOCTYPE", 9)) {
			for (n = 9; str[n] && str[n] != '['; n++) ; /* Look for a '[' inside the DOCTYPE, which would mean that we should be looking for a "]>" tag end */
			nn = 0;
			if (str[n]) { /* '[' was found */
				if (strncmp(str+len-2, "]>", 2)) return TAG_PARTIAL; /* There probably is a '>' inside the DOCTYPE */
				nn = 1;
			}
			xmlnode->tag = (char*)__malloc(len-9-nn); /* 'len' - "<!DOCTYPE" and ">" + '\0' */
			if (xmlnode->tag == NULL) return TAG_ERROR;
			strncpy(xmlnode->tag, str+9, len-10-nn);
			xmlnode->tag[len-10-nn] = '\0';
			xmlnode->tag_type = TAG_DOCTYPE;

			return TAG_DOCTYPE;
		}
	}
	
	/* Test user tags */
	for (nn = 0; nn < _user_tags.n_tags; nn++) {
		n = _parse_special_tag(str, len, &_user_tags.tags[nn], xmlnode);
		switch (n) {
			case TAG_ERROR:	return TAG_NONE;	/* Error => exit */
			case TAG_NONE:	break;				/* Nothing found => do nothing */
			default:		return (TagType)n;	/* Tag found => return it */
		}
	}

	if (str[1] == '/') tag_end = 1;
	
	/* tag starts at index 1 (or 2 if tag end) and ends at the first space or '/>' */
	for (n = 1 + tag_end; str[n] && str[n] != '>' && str[n] != '/' && !isspace(str[n]); n++) ;
	xmlnode->tag = (char*)__malloc(n - tag_end);
	if (xmlnode->tag == NULL) return TAG_ERROR;
	strncpy(xmlnode->tag, str+1+tag_end, n-1-tag_end);
	xmlnode->tag[n-1-tag_end] = '\0';
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
		pt = (XMLAttribute*)__realloc(xmlnode->attributes, (xmlnode->n_attributes + 1) * sizeof(XMLAttribute));
		if (pt == NULL) goto parse_err;
		
		xmlnode->n_attributes++;
		xmlnode->attributes = pt;
		while (*p && isspace(*++p)) ; /* Skip spaces */
		if (isquote(*p)) { /* Attribute value starts with a quote, look for next one, ignoring protected ones with '\' */
			for (nn = p-str+1; str[nn] && str[nn] != *p; nn++) {
				if (str[nn] == '\\') nn++;
			}
			nn++;
		}
		else { /* Attribute value stops at first space or end of XML string */
			for (nn = p-str+1; str[nn] && !isspace(str[nn]) && str[nn] != '/' && str[nn] != '>'; nn++) ; /* Go to the end of the attribute value */
		}
		
		/* Here 'str[nn]' is '>' */
		/* the attribute definition ('attrName="attr val"') is between 'str[n]' and 'str[nn]' */
		c = str[nn]; /* Backup character */
		str[nn] = '\0'; /* End string to call 'parse_XML_attribute' */
		if (!XML_parse_attribute(str+n, &xmlnode->attributes[xmlnode->n_attributes - 1])) goto parse_err;
		str[nn] = c;
		
		n = nn;
	}
	
	fprintf(stderr, "\nWE SHOULD NOT BE HERE!\n[%s]\n\n", str);
	
parse_err:
	(void)XMLNode_free(xmlnode);

	return TAG_ERROR;
}

static int _parse_data_SAX(void* in, const DataSourceType in_type, const SAX_Callbacks* sax, SAX_Data* sd)
{
	char *line, *txt_end, *p;
	XMLNode node;
	int ret, exit, sz, n0, ncr;
	TagType tag_type;
	int (*meos)(void* ds) = (in_type == DATA_SOURCE_BUFFER ? (int(*)(void*))_beob : (int(*)(void*))feof);

	if (sax->start_doc != NULL && !sax->start_doc(sd)) return true;
	if (sax->all_event != NULL && !sax->all_event(XML_EVENT_START_DOC, NULL, (char*)sd->name, 0, sd)) return true;

	ret = true;
	exit = false;
	sd->line_num = 1; /* Line counter, starts at 1 */
	sz = 0; /* 'line' buffer size */
	(void)XMLNode_init(&node);
	while ((n0 = read_line_alloc(in, in_type, &line, &sz, 0, '\0', '>', true, '\n', &ncr)) != 0) {
		(void)XMLNode_free(&node);
		for (p = line; *p && isspace(*p); p++) ; /* Checks if text is only spaces */
		if (*p == '\0') break;
		sd->line_num += ncr;

		/* Get text for 'father' (i.e. what is before '<') */
		while ((txt_end = strchr(line, '<')) == NULL) { /* '<' was not found, indicating a probable '>' inside text (should have been escaped with '&gt;' but we'll handle that ;) */
			n0 = read_line_alloc(in, in_type, &line, &sz, n0, 0, '>', true, '\n', &ncr); /* Go on reading the file from current position until next '>' */
			sd->line_num += ncr;
			if (!n0) {
				if (sax->on_error == NULL && sax->all_event == NULL)
					fprintf(stderr, "%s:%d: MEMORY ERROR.\n", sd->name, sd->line_num);
				else {
					if (sax->on_error != NULL && !sax->on_error(PARSE_ERR_MEMORY, sd->line_num, sd)) break;
					if (sax->all_event != NULL && !sax->all_event(XML_EVENT_ERROR, NULL, (char*)sd->name, PARSE_ERR_SYNTAX, sd)) break;
				}
				ret = false;
				break; /* 'txt_end' is still NULL here so we'll display the syntax error below */
			}
		}
		if (txt_end == NULL) { /* Missing tag start */
			if (sax->on_error == NULL && sax->all_event == NULL)
				fprintf(stderr, "%s:%d: ERROR: Unexpected end character '>', without matching '<'!\n", sd->name, sd->line_num);
			else {
				if (sax->on_error != NULL && !sax->on_error(PARSE_ERR_UNEXPECTED_TAG_END, sd->line_num, sd)) break;
				if (sax->all_event != NULL && !sax->all_event(XML_EVENT_ERROR, NULL, (char*)sd->name, PARSE_ERR_UNEXPECTED_TAG_END, sd)) break;
			}
			ret = false;
			break;
		}
		/* First part of 'line' (before '<') is to be added to 'father->text' */
		*txt_end = '\0'; /* Have 'line' be the text for 'father' */
		if (*line != '\0' && (sax->new_text != NULL || sax->all_event != NULL)) {
			if (sax->new_text != NULL && !sax->new_text(str_unescape(line), sd)) break;
			if (sax->all_event != NULL && !sax->all_event(XML_EVENT_TEXT, NULL, line, sd->line_num, sd)) break;
		}
		*txt_end = '<'; /* Restores tag start */

		switch (tag_type = XML_parse_1string(txt_end, &node)) {
			case TAG_ERROR: /* Memory error */
				if (sax->on_error == NULL && sax->all_event == NULL)
					fprintf(stderr, "%s:%d: MEMORY ERROR.\n", sd->name, sd->line_num);
				else {
					if (sax->on_error != NULL && !sax->on_error(PARSE_ERR_MEMORY, sd->line_num, sd)) break;
					if (sax->all_event != NULL && !sax->all_event(XML_EVENT_ERROR, NULL, (char*)sd->name, PARSE_ERR_SYNTAX, sd)) break;
				}
				ret = false;
				break;
		
			case TAG_NONE:
				p = strchr(txt_end, '\n');
				if (p != NULL) *p = '\0';
				if (sax->on_error == NULL && sax->all_event == NULL)
					fprintf(stderr, "%s:%d: SYNTAX ERROR (%s%s).\n", sd->name, sd->line_num, txt_end, p == NULL ? "" : "...");
				else {
					if (sax->on_error != NULL && !sax->on_error(PARSE_ERR_SYNTAX, sd->line_num, sd)) break;
					if (sax->all_event != NULL && !sax->all_event(XML_EVENT_ERROR, NULL, (char*)sd->name, PARSE_ERR_SYNTAX, sd)) break;
				}
				ret = false;
				break;

			case TAG_END:
				if (sax->end_node != NULL || sax->all_event != NULL) {
					if (sax->end_node != NULL && !sax->end_node(&node, sd)) break;
					if (sax->all_event != NULL && !sax->all_event(XML_EVENT_END_NODE, &node, NULL, sd->line_num, sd)) break;
				}
				break;

			default: /* Add 'node' to 'father' children */
				/* If the line looks like a comment (or CDATA) but is not properly finished, loop until we find the end. */
				while (tag_type == TAG_PARTIAL) {
					n0 = read_line_alloc(in, in_type, &line, &sz, n0, '\0', '>', true, '\n', &ncr); /* Go on reading the file from current position until next '>' */
					sd->line_num += ncr;
					if (!n0) {
						if (sax->on_error == NULL && sax->all_event == NULL)
							fprintf(stderr, "%s:%d: MEMORY ERROR.\n", sd->name, sd->line_num);
						else {
							if (sax->on_error != NULL && !sax->on_error(PARSE_ERR_MEMORY, sd->line_num, sd)) break;
							if (sax->all_event != NULL && !sax->all_event(XML_EVENT_ERROR, NULL, (char*)sd->name, PARSE_ERR_SYNTAX, sd)) break;
						}
						ret = false;
						break;
					}
					txt_end = strchr(line, '<'); /* In case 'line' has been moved by the '__realloc' in 'read_line_alloc' */
					tag_type = XML_parse_1string(txt_end, &node);
				}
				if (ret == false) break;
				if (sax->start_node != NULL && !sax->start_node(&node, sd)) break;
				if (sax->all_event != NULL && !sax->all_event(XML_EVENT_START_NODE, &node, NULL, sd->line_num, sd)) break;
				if (node.tag_type != TAG_FATHER && (sax->end_node != NULL || sax->all_event != NULL)) {
					if (sax->end_node != NULL && !sax->end_node(&node, sd)) break;
					if (sax->all_event != NULL && !sax->all_event(XML_EVENT_END_NODE, &node, NULL, sd->line_num, sd)) break;
				}
			break;
		}
		if (exit == true || ret == false || meos(in)) break;
	}
	__free(line);
	(void)XMLNode_free(&node);

	if (sax->end_doc != NULL && !sax->end_doc(sd)) return ret;
	if (sax->all_event != NULL) (void)sax->all_event(XML_EVENT_END_DOC, NULL, (char*)sd->name, sd->line_num, sd);

	return ret;
}

int SAX_Callbacks_init(SAX_Callbacks* sax)
{
	if (sax == NULL) return false;

	sax->start_doc = NULL;
	sax->start_node = NULL;
	sax->end_node = NULL;
	sax->new_text = NULL;
	sax->on_error = NULL;
	sax->end_doc = NULL;
	sax->all_event = NULL;

	return true;
}

int DOMXMLDoc_doc_start(SAX_Data* sd)
{
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;

	dom->current = NULL;
	dom->error = PARSE_ERR_NONE;
	dom->line_error = 0;

	return true;
}

int DOMXMLDoc_node_start(const XMLNode* node, SAX_Data* sd)
{
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;
	XMLNode* new_node;
	int i;

	if ((new_node = XMLNode_dup(node, false)) == NULL) goto node_start_err;
	
	if (dom->current == NULL) {
		if ((i = _add_node(&dom->doc->nodes, &dom->doc->n_nodes, new_node)) < 0) goto node_start_err;

		if (dom->doc->i_root < 0 && node->tag_type == TAG_FATHER) dom->doc->i_root = i;
	}
	else {
		if (_add_node(&dom->current->children, &dom->current->n_children, new_node) < 0) goto node_start_err;
	}

	new_node->father = dom->current;
	dom->current = new_node;

	return true;

node_start_err:
	dom->error = PARSE_ERR_MEMORY;
	dom->line_error = sd->line_num;
	(void)XMLNode_free(new_node);
	__free(new_node);

	return false;
}

int DOMXMLDoc_node_end(const XMLNode* node, SAX_Data* sd)
{
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;

	if (dom->current == NULL || strcmp(dom->current->tag, node->tag)) {
		fprintf(stderr, "%s:%d: ERROR - End tag </%s> was unexpected", sd->name, sd->line_num, node->tag);
		if (dom->current != NULL)
			fprintf(stderr, " (</%s> was expected)\n", dom->current->tag);
		else
			fprintf(stderr, " (no node to end)\n");

		dom->error = PARSE_ERR_UNEXPECTED_NODE_END;
		dom->line_error = sd->line_num;

		return false;
	}

	dom->current = dom->current->father;

	return true;
}

int DOMXMLDoc_node_text(char* text, SAX_Data* sd)
{
	char* p = text;
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;

#if 0 /* Keep text, even if it is only spaces */
	while(*p && isspace(*p++)) ;
	if (*p == 0) return true; /* Only spaces */
#endif

	/* If there is no current node to add text to, raise an error, except if text is only spaces, in which case it is probably just formatting */
	if (dom->current == NULL) {
		while(*p && isspace(*p++)) ;
		if (*p == '\0') return true; /* Only spaces */
		dom->error = PARSE_ERR_TEXT_OUTSIDE_NODE;
		dom->line_error = sd->line_num;

		return false; /* There is some "real" text => raise an error */
	}

	if ((dom->current->text = __strdup(text)) == NULL) {
		dom->error = PARSE_ERR_MEMORY;
		dom->line_error = sd->line_num;

		return false;
	}

	return true;
}

int DOMXMLDoc_parse_error(ParseError error_num, int line_number, SAX_Data* sd)
{
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;

	dom->error = error_num;
	dom->line_error = line_number;

	/* Complete error message will be displayed in 'DOMXMLDoc_doc_end' callback */

	return false; /* Stop on error */
}

int DOMXMLDoc_doc_end(SAX_Data* sd)
{
	DOM_through_SAX* dom = (DOM_through_SAX*)sd->user;

	if (dom->error != PARSE_ERR_NONE) {
		char* msg;

		switch (dom->error) {
			case PARSE_ERR_MEMORY:				msg = "MEMORY"; break;
			case PARSE_ERR_UNEXPECTED_TAG_END:	msg = "UNEXPECTED_TAG_END"; break;
			case PARSE_ERR_SYNTAX:				msg = "SYNTAX"; break;
			case PARSE_ERR_TEXT_OUTSIDE_NODE:	msg = "TEXT_OUTSIDE_NODE"; break;
			case PARSE_ERR_UNEXPECTED_NODE_END:	msg = "UNEXPECTED_NODE_END"; break;
			default:							msg = "UNKNOWN"; break;
		}
		fprintf(stderr, "%s:%d: An error was found (%s), loading aborted...\n", sd->name, dom->line_error, msg);
		dom->current = NULL;
		(void)XMLDoc_free(dom->doc);
	}

	return true;
}

int XMLDoc_parse_file_SAX(const char* filename, const SAX_Callbacks* sax, void* user)
{
	FILE* f;
	int ret;
	SAX_Data sd;

	if (sax == NULL || filename == NULL || filename[0] == '\0') return false;

	f = fopen(filename, "rt");
	if (f == NULL) return false;

	sd.name = (char*)filename;
	sd.user = user;
	ret = _parse_data_SAX((void*)f, DATA_SOURCE_FILE, sax, &sd);
	(void)fclose(f);

	return ret;
}

int XMLDoc_parse_buffer_SAX(const char* buffer, const char* name, const SAX_Callbacks* sax, void* user)
{
	DataSourceBuffer dsb = { buffer, 0 };
	SAX_Data sd;

	if (sax == NULL || buffer == NULL) return false;

	sd.name = name;
	sd.user = user;
	return _parse_data_SAX((void*)&dsb, DATA_SOURCE_BUFFER, sax, &sd);
}

int XMLDoc_parse_file_DOM(const char* filename, XMLDoc* doc)
{
	DOM_through_SAX dom;
	SAX_Callbacks sax;

	if (doc == NULL || filename == NULL || filename[0] == '\0' || doc->init_value != XML_INIT_DONE) return false;

	strncpy(doc->filename, filename, sizeof(doc->filename));

	dom.doc = doc;
	sax.start_doc = DOMXMLDoc_doc_start;
	sax.start_node = DOMXMLDoc_node_start;
	sax.end_node = DOMXMLDoc_node_end;
	sax.new_text = DOMXMLDoc_node_text;
	sax.on_error = DOMXMLDoc_parse_error;
	sax.end_doc = DOMXMLDoc_doc_end;
	sax.all_event = NULL;

	if (!XMLDoc_parse_file_SAX(filename, &sax, &dom)) {
		(void)XMLDoc_free(doc);

		return false;
	}

	return true;
}

int XMLDoc_parse_buffer_DOM(const char* buffer, const char* name, XMLDoc* doc)
{
	DOM_through_SAX dom;
	SAX_Callbacks sax;

	if (doc == NULL || buffer == NULL || doc->init_value != XML_INIT_DONE) return false;

	dom.doc = doc;
	dom.current = NULL;
	sax.start_doc = DOMXMLDoc_doc_start;
	sax.start_node = DOMXMLDoc_node_start;
	sax.end_node = DOMXMLDoc_node_end;
	sax.new_text = DOMXMLDoc_node_text;
	sax.on_error = DOMXMLDoc_parse_error;
	sax.end_doc = DOMXMLDoc_doc_end;
	sax.all_event = NULL;

	return XMLDoc_parse_buffer_SAX(buffer, name, &sax, &dom) ? true : XMLDoc_free(doc);
}
