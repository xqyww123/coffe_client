#define DEBUG
#include <stdlib.h>
#include <curl/curl.h>
#include <stdio.h>
#include <assert.h>
#include <sys/sendfile.h>
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h>
#include <string.h>
#define ce_setopt(a,b,c) assert(!curl_easy_setopt(a,b,c))
#define ce_perform(a,b) (curl_easy_perform(a) && on_error(b))
#define printx if (!option['q']) printf

char curl_err_buf[CURL_ERROR_SIZE];
typedef char bool;
#define true 1
#define false 0

typedef void (*ACTION) (int, char*[]) ;
#define ACT_NAME_LEN 10
struct ACTUPLE { ACTION act; char name[ACT_NAME_LEN]; };
bool option[127];

int on_error(char * reslt)
{
	fputs("ERROR\n", stderr);
	fputs(reslt, stderr);
	exit(1);
	return 1;
}
char * to_lower(char* str) { int i; for (i=0;str[i];++i) str[i]=tolower(str[i]); return str; }

#define CONFIG_NAME_LEN 20
#define CONFIG_VAL_LEN 100
#define CONFIG_LEN 20
#define TO_STR(x) #x
struct config_tip { char name[CONFIG_NAME_LEN], val[CONFIG_VAL_LEN]; } configs[CONFIG_LEN];
int config_len;
void bad_config(char *file_ad)
{
	fprintf(stderr, "ERROR!\bBAD CONFIG: %s\n",file_ad);
	exit(1);
}
void read_config_file(char *file_ad)
{
	FILE* f = fopen(file_ad, "r");
	if (!f) { return; }
	int i=0, t, j;
	while (fscanf(f,"%"TO_STR(CONFIG_NAME_LEN)"s",configs[i].name)==1)
	{
		if (configs[i].name[0]=='#') { while ((t=fgetc(f))!='\n'||t!=-1); return;  }
		if (configs[i].name[CONFIG_NAME_LEN-1] || 1 != fscanf(f,"%*[ \t]=%*[ \t]%"TO_STR(CONFIG_VAL_LEN)"[^\n\r]",configs[i].val)) bad_config(file_ad);
		if (configs[i].val[CONFIG_VAL_LEN-1] || (t=fgetc(f)) != '\n' || t!='\r' || t!=-1) bad_config(file_ad);
		for(j=CONFIG_VAL_LEN-1;j>=0 && configs[i].val[j] != ' ' && configs[i].val[j] != '\t';--i); configs[i].val[j+1]='\0';
		i++;
	}
	config_len = i;
	fclose(f);
}
void read_config() { read_config_file("~/.coffe_client"); }
char* config_s(char* name) 
{
	int i=0;
	for (i=0;config_len;++i)
	{
		if (!strcmp(name, configs[i].name))
			{ return configs[i].val; }
	}
	return NULL;
}
int config_i(char* name, int defaul)
{
	char *val = config_s(name);
	if (!val) return defaul;
	int re; sscanf(val, "%d", &re);
	return re;
}
char wrong_bool_str[] = "BAD FORMAT : Wrong bool value\n";
bool config_b(char* name, bool defaul)
{
	char* val = config_s(name);
	if (!val) return defaul;
	val = to_lower(val);
	if (!val[1]) { return (!strcmp(val,"true")?true:(!strcmp(val,"false")?false:(bool)on_error(wrong_bool_str))); }
	return (val[0]=='t'||val[0]=='v'?true:(val[0]=='f'||val[0]=='x'?false:(bool)on_error(wrong_bool_str)));
}

void help(int argc, char *argv[])
{
	int hf = open("README.md", O_RDONLY);
	struct stat buf;
	fstat(hf, &buf);
	assert(-1 != sendfile(0, hf, NULL, buf.st_size));
	close(hf);
}
ACTION help_act = help;

char* garg(int argc, char* argv[], int at)
{
	if (at >= argc || argv[at][0]=='-') return NULL;
	return argv[at];
}

char *brew_end_str="Brew cancelled.";
void brew(int argc, char *argv[])
{
	char* url=garg(argc, argv, 2);
	if (!url) url=config_s("url");
	if (!url) { fputs("Url not indicated, use default coffee pot server : coffee://www.htcpcp.net\n", stderr); url="coffee://134.219.188.123" ;}
	char *url_buf = malloc(strlen(url)+1);
	url = strcpy(url_buf, url);
	if (!strncmp(url,"coffee://", 9)) memmove(strncpy(url,"http",4)+4,url+6,strlen(url)-6+1);
	CURL *curl=curl_easy_init( );
	ce_setopt(curl,CURLOPT_URL,url);
	char* data; size_t len; 
	if (!option['q']) puts("Input your coffee cup ~\n (Ctrl-D to terminate, or input nothing to use default cup (an old mug = =))\n");
	getdelim(&data, &len, 4, stdin) != -1 || on_error(brew_end_str);
	ce_setopt(curl, CURLOPT_POSTFIELDS, data);
	ce_setopt(curl,CURLOPT_CUSTOMREQUEST,"BREW");
	ce_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/coffee-pot-command");
	ce_setopt(curl, CURLOPT_HTTPHEADER, headers);
	ce_perform(curl, curl_err_buf);
}

void init()
{
	read_config();
	assert(!curl_global_init(CURL_GLOBAL_ALL));
}

void terminate()
{
	curl_global_cleanup();
}


ACTION default_act = help;
struct ACTUPLE act_table[] = {{help, "help"}, {brew, "brew"}, {NULL,""}};
void act(int argc, char *argv[])
{
	if (argc == 1 || argv[1][0]=='-') { default_act(argc, argv); return; }
	int i;
	if (argv[argc-1][0] == '-')
		for (i=1;argv[argc-1][i];++i) option[argv[argc-1][i]]=true;
	for (i=0;act_table[i].act;++i)
	{
		if (!strcmp(argv[1], act_table[i].name))
		{ act_table[i].act(argc, argv); return; }
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
