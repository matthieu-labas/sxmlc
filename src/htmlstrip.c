/*
 * htmlstrip.c
 *
 *  Created on: Dec 2, 2010
 *      Author: Matthieu Labas
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "utils.h"
#include "sxmlc.h"

#define N_LIST_INTRIC 8
typedef struct _HTMLContext {
	int in_body;
	int in_list_type[N_LIST_INTRIC];
	int next_list_item[N_LIST_INTRIC];
	int i_list;
	int in_pre;
} HTMLContext;

int html_strip(XMLEvent evt, const XMLNode* node, char* text, int n, HTMLContext* ctx)
{
	int i;

	switch(evt) {
		case XML_EVENT_START_NODE:
			/* Wait for the "body" node */
			if (!strcasecmp(node->tag, "body")) ctx->in_body = true;

			if (!ctx->in_body) break; /* Not in body => do not display anything */

			if (!strcasecmp(node->tag, "br")) printf("\n"); /* Line break */
			else if (!strcasecmp(node->tag, "p")) printf("\n"); /* Paragraph */
			else if (!strcasecmp(node->tag, "pre")) {
				printf("\n");
				ctx->in_pre = true;
			}
			else if (!strcasecmp(node->tag, "ul") || !strcasecmp(node->tag, "ol")) { /* (Un)ordered list */
				if (ctx->i_list < N_LIST_INTRIC-1) {
					ctx->i_list++;
					ctx->in_list_type[ctx->i_list] = (node->tag[0] == 'u' ? 1 : 2);
					ctx->next_list_item[ctx->i_list] = 1;
				}
			}
			else if (!strcasecmp(node->tag, "li")) { /* List item */
				printf("\n");
				for (i = 0; i <= ctx->i_list; i++) printf("\t");
				switch (ctx->in_list_type[ctx->i_list]) {
					case 1: /* Unordered list */
						printf("* ");
						break;

					case 2: /* Ordered list */
						printf("%d. ", ctx->next_list_item[ctx->i_list]++);
						break;

					default:
						break;
				}
			}
			else if ((node->tag[0] == 'h' || node->tag[0] == 'h') && isdigit(node->tag[1])) printf("\n\n"); /* Header */
			break;

		case XML_EVENT_END_NODE:
			if (!strcasecmp(node->tag, "body")) ctx->in_body = false;

			if (!ctx->in_body) break;

			if (!strcasecmp(node->tag, "p") || !strcasecmp(node->tag, "pre")) printf("\n"); /* Paragraph end */
			else if (!strcasecmp(node->tag, "pre")) {
				printf("\n");
				ctx->in_pre = false;
			}
			else if (!strcasecmp(node->tag, "ol") || !strcasecmp(node->tag, "ul")) { /* List end */
				if (ctx->i_list >= 0) ctx->i_list--;
				printf("\n");
			}
			else if ((node->tag[0] == 'h' || node->tag[0] == 'h') && isdigit(node->tag[1])) printf("\n\n"); /* Header end */
			break;

		case XML_EVENT_TEXT:
			if (!ctx->in_body) break;

			if (!ctx->in_pre && isspace(text[0])) printf(" ");
			i = !ctx->in_pre && isspace(text[strlen(text)-1]);
			printf(html2str(strip_spaces(text, ctx->in_pre ? 0 : ' ', 0), NULL));
			if (i) printf(" ");
			break;

		case XML_EVENT_ERROR:
			fprintf(stderr, "%s: ERROR %d\n", text, n);
			break;

		default:
			break;
	}

	return true;
}

int usage(char* progname)
{
	return 1;
}

#if 0
int main(int argc, char** argv)
{
	SAX_Callbacks sax = { NULL, NULL, NULL, NULL, NULL, NULL, html_strip };
	HTMLContext ctx;

	//if (argc <= 1) return usage(argv[0]);

	argv[1] = "/home/matth/Code/workspace/sxmlc/doc/howto.html";
	memset(&ctx, 0, sizeof(ctx));
	XMLDoc_parse_file_SAX(argv[1], &sax, &ctx);

	return 1;
}
#endif
