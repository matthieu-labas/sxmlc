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
#ifndef _SXMLCSEARCH_H_
#define _SXMLCSEARCH_H_

/*
 XML search parameters. Can be initialized from an XPath string.
 A pointer to such structure is given to search functions which can modify
 its content (the 'from' structure).
 */
/* TODO: Add filter on node type ? */
typedef struct _XMLSearch {
	/*
	 Search for nodes which tag match this 'tag' field.
	 If NULL or an empty string, all nodes will be matching.
	 */
	char* tag;

	/*
	 Search for nodes which attributes match all the ones described.
	 If NULL, all nodes will be matching.
	 The 'attribute->active' should be 'false' to de-activate test on a specific attribute.
	 The 'attribute->name' should not be NULL. If corresponding 'attribute->value'
	 is NULL or an empty-string, search will return the first node with an attribute
	 'attribute->name', no matter what is its value.
	 If 'attribute->value' is not NULL, a matching node should have an attribute
	 'attribute->name' with the corresponding value 'attribute->value'.
	 */
	XMLAttribute* attributes;
	int n_attributes;

	/*
	 Search for nodes which text match this 'text' field.
	 If NULL or an empty string, all nodes will be matching.
	 */
	char* text;

	/*
	 Internal use only. Must be initialized to NULL.
	 */
	XMLNode* stop_at;
} XMLSearch;

/*
 Initialize 'search' struct to an empty search.
 No memory freeing is performed.
 Return 'false' when 'search' is NULL.
 */
int XMLSearch_init(XMLSearch* search);

/*
 Free all 'search' members.
 Return 'false' when 'search' is NULL.
 */
int XMLSearch_free(XMLSearch* search);

/*
 Set the search based on tag.
 'tag' should be NULL or empty to search for any node (e.g. search based on attributes
 only). In this case, the previous tag is freed.
 Return 'true' upon successful completion, 'false' for memory error.
 */
int XMLSearch_search_set_tag(XMLSearch* search, char* tag);

/*
 Add an attribute search criteria.
 'attr_name' is mandatory. 'attr_value' should be NULL to test for attribute presence only
 (no test on value).
 An empty string for 'attr_value' is not an equivalent to 'NULL'!
 Return the index of the new attribute, or '-1' for memory error.
 */
int XMLSearch_search_add_attribute(XMLSearch* search, char* attr_name, char* attr_value);

/*
 Search for attribute 'attr_name' in Search attribute list and return its index
 or '-1' if not found.
 */
int XMLSearch_get_attribute_index(XMLSearch* search, char* attr_name);

/*
 Removes the search attribute given by its index 'i_attr'.
 */
int XMLSearch_remove_attribute(XMLSearch* search, int i_attr);

/*
 Set the search based on text content.
 'text' should be NULL or empty to search for any node (e.g. search based on attributes
 only). In this case, the previous text is freed.
 Return 'true' upon successful completion, 'false' for memory error.
 */
int XMLSearch_search_set_text(XMLSearch* search, char* text);

/*
 Check whether a 'node' matches 'search' criterias.
 'node->tag_type' should be 'TAG_FATHER' or 'TAG_SELF' only.
 Return 'false' when no match is found or invalid arguments, 'true'
 if 'node' is a match.
 */
int XMLSearch_node_matches(XMLNode* node, XMLSearch* search);

/*
 Search next matching node, according to search parameters given by 'search'.
 Search starts from node 'from' by scanning all its children.
 'from' ITSELF IS NOT CHECKED ! Direct call to 'XMLSearch_node_matches(from, search);' should
 be made if necessary.
 'test_siblings' should be 'true' if search has to be performed on 'from' siblings as well, in which
 case it will start from the next sibling after 'from'.
 'test_ascend' should be 'true' if search has to be run on uncles as well (siblings of 'from' father).
 <root>
	<a>
		<a1/>
		<a2>
			<a2a/>
			<a2b/>
		</a2>
		<a3/>
	</a>
	<b>
		<b1/>
		<b2/>
		<b3/>
	</b>
 </root>
 If 'from' is node '<a2>':
	- if 'test_siblings' if 'false', search will scan nodes '<a2a/>' and '<a2b/>' and
	  stop, no matter the value of 'test_ascend'.
	- if 'test_siblings' is 'true', search will scan nodes '<a2a/>' and '<a2b/>' then
	  proceed to '<a3/>' and stop, unless 'test_ascend' is 'true', in which case it will
	  proceed with nodes '<b>', '<b1/>', '<b2/>' and '<b3/>'.
 If the document has several root nodes, a complete search in the document should be performed
 by manually calling 'XMLSearch_next' on each root node in a for loop.
 */
XMLNode* XMLSearch_next(XMLNode* from, XMLSearch* search);

#endif
