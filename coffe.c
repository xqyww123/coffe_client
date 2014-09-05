#define DEBUG
#include <stdlib.h>
#include <curl/curl.h>
#include <stdio.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>
#define ce_setopt(a,b,c) assert(!curl_easy_setopt(a,b,c))
#define ce_perform(a,b) (curl_easy_perform(a) && on_error(b))

typedef void (*ACTION) (int, char*[]) ;
#define ACT_NAME_LEN 10
struct ACTUPLE { ACTION act; char name[ACT_NAME_LEN]; };

int on_error(char * reslt)
{
	printf("ERROR!\n%s", reslt);
	exit(1);
	return 1;
}

void help(int argc, char *argv[])
{
	int hf = open("README.md", O_RDONLY);
	struct stat buf;
	fstat(hf, &buf);
	assert(-1 != sendfile(0, hf, NULL, buf.st_size));
	close(hf);
}

void init()
{
	assert(!curl_global_init(CURL_GLOBAL_ALL));
}

void terminate()
{
	curl_global_cleanup();
}


ACTION help_act = help;
ACTION default_act = help;
struct ACTUPLE act_table[] = {{help, "help"}, {NULL,""}};
void act(int argc, char *argv[])
{
	if (argc == 1) { default_act(argc, argv); return; }
	int i;
	for (i=0;act_table[i].act;++i)
	{
		if (!strcmp(argv[1], act_table[i].name))
		{ act_table[i].act(argc, argv); }
	}
	default_act(argc, argv);
}

int main(int argc, char* argv[])
{
	init();
	act(argc, argv);
	terminate();
	return 0;
}
