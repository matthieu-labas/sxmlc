#if defined(WIN32) || defined(WIN64)
#pragma warning(disable : 4996)
#include <conio.h>
#endif

#include <stdio.h>
#ifdef linux
#include <curses.h>
#endif
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include "../utils.h"
#include "../sxmlc.h"
#include "../sxmlsearch.h"

void test_gen(void)
{
	XMLNode *node, *node1;
	XMLDoc doc;
	
	XMLDoc_init(&doc);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, "xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"");
	XMLNode_set_type(node, TAG_INSTR);
	XMLDoc_add_node(&doc, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, " Pre-comment ");
	XMLNode_set_type(node, TAG_COMMENT);
	XMLDoc_add_node(&doc, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "\nAnother one\nMulti-line...\n");
	XMLNode_set_type(node, TAG_COMMENT);
	XMLDoc_add_node(&doc, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "properties");
	XMLNode_set_type(node, TAG_FATHER);
	XMLDoc_add_node(&doc, node); // Becomes root node
	
	node = XMLNode_alloc();
	XMLNode_set_type(node, TAG_FATHER);
	XMLNode_set_tag(node, "Hello World!");
	XMLDoc_add_child_root(&doc, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "data");
	XMLNode_set_attribute(node, "type", "code");
	XMLNode_set_text(node, "a >= b && b <= c");
	XMLDoc_add_child_root(&doc, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "structure1");
	XMLNode_set_attribute(node, "name", "spatioconf");
	XMLDoc_add_child_root(&doc, node);
	
	node1 = XMLNode_alloc();
	XMLNode_set_tag(node1, "structure2");
	XMLNode_set_attribute(node1, "name", "files");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "property3");
	XMLNode_set_attribute(node, "name", "aaa");
	XMLNode_set_attribute(node, "value", "<truc>");
	XMLNode_add_child(node1, node);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "property4");
	XMLNode_set_attribute(node, "name", "bbb");
	XMLNode_set_attribute(node, "readonly", "true");
	XMLNode_set_attribute(node, "value", "txt");
	XMLNode_remove_attribute(node, XMLNode_search_attribute(node, "readonly", 0));
	XMLNode_add_child(node1, node);
	node->attributes[1].active = false;
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "structure5");
	XMLNode_set_attribute(node, "name", "conf2");
	XMLDoc_add_child_root(&doc, node);
	node->active = false;
	
	node1 = XMLNode_alloc();
	XMLNode_set_tag(node1, "property6");
	XMLNode_set_attribute(node1, "name", "ddd");
	XMLNode_set_attribute(node1, "readonly", "false");
	XMLNode_set_attribute(node1, "value", "machin2");
	XMLNode_add_child(node, node1);
	
	node = XMLNode_alloc();
	XMLNode_set_tag(node, "property7");
	XMLNode_set_attribute(node, "name", "eee");
	XMLNode_set_attribute(node, "value", "machin3");
	XMLDoc_add_child_root(&doc, node);
	XMLDoc_print(&doc, stdout, "\n", "    ", false, 0, 4);

	XMLDoc_free(&doc);
}

void test_DOM(void)
{
	FILE* f = NULL;
	XMLDoc doc;

	XMLDoc_init(&doc);

#if defined(WIN32) || defined(WIN64)
	if (!XMLDoc_parse_file_DOM("G:\\Code\\Workspace\\sxmlc\\data\\test.xml", &doc))
#else
	if (!XMLDoc_parse_file_DOM("/home/matth/Code/workspace/sxmlc/data/test.xml", &doc))
#endif
		printf("Error while loading\n");
#if defined(WIN32) || defined(WIN64)
	f = fopen("G:\\Code\\Workspace\\sxmlc\\data\\testout.xml", "w+t");
#else
	f = fopen("/home/matth/Code/workspace/sxmlc/data/testout.xml", "w+t");
#endif
	if (f == NULL) f = stdout;
	XMLDoc_print(&doc, f, "\n", "\t", false, 0, 4);
	/*{
	XMLNode* node;
	for (node = doc.nodes[doc.i_root]; node != NULL; node = XMLNode_next(node))
		printf("<%s>\n", node->tag);
	}*/
	if (f != stdout) fclose(f);
	printf("\nFreeing...\n");
	XMLDoc_free(&doc);
}

typedef struct _sxs {
	int n_nodes;
	int n_match;
	XMLSearch search;
} SXS;

int inc_node(const XMLNode* node, SAX_Data* sd)
{
	SXS* sxs = (SXS*)sd->user;

	sxs->n_nodes++;
	if (XMLSearch_node_matches(node, &sxs->search)) sxs->n_match++;

	return true;
}

void test_speed_SAX(void)
{
	SAX_Callbacks sax;
	SXS sxs;
	clock_t t0;

	SAX_Callbacks_init(&sax);
	sax.start_node = inc_node;
	sxs.n_nodes = 0;
	sxs.n_match = 0;
	XMLSearch_init(&sxs.search);
	XMLSearch_search_set_tag(&sxs.search, "incategory");
	XMLSearch_search_add_attribute(&sxs.search, "category", "category*", true);
	printf("[SAX] Loading...\n");
	t0 = clock();
	if (!XMLDoc_parse_file_SAX("/home/matth/Code/tmp/big.xml", &sax, &sxs))
		printf("Error while loading\n");
	printf("[SAX] Loaded %d nodes in %d ms, found %d match\n", sxs.n_nodes, (int)((1000.0f * (clock() - t0)) / CLOCKS_PER_SEC), sxs.n_match);
	XMLSearch_free(&sxs.search, false);
}

void test_speed_DOM(void)
{
	XMLDoc doc;
	XMLSearch search;
	XMLNode* node;
	int n_match;
	clock_t t0, t1;

	XMLDoc_init(&doc);

	printf("[DOM] Loading...\n");
	t0 = clock();
	if (!XMLDoc_parse_file_DOM("/home/matth/Code/tmp/big.xml", &doc))
		printf("Error while loading\n");
	t1 = clock();
	printf("[DOM] Loaded in %d ms\n", (int)((1000.0f * (t1 - t0)) / CLOCKS_PER_SEC));
	XMLSearch_init(&search);
	XMLSearch_search_set_tag(&search, "incategory");
	XMLSearch_search_add_attribute(&search, "category", "category*", true);
	n_match = 0;
	node = XMLDoc_root(&doc); //doc.nodes[doc.i_root];
	printf("[DOM] Searching...\n");
	t0 = clock();
	while ((node = XMLSearch_next(node, &search)) != NULL) {
		n_match++;
	}
	printf("[DOM] Found %d matching nodes in %d ms\n", n_match, (int)((1000.0f * (clock() - t0)) / CLOCKS_PER_SEC));
	XMLSearch_free(&search, false);
	t0 = clock();
	XMLDoc_free(&doc);
	printf("[DOM] Freed in %d ms\n", (int)((1000.0f * (clock() - t0)) / CLOCKS_PER_SEC));
}

static const char* tag_type_names[] = {
	"TAG_NONE",
	"TAG_PARTIAL",
	"TAG_FATHER",
	"TAG_SELF",
	"TAG_END",
	"TAG_PROLOG",
	"TAG_COMMENT",
	"TAG_CDATA",
	"TAG_DOCTYPE"
};

int start_node(const XMLNode* node, SAX_Data* sd)
{
	int i;
	printf("Start node %s <%s>\n", node->tag_type == TAG_USER+1 ? "MONTAG" : tag_type_names[node->tag_type], node->tag);
	for (i = 0; i < node->n_attributes; i++)
		printf("\t%s=\"%s\"\n", node->attributes[i].name, node->attributes[i].value);
	return true;
}

int end_node(const XMLNode* node, SAX_Data* sd)
{
	printf("End node %s <%s>\n", node->tag_type == TAG_USER+1 ? "MONTAG" : tag_type_names[node->tag_type], node->tag);
	return true;
}

int new_text(const char* text, SAX_Data* sd)
{
	char* p = (char*)text;
	while(*p && isspace(*p++)) ;
	if (*p)
		printf("Text: [%s]\n", text);
	return true;
}

int allin1(XMLEvent event, const XMLNode* node, char* text, const int n, SAX_Data* sd)
{
	switch(event) {
		case XML_EVENT_START_DOC: printf("Document start\n\n"); return true;
		case XML_EVENT_START_NODE: return start_node(node, sd);
		case XML_EVENT_END_NODE: return end_node(node, sd);
		case XML_EVENT_TEXT: return new_text(text, sd);
		case XML_EVENT_ERROR: printf("%s:%d: ERROR %d\n", sd->name, sd->line_num, n); return true;
		case XML_EVENT_END_DOC: printf("\nDocument end\n"); return true;
		default: return true;
	}
}

void test_SAX(void)
{
	SAX_Callbacks sax;

	SAX_Callbacks_init(&sax);
	//sax.start_node = NULL;//start_node;
	//sax.end_node = NULL;//end_node;
	//sax.new_text = NULL;//new_text;
	sax.all_event = allin1;
	if (!XMLDoc_parse_file_SAX("/home/matth/Code/workspace/sxmlc/data/test.xml", &sax, NULL))
		printf("Error while loading\n");
}

void test_SAX_buffer(void)
{
	SAX_Callbacks sax;

	SAX_Callbacks_init(&sax);
	//sax.start_node = NULL;//start_node;
	//sax.end_node = NULL;//end_node;
	//sax.new_text = NULL;//new_text;
	sax.all_event = allin1;
	if (!XMLDoc_parse_buffer_SAX("<xml><a>text</a><b name='matth'/></xml>", "Buffer1", &sax, NULL))
		printf("Error while loading\n");
}

int depth, max_depth;
int my_start(const XMLNode* node, SAX_Data* sd)
{
	if(++depth > max_depth) max_depth = depth;
	return DOMXMLDoc_node_start(node, sd);
}
int my_end(const XMLNode* node, SAX_Data* sd)
{
	depth--;
	return DOMXMLDoc_node_end(node, sd);
}
void test_DOM_from_SAX(void)
{
	DOM_through_SAX dom;
	SAX_Callbacks sax;
	XMLDoc doc;

	XMLDoc_init(&doc);
	dom.doc = &doc;
	dom.current = NULL;
	SAX_Callbacks_init(&sax);
	sax.start_node = my_start;
	sax.end_node = my_end;
	sax.new_text = DOMXMLDoc_node_text;
	depth = max_depth = 0;
	if (!XMLDoc_parse_file_SAX("/home/matth/Code/tmp/big.xml", &sax, &dom))
		printf("Failed\n");
	//XMLDoc_print(&doc, stdout, "\n", "  ", 0, 4);
	XMLDoc_free(&doc);
	printf("Max depth: %d\n", max_depth);
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

	printf("Start search '%s'\n", XMLSearch_get_XPath_string(&search[0], &xpath, '\"'));
	free(xpath);
	node = XMLDoc_root(&doc); //doc.nodes[doc.i_root];
	while ((node = XMLSearch_next(node, &search[0])) != NULL) {
		printf("Found match: ");
		XMLNode_print(node, stdout, NULL, NULL, false, 0, 0, 0);
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
		printf("%s\n %s\n", xpath, XMLSearch_get_XPath_string(&search, &xpath2, '\''));
	else
		printf("Error\n");
	XMLSearch_free(&search, true);
}

static void tstre(char* s, char* p)
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

void test_NodeXPath(void)
{
	XMLNode node, node1;
	char* buf;

	XMLNode_init(&node);
	XMLNode_set_tag(&node, "monroot");

	XMLNode_init(&node1);
	XMLNode_set_tag(&node1, "montag");
	XMLNode_set_text(&node1, "This <is> \"some\" text & chars");
	XMLNode_set_attribute(&node1, "name", "first one");
	XMLNode_set_attribute(&node1, "readonly", "fa<l>se");
	XMLNode_set_attribute(&node1, "value", "T\"B\"D");

	XMLNode_add_child(&node, &node1);

	buf = NULL;
	printf(XMLNode_get_XPath(&node1, &buf, true));
	free(buf);
}

#if 1
int main(int argc, char** argv)
{
	XML_register_user_tag(TAG_USER+1, "<#[MONTAG-", "-]>");
	test_gen();
	//test_DOM();
	//test_SAX();
	//test_SAX_buffer();
	//test_DOM_from_SAX();
	//test_search();
	//test_xpath();
	//test_regexp();
	//test_split();
	//test_speed_DOM();
	//test_speed_SAX();
	//test_NodeXPath();

#if defined(WIN32) || defined(WIN64)
	_getch();
#elif defined(linux)
	//getch();
#endif
	return 0;
}
#endif
