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

#include <string.h>
#include <stdlib.h>
#include "sxmlc.h"
#include "sxmlsearch.h"
#include "utils.h"

#define INVALID_XMLNODE_POINTER ((XMLNode*)-1)

int XMLSearch_init(XMLSearch* search)
{
	if (search == NULL) return false;

	search->tag = NULL;
	search->text = NULL;
	search->attributes = NULL;
	search->n_attributes = 0;
	search->stop_at = INVALID_XMLNODE_POINTER; /* Because 'NULL' can be a valid value */

	return true;
}

int XMLSearch_free(XMLSearch* search)
{
	int i;

	if (search == NULL) return false;

	if (search->tag != NULL) {
		free(search->tag);
		search->tag = NULL;
	}

	if (search->n_attributes > 0 && search->attributes != NULL) {
		for (i = 0; i < search->n_attributes; i++) {
			if (search->attributes[i].name != NULL) free(search->attributes[i].name);
			if (search->attributes[i].value != NULL) free(search->attributes[i].value);
		}
		free(search->attributes);
		search->n_attributes = 0;
		search->attributes = NULL;
	}

	return true;
}

int XMLSearch_search_set_tag(XMLSearch* search, char* tag)
{
	if (search == NULL) return false;

	if (tag == NULL && search->tag != NULL) free(search->tag);

	search->tag = strcpy_alloc(tag);

	return true;
}

int XMLSearch_search_set_text(XMLSearch* search, char* text)
{
	if (search == NULL) return false;

	if (text == NULL && search->text != NULL) free(search->text);

	search->text = strcpy_alloc(text);

	return true;
}

int XMLSearch_search_add_attribute(XMLSearch* search, char* attr_name, char* attr_value)
{
	int i, n;
	XMLAttribute* p;

	if (search == NULL) return -1;

	if (attr_name == NULL || attr_name[0] == 0) return -1;

	n = search->n_attributes + 1;
	p = (XMLAttribute*)realloc(search->attributes, n * sizeof(XMLAttribute));
	if (p == NULL) return -1;

	i = n - 1;
	p[i].active = true;
	p[i].name = strcpy_alloc(attr_name);
	if (p[i].name == NULL) {
		search->attributes = realloc(p, (n-1)*sizeof(XMLAttribute)); /* Reverse back to original size */
		return -1;
	}

	if (attr_value != NULL) {
		p[i].value = strcpy_alloc(attr_value);
		if (p[i].value == NULL) {
			free(p[i].name);
			search->attributes = realloc(p, (n-1)*sizeof(XMLAttribute)); /* Reverse back to original size */
			return -1;
		}
	}

	search->n_attributes = n;
	search->attributes = p;

	return i;
}

int XMLSearch_get_attribute_index(XMLSearch* search, char* attr_name)
{
	int i;

	if (search == NULL || attr_name == NULL || attr_name[0] == 0) return -1;

	for (i = 0; i < search->n_attributes; i++) {
		if (!strcmp(search->attributes[i].name, attr_name)) return i;
	}

	return -1;
}

int XMLSearch_remove_attribute(XMLSearch* search, int i_attr)
{
	if (search == NULL || i_attr < 0 || i_attr >= search->n_attributes) return false;

	/* Free attribute fields first */
	if (search->attributes[i_attr].name != NULL) free(search->attributes[i_attr].name);
	if (search->attributes[i_attr].value != NULL) free(search->attributes[i_attr].value);

	memmove(&search->attributes[i_attr], &search->attributes[i_attr+1], (search->n_attributes - i_attr - 1) * sizeof(XMLAttribute));
	search->attributes = (XMLAttribute*)realloc(search->attributes, --(search->n_attributes) * sizeof(XMLAttribute)); /* Frees memory */

	return true;
}


static int attribute_matches(XMLAttribute* to_test, XMLAttribute* pattern)
{
	if (to_test == NULL && pattern == NULL) return true;

	if (to_test == NULL || pattern == NULL) return false;
	
	/* Inactive attribute */
	if (!to_test->active || !pattern->active) return false;

	/* No test on name => match */
	if (pattern->name == NULL || pattern->name[0] == 0) return true;

	/* Test on name fails => no match */
	if (!regstrcmp(to_test->name, pattern->name)) return false;

	/* No test on value => match */
	if (pattern->value == NULL) return true;

	/* Test on value fails => no match */
	if (!regstrcmp(to_test->value, pattern->value)) return false;

	return true;
}

int XMLSearch_node_matches(XMLNode* node, XMLSearch* search)
{
	int i, j;

	if (node == NULL) return false;

	if (search == NULL) return true;

	/* No comments, prolog, or such type of nodes are tested */
	if (node->tag_type != TAG_FATHER && node->tag_type != TAG_SELF) return false;

	/* Check tag */
	if (search->tag != NULL && !regstrcmp(node->tag, search->tag)) return false;

	/* Check text */
	if (search->text != NULL && !regstrcmp(node->text, search->text)) return false;

	/* Check attributes */
	if (search->attributes != NULL) {
		for (i = 0; i < search->n_attributes; i++) {
			for (j = 0; j < node->n_attributes; j++) {
				if (!node->attributes[j].active) continue;
				if (attribute_matches(&node->attributes[j], &search->attributes[i])) break;
			}
			if (j >= node->n_attributes) return false; /* All attributes where scanned without a successful match */
		}
	}
		
	return true;
}

XMLNode* XMLSearch_next(XMLNode* from, XMLSearch* search)
{
	XMLNode* node;

	if (search == NULL || from == NULL) return NULL;

	/* Initialize the 'stop_at' node on first search, to remember where to stop as there will be multiple calls */
	/* 'stop_at' can be NULL when 'from' is a root node, that is why it should be initialized with something else than NULL */
	if (search->stop_at == INVALID_XMLNODE_POINTER) search->stop_at = XMLNode_next_sibling(from);

	for (node = XMLNode_next(from); node != search->stop_at; node = XMLNode_next(node)) { /* && node != NULL */
		if (XMLSearch_node_matches(node, search)) return node;
	}

	return NULL;
}
