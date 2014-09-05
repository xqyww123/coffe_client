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
#define min(a,b) ((a<b)?a:b)

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
	fputs("ERROR", stderr);
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

void prt_re(FILE* cv)
{
	int c;
	puts("<type any key to view>");
	getchar(); putchar('\n');
	while ((c=fgetc(cv)) != EOF)
	   	putchar(c);
	putchar('\n');
}
int docode(CURL* curl, FILE* cv)
{
	int rcode; char * buf; int c;
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rcode);
	switch(rcode)
	{
		case 406:
			puts("406 Not Acceptable = = \nI'm very sorry but the HTCPCP server is unable to provide the requested addition.\
The response of the server may indicate the reason:\n");
			break;
		case 418:
		case 417:
			printf("%d I'm a teapot >..< \nIt's seems that you indicate a wrong server. The HTCPCP server is a teapot. the resulting entity body may be short and stout. Demonstrations of this behaviour exist. server response:\n", rcode);
			break;
		case 200:
			return 0;
		default:
			printf("%d ???:\n Well, I don't know the return code, in fact it doesn't exist in RFC2324!!!\nServer's response may be helpful:\n", rcode);
			break;
	}
	prt_re(cv);
	return 1;
}

char* get_url(int argc, char* argv[])
{
	char* url=garg(argc, argv, 2);
	if (!url) url=config_s("url");
	if (!url) { fputs("Url not indicated, use default coffee pot server : coffee://www.error418.org\n", stderr); url="coffee://www.error418.org" ;}
	char *url_buf = malloc(strlen(url)+1);
	url = strcpy(url_buf, url);
	if (!strncmp(url,"coffee://", 9)) memmove(strncpy(url,"http",4)+4,url+6,strlen(url)-6+1);
	return url;
}

char* adds[] = {"Cream", "Half-and-half", "Whole-milk", "Part-Skim", "Skim", "Non-Dairy", "Vanilla", \
		"Almond", "Raspberry", "Chocolate", "Whisky", "Rum", "Kahlua", "Aquavit", ""};
int add_len;
void prt_adds()
{
	puts("Additions ~ :");
	int i; for (i=0;i<add_len;++i) printf("%d: %s\t", i, adds[i]);
	putchar('\n');
}
char* mnacts[] = {"list", "select", "want", "drop", "mine", "ok", "finish",""};
void set_menu(struct curl_slist* headers)
{
	puts("Select your addition~ :");
	prt_adds();
	char *in = malloc(1024);
	bool *v = malloc(sizeof(bool[add_len])), whilend = true;
	int i,code;
	while(whilend)
	{
		printf("menu> ");
		scanf("%s", in);
		for (i=0;*mnacts[i] && strncmp(mnacts[i],in,min(strlen(mnacts[i]), strlen(in)));++i);
		if (!*mnacts[i]) i=-1;
		switch(code=i)
		{
			case -1:
				puts("ala >..< invalid command = =\nvalid commands:");
				for (i=0;*mnacts[i];++i) printf("%s\t", mnacts[i]);
				putchar('\n'); break;
			case 0:
				prt_adds(); break;
			case 1:
			case 2:
				if (scanf("%d", &i) != 1) { puts("BAD Usage = =, <select|want> <addition index>"); }
				if (i < 0 || i>=add_len) { puts("BAD Usage = =, index out of range = ="); }
				v[i] = true; 
				printf("Index %d : %s, selected.\n", i, adds[i]);
				break;
			case 3:
				if (scanf("%d", &i) != 1) { puts("BAD Usage = =, <select|want> <addition index>"); }
				if (i < 0 || i>=add_len) { puts("BAD Usage = =, index out of range = ="); }
				v[i] = false;
				printf("Index %d : %s, droped.\n", i, adds[i]);
				break;
			case 4:
				puts("Your additions ~ :");
				int i; for (i=0;i<add_len;++i) { if (v[i]) printf("%d: %s\t", i, adds[i]); }
				putchar('\n');
			   	break;
			case 5:
			case 6:
				puts("Okay, your additions will submit ~");
				for (i=0;i<add_len;++i) { if (v[i]) printf("%d: %s\t", i, adds[i]); }
				i = 0;
				do { if (i!='\r' && i!='\n') puts("\n Is that okay? [y/n]"); } while ((i=tolower(getchar()))!='y' && i!='n');
				if (i=='y') { puts("Okay, submitting."); whilend = false; }
				else { puts("Cancelled."); }
				break;
			default:
				puts("BAD Usage = =");
		}
	}
	bool fisrt;
	fisrt = true;
	int st=strlen("Content-Type: ");
	int hlen=st-1;
	for (i=0;i<add_len;++i)
	{
		if (v[i]) hlen += strlen(adds[i]) + 1;
	}
	char *tah=malloc(sizeof(char[hlen+1]));
	strcpy(tah, "Content-Type: ");
	for (i=0;i<add_len;++i)
	{
		if (v[i]) 
		{
			if (fisrt) { fisrt=false;   }
			else { tah[st++]=';'; }
			strcpy(tah+st,adds[i]);
			st+=strlen(adds[i]);
		}
	}
	puts(tah);
	assert(curl_slist_append(headers,tah ));
	free(in);
}

char *brew_end_str="Brew cancelled.";
void brew(int argc, char *argv[])
{
	char* url = get_url(argc, argv);
	CURL *curl=curl_easy_init( );
	char* data=NULL; size_t len=0; 
	if (!option['q']) puts("Input your coffee cup ~\n (Ctrl-D to terminate, or input nothing to use default cup (an old mug = =))");
	if (getdelim(&data, &len, 4, stdin) == -1) on_error(brew_end_str);
	ce_setopt(curl,CURLOPT_URL,url);
	ce_setopt(curl, CURLOPT_POSTFIELDS, data);
	ce_setopt(curl,CURLOPT_CUSTOMREQUEST,"BREW");
	ce_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
	struct curl_slist *headers=NULL;
	assert(curl_slist_append(headers, "Content-Type: application/coffee-pot-command"));
	set_menu(headers);
	ce_setopt(curl, CURLOPT_HTTPHEADER, headers);
	FILE* cv=fmemopen(NULL,sizeof(char[10240]),"w+");
	ce_setopt(curl, CURLOPT_WRITEDATA, cv);
	ce_perform(curl, curl_err_buf);
	fseek(cv,0,SEEK_SET);
	if (!docode(curl, cv))
	{
		int rcode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rcode);
		if (rcode == 200)
		{
			puts("200 Succ ~~!\n Hot Coffee is hotting ~");
			prt_re(cv);
		}
		else { fputs("Sorry, something I don't know happed = =.", stderr); }
	}
	fclose(cv);
	free(url);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
}

void get_coffee(int argc, char* argv[])
{
	char *url = get_url(argc, argv);
	CURL *curl=curl_easy_init( );
	ce_setopt(curl,CURLOPT_URL,url);
	ce_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/coffee-pot-command");
	ce_setopt(curl, CURLOPT_HTTPHEADER, headers);
	FILE* cv=fmemopen(NULL,sizeof(char[10240]),"w+");
	ce_setopt(curl, CURLOPT_WRITEDATA, cv);
	ce_setopt(curl, CURLOPT_HTTPGET, 1);
	puts("Getting...");
	ce_perform(curl, curl_err_buf);
	fseek(cv,0,SEEK_SET);
	if (!docode(curl, cv))
	{
		int rcode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rcode);
		if (rcode == 200)
		{
			puts("200 Succ ~~!\n The fresh hot coffee is here ~ :");
			prt_re(cv);
		}
		else { fputs("Sorry, something I don't know happed = =.", stderr); }
	}
	fclose(cv);
	free(url);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
}

void propfind(int argc, char* argv[])
{
	char *url = get_url(argc, argv);
	CURL *curl=curl_easy_init( );
	ce_setopt(curl,CURLOPT_URL,url);
	ce_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/coffee-pot-command");
	ce_setopt(curl, CURLOPT_HTTPHEADER, headers);
	FILE* cv=fmemopen(NULL,sizeof(char[10240]),"w+");
	ce_setopt(curl, CURLOPT_WRITEDATA, cv);
	ce_setopt(curl,CURLOPT_CUSTOMREQUEST,"PROPFIND");
	puts("Getting...");
	ce_perform(curl, curl_err_buf);
	fseek(cv,0,SEEK_SET);
	if (!docode(curl, cv))
	{
		int rcode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rcode);
		if (rcode == 200)
		{
			puts("200 Succ ~~!\n The fresh hot coffee is here ~ :");
			prt_re(cv);
		}
		else { fputs("Sorry, something I don't know happed = =.", stderr); }
	}
	fclose(cv);
	free(url);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
}

void when(int argc, char* argv[])
{
	char *url = get_url(argc, argv);
	CURL *curl=curl_easy_init( );
	ce_setopt(curl,CURLOPT_URL,url);
	ce_setopt(curl, CURLOPT_ERRORBUFFER, curl_err_buf);
	struct curl_slist *headers=NULL;
	headers = curl_slist_append(headers, "Content-Type: application/coffee-pot-command");
	ce_setopt(curl, CURLOPT_HTTPHEADER, headers);
	FILE* cv=fmemopen(NULL,sizeof(char[10240]),"w+");
	ce_setopt(curl, CURLOPT_WRITEDATA, cv);
	ce_setopt(curl,CURLOPT_CUSTOMREQUEST,"WHEN");
	puts("Getting...");
	ce_perform(curl, curl_err_buf);
	fseek(cv,0,SEEK_SET);
	if (!docode(curl, cv))
	{
		int rcode;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &rcode);
		if (rcode == 200)
		{
			puts("200 Succ ~~!\n The fresh hot coffee is here ~ :");
			prt_re(cv);
		}
		else { fputs("Sorry, something I don't know happed = =.", stderr); }
	}
	fclose(cv);
	free(url);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
}

void init()
{
	read_config();
	assert(!curl_global_init(CURL_GLOBAL_ALL));
	int i=0; for (i=0;*adds[i];++i); add_len=i;
}

void terminate()
{
	curl_global_cleanup();
}


ACTION default_act = help;
struct ACTUPLE act_table[] = {{help, "help"}, {propfind, "profind"}, {when, "when"}, {get_coffee, "get"}, {brew, "brew"}, {NULL,""}};
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
