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
#ifndef _CXML_H_
#define _CXML_H_

#define SXMLC_VERSION "1.1.0"

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

/* Tag types */
#define TAG_NONE	0
#define TAG_FATHER	1	/* <tag> - Next tags will be children of this one. */
#define TAG_SELF	2	/* <tag/> - Standalone tag. */
#define TAG_END		3	/* </tag> - End of father tag/ */
#define TAG_PROLOG	4	/* <?prolog?> - Prolog tag located at the beginning of XML files. */
#define TAG_COMMENT	5	/* <!--comment--> */
#define TAG_PARTIAL_COMMENT	6	/* <!--comment > - Comment containing a '>' which stopped file reading */

/* TODO: Performance improvement with some fixed-sized strings ??? */

typedef struct _XMLAttribute {
	char* name;
	char* value;
} XMLAttribute;

/*
 An XML node.
 */
typedef struct _XMLNode {
	char* tag;					/* Tag name */
	char* text;					/* Text inside the node */
	XMLAttribute* attributes;
	int n_attributes;
	
	struct _XMLNode* father;	/* NULL if root */
	struct _XMLNode** children;
	int n_children;
	
	int tag_type;	/* Tag type ('TAG_FATHER', 'TAG_SELF' or 'TAG_END') */
	
	void* user;	/* Pointer for user data associated to the node */
} XMLNode;

/*
 An XML document.
 */
typedef struct _XMLDoc {
	char filename[256];
	XMLNode** pre_root;	/* Tags before the root one (such as prolog or comments). Whole content is located in 'tag' field */
	int n_pre_root;
	XMLNode* root;		/* NULL if document is empty */
} XMLDoc;

/*
 User callbacks used for SAX parsing. Return values of these callbacks should be 0 to stop parsing.
 Members can be set to NULL to disable handling of some events.
 All parameters are pointers to structures that will no longer be available after callback returns. It is recommended that
 the callback uses the information and stores it in its own data structure.
 */
typedef struct _SAX_Callbacks {
	/*
	 Callback called when root 'root' node has been found.
	 */
	int (*start_doc)(const XMLNode* root);
	/*
	 Callback called when the end of the document has been reached.
	 */
	int (*end_doc)(const XMLNode* root);
	/*
	 Callback called when a new node starts (e.g. '<tag>' or '<tag/>').
	 N.B. '<tag/>' will trigger an immediate call to the 'end_node' callback
	 after the 'start_node' callback.
	 */
	int (*start_node)(const XMLNode* node);
	/*
	 Callback called when a node ends (e.g. '</tag>' or '<tag/>').
	 */
	int (*end_node)(const XMLNode* node);
	/*
	 Callback called when a new attribute has been found for the last node.
	 */
	int (*new_attribute)(const char* name, const char* value);
	/*
	 Callback called when text has been found in the last node.
	 */
	int (*new_text)(const char* text);
} SAX_Callbacks;

/* --- XMLNode methods --- */

/*
 Fills 'xmlattr' with 'xmlattr->name' to 'attrName' and 'xmlattr->value' to 'attr Value'.
 'str' is supposed 'attrName="attr Value"'.
 Return 0 if not enough memory or bad parameters (NULL 'str' or 'xmlattr').
        2 if last quote is missing in the attribute value.
		1 if 'xmlattr' was filled correctly.
 */
int parse_XML_attribute(char* str, XMLAttribute* xmlattr);

/*
 Allocate and initialize XML nodes.
 'n' is the number of contiguous elements to allocate (to create and array).
 Return 'NULL' if not enough memory or the pointer to the elements otherwise.
 */
XMLNode* XMLNode_alloc(int n);

/*
 Initialize an already-allocated XMLNode.
 */
void XMLNode_init(XMLNode* node);

/*
 Free a node and all its children.
 */
void XMLNode_free(XMLNode* node);

/*
 Free XMLNode 'dst' and copy 'src' to 'dst', along with its children if specified.
 If 'src' is NULL, 'dst' is freed and initialized.
 */
int XMLNode_copy(XMLNode* dst, XMLNode* src, int copy_children);

/*
 Set 'node' tag.
 Return 'false' for memory error, 'true' otherwise.
 */
int XMLNode_set_tag(XMLNode* node, char* tag);

/*
 Initialize 'node' as a comment tag.
 */
int XMLNode_set_comment(XMLNode* node, char* comment);

/*
 Add an attribute to 'node'.
 The attribute has a 'name' and a 'value'.
 Return the new number of attributes, or -1 for memory problem.
 */
int XMLNode_add_attribute(XMLNode* node, char* attr_name, char* attr_value);

/*
 Search for the attribute 'attr_name' in 'node', starting from index 'isearch'
 and returns its index, or -1 if not found or error.
 */
int XMLNode_search_attribute(XMLNode* node, char* attr_name, int isearch);

/*
 Remove attribute index 'iattr'.
 Return the new number of attributes.
 */
int XMLNode_remove_attribute(XMLNode* node, int iattr);

/*
 Set node text.
 Return 'true' when successful, 'false' on error.
 */
int XMLNode_set_text(XMLNode* node, char* text);

/*
 Add a child to a node.
 Return 'false' for memory problem, 'true' otherwise.
 */
int XMLNode_add_child(XMLNode* node, XMLNode* child);

/*
 Search for 'tag' in direct children of 'node', starting from index 'isearch'
 and return its index, or -1 if not found or error.
 */
int XMLNode_search_child(XMLNode* node, char* tag, int isearch);

/*
 Remove child index 'ichild'.
 Return the new number of children.
 */
int XMLNode_remove_child(XMLNode* node, int ichild);


/* --- XMLDoc methods --- */


/*
 Initializes an already-allocated XML document.
 */
void XMLDoc_init(XMLDoc* doc);

/*
 Frees an XML document.
 */
void XMLDoc_free(XMLDoc* doc);

/*
 Set the new 'doc' root tag. If 'root' is NULL, free 'doc' root node.
 Return 'false' if bad arguments, 'true' otherwise.
 */
int XMLDoc_set_root(XMLDoc* doc, XMLNode* root);

/*
 Add a "pre-root" node such as prolog or comments, specifying the type.
 Return 'false' if bad arguments or memory error, 'true' otherwise.
 */
int XMLDoc_add_pre_root_node(XMLDoc* doc, XMLNode* node, int tag_type);

/*
 Prints the node and its children to a file (that can be stdout).
 - 'tag_sep' is the string to use to separate tags from each other (usually "\n").
 - 'child_sep' is the additionnal string to put for each child level (usually "\t").
 - 'sz_line' is the maximum number of characters that can be put on a single line. The
   tag remainder will be output to extra lines.
 - 'nb_char_tab' is how many characters should be counted for a single '\t' when counting
   characters in the line. It usually is 8 or 4, but at least 1.
 - 'depth' is an internal parameter that is used to determine recursively how deep we are in
   the tree. It should be initialized to 0 at first call.
 */
void XMLNode_print(XMLNode* node, FILE* f, char* tag_sep, char* child_sep, int sz_line, int nb_char_tab, int depth);

/*
 Prints the XML document using 'XMLNode_print':
 - print the pre-root nodes (if any)
 - print the root node (if any)
 */
void XMLDoc_print(XMLDoc* doc, FILE* f, char* tag_sep, char* child_sep, int sz_line, int nb_char_tab);

/*
 Creates a new XML document from a given 'filename' and loads it to 'doc'.
 Return 'false' in case of error (memory or unavailable filename, malformed document), 'true' otherwise.
 */
int XMLDoc_parse_file_DOM(char* filename, XMLDoc* doc);

/*
 Parse an XML document from a given 'filename', calling SAX callbacks given in the 'sax' structure.
 Return 'false' in case of error (memory or unavailable filename, malformed document), 'true' otherwise.
 */
int XMLDoc_parse_file_SAX(char* filename, SAX_Callbacks* sax);

/*
 Parse an XML file using the DOM implementation (it is a direct call to 'XMLDoc_parse_file_DOM' function).
 */
int XMLDoc_parse_file(char* filename, XMLDoc* doc);

#endif
