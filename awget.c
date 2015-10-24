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

FILE *readFile(char *filename)
{
    FILE *ifp, *ofp;

    /*Use chaingang file from local directry*/
    if (strcmp(filename, "chaingang.txt") == 0)
    {
        ifp = fopen("chaingang.txt", "r");
        if (ifp == NULL)
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
        if (ifp == NULL)
        {
            printf("Error while opening %s file.\n", filename);
            help();
            exit(-1);
        }
        return ifp;
    }
}

char *getChainFileName(int argc, char *argv[])
{
    int c;
    char *url;
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

    if (urlFlag == 0)
    {
        help();
        return NULL;
    }

    return url;
}

void printFileContents(FILE *file)
{
    char c;
    while ((c = fgetc(file)) != EOF)
    {
        printf("%c", c);
    }
    rewind(file);
    printf("\n");
}

char *getPort(char *line)
{
    char *tmp;
    tmp = strtok(line, "\t");
    tmp = strtok(NULL, "\t");
    int len = strlen(tmp);
    int current = 0;
    while (current < len)
    {
        if (tmp[current] == '\n') tmp[current] = '\0';
        current++;
    }
    return tmp;
}

char *getIP(char *line)
{
    char *tmp;
    tmp = strtok(line, "\t");
    return tmp;
}

uint32_t getFileLength(FILE *file)
{
    fseek(file, 0L, SEEK_END);

    int fileSize = ftell(file);
    rewind(file);
    return fileSize;
}

char *getFileName(char *url)
{
    int urlLength = strlen(url);
    int i = 0;
    int lastSlashPosition = 0;
    char c;
    for (i; i < urlLength; i++)
    {
        if (url[i] == '/')
        {
            lastSlashPosition = i;
        }
    }

    char *filename = calloc(urlLength - lastSlashPosition, sizeof(char));
    memcpy(filename, url + lastSlashPosition + 1, urlLength - lastSlashPosition);
    if (lastSlashPosition == 0)
    {
        return "index.html";
    }
    else
    {
        return filename;
    }
}


void sendFileToRandomSS(char *IPAddress, char *portNumber, FILE *file, char *url)
{
    int sockfd, status, connect_status;
    struct sockaddr_in serv_addr;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


    if ((status = getaddrinfo(IPAddress, portNumber, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);//
    }


    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    connect_status = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
    if (connect_status < 0)
    {
        perror("Error trying to connect\n");
        return;
    }

    /*Set variables for packet header*/
    uint16_t lenghtOfURL = strlen(url);
    uint32_t totalLength = htonl(1000);
    uint16_t lengthOfURLNetworkFormat = htons(lenghtOfURL);
    char *urlData = (char *) calloc(6 + strlen(url), sizeof(char));
    /*Fill buffer with correct information*/
    memcpy(urlData + 0, &totalLength, 4);
    memcpy(urlData + 4, &lengthOfURLNetworkFormat, 2);
    memcpy(urlData + 6, url, strlen(url));

    int a = 0;
    for (a = 0; a < 6 + strlen(url); a++)
    {
        char c = urlData[a];
        if (DEBUG) printf("%c\n", c);
    }


    if (send(sockfd, urlData, 6 + strlen(url), 0) < 0)
    {
        printf("Send failed\n");
        abort();
    }

    char *rec_buffer = calloc(500, sizeof(char));

    int recv_status = recv(sockfd, rec_buffer, 500, 0);

    free(rec_buffer);


    uint32_t fileLength = getFileLength(file);

    ///////////////// check size //////////////
    if (fileLength < 401)
    {
        if (DEBUG) printf("File length under 400bytes\n");
        char *data = (char *) calloc(406, sizeof(char));

        uint16_t packetLength = fileLength;
        uint32_t fileLengthSend = htonl(fileLength);
        uint16_t packetLengthSend = htons(packetLength);

        memcpy(data + 0, &fileLengthSend, 4);
        memcpy(data + 4, &packetLengthSend, 2);
        char ch;
        int counter = 0;

        while ((ch = fgetc(file)) != EOF)
        {
            data[counter + 6] = ch;
            counter++;
        }

        if (send(sockfd, data, 6 + fileLength, 0) < 0)
        {
            printf("Send failed\n");
            abort();
        }

        free(data);

        char *rec_buffer = calloc(500, sizeof(char));

        int recv_status = recv(sockfd, rec_buffer, 500, 0);

        free(rec_buffer);
    }
    else
    {
        /*File over 400 bytes*/
        if (DEBUG) printf("File length over 400bytes\n");

        /*All parts of file will be 400 bytes*/
        //if(fileLength % 400 == 0)
        //{
        /******************************************Dont forget to do this zach*****************************************/
        //}
        /*Last part of file will be less than 400 bytes*/
        //else
        //{
        /*Find out how many times file needs to be split*/
        int numberOfParts = fileLength / 400;
        /*Figure out last part of files size*/
        int sizeOfLastPart = fileLength % 400;
        /*Loop through the file the number of times it is split*/
        int i = 0;
        for (i; i <= numberOfParts; i++)
        {
            if (DEBUG) printf("In iteration %d of the loop for breaking up packets\n", i);
            char *data;
            /*Last part of the file.  Could be smaller than 400 bytes*/
            if (numberOfParts == i)
            {
                if (DEBUG) printf("Sending last part of file\n");
                data = (char *) calloc(6 + sizeOfLastPart, sizeof(char));
                /*Go to right position in file*/
                fseek(file, i * 400, SEEK_SET);

                /*Set variables for packet header*/
                uint16_t packetLength = 6 + sizeOfLastPart;
                uint32_t fileLengthSend = htonl(fileLength);
                uint16_t packetLengthSend = htons(packetLength);

                /*Fill buffer with correct information*/
                memcpy(data + 0, &fileLengthSend, 4);
                memcpy(data + 4, &packetLengthSend, 2);

                /*Write chars into buffer*/
                int j = 0;
                for (j; j < sizeOfLastPart; j++)
                {
                    char ch;
                    ch = fgetc(file);
                    data[6 + j] = ch;
                }

                if (send(sockfd, data, 6 + sizeOfLastPart, 0) < 0)
                {
                    printf("Send failed\n");
                    abort();
                }

                if (DEBUG) printf("send in loop iteration %d is:\n", i);
                char c;
                int k = 0;
                for (k = 0; k < sizeOfLastPart + 6; k++)
                {
                    if (DEBUG) printf("%c", data[k]);
                }


                free(data);

                char *rec_buffer = calloc(500, sizeof(char));

                int recv_status = recv(sockfd, rec_buffer, 500, 0);

                free(rec_buffer);
            }
                /*Middle of file where the parts are still 400 bytes*/
            else
            {
                if (DEBUG) printf("Sending middle parts of file\n");
                data = (char *) calloc(406, sizeof(char));
                /*Go to right position in file*/
                fseek(file, i * 400, SEEK_SET);

                /*Set variables for packet header*/
                uint16_t packetLength = 400;
                uint32_t fileLengthSend = htonl(fileLength);
                uint16_t packetLengthSend = htons(packetLength);

                /*Fill buffer with correct information*/
                memcpy(data + 0, &fileLengthSend, 4);
                memcpy(data + 4, &packetLengthSend, 2);

                /*Write chars into buffer*/
                int j = 0;
                for (j; j < 400; j++)
                {
                    char ch;
                    ch = fgetc(file);
                    data[6 + j] = ch;
                }

                if (send(sockfd, data, 406, 0) < 0)
                {
                    printf("Send failed\n");
                    abort();
                }

                if (DEBUG) printf("send in loop iteration %d is:\n", i);
                int k = 0;
                char c;
                for (k = 6; k < 406; k++)
                {
                    if (DEBUG) printf("%c", data[k]);
                }

                free(data);

                char *rec_buffer = calloc(500, sizeof(char));

                int recv_status = recv(sockfd, rec_buffer, 500, 0);

                free(rec_buffer);
                //}
            }
        }
    }

    uint32_t total_bytes_received = 0;
    uint32_t file_length = 1;
    unsigned int msg_length;
    char *ack = "Ack";

    char *outBuff = NULL;


    //FILE *outfileptr = fopen("outfile.txt", "w+");

    int packetCounter = 0;
    while (total_bytes_received < file_length)
    {
        packetCounter++;
        printf("***************Packet counter is %d\n", packetCounter);
        char *receiverBuffer = (char *) calloc(500, sizeof(char));
        msg_length = 0;
        recv_status = recv(sockfd, receiverBuffer, 500, 0);
        if (recv_status < 0)
        { perror("Error: receive failed\n"); }


        memcpy(&file_length, receiverBuffer + 0, 4);
        file_length = ntohl(file_length);
        memcpy(&msg_length, receiverBuffer + 4, 2);
        msg_length = ntohs(msg_length);

        if (outBuff == NULL)
        {
            outBuff = (char *) calloc(file_length, sizeof(char));
        }


        if (DEBUG) printf("File Length: %zu\n", file_length);
        if (DEBUG) printf("Message Length: %zu\n", msg_length);
        if (DEBUG) printf("Bytes Received: %d\n", total_bytes_received);

        char *data = calloc(msg_length, sizeof(char));
        memcpy(data, receiverBuffer + 6, msg_length);
        if (DEBUG) printf("Message is: %s\n", data);
        if (DEBUG) printf("length of data is %d\n", strlen(data));

        memcpy(outBuff + total_bytes_received, data, msg_length);
        total_bytes_received += msg_length;

        free(data);
        if (DEBUG) printf("free worked\n");
        memset(rec_buffer, 0, 500);

        /* send ack to add temporary to delay */
        if (send(sockfd, ack, strlen(ack), 0) < 0)
        {
            int send_status = send(sockfd, ack, strlen(ack), 0);
            while (send_status < 0)
            {
                send_status = send(sockfd, ack, strlen(ack), 0);
            }

        }
    }
    if (DEBUG) printf("out of loop\n");
    //fclose(outfileptr);
    if (DEBUG) printf("Closed file pointer\n");
    //FILE *outfileptr = fopen(getFileName(url), "w");
    FILE *outfileptr = fopen("testoutputfile", "w");
    fprintf(outfileptr, outBuff);
    if (DEBUG) printf("%d\n", strlen(outBuff));
    fclose(outfileptr);

}

int generateRandomNumber()
{
    time_t t;
    srand((unsigned) time(&t));
    return rand();
}

/*This is not truly random.  If we have time we should try to fix it.*/
char *getRandomSS(FILE *filename)
{
    char *contentsOfLine = calloc(100, sizeof(char));
    int numberOfLinesInFile = 0;
    int lineCounter = 1;
    char ch;
    int positionOfCharInLine = 0;
    int randomNumber = 0;

    char *line = calloc(30, sizeof(char));

    /*Get the characters on the first line of the file which will be the number of lines in the file*/
    while ((ch = fgetc(filename)) != '\n')
    {
        if (ch != '\n')
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

//    if(randomNumber == numberOfLinesInFile)
//    {
//        randomNumber = randomNumber - 1;
//    }


    while (fgets(line, 30, filename))
    {
        if (lineCounter == randomNumber)
        {
            if (DEBUG) printf("Found the line to return\n");
            return line;
        }
        lineCounter++;
        memset(line, 0, 30);
    }
}

int main(int argc, char **argv)
{
    /*File containing the stepping stones passed in from the user, or frome the chaingaing.txt file*/
    FILE *chaingangFile;
    char *URL;
    /*If only one argument is supplied look for the chaingaing file in the local directory*/
    if (argc == 2)
    {
        char *filename = "chaingang.txt";
        URL = argv[1];
        if (DEBUG) printf("URL is %s\n", URL);
        chaingangFile = readFile(filename);
    }
    else if (argc == 4)
    {
        URL = argv[1];
        char *chainFileName = getChainFileName(argc, argv);

        if (chainFileName == NULL)
        {
            help();
            return -1;
        }
        if (DEBUG) printf("URL is %s\n", URL);
        if (DEBUG) printf("Chainfile name is %s\n", chainFileName);
        chaingangFile = readFile(chainFileName);
    }
    else
    {
        help();
        return -1;
    }

    if (DEBUG) printFileContents(chaingangFile);
    char *firstSS = getRandomSS(chaingangFile);
    char *tmpLine = calloc(0, strlen(firstSS));
    memcpy(tmpLine, firstSS, strlen(firstSS));
    char *firstSSIP = getIP(tmpLine);
    memset(tmpLine, 0, strlen(firstSS));
    memcpy(tmpLine, firstSS, strlen(firstSS));
    char *firstSSPort = getPort(tmpLine);

    if (DEBUG) printf("%s %s\n", firstSSIP, firstSSPort);
    if (DEBUG) printf("Size of port %zu\n", strlen(firstSSPort));

    //sendFileToRandomSS(firstSSIP, atoi(firstSSPort));
    sendFileToRandomSS(firstSSIP, firstSSPort, chaingangFile, URL);

    //printf("File size is %d\n", getFileLength(chaingangFile));


    return 0;
}

