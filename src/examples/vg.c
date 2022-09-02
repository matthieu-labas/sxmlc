#if 1
#include <stdlib.h>
#include "sxmlc.h"
#include "sxmlsearch.h"

const SXML_CHAR xpath[] = C2SX("/tagFather[@name, @id!='0', .='toto*']/tagChild[.='text', @attrib='value']");
//const SXML_CHAR xpath[] = C2SX("/tagFather[.='toto*']");

int main() {

    XMLSearch search;
    SXML_CHAR* xpath2 = NULL;
    if (XMLSearch_init_from_XPath(xpath, &search)) {
        //printf("OK\n");
        printf("%s\n%s\n", xpath, XMLSearch_get_XPath_string(&search, &xpath2, '\''));
		free(xpath2);
	} else
        printf("Error\n");
    XMLSearch_free(&search, true);
    
    return 0;
}
#endif
