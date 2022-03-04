/*
 * parsehelptoc.c
 *
 *  Created on: Feb 25, 2022
 *      Author: dad
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif  // HAVE_CONFIG_H

#if !defined(_BSD_SOURCE)
#	define _BSD_SOURCE
#   define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>
#include <fcntl.h>


#ifdef WINDOZE
#define ssize_t int
ssize_t getline(char** lineptr, size_t* n, FILE* stream)
{
	if ( fgets(*lineptr, (int)n, stream) == NULL ) return(-1);
	return strlen( *lineptr);
}
#endif

#define LINELEN	250
#define NUMSITE 1000
#define NUMURL 50001
#define LINELENp5	LINELEN + 5
#define NUMPORTS	24
#define SWDATAFMT	0


int numcomps = 0;

typedef struct paramdata
{
	xmlChar *						 name;
	xmlChar *						 value;
} ParamStructure;
int numoui = 0;

ParamStructure * ParamSp;
ParamStructure ParamS;

typedef struct helpdata
{
	char *						 Name;
	char *						 Local;
	char *						 Url;
} helpStructure;
int numhelps = 0;

helpStructure * HelpSdb = NULL;
helpStructure HelpS;


/*
 ** A structure for holding a single date and time.
 */
typedef struct DateTime DateTime;
struct DateTime {
    int64_t iJD;        /* The julian day number times 86400000 */
    int Y, M, D;        /* Year, month, and day */
    int h, m;           /* Hour and minutes */
    int tz;             /* Timezone offset in minutes */
    double s;           /* Seconds */
    char validJD;       /* True (1) if iJD is valid */
    char rawS;          /* Raw numeric value stored in s */
    char validYMD;      /* True (1) if Y,M,D are valid */
    char validHMS;      /* True (1) if h,m,s are valid */
    char validTZ;       /* True (1) if tz is valid */
    char tzSet;         /* Timezone was set explicitly */
    char isError;       /* An overflow has occurred */
};

/*
<?xml version="1.0" encoding="UTF-8"?>
<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
   <url>
      <loc>http://www.example.com/</loc>
      <lastmod>2005-01-01</lastmod>
<!-- date --iso-8601=seconds -u -r /home/foo/www/bar.php >> sitemap... -->
      <changefreq>monthly</changefreq>
      <priority>0.8</priority>
   </url>
   <url>
      <loc>http://www.example.com/catalog?item=12&amp;desc=vacation_hawaii</loc>
      <changefreq>weekly</changefreq>
   </url>
   <url>
      <loc>http://www.example.com/catalog?item=73&amp;desc=vacation_new_zealand</loc>
      <lastmod>2004-12-23</lastmod>
      <changefreq>weekly</changefreq>
   </url>
   <url>
      <loc>http://www.example.com/catalog?item=74&amp;desc=vacation_newfoundland</loc>
      <lastmod>2004-12-23T18:00:15+00:00</lastmod>
      <priority>0.3</priority>
   </url>
   <url>
      <loc>http://www.example.com/catalog?item=83&amp;desc=vacation_usa</loc>
      <lastmod>2004-11-23</lastmod>
   </url>
</urlset>
*/
typedef struct urldata
{
	uint64_t					lastmod;
	xmlChar *					loc;
	char *						filename;
	char *					    priority;
	char *						changefreq;
} UrlDbStructure;
int numurls = 0;

UrlDbStructure * urldb = NULL;
UrlDbStructure urlpt;

/*
<?xml version="1.0" encoding="UTF-8"?>
<sitemapindex xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
	<sitemap>
		<loc>http://www.kuras-sea.net/main.xml.gz</loc>
		<lastmod>2022-01-01</lastmod>
	</sitemap>
	<sitemap>
		<loc>http://www.kuras-sea.net/passwin.xml.gz</loc>
		<lastmod>2022-01-01</lastmod>
	</sitemap>
	<sitemap>
		<loc>http://www.kuras-sea.net/passwinhelp.xml.gz</loc>
		<lastmod>2022-01-01</lastmod>
	</sitemap>
</sitemapindex>
*/
typedef struct sitemapdata
{
	uint64_t					lastmod;
	xmlChar *					loc;
	char *						filename;
	char *						unzipedfilename;
	int							changed;
	int							numurls;
} SiteMapDbStructure;
int numsites = 0;

SiteMapDbStructure * sitedb = NULL;
SiteMapDbStructure site;

int fmtType = 0;

//int findmac(unsigned long long tmacll);
unsigned long ipstr2l(char *tip);
int macstr2ll(char *tmac, unsigned long long *tmacll, unsigned long long *tmaskll, int format);
void printmac(unsigned long long MAC, FILE *fp2);
void sprintmac(unsigned long long MAC, char *buff);
void printip(unsigned long IP, FILE *fp2);
void sprintip(unsigned long IP, char *buff);
unsigned short getbytell(unsigned long long MAC, int i);
unsigned short getbytel(unsigned long IP, int i);
void parseScan (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int parsline3(char *line,int nlabs,int *colns,char **cdata,char * delims);
int loadsitedata(xmlDocPtr sitedoc, FILE *fp2);
int loadurldata(xmlDocPtr urldoc, int *changedflag, FILE *fp2);
int parseul (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int parseli (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int parseobject (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int parseSitemap (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int parseurldata (xmlDocPtr doc, xmlNodePtr cur, int *numnew, int *changedflag, FILE *fp2);
ParamStructure* parseparam (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2);
int savesitedata(xmlDocPtr macdoc);
extern int unzipfile (const char * file_name);
int saveurldata(xmlDocPtr urldoc, int numurls);
int savehelpsdata(xmlDocPtr urldoc, int filegenIndex);

// Calculates time since epoch including milliseconds
uint64_t ParseTimeToEpochMillis(const char *str, bool *error);

// Creates an ISO timestamp with milliseconds from epoch with millis.
// The buffer size (resultLen) for result must be at least 100 bytes.
void TimeFromEpochMillis(uint64_t epochMillis, char *result, int resultLen, bool *error);
/*  sample usage:
// Calculate milliseconds since epoch
std::string timeStamp = "2019-09-02T22:02:24.355Z";
bool error;
uint64_t time = ParseTimeToEpochMillis(timeStamp.c_str(), &error);

// Get ISO timestamp with milliseconds component from epoch in milliseconds.
// Multiple by 1000 in case you have a standard epoch in seconds)
uint64_t epochMillis = 1567461744355; // == "2019-09-02T22:02:24.355Z"
char result[100] = {0};
TimeFromEpochMillis(epochMillis, result, sizeof(result), &error);
std::string resultStr(result); // == "2019-09-02T22:02:24.355Z"
*/

xmlDocPtr sitedoc;
xmlDocPtr urldoc;
xmlNodePtr maccur;
xmlNodePtr macroot;
xmlNodePtr urlroot;

FILE *fp;
FILE *fp2;
FILE *fpconf;
int tosend = 0;
char tempbuf[100000];
char sendbuf[1000];
int strseq = 0;
int echo2stdout = 0;
int hangitup = 0;
#define NUMSTR	9
#define NUMFMTS	2
int mongoInitCalled = 0;
bool error;
char *servername = NULL;
char *defchangefreq = "monthly";
char *defpriority = "0.5";
char *urldirname[NUMSITE] = {NULL};
char *sitemapfilename[NUMSITE] = {NULL};
char *basefqdn = NULL;
int numfilegen = 0;

int main(int argc, char* argv[])
{
	char * line = NULL;
	size_t len = LINELEN;
	ssize_t read;
	size_t size = LINELENp5;
	int first = 0;
	int second = 0;
	int third = 0;
	int fourth = 0;
	char *filename;
	char *outfilename;
	char *defconffile = "swagmac.conf";
	char *confFile;
	int i;
	int numfiles = 0;
	int filetype;

	ParamSp = &ParamS;
	confFile = defconffile;
	if (argc > 1) {
			if ((strlen(argv[1]) > 0)) confFile = argv[1];
		}
	if (argc > 2) {
		if ((strcmp(argv[2], "all") == 0) | (strcmp(argv[2], "first") == 0)) first = 1;
		if ((strcmp(argv[2], "all") == 0) | (strcmp(argv[2], "second") == 0)) second = 1;
		if ((strcmp(argv[2], "all") == 0) | (strcmp(argv[2], "third") == 0)) third = 1;
		if ((strcmp(argv[2], "all") == 0) | (strcmp(argv[2], "fourth") == 0)) fourth = 1;
	}
	else {
		first = 1;
		second = 1;
		third = 1;
		fourth = 1;
	}

	line = (char*)calloc(1,size);
	sitedb = (SiteMapDbStructure*)calloc(1,sizeof(SiteMapDbStructure)*NUMSITE);
	HelpSdb = (helpStructure*)calloc(1,sizeof(helpStructure)*NUMURL);


	fpconf = fopen(confFile, "r");
// 1,opsiMACs.csv,maclist.txt,0
	if (fpconf == NULL)return(12);
	printf(" processing %s\n",confFile);
	int confcolns[] = {0,1,2,3};
#define NCONFLABS 4
	int nconflabs = NCONFLABS;
	char * confdata[NCONFLABS];
	for (i=0;i<nconflabs;i++) confdata[i] = (char*)malloc(LINELEN);

	while ((read = getline(&line, &len, fpconf)) != -1) {
		//	        printf("Retrieved config line of length %zu:\n", read);
		line[read - 1] = '\000';
		numfiles++;
		fprintf(stderr," config input found : \"%s\"\n", line);
		if (parsline3(line,nconflabs,confcolns,confdata,(char *)",") != 0) continue;
		filename = NULL;
		outfilename = NULL;
		filetype = 0;
		fmtType = 0;
		if (strlen(confdata[0]) > 0) {
			if ( sscanf(confdata[0],"%u",&filetype) != 1 ) return (0);
		}
		if (strlen(confdata[1]) > 0) filename = confdata[1];
		if (strlen(confdata[2]) > 0) outfilename = confdata[2];
		if (strlen(confdata[3]) > 0) {
			if ( sscanf(confdata[3],"%u",&fmtType) != 1 ) return (0);
		}



		switch (filetype)
		{
			case 1:
				if (first) {
//				line = (char*)malloc(size);
//					urldb = (UrlDbStructure*)malloc(sizeof(UrlDbStructure)*NUMURL);
//					sitedb = (SiteMapDbStructure*)malloc(sizeof(SiteMapDbStructure)*NUMSITE);
					// save the basefqdn in case it is needed during the helpuri to sitemap step
		    		if (!outfilename) {
		    			basefqdn = (char*)malloc(1);
		    			*basefqdn = '\000';
		    		} else {
		    			basefqdn = (char*)malloc(strlen(outfilename)+1);
						strcpy (basefqdn, outfilename);
		    		}
					sitedoc = xmlParseFile(filename);
					if (sitedoc == NULL ) {
							fprintf(stderr,"sitemap document not parsed successfully. \nUnable to open old sitemap.xml file\n");
					} else {
							printf(" loading old sitemap data\n");
							fp2 = fopen("oldsitelist.txt", "w");
							if (fp2 == NULL) return(2);
							loadsitedata(sitedoc,fp2);
							xmlFreeDoc(sitedoc);
							fclose(fp2);
					}
				}
				break;

		    case 2:

		/*
		<?xml version="1.0" encoding="UTF-8"?>
		<!DOCTYPE nmaprun>
		<host><status state="up" reason="arp-response" reason_ttl="0"/>
		<address addr="10.0.0.1" addrtype="ipv4"/>
		<address addr="5C:F4:AB:6D:F4:FB" addrtype="mac" vendor="ZyXEL Communications"/>
		<hostnames>
		<hostname name="_gateway" type="PTR"/>
		</hostnames>
		<times srtt="700" rttvar="5000" to="100000"/>
		</host>
	*/

				if (second) {
					xmlDocPtr doc;
					xmlNodePtr cur;
					int numnew = 0;
					if ( filename == NULL ) filename = "scan2X.txt";
					printf(" processing %s\n",filename);

					fp2 = fopen(outfilename, "w");
					if (fp2 == NULL)return(2);

					doc = xmlParseFile(filename);
					if (doc == NULL ) {
							fprintf(stderr,"Document not parsed successfully. \n");
							return(1);
						}

					cur = xmlDocGetRootElement(doc);

					if (cur == NULL) {
						fprintf(stderr,"empty document\n");
						xmlFreeDoc(doc);
						return(2);
					}

					if (xmlStrcmp(cur->name, (const xmlChar *) "html")) {
							fprintf(stderr,"document of the wrong type, root node != html");
							xmlFreeDoc(doc);
							return(3);
					}

					cur = cur->xmlChildrenNode;
					fprintf(stderr,"------------------\n");
					while (cur != NULL) {
						fprintf(stderr,"name = %s\n", cur->name);
						if ((!xmlStrcmp(cur->name, (const xmlChar *)"body"))){
							fprintf(fp2,"<body>\n");
							parseScan (doc, cur, &numnew, fp2);
							fprintf(fp2,"</body>\n");
							fprintf(stderr,"------------------\n");
						}
					cur = cur->next;
					}

					xmlFreeDoc(doc);
					printf(" %i objects found\n",numnew);
					fclose(fp2);
				}
				break;

		    case 3:
		    	if (third) {
		    		if (!filename) {
		    			fprintf(stderr,"bad config file... servername is blank\n");
		    			break;
		    		}
					servername = (char*)malloc(strlen(filename)+1);
					strcpy (servername, filename);
		    	}
				break;

		    case 4:
		    	if (fourth) {
		    		if (!outfilename) {
		    			fprintf(stderr,"bad config file... output sitemap file name is blank\n");
		    			break;
		    		}
		    		if (!filename) {
		    			urldirname[numfilegen] = (char*)malloc(1);
		    			*(urldirname[numfilegen]) = '\000';
		    		} else {
						urldirname[numfilegen] = (char*)malloc(strlen(filename)+1);
						strcpy (urldirname[numfilegen], filename);
		    		}
					sitemapfilename[numfilegen] = (char*)malloc(strlen(outfilename)+1);
					strcpy (sitemapfilename[numfilegen], outfilename);
					numfilegen++;
		    	}
				break;

		    case 5:

				break;

		    case 6:
				break;

		    case 7:
		    	break;

		    default:
		    	break;
// end of switch statement on file type
		}

// end of while on config file
	}
	fprintf(stderr," %i config lines processes\n",numfiles);
	fclose(fpconf);
	for (i=0;i<nconflabs;i++) free(confdata[i]);


	sitedoc = xmlNewDoc(BAD_CAST "1.0");
	macroot = xmlNewNode(NULL, BAD_CAST "sitemapindex");
	xmlDocSetRootElement(sitedoc, macroot);
	savesitedata(sitedoc);
// Dumping MAC data to output file
	xmlSaveFormatFileEnc("sitedataOut.xml", sitedoc, "UTF-8", 1);
//free the document
	xmlFreeDoc(sitedoc);
//Free the global variables that may
//have been allocated by the parser.
	xmlCleanupParser();
// this is to debug memory for regression tests
//	    xmlMemoryDump();

// check if we need to create some new sitemap files for the help index
	if (numfilegen && numhelps) {
		fprintf(stderr, " processing help index into sitemap files, %i sitemap files requested, %i help index entries\n",numfilegen,numhelps);
		for (i=0;i<numfilegen;i++) {
			fprintf(stderr, " processing %s\n",sitemapfilename[i]);
			if (remove(sitemapfilename[i]) == 0) {
					fprintf(stderr," %s file deleted successfully.\n",sitemapfilename[i]);
				} else {
					fprintf(stderr," %s file delete failed.\n",sitemapfilename[i]);
				}

			urldoc = xmlNewDoc(BAD_CAST "1.0");
			if (urldoc == NULL ) {
					fprintf(stderr,"sitemap tree not created successfully. \nUnable to create new %s file\n",sitemapfilename[i]);
			} else {
				// <urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
				fprintf(stderr," building sitemap data for %s\n",sitemapfilename[i]);
				urlroot = xmlNewNode(NULL, BAD_CAST "urlset");
				xmlDocSetRootElement(urldoc, urlroot);
				savehelpsdata(urldoc,i);
				// Dumping sitmap data to output file
				xmlSaveFormatFileEnc(sitemapfilename[i], urldoc, "UTF-8", 1);
				//free the document
				xmlFreeDoc(urldoc);
			}
		}
	}

	if (second) puts("!!!Done!!");
	return EXIT_SUCCESS;
}



unsigned long ipstr2l(char *tip) {
	unsigned int b[4];
	unsigned long ip;
	if (strlen(tip) < 7 ) return (0);
	if ( sscanf(tip,"%u.%u.%u.%u",&b[3],&b[2],&b[1],&b[0]) != 4 ) return (0);
	ip = b[3];
	for (int i=2;i>-1;i--) ip = (ip<<8)+b[i];
	return(ip);
}

int macstr2ll(char *tmac, unsigned long long *tmacll, unsigned long long *tmaskll, int format) {
	unsigned int b[] = {0,0,0,0,0,0};
	int masksize;
	if (tmac[2] == ':' ) format = 1;
	if (tmac[2] == '-' ) format = 2;
	if (strlen(tmac) == 12 ) {
		if ( format == 1 ) {
			return(-1);
		} else if ( format == 2 ) {
			return(-2);;
		} else {
			if ( sscanf(tmac,"%12llx",tmacll) != 1 ) return (-15);
		}
		*tmaskll = 0xffffffffffff;
		return(0);
	}else if (strlen(tmac) == 17 ) {
		if ( format == 1 ) {
			if ( sscanf(tmac,"%2x:%2x:%2x:%2x:%2x:%2x",&b[5],&b[4],&b[3],&b[2],&b[1],&b[0]) != 6 ) return (-3);
		} else if ( format == 2 ) {
			if ( sscanf(tmac,"%2x-%2x-%2x-%2x-%2x-%2x",&b[5],&b[4],&b[3],&b[2],&b[1],&b[0]) != 6 ) return (-4);
		} else {
			return(-5);
		}
		*tmaskll = 0xffffffffffff;
	}else if (strlen(tmac) == 8 ) {
		if ( format == 1 ) {
			if ( sscanf(tmac,"%2x:%2x:%2x",&b[5],&b[4],&b[3]) != 3 ) return (-6);
		} else if ( format == 2 ) {
			if ( sscanf(tmac,"%2x-%2x-%2x",&b[5],&b[4],&b[3]) != 3 ) return (-7);
		} else {
			return(-8);
		}
		*tmaskll = 0xffffff000000;
	}else if (strchr(tmac,'/') == NULL) {
		return (-9);
	} else {
		if ( format == 1 ) {
			if ( sscanf(tmac,"%2x:%2x:%2x:%2x:%2x:%2x/%i",&b[5],&b[4],&b[3],&b[2],&b[1],&b[0],&masksize) != 7 ) return (-10);
		} else if ( format == 2 ) {
			if ( sscanf(tmac,"%2x-%2x-%2x-%2x-%2x-%2x/%i",&b[5],&b[4],&b[3],&b[2],&b[1],&b[0],&masksize) != 7 ) return (-11);
		} else {
			return(-12);
		}
		if ( masksize > 48 ) return (-13);
		if ( masksize < 0 ) return (-14);
		*tmaskll = 0x1;
		for (int i=1;i<masksize;i++) *tmaskll = (*tmaskll<<1)+0x1;
		for (int i=masksize;i<48;i++) *tmaskll = *tmaskll<<1;
	}
	*tmacll = b[5];
	for (int i=4;i>-1;i--) *tmacll = (*tmacll<<8)+b[i];
	return(0);
}

void printmac(unsigned long long MAC, FILE *fp2) {
	for (int i=5; i>0;i--) {
		//printf("%02x:",getbytell(MAC,i));
		fprintf(fp2,"%02x:",getbytell(MAC,i));
	}
	//printf("%02x\n",getbytell(MAC,0));
	fprintf(fp2,"%02x\n",getbytell(MAC,0));
	return;
}

void sprintmac(unsigned long long MAC, char * buff) {
	char temp[30];
	buff[0] = '\000';
	for (int i=5; i>0;i--) {
		sprintf(temp,"%02x:",getbytell(MAC,i));
		strcat(buff,temp);
	}
	sprintf(temp,"%02x",getbytell(MAC,0));
	strcat(buff,temp);
	return;
}

void printip(unsigned long IP, FILE *fp2) {
	for (int i=3; i>0;i--) {
		//printf("%u.",getbytel(IP,i));
		fprintf(fp2,"%u.",getbytel(IP,i));
	}
	//printf("%u\n",getbytel(IP,0));
	fprintf(fp2,"%u\n",getbytel(IP,0));
	return;
}

void sprintip(unsigned long IP, char *buff) {
	char temp[30];
	buff[0] = '\000';
	for (int i=3; i>0;i--) {
		sprintf(temp,"%u.",getbytel(IP,i));
		strcat(buff,temp);
	}
	sprintf(temp,"%u",getbytel(IP,0));
	strcat(buff,temp);
	return;
}

unsigned short getbytell(unsigned long long MAC, int i) {
	return((MAC>>8*i)&255);
}

unsigned short getbytel(unsigned long IP, int i) {
	return((IP>>8*i)&255);
}

void parseScan (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2) {

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		fprintf(stderr,"name = %s\n", cur->name);
	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"ul"))) {
	    	parseul (doc, cur, numnew, fp2);
	    	cur = cur->next;
	    	continue;
	    }
	    cur = cur->next;
	}

    return;
}

int parsline3(char *line,int nlabs,int *colns,char **cdata,char * delims) {
//	ica-2007	IntelCor	00:21:6b:eb:c1:2a	-	User	6.75 GB	309 MB	07/30/2020 6:12 pm	09/07/2020 10:15 am
// all fields are separated by delimiters specified by delims
// nlabs is the number of fields to process
// colns[] is an array of integer field indecies
// cdata[] is the return array of character strings (each max lenght LINELEN) of parsed output
	char * templine;
	char * pch;
	int i;

	for (i=0;i<nlabs;i++) *(cdata[i]) = '\000';
	if (strlen(line) < 4 ) return (-1);
	templine=strdup(line);

	pch = strsep (&templine,delims);
	int index = 0;
	while (pch != NULL) {
		for (i=0;i<nlabs;i++) {
			if ( colns[i] == index ) {
				strcpy(cdata[i],pch);
			}
		}
	    pch = strsep (&templine, delims);
	    index++;
	}
	free(templine);

	return(0);

}

int loadurldata(xmlDocPtr doc, int *changedflag, FILE *fp2) {
	/*
	<?xml version="1.0" encoding="UTF-8"?>
	<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
	   <url>
	      <loc>http://www.example.com/</loc>
	      <lastmod>2005-01-01</lastmod>
	<!-- date --iso-8601=seconds -u -r /home/foo/www/bar.php >> sitemap... -->
	      <changefreq>monthly</changefreq>
	      <priority>0.8</priority>
	   </url>
	</urlset>
	*/

	xmlNodePtr cur;
	int numurls = 0;

	if (doc == NULL ) {
			fprintf(stderr,"Document not parsed successfully. \n");
			return(-1);
		}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return(-2);
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "urlset")) {
			fprintf(stderr,"document of the wrong type, root node != urlset");
			xmlFreeDoc(doc);
			return(-3);
	}


	if(urldb != NULL) free(urldb);
	urldb = (UrlDbStructure*)calloc(1,sizeof(UrlDbStructure)*NUMURL);
	numurls = 0;

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
    	if ((urldb+numurls)->loc != NULL) free((urldb+numurls)->loc);
    	(urldb+numurls)->lastmod = 0;
    	if ((urldb+numurls)->changefreq != NULL) free((urldb+numurls)->changefreq);
    	if ((urldb+numurls)->priority != NULL) free((urldb+numurls)->priority);
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"url"))){
			if ( parseurldata (doc, cur, &numurls, changedflag, fp2) ) {
				fprintf(stderr,"!!!!!!!!!!   WARNING WILL ROBINSON, WARNING   !!!!!!!!  ABORTING URL LOADING OF CURRENT FILE  !!!!!!!!!!\n");
				break;
			}
		}
	cur = cur->next;
	}

	fprintf(stderr," %i old urls found\n",numurls);

	return(numurls);

}

int parseurldata (xmlDocPtr doc, xmlNodePtr cur, int *numnew, int *changedflag, FILE *fp2) {
		xmlNodePtr sub;
		xmlNodePtr sub2;
		xmlChar* Name = NULL;
		xmlChar* Local = NULL;
		xmlChar* URL = NULL;
	    struct stat attrib;
	    uint64_t filetime;
/*
  	   <url>
	      <loc>http://www.example.com/</loc>
	      <lastmod>2005-01-01</lastmod>
	<!-- date --iso-8601=seconds -u -r /home/foo/www/bar.php >> sitemap... -->
	      <changefreq>monthly</changefreq>
	      <priority>0.8</priority>
	   </url>
*/
		    if ((!xmlStrcmp(cur->name, (const xmlChar *)"url"))) {
		    	sub = cur->xmlChildrenNode;
		    	while (sub != NULL) {
		    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"loc"))) {
		    			sub2 = sub->xmlChildrenNode;
		    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
		    				fprintf(stderr,"error parsing sitemap loc - non text child\n");
		    				return(1);
		    			}
		    			(urldb+*numnew)->loc = xmlStrdup(sub2->content);
		    			fprintf(stderr," loc content = %s\n",sub2->content);
		    		}
		    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"lastmod"))) {
		    			sub2 = sub->xmlChildrenNode;
		    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
		    				fprintf(stderr,"error parsing url lastmod - non text child\n");
		    				return(1);
		    			}
		    			(urldb+*numnew)->lastmod = ParseTimeToEpochMillis((char *)(sub2->content), &error);
		    			if(error){
		    				fprintf(stderr,"error coverting iso-8601 to integer time stamp\n");
		    				return(1);
		    			}
		    			fprintf(stderr," lastmod content = %s\n",sub2->content);
		    		}
		    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"changefreq"))) {
		    			sub2 = sub->xmlChildrenNode;
		    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
		    				fprintf(stderr,"error parsing sitemap loc - non text child\n");
		    				return(1);
		    			}
		    			(urldb+*numnew)->changefreq = (char*)xmlStrdup(sub2->content);
		    			fprintf(stderr," changefreq content = %s\n",sub2->content);
		    		}
		    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"priority"))) {
		    			sub2 = sub->xmlChildrenNode;
		    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
		    				fprintf(stderr,"error parsing sitemap loc - non text child\n");
		    				return(1);
		    			}
		    			(urldb+*numnew)->priority = (char*)xmlStrdup(sub2->content);
		    			fprintf(stderr," priority content = %s\n",sub2->content);
		    		}
		    		sub = sub->next;
		    		continue;
		    	}
		    	// verify server is correct
		    	if (strncmp((char*)(urldb+*numnew)->loc,servername,strlen(servername)) ) {
		    		fprintf(stderr," bad url in sitemap: url = %s, expected server = %s\n",(char*)(urldb+*numnew)->loc,servername);
		    		return(2);
		    	}
		    	// strip off the file lolcation from the url and save it in urldb->filename
		    	if ( strlen((char*)(urldb+*numnew)->loc) == strlen(servername) ){
		    		fprintf(stderr," bad url in sitemap, no file location provided: url = %s, expected server = %s\n",(char*)(urldb+*numnew)->loc,servername);
		    		return(2);
		    	}
		    	(urldb+*numnew)->filename = (char*)malloc(strlen(((char*)(urldb+*numnew)->loc))+1);
		    	strcpy((urldb+*numnew)->filename,((char*)(urldb+*numnew)->loc)+strlen(servername));
			    stat((urldb+*numnew)->filename, &attrib);
			    filetime = ((uint64_t)(intmax_t)attrib.st_mtim.tv_sec)*1000 + ((uint64_t)attrib.st_mtim.tv_nsec)/1000000;
			    if ( filetime > (urldb+*numnew)->lastmod) {
			    	(urldb+*numnew)->lastmod = filetime;
			    	(*changedflag)++;
			    }


#ifdef DEBUG
		    	if (*numnew == 0) (urldb+*numnew)->lastmod = filetime;
#endif	// DEBUG
		    	// increment count
		    	(*numnew)++;
		    	fprintf(fp2,"<a href=\"%s\" target=\"content\">%s</a><br>\n",Local,Name);
		    	if (URL) fprintf(stderr,"**********  URL = %s  **********\n",URL);
		    	return(0);
	 	    }
	 	    return(1);
}

int loadsitedata(xmlDocPtr doc, FILE *fp2){
	xmlNodePtr cur;

	if (doc == NULL ) {
			fprintf(stderr,"Document not parsed successfully. \n");
			return(1);
		}

	cur = xmlDocGetRootElement(doc);

	if (cur == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(doc);
		return(2);
	}

	if (xmlStrcmp(cur->name, (const xmlChar *) "sitemapindex")) {
			fprintf(stderr,"document of the wrong type, root node != sitemapindex");
			xmlFreeDoc(doc);
			return(3);
	}

	cur = cur->xmlChildrenNode;
	fprintf(fp2,"------------------\n");
	while (cur != NULL) {
		if ((!xmlStrcmp(cur->name, (const xmlChar *)"sitemap"))){
			parseSitemap (doc, cur, &numsites, fp2);
			fprintf(fp2,"------------------\n");
		}
	cur = cur->next;
	}

	printf(" %i old sitemaps found\n",numsites);

	return(0);
}

int parseSitemap (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2)
{
	xmlNodePtr sub;
	xmlNodePtr sub2;
	xmlChar* Name = NULL;
	xmlChar* Local = NULL;
	xmlChar* URL = NULL;
	int changedflag;

	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"sitemap"))) {
	    	sub = cur->xmlChildrenNode;
	    	while (sub != NULL) {
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"loc"))) {
	    			sub2 = sub->xmlChildrenNode;
	    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
	    				fprintf(stderr,"error parsing sitemap loc - non text child\n");
	    				return(1);
	    			}
	    			(sitedb+*numnew)->loc = xmlStrdup(sub2->content);
	    			fprintf(stderr," loc content = %s\n",sub2->content);
	    		}
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"lastmod"))) {
	    			sub2 = sub->xmlChildrenNode;
	    			if ((xmlStrcmp(sub2->name, (const xmlChar *)"text"))) {
	    				fprintf(stderr,"error parsing sitemap lastmod - non text child\n");
	    				return(1);
	    			}
	    			(sitedb+*numnew)->lastmod = ParseTimeToEpochMillis((char *)(sub2->content), &error);
	    	    	// mark the map as unchanged
	    	    	(sitedb+*numnew)->changed = 0;
	    			if(error){
	    				fprintf(stderr,"error coverting iso-8601 to integer time stamp\n");
	    				return(1);
	    			}
	    			fprintf(stderr," lastmod content = %s\n",sub2->content);
	    		}
	    		sub = sub->next;
	    		continue;
	    	}
	    	// verify server is correct
	    	if (strncmp((char*)(sitedb+*numnew)->loc,servername,strlen(servername)) ) {
	    		fprintf(stderr," bad url in sitemap: url = %s, expected server = %s\n",(char*)(sitedb+*numnew)->loc,servername);
	    		return(2);
	    	}
	    	// strip off the file lolcation from the url and save it in sitedb->filename
	    	if ( strlen((char*)(sitedb+*numnew)->loc) == strlen(servername) ){
	    		fprintf(stderr," bad url in sitemap, no file location provided: url = %s, expected server = %s\n",(char*)(sitedb+*numnew)->loc,servername);
	    		return(2);
	    	}
	    	(sitedb+*numnew)->filename = (char*)malloc(strlen(((char*)(sitedb+*numnew)->loc))+1);
	    	strcpy((sitedb+*numnew)->filename,((char*)(sitedb+*numnew)->loc)+strlen(servername));
//	    	char * cwd = HTGetCurrentDirectoryURL();
//	    	char * absolute_url = HTParse(url, cwd, PARSE_ALL);
	    	/////////////////////////////////////////////////////////
	    	// check for compressed file - will automatically unzip
	    	// note that we will not re-zip these files - you need to
	    	// re-zip them manually. (We want to make sure that
	    	// the .gz format is comapable with the web scrapers)
	    	/////////////////////////////////////////////////////////
	    	// set the unzippped filename to filenameinitially
			(sitedb+*numnew)->unzipedfilename = (char*)malloc(strlen(((char*)(sitedb+*numnew)->loc))+1);
			strcpy((sitedb+*numnew)->unzipedfilename,(sitedb+*numnew)->filename);
			// check for gz file...
	    	int fnlen = strlen((char*)(sitedb+*numnew)->filename);
	    	if ( fnlen > 3 ) {
	    		if ( strcmp(((char*)(sitedb+*numnew)->filename)+fnlen-4,".gz") ) {
	    			if (!unzipfile ((char*)(sitedb+*numnew)->filename)) {
	    				fprintf(stderr,"%s unzipped\n", (sitedb+*numnew)->filename);
	    				// if the file was zipped, modify the saved name of the unzipped file
	    				int less3 = strlen((sitedb+*numnew)->unzipedfilename)-3;
	    				*(((sitedb+*numnew)->unzipedfilename)+less3) = '\000';
	    			}
	    		}
	    	}
	    	(sitedb+*numnew)->changed = 0;
			urldoc = xmlParseFile((sitedb+*numnew)->unzipedfilename);
			if (urldoc == NULL ) {
					fprintf(stderr,"sitemap document not parsed successfully. \nUnable to open %s file\n",(sitedb+*numnew)->unzipedfilename);
			} else {
					printf(" loading old sitemap data from %s\n",(sitedb+*numnew)->unzipedfilename);
					changedflag = 0;
					(sitedb+*numnew)->numurls = loadurldata(urldoc,&changedflag,fp2);
					if ( (sitedb+*numnew)->numurls < 0) {
						fprintf(stderr,"!!!!!!    loadurldata failed on %s\n",(sitedb+*numnew)->unzipedfilename);
					}
					(sitedb+*numnew)->changed = changedflag;
					xmlFreeDoc(urldoc);
			}

#ifdef DEBUG
	    	if (*numnew == 0) (sitedb+*numnew)->changed = 1;
#endif	// DEEBUG
	    	if ((sitedb+*numnew)->changed) {
	    		fprintf(stderr, " %i files referenced in %s have changed... updating file\n",(sitedb+*numnew)->changed,(sitedb+*numnew)->unzipedfilename);
	    		if (remove((sitedb+*numnew)->unzipedfilename) == 0) {
	    		        fprintf(stderr," %s file deleted successfully.\n",(sitedb+*numnew)->unzipedfilename);
	    		    } else {
	    		        fprintf(stderr," %s file delete failed.\n",(sitedb+*numnew)->unzipedfilename);
	    		    }

	    		urldoc = xmlNewDoc(BAD_CAST "1.0");
				if (urldoc == NULL ) {
						fprintf(stderr,"sitemap tree not created successfully. \nUnable to create new %s file\n",(sitedb+*numnew)->unzipedfilename);
				} else {
					// <urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
					fprintf(stderr," rebuilding sitemap data for %s\n",(sitedb+*numnew)->unzipedfilename);
					urlroot = xmlNewNode(NULL, BAD_CAST "urlset");
					xmlDocSetRootElement(urldoc, urlroot);
					saveurldata(urldoc,(sitedb+*numnew)->numurls);
					// Dumping sitmap data to output file
					xmlSaveFormatFileEnc((sitedb+*numnew)->unzipedfilename, urldoc, "UTF-8", 1);
					//free the document
					xmlFreeDoc(urldoc);
					changedflag = 0;
				}
	    	}
	    	// increment count
	    	(*numnew)++;
	    	fprintf(fp2,"<a href=\"%s\" target=\"content\">%s</a><br>\n",Local,Name);
	    	if (URL) fprintf(stderr,"**********  URL = %s  **********\n",URL);
	    	return(0);
 	    }
 	    return(1);
}

int saveurldata(xmlDocPtr urldoc, int numurls) {
//	xmlNodePtr cur;
	xmlNodePtr node;
	xmlNodePtr root;
	int i;
	int numsaved = 0;
	char result[LINELEN];
	/*
	<?xml version="1.0" encoding="UTF-8"?>
	<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
	   <url>
	      <loc>http://www.example.com/</loc>
	      <lastmod>2005-01-01</lastmod>
	<!-- date --iso-8601=seconds -u -r /home/foo/www/bar.php >> sitemap... -->
	      <changefreq>monthly</changefreq>
	      <priority>0.8</priority>
	   </url>
	   <url>
	      <loc>http://www.example.com/catalog?item=12&amp;desc=vacation_hawaii</loc>
	      <changefreq>weekly</changefreq>
	   </url>
	   <url>
	      <loc>http://www.example.com/catalog?item=73&amp;desc=vacation_new_zealand</loc>
	      <lastmod>2004-12-23</lastmod>
	      <changefreq>weekly</changefreq>
	   </url>
	   <url>
	      <loc>http://www.example.com/catalog?item=74&amp;desc=vacation_newfoundland</loc>
	      <lastmod>2004-12-23T18:00:15+00:00</lastmod>
	      <priority>0.3</priority>
	   </url>
	   <url>
	      <loc>http://www.example.com/catalog?item=83&amp;desc=vacation_usa</loc>
	      <lastmod>2004-11-23</lastmod>
	   </url>
	</urlset>
	*/

	if (urldoc == NULL ) {
			fprintf(stderr,"Document not parsed successfully. \n");
			return(1);
		}

	root = xmlDocGetRootElement(urldoc);

	if (root == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(urldoc);
		return(2);
	}

	if (xmlStrcmp(root->name, (const xmlChar *) "urlset")) {
			fprintf(stderr,"document of the wrong type, root node != urlset");
			xmlFreeDoc(urldoc);
			return(3);
	}
	xmlNewProp(root, BAD_CAST "xmlns", BAD_CAST "http://www.sitemaps.org/schemas/sitemap/0.9" );

//	cur = cur->xmlChildrenNode;
	numsaved = 0;
	if ( numurls ) {
		for (i = 0; i < numurls; i++) {
			node = xmlNewChild(root, NULL, BAD_CAST "url", NULL);
			xmlNewChild(node, NULL, BAD_CAST "loc", BAD_CAST xmlStrdup( (xmlChar *)((urldb+numsaved)->loc) ));
			TimeFromEpochMillis( ((urldb+numsaved)->lastmod), result, sizeof(result), &error);
			if(error){
				fprintf(stderr,"error coverting integer time stamp to iso-8601 in saveurldata\n");
				return(1);
			}
			xmlNewChild(node, NULL, BAD_CAST "lastmod", BAD_CAST xmlStrdup( (xmlChar *)result ));
			if ((urldb+numsaved)->changefreq) {
				xmlNewChild(node, NULL, BAD_CAST "changefreq", BAD_CAST xmlStrdup( (xmlChar *)(urldb+numsaved)->changefreq ));
			} else {
				xmlNewChild(node, NULL, BAD_CAST "changefreq", BAD_CAST xmlStrdup( (xmlChar *)defchangefreq ));
			}
			if ((urldb+numsaved)->priority) {
				xmlNewChild(node, NULL, BAD_CAST "priority", BAD_CAST xmlStrdup( (xmlChar *)(urldb+numsaved)->priority ));
			} else {
				xmlNewChild(node, NULL, BAD_CAST "priority", BAD_CAST xmlStrdup( (xmlChar *)defpriority ));
			}
			numsaved++;
		}
	}

	printf(" %i urls saved\n",numsaved);

	return(0);
}

int savehelpsdata(xmlDocPtr urldoc, int filegenIndex) {
	//	xmlNodePtr cur;
		xmlNodePtr node;
		xmlNodePtr root;
		int i;
		int numsaved = 0;
		char result[LINELEN];
		/*
		<?xml version="1.0" encoding="UTF-8"?>
		<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
		   <url>
		      <loc>http://www.example.com/</loc>
		      <lastmod>2005-01-01</lastmod>
		<!-- date --iso-8601=seconds -u -r /home/foo/www/bar.php >> sitemap... -->
		      <changefreq>monthly</changefreq>
		      <priority>0.8</priority>
		   </url>
		   <url>
		      <loc>http://www.example.com/catalog?item=12&amp;desc=vacation_hawaii</loc>
		      <changefreq>weekly</changefreq>
		   </url>
		   <url>
		      <loc>http://www.example.com/catalog?item=73&amp;desc=vacation_new_zealand</loc>
		      <lastmod>2004-12-23</lastmod>
		      <changefreq>weekly</changefreq>
		   </url>
		   <url>
		      <loc>http://www.example.com/catalog?item=74&amp;desc=vacation_newfoundland</loc>
		      <lastmod>2004-12-23T18:00:15+00:00</lastmod>
		      <priority>0.3</priority>
		   </url>
		   <url>
		      <loc>http://www.example.com/catalog?item=83&amp;desc=vacation_usa</loc>
		      <lastmod>2004-11-23</lastmod>
		   </url>
		</urlset>
		*/

		if (urldoc == NULL ) {
				fprintf(stderr,"Document not parsed successfully. \n");
				return(1);
			}

		root = xmlDocGetRootElement(urldoc);

		if (root == NULL) {
			fprintf(stderr,"empty document\n");
			xmlFreeDoc(urldoc);
			return(2);
		}

		if (xmlStrcmp(root->name, (const xmlChar *) "urlset")) {
				fprintf(stderr,"document of the wrong type, root node != urlset");
				xmlFreeDoc(urldoc);
				return(3);
		}
		xmlNewProp(root, BAD_CAST "xmlns", BAD_CAST "http://www.sitemaps.org/schemas/sitemap/0.9" );

	//	cur = cur->xmlChildrenNode;
		numsaved = 0;
		int process = 0;
		char * tempstr = NULL;
		if ( numhelps && servername != NULL) {
			for (i = 0; i < numhelps; i++) {
				process = 0;
				if ( strlen(urldirname[filegenIndex]) == 0 && strchr((HelpSdb+i)->Local, '\\' ) == NULL ) {
					process = 1;
				} else if ( strlen(urldirname[filegenIndex]) && !strncmp ( urldirname[filegenIndex], (HelpSdb+i)->Local, strlen(urldirname[filegenIndex]) ) ) {
					process = 1;
				}

				if ( process ) {
					node = xmlNewChild(root, NULL, BAD_CAST "url", NULL);
					if (basefqdn == NULL){
						xmlNewChild(node, NULL, BAD_CAST "loc", BAD_CAST xmlStrdup( (xmlChar *)((HelpSdb+i)->Local) ));
					} else {
						tempstr = (char *)calloc(1,strlen(servername)+strlen(basefqdn)+strlen((HelpSdb+i)->Local)+1);
						strcpy(tempstr,servername);
						strcat(tempstr,basefqdn);
						strcat(tempstr,(HelpSdb+i)->Local);
						xmlNewChild(node, NULL, BAD_CAST "loc", BAD_CAST xmlStrdup( (xmlChar *)tempstr ));
						free(tempstr);
					}
					TimeFromEpochMillis( 0, result, sizeof(result), &error);
					if(error){
						fprintf(stderr,"error coverting integer time stamp to iso-8601 in saveurldata\n");
						return(1);
					}
					xmlNewChild(node, NULL, BAD_CAST "lastmod", BAD_CAST xmlStrdup( (xmlChar *)result ));
					xmlNewChild(node, NULL, BAD_CAST "changefreq", BAD_CAST xmlStrdup( (xmlChar *)defchangefreq ));
					xmlNewChild(node, NULL, BAD_CAST "priority", BAD_CAST xmlStrdup( (xmlChar *)defpriority ));
					numsaved++;
				}
			}
		}

		printf(" %i urls saved\n",numsaved);

		return(0);
}

int savesitedata(xmlDocPtr macdoc) {
//	xmlNodePtr cur;
	xmlNodePtr node;
	xmlNodePtr root;
	int i;
	int numsaved = 0;
	char result[LINELEN];
	/*
	<?xml version="1.0" encoding="UTF-8"?>
	<sitemapindex xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">
		<sitemap>
			<loc>http://www.kuras-sea.net/main.xml.gz</loc>
			<lastmod>2022-01-01</lastmod>
		</sitemap>
		<sitemap>
			<loc>http://www.kuras-sea.net/passwin.xml.gz</loc>
			<lastmod>2022-01-01</lastmod>
		</sitemap>
		<sitemap>
			<loc>http://www.kuras-sea.net/passwinhelp.xml.gz</loc>
			<lastmod>2022-01-01</lastmod>
		</sitemap>
	</sitemapindex>
	*/

	if (macdoc == NULL ) {
			fprintf(stderr,"Document not parsed successfully. \n");
			return(1);
		}

	root = xmlDocGetRootElement(macdoc);

	if (root == NULL) {
		fprintf(stderr,"empty document\n");
		xmlFreeDoc(macdoc);
		return(2);
	}

	if (xmlStrcmp(root->name, (const xmlChar *) "sitemapindex")) {
			fprintf(stderr,"document of the wrong type, root node != sitemapindex");
			xmlFreeDoc(macdoc);
			return(3);
	}
	xmlNewProp(root, BAD_CAST "xmlns", BAD_CAST "http://www.sitemaps.org/schemas/sitemap/0.9" );

//	cur = cur->xmlChildrenNode;
	numsaved = 0;
	if ( numsites ) {
		for (i = 0; i < numsites; i++) {
			node = xmlNewChild(root, NULL, BAD_CAST "sitemap", NULL);
			xmlNewChild(node, NULL, BAD_CAST "loc", BAD_CAST xmlStrdup( (xmlChar *)((sitedb+numsaved)->loc) ));
			if ((sitedb+numsaved)->changed) {
				time_t seconds;
				seconds = time(NULL);
				//xxx  print from datetime
				uint64_t epochMillis;
				epochMillis = (uint64_t )seconds * 1000;
				TimeFromEpochMillis(epochMillis, result, sizeof(result), &error);
				fprintf(stderr,"!!!! don\'t forget to re-gzip the %s file !!!!\n",(sitedb+numsaved)->unzipedfilename);
			} else {
				TimeFromEpochMillis( ((sitedb+numsaved)->lastmod), result, sizeof(result), &error);
			}
			if(error){
				fprintf(stderr,"error coverting integer time stamp to iso-8601 in savesitedata\n");
				return(1);
			}
			xmlNewChild(node, NULL, BAD_CAST "lastmod", BAD_CAST xmlStrdup( (xmlChar *)result ));
			numsaved++;
		}
	}

	printf(" %i sites saved\n",numsaved);

	return(0);
}

int parseul (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2)
{
	xmlNodePtr sub;

	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"ul"))) {
	    	fprintf(fp2,"<ul>\n");
	    	sub = cur->xmlChildrenNode;
	    	while (sub != NULL) {
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"ul"))) {
	    			parseul (doc, sub, numnew, fp2);

	    		}else if ((!xmlStrcmp(sub->name, (const xmlChar *)"li"))){
	    			parseli (doc, sub, numnew, fp2);
	    		}
	    		sub = sub->next;
	    		continue;
	    	}
	    	fprintf(fp2,"</ul>\n");
	    	return(0);
 	    }
 	    return(1);
}

int parseli (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2)
{
	xmlNodePtr sub;

	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"li"))) {
	    	fprintf(fp2,"<li>\n");
	    	sub = cur->xmlChildrenNode;
	    	while (sub != NULL) {
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"object"))) {
	    			parseobject (doc, sub, numnew, fp2);
	    		}
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"ul"))) {
	    			parseul (doc, sub, numnew, fp2);
	    		}
	    		sub = sub->next;
	    		continue;
	    	}
	    	fprintf(fp2,"</li>\n");
	    	return(0);
 	    }
 	    return(1);
}

int parseobject (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2)
{
	xmlNodePtr sub;
	xmlChar* Name = NULL;
	xmlChar* Local = NULL;
	xmlChar* URL = NULL;
	int i;

	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"object"))) {
	    	sub = cur->xmlChildrenNode;
	    	while (sub != NULL) {
	    		if ((!xmlStrcmp(sub->name, (const xmlChar *)"param"))) {
	    			if (!parseparam (doc, sub, numnew, fp2)) return(2);
	    			if ((!xmlStrcmp(ParamS.name, (const xmlChar *)"Name"))) Name = xmlStrdup(ParamS.value);
	    			if ((!xmlStrcmp(ParamS.name, (const xmlChar *)"Local"))) {
	    				Local = xmlStrdup(ParamS.value);
	    				for(i=0;i<strlen((char *)Local);i++) {
	    					if((char)Local[i] == '\\') {
	    						*((char *)(Local+i)) = '/';
	    					}
	    				}
	    			}
	    			if ((!xmlStrcmp(ParamS.name, (const xmlChar *)"URL"))) URL = xmlStrdup(ParamS.value);
	    		}
	    		sub = sub->next;
	    		continue;
	    	}
	    	if ( (char *)Name  != NULL ) (HelpSdb+numhelps)->Name  = (char*)xmlStrdup(Name);
	    	if ( (char *)Local != NULL ) (HelpSdb+numhelps)->Local = (char*)xmlStrdup(Local);
	    	if ( (char *)URL   != NULL ) (HelpSdb+numhelps)->Url   = (char*)xmlStrdup(URL);
	    	numhelps++;
	    	(*numnew)++;
	    	fprintf(fp2,"<a href=\"%s\" target=\"content\">%s</a><br>\n",Local,Name);
	    	if (URL) fprintf(stderr,"**********  URL = %s  **********\n",URL);
	    	return(0);
 	    }
 	    return(1);
}

ParamStructure * parseparam (xmlDocPtr doc, xmlNodePtr cur, int *numnew, FILE *fp2)
{
	xmlChar *key;
	xmlChar *uri;

	    if ((!xmlStrcmp(cur->name, (const xmlChar *)"param"))) {
		    key = xmlGetProp(cur, (const unsigned char *)"name");
		    uri = xmlGetProp(cur, (const unsigned char *)"value");
		    fprintf(stderr,"name: %s, ", key);
		    fprintf(stderr,"value: %s\n", uri);
		    ParamS.name = xmlStrdup(key);
			ParamS.value = xmlStrdup(uri);
		    xmlFree(uri);
		    xmlFree(key);
		    return(ParamSp);
 	    }
 	    return(NULL);
}


/*
 ** Convert zDate into one or more integers according to the conversion
 ** specifier zFormat.
 **
 ** zFormat[] contains 4 characters for each integer converted, except for
 ** the last integer which is specified by three characters.  The meaning
 ** of a four-character format specifiers ABCD is:
 **
 **    A:   number of digits to convert.  Always "2" or "4".
 **    B:   minimum value.  Always "0" or "1".
 **    C:   maximum value, decoded as:
 **           a:  12
 **           b:  14
 **           c:  24
 **           d:  31
 **           e:  59
 **           f:  9999
 **    D:   the separator character, or \000 to indicate this is the
 **         last number to convert.
 **
 ** Example:  To translate an ISO-8601 date YYYY-MM-DD, the format would
 ** be "40f-21a-20c".  The "40f-" indicates the 4-digit year followed by "-".
 ** The "21a-" indicates the 2-digit month followed by "-".  The "20c" indicates
 ** the 2-digit day which is the last integer in the set.
 **
 ** The function returns the number of successful conversions.
 */
static int GetDigits(const char *zDate, const char *zFormat, ...){
    /* The aMx[] array translates the 3rd character of each format
     ** spec into a max size:    a   b   c   d   e     f */
    static const uint16_t aMx[] = { 12, 14, 24, 31, 59, 9999 };
    va_list ap;
    int cnt = 0;
    char nextC;
    va_start(ap, zFormat);
    do{
        char N = zFormat[0] - '0';
        char min = zFormat[1] - '0';
        int val = 0;
        uint16_t max;

        assert( zFormat[2]>='a' && zFormat[2]<='f' );
        max = aMx[zFormat[2] - 'a'];
        nextC = zFormat[3];
        val = 0;
        while( N-- ){
            if( !isdigit(*zDate) ){
                goto end_getDigits;
            }
            val = val*10 + *zDate - '0';
            zDate++;
        }
        if( val<(int)min || val>(int)max || (nextC!=0 && nextC!=*zDate) ){
            goto end_getDigits;
        }
        *va_arg(ap,int*) = val;
        zDate++;
        cnt++;
        zFormat += 4;
    }while( nextC );
end_getDigits:
    va_end(ap);
    return cnt;
}

/*
 ** Parse a timezone extension on the end of a date-time.
 ** The extension is of the form:
 **
 **        (+/-)HH:MM
 **
 ** Or the "zulu" notation:
 **
 **        Z
 **
 ** If the parse is successful, write the number of minutes
 ** of change in p->tz and return 0.  If a parser error occurs,
 ** return non-zero.
 **
 ** A missing specifier is not considered an error.
 */
static int ParseTimezone(const char *zDate, DateTime *p){
    int sgn = 0;
    int nHr, nMn;
    int c;
    while( isspace(*zDate) ){ zDate++; }
    p->tz = 0;
    c = *zDate;
    if( c=='-' ){
        sgn = -1;
    }else if( c=='+' ){
        sgn = +1;
    }else if( c=='Z' || c=='z' ){
        zDate++;
        goto zulu_time;
    }else{
        return c!=0;
    }
    zDate++;
    if( GetDigits(zDate, "20b:20e", &nHr, &nMn)!=2 ){
        return 1;
    }
    zDate += 5;
    p->tz = sgn*(nMn + nHr*60);
zulu_time:
    while( isspace(*zDate) ){ zDate++; }
    p->tzSet = 1;
    return *zDate!=0;
}

/*
 ** Parse times of the form HH:MM or HH:MM:SS or HH:MM:SS.FFFF.
 ** The HH, MM, and SS must each be exactly 2 digits.  The
 ** fractional seconds FFFF can be one or more digits.
 **
 ** Return 1 if there is a parsing error and 0 on success.
 */
static int ParseHhMmSs(const char *zDate, DateTime *p){
    int h, m, s;
    double ms = 0.0;
    if( GetDigits(zDate, "20c:20e", &h, &m)!=2 ){
        return 1;
    }
    zDate += 5;
    if( *zDate==':' ){
        zDate++;
        if( GetDigits(zDate, "20e", &s)!=1 ){
            return 1;
        }
        zDate += 2;
        if( *zDate=='.' && isdigit(zDate[1]) ){
            double rScale = 1.0;
            zDate++;
            while( isdigit(*zDate) ){
                ms = ms*10.0 + *zDate - '0';
                rScale *= 10.0;
                zDate++;
            }
            ms /= rScale;
        }
    }else{
        s = 0;
    }
    p->validJD = 0;
    p->rawS = 0;
    p->validHMS = 1;
    p->h = h;
    p->m = m;
    p->s = s + ms;
    if( ParseTimezone(zDate, p) ) return 1;
    p->validTZ = (p->tz!=0)?1:0;
    return 0;
}

/*
 ** Put the DateTime object into its error state.
 */
static void DatetimeError(DateTime *p){
    memset(p, 0, sizeof(*p));
    p->isError = 1;
}

/*
 ** Convert from YYYY-MM-DD HH:MM:SS to julian day.  We always assume
 ** that the YYYY-MM-DD is according to the Gregorian calendar.
 **
 ** Reference:  Meeus page 61
 */
static void ComputeJD(DateTime *p){
    int Y, M, D, A, B, X1, X2;

    if( p->validJD ) return;
    if( p->validYMD ){
        Y = p->Y;
        M = p->M;
        D = p->D;
    }else{
        Y = 2000;  /* If no YMD specified, assume 2000-Jan-01 */
        M = 1;
        D = 1;
    }
    if( Y<-4713 || Y>9999 || p->rawS ){
        DatetimeError(p);
        return;
    }
    if( M<=2 ){
        Y--;
        M += 12;
    }
    A = Y/100;
    B = 2 - A + (A/4);
    X1 = 36525*(Y+4716)/100;
    X2 = 306001*(M+1)/10000;
    p->iJD = (int64_t)((X1 + X2 + D + B - 1524.5 ) * 86400000);
    p->validJD = 1;
    if( p->validHMS ){
        p->iJD += p->h*3600000 + p->m*60000 + (int64_t)(p->s*1000);
        if( p->validTZ ){
            p->iJD -= p->tz*60000;
            p->validYMD = 0;
            p->validHMS = 0;
            p->validTZ = 0;
        }
    }
}

/*
 ** Parse dates of the form
 **
 **     YYYY-MM-DD HH:MM:SS.FFF
 **     YYYY-MM-DD HH:MM:SS
 **     YYYY-MM-DD HH:MM
 **     YYYY-MM-DD
 **
 ** Write the result into the DateTime structure and return 0
 ** on success and 1 if the input string is not a well-formed
 ** date.
 */
static int ParseYyyyMmDd(const char *zDate, DateTime *p){
    int Y, M, D, neg;

    if( zDate[0]=='-' ){
        zDate++;
        neg = 1;
    }else{
        neg = 0;
    }
    if( GetDigits(zDate, "40f-21a-21d", &Y, &M, &D)!=3 ){
        return 1;
    }
    zDate += 10;
    while( isspace(*zDate) || 'T'==*(uint8_t*)zDate ){ zDate++; }
    if( ParseHhMmSs(zDate, p)==0 ){
        /* We got the time */
    }else if( *zDate==0 ){
        p->validHMS = 0;
    }else{
        return 1;
    }
    p->validJD = 0;
    p->validYMD = 1;
    p->Y = neg ? -Y : Y;
    p->M = M;
    p->D = D;
    if( p->validTZ ){
        ComputeJD(p);
    }
    return 0;
}

/* The julian day number for 9999-12-31 23:59:59.999 is 5373484.4999999.
 ** Multiplying this by 86400000 gives 464269060799999 as the maximum value
 ** for DateTime.iJD.
 **
 ** But some older compilers (ex: gcc 4.2.1 on older Macs) cannot deal with
 ** such a large integer literal, so we have to encode it.
 */
#define INT_464269060799999  ((((int64_t)0x1a640)<<32)|0x1072fdff)

/*
 ** Return TRUE if the given julian day number is within range.
 **
 ** The input is the JulianDay times 86400000.
 */
static int ValidJulianDay(int64_t iJD){
    return iJD>=0 && iJD<=INT_464269060799999;
}

/*
 ** Compute the Year, Month, and Day from the julian day number.
 */
static void ComputeYMD(DateTime *p){
    int Z, A, B, C, D, E, X1;
    if( p->validYMD ) return;
    if( !p->validJD ){
        p->Y = 2000;
        p->M = 1;
        p->D = 1;
    }else if( !ValidJulianDay(p->iJD) ){
        DatetimeError(p);
        return;
    }else{
        Z = (int)((p->iJD + 43200000)/86400000);
        A = (int)((Z - 1867216.25)/36524.25);
        A = Z + 1 + A - (A/4);
        B = A + 1524;
        C = (int)((B - 122.1)/365.25);
        D = (36525*(C&32767))/100;
        E = (int)((B-D)/30.6001);
        X1 = (int)(30.6001*E);
        p->D = B - D - X1;
        p->M = E<14 ? E-1 : E-13;
        p->Y = p->M>2 ? C - 4716 : C - 4715;
    }
    p->validYMD = 1;
}

/*
 ** Compute the Hour, Minute, and Seconds from the julian day number.
 */
static void ComputeHMS(DateTime *p){
    int s;
    if( p->validHMS ) return;
    ComputeJD(p);
    s = (int)((p->iJD + 43200000) % 86400000);
    p->s = s/1000.0;
    s = (int)p->s;
    p->s -= s;
    p->h = s/3600;
    s -= p->h*3600;
    p->m = s/60;
    p->s += s - p->m*60;
    p->rawS = 0;
    p->validHMS = 1;
}

/*
 ** Compute both YMD and HMS
 */
static void ComputeYMD_HMS(DateTime *p){
    ComputeYMD(p);
    ComputeHMS(p);
}

/*
 ** Input "r" is a numeric quantity which might be a julian day number,
 ** or the number of seconds since 1970.  If the value if r is within
 ** range of a julian day number, install it as such and set validJD.
 ** If the value is a valid unix timestamp, put it in p->s and set p->rawS.
 */
static void SetRawDateNumber(DateTime *p, double r){
    p->s = r;
    p->rawS = 1;
    if( r>=0.0 && r<5373484.5 ){
        p->iJD = (int64_t)(r*86400000.0 + 0.5);
        p->validJD = 1;
    }
}

/*
 ** Clear the YMD and HMS and the TZ
 */
static void ClearYMD_HMS_TZ(DateTime *p){
    p->validYMD = 0;
    p->validHMS = 0;
    p->validTZ = 0;
}

// modified methods to only calculate for and back between epoch and iso timestamp with millis

uint64_t ParseTimeToEpochMillis(const char *str, bool *error) {
    assert(str);
    assert(error);
    *error = false;
    DateTime dateTime;
    memset(&dateTime, 0, sizeof(dateTime));
    int res = ParseYyyyMmDd(str, &dateTime);
    if (res) {
        *error = true;
        return 0;
    }

    ComputeJD(&dateTime);
    ComputeYMD_HMS(&dateTime);

    // get fraction (millis of a full second): 24.355 => 355
    int millis = (dateTime.s - (int)(dateTime.s)) * 1000;
    uint64_t epoch = (int64_t)(dateTime.iJD/1000 - 21086676*(int64_t)10000) * 1000 + millis;

    return epoch;
}

void TimeFromEpochMillis(uint64_t epochMillis, char *result, int resultLen, bool *error) {
    assert(resultLen >= 100);
    assert(result);
    assert(error);

    int64_t seconds = epochMillis / 1000;
    int millis = epochMillis - seconds * 1000;
    DateTime x;

    *error = false;
    memset(&x, 0, sizeof(x));
    SetRawDateNumber(&x, seconds);

    /*
     **    unixepoch
     **
     ** Treat the current value of p->s as the number of
     ** seconds since 1970.  Convert to a real julian day number.
     */
    {
        double r = x.s*1000.0 + 210866760000000.0;
        if( r>=0.0 && r<464269060800000.0 ){
            ClearYMD_HMS_TZ(&x);
            x.iJD = (int64_t)r;
            x.validJD = 1;
            x.rawS = 0;
        }

        ComputeJD(&x);
        if( x.isError || !ValidJulianDay(x.iJD) ) {
            *error = true;
        }
    }

    ComputeYMD_HMS(&x);
    snprintf(result, resultLen, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
             x.Y, x.M, x.D, x.h, x.m, (int)(x.s), millis);
}

