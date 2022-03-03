#include <stdio.h>
/* For "exit". */
#include <stdlib.h>
/* For "strerror". */
#include <string.h>
/* For "errno". */
#include <errno.h>
#include <zlib.h>

/* if "test" is true, print an error message and halt execution. */

#define FAIL(test,message) {                             \
        if (test) {                                      \
            fprintf (stderr, "%s:%d: " message           \
                     " file '%s' failed: %s\n",          \
                     __FILE__, __LINE__, file_name,      \
                     strerror (errno));                  \
            exit (EXIT_FAILURE);                         \
        }                                                \
    }

#define FAIL2(test,message) {                             \
        if (test) {                                      \
            fprintf (stderr, "%s:%d: %s file '%s' failed: %s\n",          \
                     __FILE__, __LINE__, message, file_name,      \
                     strerror (errno));                  \
            exit (EXIT_FAILURE);                         \
        }                                                \
    }



#define LENGTH 0x1000

int unzipfile (const char * file_name)
{

	gzFile file;
	FILE * fileout;

    /* Open the gz file. */
    int slen;
    if ( (slen = strlen(file_name)) < 4 ) {
    	fprintf(stderr,"bad file name in unzipfile\n");
    	return(1);
    }
    if (strcmp(file_name+slen-3,".gz")) {
    	fprintf(stderr,"file extention in unzipfile not \".gz\"\n");
    	return(1);
    }
    file = gzopen (file_name, "rb");
    FAIL (! file, "gzopen");

    /* Open the output file. */
    char * newfilename = NULL;
    newfilename = (char*)malloc(slen+1);
    strcpy(newfilename,file_name);
    newfilename[slen-3] = '\000';
    fileout = fopen (newfilename, "wb");
    free(newfilename);
    FAIL (! fileout, "open");

    while (1) {
         int err;
         int bytes_read;
         unsigned char buffer[LENGTH];
         bytes_read = gzread (file, buffer, LENGTH - 1);
         buffer[bytes_read] = '\0';
         fprintf (fileout,"%s", buffer);
         if (bytes_read < LENGTH - 1) {
             if (gzeof (file)) {
                 break;
             }
             else {
                 const char * error_string;
                 error_string = gzerror (file, & err);
                 FAIL2 (err, error_string);
             }
         }
     }
     FAIL (gzclose (file), "gzclose");
     FAIL (fclose (fileout), "close");
     return 0;
}
