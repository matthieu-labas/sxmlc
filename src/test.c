#pragma warning(disable : 4996)

#include <stdio.h>
//#include <conio.h>
#include <ctype.h>
#include <stdlib.h>
#include "utils.h"
#include "sxmlc.h"
#include "sxmlsearch.h"

void test_gen(void)
{
	XMLNode *node, *node1;
	XMLDoc doc;
	
	XMLDoc_init(&doc);

	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
	XMLDoc_add_node(&doc, node, TAG_PROLOG);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, " Pre-comment ");
	XMLDoc_add_node(&doc, node, TAG_COMMENT);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "\nAnother one\nMulti-line...\n");
	XMLDoc_add_node(&doc, node, TAG_COMMENT);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "properties");
	XMLDoc_add_node(&doc, node, TAG_FATHER); // Becomes root node
	
	node = XMLNode_alloc(1);
	XMLNode_set_comment(node, "Hello World!");
	XMLDoc_add_child_root(&doc, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "data");
	XMLNode_set_text(node, "a >= b && b <= c");
	XMLDoc_add_child_root(&doc, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "structure1");
	XMLNode_set_attribute(node, "name", "spatioconf");
	XMLDoc_add_child_root(&doc, node);
	
	node1 = XMLNode_alloc(1);
	XMLNode_set_tag(node1, "structure2");
	XMLNode_set_attribute(node1, "name", "files");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property3");
	XMLNode_set_attribute(node, "name", "aaa");
	XMLNode_set_attribute(node, "value", "<truc>");
	XMLNode_add_child(node1, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property4");
	XMLNode_set_attribute(node, "name", "bbb");
	XMLNode_set_attribute(node, "readonly", "true");
	XMLNode_set_attribute(node, "value", "txt");
	XMLNode_remove_attribute(node, XMLNode_search_attribute(node, "readonly", 0));
	XMLNode_add_child(node1, node);
	node->attributes[1].active = false;
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "structure5");
	XMLNode_set_attribute(node, "name", "conf2");
	XMLDoc_add_child_root(&doc, node);
	node->active = false;
	
	node1 = XMLNode_alloc(1);
	XMLNode_set_tag(node1, "property6");
	XMLNode_set_attribute(node1, "name", "ddd");
	XMLNode_set_attribute(node1, "readonly", "false");
	XMLNode_set_attribute(node1, "value", "machin2");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property7");
	XMLNode_set_attribute(node, "name", "eee");
	XMLNode_set_attribute(node, "value", "machin3");
	XMLDoc_add_child_root(&doc, node);
	XMLDoc_print(&doc, stdout, "\n", "    ", 0, 4);
	XMLDoc_free(&doc);
}

void test_DOM(void)
{
	FILE* f = NULL;
	XMLDoc doc;

	XMLDoc_init(&doc);

	if (!XMLDoc_parse_file_DOM("/home/matth/Code/workspace/sxmlc/data/test.xml", &doc))
		printf("Error while loading\n");
	//f = fopen("D:\\Sources\\sxmlc\\data\\test.xml", "w+t");
	//f = fopen("/home/matth/Code/workspace/sxmlc/data/testout.xml", "w+t");
	if (f == NULL) f = stdout;
	XMLDoc_print(&doc, f, "\n", "\t", 0, 4);
	/*{
	XMLNode* node;
	for (node = doc.nodes[doc.i_root]; node != NULL; node = XMLNode_next(node))
		printf("<%s>\n", node->tag);
	}*/
	if (f != stdout) fclose(f);
	printf("\nFreeing...\n");
	XMLDoc_free(&doc);
}

static const char* tag_type_names[] = {
	"TAG_NONE",
	"TAG_FATHER",
	"TAG_SELF",
	"TAG_END",
	"TAG_PROLOG",
	"TAG_COMMENT",
	"TAG_PARTIAL_COMMENT",
	"TAG_CDATA",
	"TAG_PARTIAL_CDATA"
};

int start_node(const XMLNode* node, XMLDoc* doc)
{
	int i;
	printf("Start node %s <%s>\n", tag_type_names[node->tag_type], node->tag);
	for (i = 0; i < node->n_attributes; i++)
		printf("\t%s=\"%s\"\n", node->attributes[i].name, node->attributes[i].value);
	return true;
}

int end_node(const XMLNode* node, XMLDoc* doc)
{
	printf("End node %s <%s>\n", tag_type_names[node->tag_type], node->tag);
	return true;
}

int new_text(const char* text, XMLDoc* doc)
{
	char* p = (char*)text;
	while(*p && isspace(*p++)) ;
	if (*p)
		printf("Text: [%s]\n", text);
	return true;
}

void test_SAX(void)
{
	SAX_Callbacks sax;

	sax.start_node = start_node;
	sax.end_node = end_node;
	sax.new_text = new_text;
	if (!XMLDoc_parse_file_SAX("/home/matth/Code/workspace/sxmlc/data/test.xml", &sax, NULL)) {
		printf("Error while loading\n");
		return;
	}
}

void test_search(void)
{
	XMLDoc doc;
	XMLSearch search[3];
	XMLNode* node;
	char* xpath = NULL;

	XMLDoc_init(&doc);

	XMLSearch_init(&search[0]);
	XMLSearch_init(&search[1]);
	XMLSearch_init(&search[2]);

	XMLSearch_search_set_tag(&search[0], "st*");
	XMLSearch_search_add_attribute(&search[0], "name", "st*sub*", true);
	XMLSearch_search_add_attribute(&search[0], "valid", "false", false);
	//XMLSearch_search_set_text(&search[0], "*inside *");

	XMLSearch_search_set_tag(&search[1], "property");
	XMLSearch_search_add_attribute(&search[1], "name", "t?t?", true);

	XMLSearch_search_set_children_search(&search[0], &search[1]);

	if (!XMLDoc_parse_file_DOM("/home/matth/Code/workspace/sxmlc/data/test.xml", &doc)) {
		printf("Error while loading\n");
		return;
	}

	printf("Start search '%s'\n", XMLSearch_get_XPath_string(&search[0], &xpath, '\''));
	free(xpath);
	node = XMLDoc_root(&doc); //doc.nodes[doc.i_root];
	while ((node = XMLSearch_next(node, &search[0])) != NULL) {
		printf("Found match: ");
		XMLNode_print(node, stdout, NULL, NULL, 0, 0, 0);
		printf("\n");
	}
	printf("End search\n");

	XMLSearch_free(&search[0], false);

	XMLDoc_free(&doc);
}

void test_xpath(void)
{
	XMLSearch search;
	char* xpath2 = NULL;
	char xpath[] = "/tagFather[@name, @id!='0', .='toto*']/tagChild[.='text', @attrib='value']";

	if (XMLSearch_init_from_XPath(xpath, &search))
		printf("[%s] => [%s]\n", xpath, XMLSearch_get_XPath_string(&search, &xpath2, '\''));
	else
		printf("Error\n");
	XMLSearch_free(&search, true);
}

void tstre(char* s, char* p)
{
	if (regstrcmp(s, p))
		printf("'%s' and '%s' match\n", s, p);
	else
		printf("'%s' and '%s' DON'T match\n", s, p);
}

void test_regexp(void)
{
	tstre("abc123", "*");
	tstre("abc123", "abc123");
	tstre("abc123", "abc123*");
	tstre("abc123", "aXc123");
	tstre("abc123", "a?c123");
	tstre("abc123", "abc*");
	tstre("abc123", "*123");
	tstre("abc123", "a*3");
	tstre("abc123", "a*1?3");
	tstre("abc1X3", "a*1?3");
	tstre("abc123", "a*1?4?");
	tstre("abc123", "a*1?3*");
	tstre("ab?123", "a*1?3*");
	tstre("ab\\123", "ab\\\\123");
	tstre("ab?123", "ab\\?12*");
	tstre("st2sub1", "st?sub*");
}

void print_split(char* str)
{
	int i, l0, l1, r0, r1;

	if (split_left_right(str, '=', &l0, &l1, &i, &r0, &r1, true, true)) {
		printf("[%s]%s - Left[%d;%d]: [", str, (i < 0 ? " (no sep)": ""), l0, l1);
		for (i = l0; i <= l1; i++) fputc(str[i], stdout);
		printf("], right[%d;%d]: [", r0, r1);
		for (i = r0; i <= r1; i++) fputc(str[i], stdout);
		printf("]\n");
	}
	else
		printf("Malformed [%s]\n", str);
}
void test_split(void)
{
	print_split("attrib=\"value\"");
	print_split("attrib = \"value\"");
	print_split("'attrib' = 'va\\'lue'");
	print_split("'attri\\'b ' = 'va\\'lue'");
	print_split(" attrib = 'va'lue'");
	print_split("\"att\"rib \" = \"val\\\"ue\"");
	print_split("attrib = \"value\"");
	print_split("attrib=\" val ue \"");
	print_split("attrib=");
	print_split("attrib=''");
	print_split("attrib");
}

int main(int argc, char** argv)
{
	//test_gen();
	//test_DOM();
	//test_SAX();
	test_search();
	//test_xpath();
	//test_regexp();
	//test_split();

	//_getch();
	return 0;
}
