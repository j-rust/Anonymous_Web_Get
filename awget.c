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
            printf("Error while opening the chaingang.txt file.\n");
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
            printf("Error while opening %s file.\n", filename);
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
    rewind(file);
    printf("\n");
}

char * getPort(char * line)
{
    char* tmp;
    tmp = strtok(line, "\t");
    tmp = strtok(NULL, "\t");
    return tmp;
}

char* getIP(char* line){
    char* tmp;
    tmp = strtok(line, "\t");
    return tmp;
}

/*
void sendFileToRandomSS()
{
    if(DEBUG) printf("Calling sendFileToRandomSS\n");
    char* next_ss_info = getNextSteppingStone();
    if(DEBUG){
        exit(0);
    }
    if(next_ss_info == NULL){
        // call wget and send the file down the chain
    }

    int sockfd, status, connect_status;
    struct sockaddr_in serv_addr;
    struct addrinfo hints;
    struct addrinfo *servinfo;
    char* ip = NULL;
    char* port = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


    if ((status = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if(connect_status < 0){
        perror("Error trying to connect\n");
        return;
    }

}

*/

int generateRandomNumber()
{
    time_t t;
    srand((unsigned) time(&t));
    return rand();
}

/*This is not truly random.  If we have time we should try to fix it.*/
char* getRandomSS(FILE * filename)
{
    char* contentsOfLine = calloc(100, sizeof(char));
    int numberOfLinesInFile = 0;
    int lineCounter = 0;
    char ch;
    int positionOfCharInLine = 0;
    int randomNumber = 0;

    /*Get the characters on the first line of the file which will be the number of lines in the file*/
    while((ch = fgetc(filename) ) != '\n' )
    {
        if(ch != '\n')
        {
            contentsOfLine[positionOfCharInLine] = ch;
        }
        else
        {
            break;
        }
        positionOfCharInLine++;
    }

    numberOfLinesInFile = atoi(contentsOfLine);
    randomNumber = (generateRandomNumber() % numberOfLinesInFile) + 1;
    positionOfCharInLine = 0;

    if(randomNumber == numberOfLinesInFile)
    {
        randomNumber = randomNumber - 1;
    }

    do
    {
        if(ch == '\n' && lineCounter == randomNumber)
        {
            return contentsOfLine;
        }
        else if(ch == '\n')
        {
            lineCounter++;
            memset(contentsOfLine, 0, sizeof(contentsOfLine));
            positionOfCharInLine = 0;
        }
        else
        {
            contentsOfLine[positionOfCharInLine] = ch;
            positionOfCharInLine++;
        }
    }while((ch = fgetc(filename) ) != EOF );
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
    char *firstSS = getRandomSS(chaingangFile);
    char *tmpLine = calloc(0, strlen(firstSS));
    memcpy(tmpLine, firstSS, strlen(firstSS));
    char * firstSSIP = getIP(tmpLine);
    memset(tmpLine, 0, strlen(firstSS));
    memcpy(tmpLine, firstSS, strlen(firstSS));
    char * firstSSPort = getPort(tmpLine);

    if(DEBUG) printf("IP is %s, port is %s\n", firstSSIP, firstSSPort);



    return 0;
}

