//
// Created by zach on 10/20/15.
//

#include "awget.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

#define DEBUG 1

void help()
{
    printf("usage: To run awget you must provide the url of the file you would like to download.  You may also provide"
           " the location of the chainfile with -c chainfile");
}

FILE * readFile(char * filename)
{
    FILE *ifp, *ofp;

    /*Use chaingang file from local directry*/
    if(strcmp(filename, "chaingang.txt") == 0)
    {
        ifp = fopen("chaingang.txt", "r");
        if(ifp == NULL)
        {
            perror("Error while opening the chaingang.txt file.\n");
            help();
            exit(-1);
        }
        return ifp;
    }
        /*Use user specified filename*/
    else
    {
        ifp = fopen(filename, "r");
        if(ifp == NULL)
        {
            perror("Error while opening %s file.\n", filename);
            help();
            exit(-1);
        }
        return ifp;
    }
}

char * getChainFileName(int argc, char *argv[])
{
    int c;
    char * url;
    int urlFlag = 0;

    opterr = 0;
    while ((c = getopt(argc, argv, "c:")) != -1)
        switch (c)
        {
            case 'c':
                url = optarg;
                urlFlag = 1;
                break;
            case 'h':
                help();
                return NULL;
            case '?':
                if (optopt == 'c')
                {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                    help();
                }
                else if (isprint(optopt))
                {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                    help();
                }
                else
                {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                    help();
                }
                return NULL;
            default:
                printf("aborting\n");
                abort();
        }

    if(urlFlag == 0)
    {
        help();
        return NULL;
    }

    return url;
}

void printFileContents(FILE * file)
{
    char c;
    while((c = fgetc(file) ) != EOF )
    {
        printf("%c", c);
    }
    printf("\n");
}

int main(int argc, char **argv)
{
    /*File containing the stepping stones passed in from the user, or frome the chaingaing.txt file*/
    FILE * chaingangFile;
    char * URL;
    /*If only one argument is supplied look for the chaingaing file in the local directory*/
    if(argc == 2)
    {
        char * filename = "chaingang.txt";
        URL = argv[1];
        if(DEBUG) printf("URL is %s\n", URL);
        chaingangFile = readFile(filename);
    }
    else if(argc == 4)
    {
        URL = argv[1];
        char * chainFileName = getChainFileName(argc, argv);

        if(chainFileName == NULL)
        {
            help();
            return -1;
        }
        if(DEBUG) printf("URL is %s\n", URL);
        if(DEBUG) printf("Chainfile name is %s\n", chainFileName);
        chaingangFile = readFile(chainFileName);
    }
    else
    {
        help();
        return -1;
    }

    if(DEBUG) printFileContents(chaingangFile);


    return 0;
}

