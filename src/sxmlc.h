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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define SXMLC_VERSION "3.2.0"

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif

/* Node types */
#define TAG_ERROR	(-1)
#define TAG_NONE	0
#define TAG_PARTIAL	1	/* Tag containing a legal '>' which stopped file reading */
#define TAG_FATHER	2	/* <tag> - Next nodes will be children of this one. */
#define TAG_SELF	3	/* <tag/> - Standalone node. */
#define TAG_END		4	/* </tag> - End of father node. */
#define TAG_INSTR	5	/* <?prolog?> - Processing instructions, or prolog node. */
#define TAG_COMMENT	6	/* <!--comment--> */
#define TAG_CDATA	7	/* <![CDATA[ ]]/> - CDATA node */
#define TAG_DOCTYPE	8	/* <!DOCTYPE [ ]> - DOCTYPE node */

#define TAG_USER	100	/* User-defined tag start */

/* TODO: Performance improvement with some fixed-sized strings ??? (e.g. XMLAttribute.name[64], XMLNode.tag[64]) */

typedef struct _XMLAttribute {
	char* name;
	char* value;
	int active;
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
	
	int tag_type;	/* Node type ('TAG_FATHER', 'TAG_SELF' or 'TAG_END') */
	int active;		/* 'true' to tell that node is active and should be displayed by 'XMLDoc_print' */
/* TODO: Add pointer to next sibling ? */
	
	void* user;	/* Pointer for user data associated to the node */
} XMLNode;

/*
 An XML document.
 */
#define XMLDOC_INIT_DONE 0x22051977
typedef struct _XMLDoc {
	char filename[256];
	XMLNode** nodes;	/* Nodes of the document, including prolog, comments and root nodes */
	int n_nodes;
	int i_root;		/* Index of first root node in 'nodes', -1 if document is empty */
	int init_value;	/* Initialized to 'XMLDOC_INIT_DONE' to indicate that document has been initialized properly */
/* TODO: Add 'root' member as a shortcut to nodes[i_nodes] ? */
} XMLDoc;

/*
 Register an XML tag, giving its 'start' and 'end' string, which should include '<' and '>'.
 The 'tag_type' is user-given and has to be less than or equal to 'TAG_USER'. It will be
 returned as the 'tag_type' member of the XMLNode struct. Note that no test is performed
 to check for an already-existing tag_type.
 Return 'false' if the 'tag_type' is invalid or the new tag could not be registered (e.g. when
 'start' does not start with '<' or 'end' does not end with '>').
 */
int XML_register_user_tag(int tag_type, char* start, char* end);

/*
 Events that can happen when loading an XML document.
 These will be passed to the 'all_event' callback of the SAX parser.
 */
typedef enum _XMLEvent {
	XML_EVENT_START,
	XML_EVENT_END,
	XML_EVENT_TEXT
} XMLEvent;

/*
 User callbacks used for SAX parsing. Return values of these callbacks should be 0 to stop parsing.
 Members can be set to NULL to disable handling of some events.
 All parameters are pointers to structures that will no longer be available after callback returns. It is recommended that
 the callback uses the information and stores it in its own data structure.
 */
typedef struct _SAX_Callbacks {
	/*
	 Callback called when a new node starts (e.g. '<tag>' or '<tag/>').
	 If any, attributes can be read from 'node->attributes'.
	 N.B. '<tag/>' will trigger an immediate call to the 'end_node' callback
	 after the 'start_node' callback.
	 */
	int (*start_node)(const XMLNode* node, void* user);

	/*
	 Callback called when a node ends (e.g. '</tag>' or '<tag/>').
	 */
	int (*end_node)(const XMLNode* node, void* user);

	/*
	 Callback called when text has been found in the last node.
	 */
	int (*new_text)(const char* text, void* user);

	/*
	 Callback called when text has been found in the last node.
	 'event' is the type of event for which the callback was called:
	 	 XML_EVENT_START:
	 	 	 'node' is the node starting, with tag and all attributes initialized.
	 	 	 'textL' and 'textR' are NULL.
	 	 XML_EVENT_END:
	 	 	 'node' is the node ending, with tag, attributes and text initialized.
	 	 	 'textL' and 'textR' are NULL.
	 	 XML_EVENT_TEXT:
	 	 	 'node' is NULL.
	 	 	 'textL' is the text to be added to last node started and not finished.
	 	 	 'textR' is NULL.
	 */
	int (*all_event)(XMLEvent event, const XMLNode* node, const char* textL, const char* textR, void* user);
} SAX_Callbacks;

/*
 Set of SAX callbacks used by 'XMLDoc_parse_file_DOM'.
 These are made available to be able to load an XML document using DOM implementation
 with user-defined code at some point (e.g. counting nodes, running search, ...).
 In this case, the 'XMLDoc_parse_file_SAX' has to be called instead of the 'XMLDoc_parse_file_DOM',
 providing either these callbacks directly, or a functions calling these callbacks.
 To do that, you should initialize the 'doc' member of a 'DOM_through_SAX' struct and call the
 'XMLDoc_parse_file_SAX' giving this struct as a the user pointer.
 */

typedef struct _DOM_through_SAX {
	XMLDoc* doc;		/* Document to fill up */
	XMLNode* current;	/* For internal use */
} DOM_through_SAX;

int DOMXMLDoc_node_start(const XMLNode* node, DOM_through_SAX* dom);
int DOMXMLDoc_node_end(const XMLNode* node, DOM_through_SAX* dom);
int DOMXMLDoc_node_text(const char* text, DOM_through_SAX* dom);

/* --- XMLNode methods --- */

/*
 Fills 'xmlattr' with 'xmlattr->name' to 'attrName' and 'xmlattr->value' to 'attr Value'.
 'str' is supposed to be like 'attrName[ ]=[ ]["]attr Value["]'.
 Return 0 if not enough memory or bad parameters (NULL 'str' or 'xmlattr').
        2 if last quote is missing in the attribute value.
		1 if 'xmlattr' was filled correctly.
 */
int XML_parse_attribute(const char* str, XMLAttribute* xmlattr);

/*
 Reads a string that is supposed to be an xml tag like '<tag (attribName="attribValue")* [/]>' or '</tag>'.
 Fills the 'xmlnode' structure with the tag name and its attributes.
 Returns 0 if an error occurred (malformed 'str' or memory). 'TAG_*' when string is recognized.
 */
int XML_parse_1string(char* str, XMLNode* xmlnode);

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
int XMLNode_copy(XMLNode* dst, const XMLNode* src, int copy_children);

/*
 Set the active/inactive state of 'node'.
 Set 'active' to 'true' to activate 'node' and all its children, and enable its use
 in other functions (e.g. 'XMLDoc_print', 'XMLNode_search_child').
 */
void XMLNode_set_active(XMLNode* node, int active);

/*
 Set 'node' tag.
 Return 'false' for memory error, 'true' otherwise.
 */
int XMLNode_set_tag(XMLNode* node, const char* tag);

/*
 Initialize 'node' as a comment node.
 Equivalent to 'XMLNode_set_tag(node, comment); node->tag_type = TAG_COMMENT;'.
 */
int XMLNode_set_comment(XMLNode* node, const char* comment);

/*
 Add an attribute to 'node' or update an existing one.
 The attribute has a 'name' and a 'value'.
 Return the new number of attributes, or -1 for memory problem.
 */
int XMLNode_set_attribute(XMLNode* node, const char* attr_name, const char* attr_value);

/*
 Search for the attribute 'attr_name' in 'node', starting from index 'isearch'
 and returns its index, or -1 if not found or error.
 */
int XMLNode_search_attribute(const XMLNode* node, const char* attr_name, int isearch);

/*
 Remove attribute index 'i_attr'.
 Return the new number of attributes or -1 on invalid arguments.
 */
int XMLNode_remove_attribute(XMLNode* node, int i_attr);

/*
 Set node text.
 Return 'true' when successful, 'false' on error.
 */
int XMLNode_set_text(XMLNode* node, const char* text);

/*
 Add a child to a node.
 Return 'false' for memory problem, 'true' otherwise.
 */
int XMLNode_add_child(XMLNode* node, const XMLNode* child);

/*
 Search for 'tag' in direct children of 'node', starting from index 'isearch'
 and return its index, or -1 if not found or error.
 */
int XMLNode_search_child(const XMLNode* node, const char* tag, int isearch);

/*
 Remove child index 'ichild'.
 Return the new number of children or -1 on invalid arguments.
 */
int XMLNode_remove_child(XMLNode* node, int ichild);

/*
 Return 'true' if 'node1' is the same as 'node2' (i.e. same tag, same active attributes).
 */
int XMLNode_equal(const XMLNode* node1, const XMLNode* node2);

/*
 Return the next sibling of node 'node', or NULL if 'node' is invalid or the last child
 or if its father could not be determined (i.e. 'node' is a root node).
 */
XMLNode* XMLNode_next_sibling(const XMLNode* node);

/*
 Shortcut macro to return the next node in XML order i.e. first child or next sibling, or NULL
 if 'node' is invalid or the end of its root node is reached.
 */
XMLNode* XMLNode_next(const XMLNode* node);


/* --- XMLDoc methods --- */


/*
 Initializes an already-allocated XML document.
 */
void XMLDoc_init(XMLDoc* doc);

/*
 Free an XML document.
 Return 'false' if 'doc' was not initialized.
 */
int XMLDoc_free(XMLDoc* doc);

/*
 Set the new 'doc' root node among all existing nodes in 'doc'.
 Return 'false' if bad arguments, 'true' otherwise.
 */
int XMLDoc_set_root(XMLDoc* doc, int i_root);

/*
 Add a node to the document, specifying the type.
 If its type is TAG_FATHER, it also sets the document root type.
 Return the node index, or -1 if bad arguments or memory error.
 */
int XMLDoc_add_node(XMLDoc* doc, XMLNode* node, int tag_type);

/*
 Shortcut macro to retrieve root node from a document.
 Equivalent to
 doc->nodes[doc->i_root]
 */
#define XMLDoc_root(doc) (doc)->nodes[(doc)->i_root]

/*
 Shortcut macro to add a node to 'doc' root node.
 Equivalent to
 XMLDoc_add_child_root(XMLDoc* doc, XMLNode* child);
 */
#define XMLDoc_add_child_root(doc, child) XMLNode_add_child((doc)->nodes[(doc)->i_root], (child))

/*
 Prints the node and its children to a file (that can be stdout).
 - 'tag_sep' is the string to use to separate nodes from each other (usually "\n").
 - 'child_sep' is the additional string to put for each child level (usually "\t").
 - 'sz_line' is the maximum number of characters that can be put on a single line. The
   node remainder will be output to extra lines.
 - 'nb_char_tab' is how many characters should be counted for a single '\t' when counting
   characters in the line. It usually is 8 or 4, but at least 1.
 - 'depth' is an internal parameter that is used to determine recursively how deep we are in
   the tree. It should be initialized to 0 at first call.
 */
void XMLNode_print(const XMLNode* node, FILE* f, const char* tag_sep, const char* child_sep, int sz_line, int nb_char_tab, int depth);

/*
 Prints the XML document using 'XMLNode_print':
 - print the pre-root nodes (if any)
 - print the root node (if any)
 */
void XMLDoc_print(const XMLDoc* doc, FILE* f, const char* tag_sep, const char* child_sep, int sz_line, int nb_char_tab);

/*
 Creates a new XML document from a given 'filename' and loads it to 'doc'.
 Return 'false' in case of error (memory or unavailable filename, malformed document), 'true' otherwise.
 */
int XMLDoc_parse_file_DOM(const char* filename, XMLDoc* doc);

/*
 Parse an XML document from a given 'filename', calling SAX callbacks given in the 'sax' structure.
 'user' is a user-given pointer that will be given back to all callbacks.
 Return 'false' in case of error (memory or unavailable filename, malformed document), 'true' otherwise.
 */
int XMLDoc_parse_file_SAX(const char* filename, const SAX_Callbacks* sax, void* user);

/*
 Parse an XML file using the DOM implementation (it is a direct call to 'XMLDoc_parse_file_DOM' function).
 */
int XMLDoc_parse_file(const char* filename, XMLDoc* doc);

#ifdef __cplusplus
}
#endif

#endif
