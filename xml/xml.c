#include <stdio.h>
#include <stdlib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

int main(int argc, char **argv)
{
	xmlDocPtr pdoc = NULL;
	xmlNodePtr proot = NULL, pcur = NULL;
/*****************打开xml文档********************/
	xmlKeepBlanksDefault(0);	//必须加上，防止程序把元素前后的空白文本符号当作一个node
	pdoc = xmlReadFile(argv[1], "ISO-8859-1", XML_PARSE_RECOVER);	//libxml只能解析UTF-8格式数据

	if (pdoc == NULL) {
		printf("error:can't open file!\n");
		exit(1);
	}

/*****************获取xml文档对象的根节对象********************/
	proot = xmlDocGetRootElement(pdoc);

	if (proot == NULL) {
		printf("error: file is empty!\n");
		exit(1);
	}

/*****************查找书店中所有书籍的名称********************/
	pcur = proot->xmlChildrenNode;

	while (pcur != NULL) {
		//printf("screen: %s\n", ((char *)
		printf("%s\n", ((char *)
				XML_GET_CONTENT(pcur->xmlChildrenNode)));
//如同标准C中的char类型一样，xmlChar也有动态内存分配，字符串操作等 相关函数。例如xmlMalloc是动态分配内存的函数；xmlFree是配套的释放内存函数；xmlStrcmp是字符串比较函数等。
//对于char* ch="book", xmlChar* xch=BAD_CAST(ch)或者xmlChar* xch=(const xmlChar *)(ch)
//对于xmlChar* xch=BAD_CAST("book")，char* ch=(char *)(xch)
//if (!xmlStrcmp(pcur->name, BAD_CAST("book")))
#if 0
		if (!xmlStrcmp(pcur->name, BAD_CAST("screen"))) {
			xmlNodePtr nptr = pcur->xmlChildrenNode;
			while (pcur != NULL) {
				printf("screen: %s\n", ((char *)
							XML_GET_CONTENT
							(nptr->
							 xmlChildrenNode)));
				if (!xmlStrcmp(nptr->name, BAD_CAST("title"))) {
					printf("title: %s\n", ((char *)
							       XML_GET_CONTENT
							       (nptr->
								xmlChildrenNode)));
					break;
				}
			}

		}
#endif
		pcur = pcur->next;
	}

/*****************释放资源********************/
	xmlFreeDoc(pdoc);
	xmlCleanupParser();
	xmlMemoryDump();
	return 0;
}
