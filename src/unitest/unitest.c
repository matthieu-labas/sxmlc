#if defined(WIN32) || defined(WIN64)
#pragma warning(disable : 4996)
#include <conio.h>
#else
#define _getch getchar
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
//#define SXMLC_UNICODE
#include "../sxmlc.h"
#include "../sxmlsearch.h"

typedef enum {
	TEST_ERROR = -1,
	TEST_OK = 0,
	TEST_WARN = 1,
} test_result;

// See https://stackoverflow.com/a/240370/1098603
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

#define NOP ;

#define UTEST(test, fmt) do { if (test) { sprintf(msg, fmt, value, expected, error_message == NULL || !error_message[0] ? "" : error_message, at); return false; } return true; } while(0)

// FIXME: Use snprintf(msg) for asserts
static int _assert_true(int value, char* error_message, char* at, char* msg)
{
#define expected true // For sprintf
	UTEST(!value, "%3$s\t(@%4$s)");
#undef expected
}
static int _assert_equals_i(int expected, int value, char* error_message, char* at, char* msg)
{
	UTEST(value != expected, "%d != %d: %s\t(@%s)");
}
static int _assert_equals_f(double expected, double value, char* error_message, char* at, char* msg)
{
	UTEST(value != expected, "%g != %g: %s\t(@%s)");
}
static int _assert_equals_s(char* expected, char* value, char* error_message, char* at, char* msg)
{
	UTEST(strcmp(expected, value), "[%s] != [%s]: %s\t(@%s)");
}

#define assert_true(value, ret, message, clean) do { if (!_assert_true((value), (message), AT, msg)) { clean; return (ret); } } while(0)
#define assert_equals_i(expected, value, ret, message, clean) do { if (!_assert_equals_i((expected), (value), (message), AT, msg)) { clean; return (ret); } } while(0)
#define assert_equals_f(expected, value, ret, message, clean) do { if (!_assert_equals_f((expected), (value), (message), AT, msg)) { clean; return (ret); } } while(0)
#define assert_equals_s(expected, value, ret, message, clean) do { if (!_assert_equals_s((expected), (value), (message), AT, msg)) { clean; return (ret); } } while(0)

static int max_len_name = 0;

#ifndef NO_VTCOLORS // For normal OS
#define NORM	"\e[0m"
#define BOLD	"\e[1m"
#define DEF		"\e[39m"
#define GREEN	"\e[92m"
#define RED		"\e[91m"
#define ORANGE	"\e[93m"
#else // For Windows
#define NORM
#define BOLD
#define DEF
#define GREEN
#define RED
#define ORANGE
#endif

static void print_result(char* test, test_result res, char* descr)
{
	char* tres = "???";
	switch (res) { // TODO: Terminal colors
		case TEST_ERROR: tres = RED"ERROR"DEF; break;
		case TEST_OK: tres = GREEN" OK  "DEF; break;
		case TEST_WARN: tres = ORANGE"WARN "DEF; break;
	}
	printf(BOLD"%*s"NORM" [%5s]", max_len_name, test, tres);
	if (descr == NULL || descr[0] == 0) {
		printf("\n");
	} else {
		printf(" %s\n", descr);
	}
}

static test_result _test_not_implemented(char* msg)
{
	sprintf(msg, "(not implemented)");
	return TEST_WARN;
}

/*
 * Generate XML, marking some nodes and attributes as inactive
 * Save it to a file on disk
 * Reads the file back and check content

<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!-- Pre-comment -->
<!--
Another one
Multi-line...
-->
<properties>
	<!--Hello World!-->
	<data type="code">a &gt;= b &amp;&amp; b &lt;= c</data>
	<structure1 name="spatioconf">
		<structure2 name="files">
			<property3 name="aaa" value="&lt;truc&gt;" />
			<property4 name="bbb" readonly="true" value="txt" />  --->>> <property4 name="bbb" /> ('readonly' property is removed, 'value' property is inactive)
		</structure2>
	</structure1>
	--->>> <structure5 name="conf2"> (node inactive)
	--->>>	<property6 name="ddd" readonly="false" value="machin2" /> (hidden because father is inactive)
	--->>> </structure5>
	<property7 name="eee" value="machin3" />
</properties>

 */
#if defined(WIN32) || defined(WIN64)
#define FIC_NAME "C:\\Temp\\testout.xml"
#else
#define FIC_NAME "/tmp/testout.xml"
#endif

#define INSTR C2SX("xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"")

static test_result test_parse(char* msg)
{
	// TODO: Test basic cases: <tag/> <tag attrib/> <tag attrib=/> <tag attrib ='' /> <tag attrib attrib2=toto /> <tag attrib= attrib2 = long/> <tag attrib = "value" />
}

static test_result test_gen_file(char* msg)
{
	XMLNode *node, *node1;

	XMLDoc doc;
	XMLDoc_init(&doc);

	node = XMLNode_new(TAG_INSTR, INSTR, NULL);
	XMLDoc_add_node(&doc, node);

	node = XMLNode_new_comment(C2SX(" Pre-comment "));
	XMLDoc_add_node(&doc, node);

	node = XMLNode_new_comment(C2SX("\nAnother one\nMulti-line...\n"));
	XMLDoc_add_node(&doc, node);

	node = XMLNode_new(TAG_FATHER, C2SX("properties"), NULL);
	XMLDoc_add_node(&doc, node); // Becomes root node

	node = XMLNode_new_comment(C2SX("Hello World!"));
	XMLDoc_add_child_root(&doc, node);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("data"));
	XMLNode_set_attribute(node, C2SX("type"), C2SX("code"));
	XMLNode_set_text(node, C2SX("a >= b && b <= c"));
	XMLDoc_add_child_root(&doc, node);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("structure1"));
	XMLNode_set_attribute(node, C2SX("name"), C2SX("spatioconf"));
	XMLDoc_add_child_root(&doc, node);

	node1 = XMLNode_alloc();
	XMLNode_set_tag(node1, C2SX("structure2"));
	XMLNode_set_attribute(node1, C2SX("name"), C2SX("files"));
	XMLNode_add_child(node, node1);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("property3"));
	XMLNode_set_attribute(node, C2SX("name"), C2SX("aaa"));
	XMLNode_set_attribute(node, C2SX("value"), C2SX("<truc>"));
	XMLNode_add_child(node1, node);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("property4"));
	XMLNode_set_attribute(node, C2SX("name"), C2SX("bbb"));
	XMLNode_set_attribute(node, C2SX("readonly"), C2SX("true"));
	XMLNode_set_attribute(node, C2SX("value"), C2SX("txt"));
	assert_equals_i(3, node->n_attributes, TEST_ERROR, C2SX("Wrong number of attributes #1"), NOP);
	XMLNode_remove_attribute(node, XMLNode_search_attribute(node, C2SX("readonly"), 0));
	assert_equals_i(2, node->n_attributes, TEST_ERROR, C2SX("Wrong number of attributes #2"), NOP);
	XMLNode_add_child(node1, node);
	node->attributes[1].active = false;
	assert_equals_i(1, XMLNode_get_attribute_count(node), TEST_ERROR, C2SX("Wrong number of attributes #3"), NOP);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("structure5"));
	XMLNode_set_attribute(node, C2SX("name"), C2SX("conf2"));
	XMLDoc_add_child_root(&doc, node);
	assert_equals_i(4, XMLNode_get_children_count(XMLDoc_root(&doc)), TEST_ERROR, C2SX("Wrong number of children nodes #1"), NOP);
	node->active = false;
	assert_equals_i(3, XMLNode_get_children_count(XMLDoc_root(&doc)), TEST_ERROR, C2SX("Wrong number of children nodes #2"), NOP);

	node1 = XMLNode_alloc();
	XMLNode_set_tag(node1, C2SX("property6"));
	XMLNode_set_attribute(node1, C2SX("name"), C2SX("ddd"));
	XMLNode_set_attribute(node1, C2SX("readonly"), C2SX("false"));
	XMLNode_set_attribute(node1, C2SX("value"), C2SX("machin2"));
	XMLNode_add_child(node, node1);

	node = XMLNode_alloc();
	XMLNode_set_tag(node, C2SX("property7"));
	XMLNode_set_attribute(node, C2SX("name"), C2SX("eee"));
	XMLNode_set_attribute(node, C2SX("value"), C2SX("machin3"));
	XMLDoc_add_child_root(&doc, node);

	FILE* f = fopen(FIC_NAME, "w+t");
	assert_true(f != NULL, TEST_WARN, "Cannot create file", NOP);
	XMLDoc_print(&doc, f, C2SX("\n"), C2SX("    "), false, 0, 4);
	fclose(f);

	assert_true(XMLDoc_free(&doc), TEST_ERROR, C2SX("Cannot free document #1"), NOP);

	// Read back raw file
	f = fopen(FIC_NAME, "rt");
	assert_true(f != NULL, TEST_WARN, C2SX("Cannot open file for reading"), NOP);

	char* lines[] = {
		"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>",
		"<!-- Pre-comment -->",
		"<!--",
		"Another one",
		"Multi-line...",
		"-->",
		"<properties>",
		"    <!--Hello World!-->",
		"    <data type=\"code\">a &gt;= b &amp;&amp; b &lt;= c</data>",
		"    <structure1 name=\"spatioconf\">",
		"        <structure2 name=\"files\">",
		"            <property3 name=\"aaa\" value=\"&lt;truc&gt;\"/>",
		"            <property4 name=\"bbb\"/>",
		"        </structure2>",
		"    </structure1>",
		"    <property7 name=\"eee\" value=\"machin3\"/>",
		"</properties>",
	};
	char* line = NULL;
	size_t n = 0;
	for (int i = 0, l; (l = getline(&line, &n, f)) >= 0; i++) {
		// Remove '\r' and '\n'
		for (l--; l >= 0 && (line[l] == '\n' || line[l] == '\r'); l--)
			line[l] = 0;
		char b[3];
		sprintf(b, "%d", i+1);
		assert_equals_s(lines[i], line, TEST_ERROR, b, fclose(f));
	}
	fclose(f);
	if (line != NULL) {
		free(line);
	}

	return TEST_OK;
}

static test_result test_parse_file(char* msg)
{
	// Open 'fic', parse it and check content
	XMLDoc doc;

	XMLDoc_init(&doc);
	assert_true(XMLDoc_parse_file(FIC_NAME, &doc), TEST_ERROR, "Parse", NOP);
	assert_equals_i(4, doc.n_nodes, TEST_ERROR, "Wrong number of root nodes", NOP);
	assert_equals_i(3, doc.i_root, TEST_ERROR, "Bad root node", NOP);
	assert_equals_s(INSTR, doc.nodes[0]->tag, TEST_ERROR, "Wrong instruction tag", NOP);
	assert_equals_s(" Pre-comment ", doc.nodes[1]->tag, TEST_ERROR, NULL, NOP);
	assert_equals_s("\nAnother one\nMulti-line...\n", doc.nodes[2]->tag, TEST_ERROR, NULL, NOP);

	XMLNode* node1 = XMLDoc_root(&doc);
	assert_equals_s("properties", node1->tag, TEST_ERROR, "Wrong root tag", NOP);
	assert_equals_i(4, node1->n_children, TEST_ERROR, NULL, NOP);

	// <!--Hello World!-->
	XMLNode* node = XMLNode_next(node1);
	assert_equals_s("Hello World!", node1->children[0]->tag, TEST_ERROR, NULL, NOP);

	// <data type="code">a &gt;= b &amp;&amp; b &lt;= c</data>
	node = XMLNode_next(node);
	assert_equals_s("data", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_s("a &gt;= b &amp;&amp; b &lt;= c", node->text, TEST_ERROR, NULL, NOP);
	assert_equals_i(1, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("type", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("code", node->attributes[0].value, TEST_ERROR, NULL, NOP);

	// <structure1 name="spatioconf">
	node = XMLNode_next(node);
	assert_equals_s("structure1", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_i(1, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("name", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("spatioconf", node->attributes[0].value, TEST_ERROR, NULL, NOP);

	// <structure2 name="files">
	node1 = XMLNode_next(node);
	assert_true(node1->father == node, TEST_ERROR, C2SX("Wrong father node #1"), NOP);
	node = node1;
	assert_equals_s("structure2", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_i(1, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("name", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("files", node->attributes[0].value, TEST_ERROR, NULL, NOP);

	// <property3 name="aaa" value="&lt;truc&gt;"/>
	node1 = XMLNode_next(node);
	assert_true(node1->father == node, TEST_ERROR, C2SX("Wrong father node #2"), NOP);
	node = node1;
	assert_equals_s("property3", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_i(2, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("name", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("aaa", node->attributes[0].value, TEST_ERROR, NULL, NOP);
	assert_equals_s("value", node->attributes[1].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("<truc>", node->attributes[1].value, TEST_ERROR, NULL, NOP);

	// <property4 name="bbb">
	node1 = XMLNode_next(node);
	assert_true(node1->father == node->father, TEST_ERROR, C2SX("Wrong father node #3"), NOP);
	node = node1;
	assert_equals_s("property4", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_i(1, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("name", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("bbb", node->attributes[0].value, TEST_ERROR, NULL, NOP);

	// <property7 name="eee" value="machin3"/>
	node = XMLNode_next(node);
	assert_true(node->father == XMLDoc_root(&doc), TEST_ERROR, C2SX("Wrong father node #4"), NOP);
	assert_equals_s("property7", node->tag, TEST_ERROR, NULL, NOP);
	assert_equals_i(2, node->n_attributes, TEST_ERROR, NULL, NOP);
	assert_equals_s("name", node->attributes[0].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("eee", node->attributes[0].value, TEST_ERROR, NULL, NOP);
	assert_equals_s("value", node->attributes[1].name, TEST_ERROR, NULL, NOP);
	assert_equals_s("machin3", node->attributes[1].value, TEST_ERROR, NULL, NOP);

	assert_true(XMLNode_next(node) == NULL, TEST_ERROR, C2SX("Unfinished document"), NOP);

	assert_true(XMLDoc_free(&doc), TEST_ERROR, "Cannot free document #2", NOP);

	return TEST_OK;
}


static test_result test_text_node(char* msg)
{
	return _test_not_implemented(msg); // TODO
}


static test_result test_unicode(char* msg)
{
	return _test_not_implemented(msg); // TODO
}


static test_result test_user(char* msg)
{
	return _test_not_implemented(msg); // TODO
}


static test_result test_move(char* msg)
{
	XMLDoc doc; // To ease freeing all nodes
	XMLNode* root = XMLNode_new(TAG_FATHER, "root", NULL);
	XMLNode* nodes[] = {
			XMLNode_new_comment("node1"),
			XMLNode_new_comment("node2"),
			XMLNode_new_comment("node3"),
	};

	XMLDoc_init(&doc);
	XMLDoc_add_node(&doc, root);

	XMLNode_add_child(root, nodes[0]);
	XMLNode_add_child(root, nodes[1]);
	XMLNode_add_child(root, nodes[2]);

	assert_equals_i(1, XMLNode_get_index(nodes[1]), TEST_ERROR, "Bad index", NOP);
	nodes[0]->active = 0;
	assert_equals_i(1, XMLNode_get_index(nodes[2]), TEST_ERROR, "Bad index after inactive", NOP);
	nodes[0]->active = 1;

	assert_equals_i(3, root->n_children, TEST_ERROR, NULL, NOP);
	XMLNode_move_child(root, 1, 0); // Move backward
	assert_equals_s("node2", root->children[0]->tag, TEST_ERROR, "Backward #0", NOP);
	assert_equals_s("node1", root->children[1]->tag, TEST_ERROR, "Backward #1", NOP);
	assert_equals_s("node3", root->children[2]->tag, TEST_ERROR, "Backward #2", NOP);
	XMLNode_move_child(root, 0, 2); // Move forward
	assert_equals_s("node1", root->children[0]->tag, TEST_ERROR, "Forward #0", NOP);
	assert_equals_s("node3", root->children[1]->tag, TEST_ERROR, "Forward #1", NOP);
	assert_equals_s("node2", root->children[2]->tag, TEST_ERROR, "Forward #2", NOP);
	XMLNode_move_child(root, 1, 2); // Back to original
	assert_equals_s("node1", root->children[0]->tag, TEST_ERROR, "Original #0", NOP);
	assert_equals_s("node2", root->children[1]->tag, TEST_ERROR, "Original #1", NOP);
	assert_equals_s("node3", root->children[2]->tag, TEST_ERROR, "Original #2", NOP);
	XMLNode_insert_child(root, XMLNode_new_comment("nodeins"), 1);
	assert_equals_s("node1", root->children[0]->tag, TEST_ERROR, "Insert #0", NOP);
	assert_equals_s("nodeins", root->children[1]->tag, TEST_ERROR, "Insert #1", NOP);
	assert_equals_s("node2", root->children[2]->tag, TEST_ERROR, "Insert #2", NOP);
	assert_equals_s("node3", root->children[3]->tag, TEST_ERROR, "Insert #3", NOP);

	XMLDoc_free(&doc);

	return TEST_OK;
}


static test_result test_search(char* msg)
{
	return _test_not_implemented(msg); // TODO
}



struct _test {
	char* name;
	test_result (*test)(char* msg);
} test_list[] = {
		{ "PARSE MEM", test_parse },
		{ "GENERATION", test_gen_file },
		{ "PARSE FILE", test_parse_file },
		{ "TEXT NODE", test_text_node },
		{ "MOVE", test_move },
		{ "USER", test_user },
		{ "UNICODE", test_unicode },
		{ "SEARCH", test_search },
};

#if 1
int main(int argc, char** argv)
{
	int n_tests = sizeof(test_list) / sizeof(struct _test);
	for (int i = 0; i < n_tests; i++) {
		int len = strlen(test_list[i].name);
		if (len > max_len_name) {
			max_len_name = len;
		}
	}

	char msg[256];
	for (int i = 0; i < n_tests; i++) {
		msg[0] = 0;
		print_result(test_list[i].name, test_list[i].test(msg), msg);
	}

	return 0;
}
#endif
