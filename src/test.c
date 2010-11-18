#pragma warning(disable : 4996)

#include <stdio.h>
//#include <conio.h>
#include <ctype.h>
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
	XMLNode* node;

	XMLDoc_init(&doc);

	if (!XMLDoc_parse_file_DOM("/home/matth/Code/workspace/sxmlc/data/test.xml", &doc))
		printf("Error while loading\n");
	//f = fopen("D:\\Sources\\sxmlc\\data\\test.xml", "w+t");
	//f = fopen("/home/matth/Code/workspace/sxmlc/data/testout.xml", "w+t");
	if (f == NULL) f = stdout;
	XMLDoc_print(&doc, f, "\n", "\t", 0, 4);
	/*for (node = doc.nodes[doc.i_root]; node != NULL; node = XMLNode_next(node))
		printf("<%s>\n", node->tag);*/
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
	XMLSearch search;
	XMLNode* node;

	XMLDoc_init(&doc);

	XMLSearch_init(&search);
	XMLSearch_search_set_tag(&search, "b2*");
	XMLSearch_search_add_attribute(&search, "comment", NULL);
	//XMLSearch_search_add_attribute(&search, "id", "8");

	if (!XMLDoc_parse_file_DOM("/home/matth/Code/workspace/sxmlc/data/test.xml", &doc)) {
		printf("Error while loading\n");
		return;
	}

	printf("Start search\n");
	node = doc.nodes[doc.i_root];
	while ((node = XMLSearch_next(node, &search)) != NULL) {
		printf("Found match: ");
		XMLNode_print(node, stdout, NULL, NULL, 0, 0, 0);
		printf("\n");
	}
	printf("End search\n");

	XMLSearch_free(&search);

	XMLDoc_free(&doc);
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
}

int main(int argc, char** argv)
{

	//test_gen();
	//test_DOM();
	//test_SAX();
	test_search();
	//test_regexp();

	//_getch();
	return 0;
}
