#pragma warning(disable : 4996)

#include <stdio.h>
#include <conio.h>
#include "sxmlc.h"

static const char* tag_type_names[] = {
	"TAG_NONE",
	"TAG_FATHER",
	"TAG_SELF",
	"TAG_END",
	"TAG_PROLOG",
	"TAG_COMMENT",
	"TAG_PARTIAL_COMMENT"
};

int start_doc(const XMLNode* root)
{
	printf("Start doc %s <%s>\n", tag_type_names[root->tag_type], root->tag);
	return true;
}

int end_doc(const XMLNode* root)
{
	printf("End doc %s <%s>\n", tag_type_names[root->tag_type], root->tag);
	return true;
}

int start_node(const XMLNode* node)
{
	printf("Start node %s <%s>\n", tag_type_names[node->tag_type], node->tag);
	return true;
}

int end_node(const XMLNode* node)
{
	printf("End node %s <%s>\n", tag_type_names[node->tag_type], node->tag);
	return true;
}

int new_attribute(const char* name, const char* value)
{
	printf("Attribute %s=\"%s\"\n", name, value);
	return true;
}

int new_text(const char* text)
{
	printf("Text: [%s]\n", text);
	return true;
}


int main(int argc, char** argv)
{
	XMLDoc doc;
	SAX_Callbacks sax;
	XMLNode *node, *node1;
	FILE* f;
	
	XMLDoc_init(&doc);
	
	/*node = XMLNode_alloc(1);
	_parse_XML_1string("<property name=\"file\" value=\"V_0050K_W84_LandBorder/BuildA.shp\"/>", node);
	{
	int i;
	for (i = 0; i < node->n_attributes; i++)
		printf("%s: %s\n", node->attributes[i].name, node->attributes[i].value);
	}*/
	
	/* Root node */

	/*node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "properties");
	XMLDoc_set_root(&doc, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
	XMLDoc_add_pre_root_node(&doc, node, TAG_PROLOG);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, " Pre-comment ");
	XMLDoc_add_pre_root_node(&doc, node, TAG_COMMENT);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "\nAnother one\nMulti-line...\n");
	XMLDoc_add_pre_root_node(&doc, node, TAG_COMMENT);
	
	node = XMLNode_alloc(1);
	XMLNode_set_comment(node, "Hello World!");
	XMLNode_add_child(doc.root, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "data");
	XMLNode_set_text(node, "a >= b && b <= c");
	XMLNode_add_child(doc.root, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "structure1");
	XMLNode_add_attribute(node, "name", "spatioconf");
	XMLNode_add_child(doc.root, node);
	
	node1 = XMLNode_alloc(1);
	XMLNode_set_tag(node1, "structure2");
	XMLNode_add_attribute(node1, "name", "files");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property3");
	XMLNode_add_attribute(node, "name", "aaa");
	XMLNode_add_attribute(node, "value", "<truc>");
	XMLNode_add_child(node1, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property4");
	XMLNode_add_attribute(node, "name", "bbb");
	XMLNode_add_attribute(node, "readonly", "true");
	XMLNode_add_attribute(node, "value", "txt");
	XMLNode_add_child(node1, node);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "structure5");
	XMLNode_add_attribute(node, "name", "conf2");
	XMLNode_add_child(doc.root, node);
	
	node1 = XMLNode_alloc(1);
	XMLNode_set_tag(node1, "property6");
	XMLNode_add_attribute(node1, "name", "ddd");
	XMLNode_add_attribute(node1, "readonly", "false");
	XMLNode_add_attribute(node1, "value", "machin2");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc(1);
	XMLNode_set_tag(node, "property7");
	XMLNode_add_attribute(node, "name", "eee");
	XMLNode_add_attribute(node, "value", "machin3");
	XMLNode_add_child(doc.root, node);
	XMLDoc_print(&doc, stdout, "\n", "    ", 0, 4);
	XMLDoc_free(&doc);*/

	/*if (!XMLDoc_parse_file_DOM("D:\\Sources\\sxmlc\\data\\conf.xml", &doc))
		printf("Error while loading\n");
	f = fopen("D:\\Sources\\sxmlc\\data\\test.xml", "w+t");
	if (f == NULL) f = stdout;
	XMLDoc_print(&doc, f, "\n", "\t", 0, 4);
	if (f != stdout) fclose(f);
	printf("\nFreeing...\n");
	XMLDoc_free(&doc);*/
	sax.start_doc = start_doc;
	sax.end_doc = end_doc;
	sax.start_node = start_node;
	sax.end_node = end_node;
	sax.new_attribute = new_attribute;
	sax.new_text = new_text;
	if (!XMLDoc_parse_file_SAX("D:\\Sources\\sxmlc\\data\\conf.xml", &sax))
		printf("Error while loading\n");
	_getch();
	return 0;
}
