//
// Created by Jonathan Rust on 10/20/15.
//

#include "ss.h"
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <time.h>

#define DEBUG 1


void server(unsigned short port){
	int status, sockfd, clientfd, listen_success;
	struct sockaddr_in server;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	char ipstr[INET6_ADDRSTRLEN];

	memset(&server, 0, sizeof(server));
	sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(sockfd == -1){
		perror("Error establishing socket\n");
		return;
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(port);

	int bind_status = bind(sockfd, (struct sockaddr *) &server, sizeof(server));
	if (bind_status < 0) {
		perror("Error binding to socket\n");
		return;
	}


	listen_success = listen(sockfd, 10);
	if(listen_success == -1){
		perror("Unable to listen to socket\n");
		return;
	}

	struct sockaddr_in sockin;
	socklen_t socklen = sizeof(sockin);
	getsockname(sockfd, (struct sockaddr *) &sockin, &socklen);


	printf("%s %d\n", getCurrentIP(), ntohs(sockin.sin_port));

	addr_size = sizeof(their_addr);
	clientfd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);
	printf("Accepted an incoming connection\n");

	char *rec_buffer = calloc(500, sizeof(char));
	uint32_t file_length = 1;
	uint32_t total_bytes_received = 0;

	int recv_status;
	unsigned int msg_length;

	/* First packet received is the url */
	recv_status = recv(clientfd, rec_buffer, 500, 0);
	memcpy(&msg_length, rec_buffer + 4, 2);
	msg_length = ntohs(msg_length);
	char *url = calloc(msg_length, sizeof(char));
	memcpy(url, rec_buffer + 6, msg_length);
	memset(rec_buffer, 0, strlen(rec_buffer));
	char *ack = "Ack";

	printf("Received url: %s\n", url);

	/* send ack to add temporary to delay */
	if(send(clientfd, ack, strlen(ack), 0) < 0) {
//		int send_status = send(clientfd, ack, strlen(ack), 0);
//		while (send_status < 0) {
//			send_status = send(clientfd, ack, strlen(ack), 0);
//		}

		printf("Send failed\n");
	}


	FILE *ofp = fopen("chainlist.txt", "w");

	while (total_bytes_received < file_length) {
		msg_length = 0;
		recv_status = recv(clientfd, rec_buffer, 500, 0);
		if (recv_status < 0) { perror("Error: receive failed\n"); }

		memcpy(&file_length, rec_buffer + 0, 4);
		file_length = ntohl(file_length);
		memcpy(&msg_length, rec_buffer + 4, 2);
		msg_length = ntohs(msg_length);

		total_bytes_received += msg_length;

		if(DEBUG) printf("File Length: %zu\n", file_length);
		if(DEBUG)printf("Message Length: %u\n", msg_length);
		if(DEBUG)printf("Bytes Received: %d\n", total_bytes_received);

		char *data = calloc(msg_length, sizeof(char));
		memcpy(data, rec_buffer + 6, msg_length);
		if(DEBUG)printf("Message is: %s\n", data);
		fprintf(ofp, data);

		free(data);
		memset(rec_buffer, 0, 500);
		/* send ack to add temporary to delay */
		if(send(clientfd, ack, strlen(ack), 0) < 0) {
			int send_status = send(clientfd, ack, strlen(ack), 0);
			while (send_status < 0) {
				send_status = send(clientfd, ack, strlen(ack), 0);
			}

		}

	}
	fclose(ofp);

	char* next_ss = getNextSteppingStone();
	if (next_ss == NULL) {
		char cmd_buf[1024];
		snprintf(cmd_buf, sizeof(cmd_buf), "wget %s -O download_file", url);
		system(cmd_buf);

		char *tmp_buffer = calloc(100, sizeof(char));


		FILE * file;
		file = fopen("download_file", "r");

		uint32_t fileLength = getFileLength("download_file");

		///////////////// check size //////////////
		if(fileLength < 401)
		{
			printf("File length under 400bytes\n");
			char* content = (char *) calloc(406, sizeof(char));

			uint16_t packetLength = fileLength;
			uint32_t fileLengthSend = htonl(fileLength);
			uint16_t packetLengthSend = htons(packetLength);

			memcpy(content + 0, &fileLengthSend, 4);
			memcpy(content + 4, &packetLengthSend, 2);
			char ch;
			int counter = 0;

			while((ch = fgetc(file) ) != EOF )
			{
				content[counter + 6] = ch;
				counter++;
			}

			if (send(sockfd, content, 6 + fileLength, 0) < 0)
			{
				printf("Send failed\n");
				abort();
			}
			recv_status = recv(clientfd, tmp_buffer, 500, 0);

			free(content);
		}
		else
		{
			/*File over 400 bytes*/
			printf("File length over 400bytes\n");

			/*All parts of file will be 400 bytes*/
			if(fileLength % 400 == 0)
			{

			}
				/*Last part of file will be less than 400 bytes*/
			else
			{
				/*Find out how many times file needs to be split*/
				int numberOfParts = fileLength / 400;
				/*Figure out last part of files size*/
				int sizeOfLastPart = fileLength % 400;
				/*Loop through the file the number of times it is split*/
				int i = 0;
				for (i; i <= numberOfParts; i++)
				{
					if(DEBUG) printf("In iteration %d of the loop for breaking up packets\n", i);
					char *content;
					/*Last part of the file.  Could be smaller than 400 bytes*/
					if (numberOfParts == i)
					{
						if(DEBUG) printf("Sending last part of file\n");
						content = (char *) calloc(6 + sizeOfLastPart, sizeof(char));
						/*Go to right position in file*/
						fseek(file, i * 400, SEEK_SET);

						/*Set variables for packet header*/
						uint16_t packetLength = 6 + sizeOfLastPart;
						uint32_t fileLengthSend = htonl(fileLength);
						uint16_t packetLengthSend = htons(packetLength);

						/*Fill buffer with correct information*/
						memcpy(content + 0, &fileLengthSend, 4);
						memcpy(content + 4, &packetLengthSend, 2);

						/*Write chars into buffer*/
						int j = 0;
						for(j; j < sizeOfLastPart; j++)
						{
							char ch;
							ch = fgetc(file);
							content[6 + j] = ch;
						}

						if (send(clientfd, content, 6 + sizeOfLastPart, 0) < 0)
						{
							printf("Send failed\n");
							abort();
						}

						recv_status = recv(clientfd, tmp_buffer, 500, 0);

						printf("send in loop iteration %d is:\n", i);
						char c;
						int k = 0;
						for(k = 0; k < sizeOfLastPart + 6; k++)
						{
							printf("%c", content[k]);
						}


						free(content);
					}
						/*Middle of file where the parts are still 400 bytes*/
					else
					{
						if(DEBUG) printf("Sending middle parts of file\n");
						char* content_buffer = calloc(406, sizeof(char));
						/*Go to right position in file*/
						fseek(file, i * 400, SEEK_SET);

						/*Set variables for packet header*/
						uint16_t packetLength = 400;
						uint32_t fileLengthSend = htonl(fileLength);
						uint16_t packetLengthSend = htons(packetLength);

						/*Fill buffer with correct information*/
						memcpy(content_buffer + 0, &fileLengthSend, 4);
						memcpy(content_buffer + 4, &packetLengthSend, 2);

						/*Write chars into buffer*/
						int j = 0;
						for(j; j < 400; j++)
						{
							char ch;
							ch = fgetc(file);
							content_buffer[6 + j] = ch;
						}


						if (send(clientfd, content_buffer, 406, 0) < 0)
						{
							printf("Send failed\n");
							abort();
						}

						recv_status = recv(clientfd, tmp_buffer, 500, 0);

						printf("send in loop iteration %d is:\n", i);
						free(content_buffer);

					}
				}
			}
		}


	} else {
		client(next_ss, url);
	}

}

void client(char* next_ss_info, char* url){

	char *mutable_info = calloc(strlen(next_ss_info), sizeof(char));
	memcpy(mutable_info, next_ss_info, strlen(next_ss_info));

	int sockfd, status, connect_status;
	struct sockaddr_in serv_addr;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	char* ip = parseIP(mutable_info);
	if(DEBUG) printf("Parsed IP address: %s\n", ip);
	memset(mutable_info, 0, strlen(next_ss_info));
	memcpy(mutable_info, next_ss_info, strlen(next_ss_info));
	char *port = parsePort(mutable_info);
	if(DEBUG) printf("Parsed port: %s\n", port);

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

	return;
}

char* getNextSteppingStone(){
	FILE *ifp = fopen("chainlist.txt", "r");

	if (ifp == NULL) {
		perror("Can't open file chainlist.txt");
		exit(-1);
	}
	char* line_holder = calloc(100, sizeof(char));
	char c = NULL;
	int num_ss;

	c = fgetc(ifp);
	num_ss = c - '0';
	fclose(ifp);
	if(num_ss == 1) return NULL;

	num_ss--;
	removeCurrentHost(num_ss);

	return getRandomSS();

}

void removeCurrentHost(int num_ss){

	char* ip = getCurrentIP();
	FILE *ifp = fopen("chainlist.txt", "r");
	FILE *ofp = fopen("chainlist.txt.new", "w");
	char* line = calloc(30, sizeof(char));
	char *backup_line = calloc(30, sizeof(char));
	char* compIP;


	fgets(line, 30, ifp);
	if(DEBUG) printf("Read first line and got %s", line);

	fprintf(ofp, "%d\n", num_ss);

	while (fgets(line, 30, ifp)) {
		memcpy(backup_line, line, 30);
		compIP = parseIP(line);
		/* Only write to the new file if it's not the current IP address */
		printf("Compared %s to %s and got %d\n", compIP, ip, strcmp(ip, compIP));
		if (strcmp(ip, compIP) != 0) {
			fprintf(ofp, "%s", backup_line);
		}
		memset(line, 0, 30);
		memset(backup_line, 0, 30);
	}
	fclose(ifp);
	fclose(ofp);
	rename("chainlist.txt.new", "chainlist.txt");
}

char* getCurrentIP(){
	char Buf [ 200 ] ;
	struct hostent * Host = (struct hostent * ) malloc ( sizeof ( struct hostent ));
	gethostname ( Buf , 200 );
	Host = ( struct hostent * ) gethostbyname ( Buf );

	return inet_ntoa(*((struct in_addr *)Host->h_addr));
}

char* parseIP(char* line){
	char* tmp;
	tmp = strtok(line, "\t");
	return tmp;
}

char* parsePort(char *line){
	char* tmp;
	if(DEBUG) printf("Received line %s\n", line);
	tmp = strtok(line, "\t");
	if(DEBUG) printf("First port token: %s\n", tmp);
	tmp = strtok(NULL, "\t");
	return tmp;
}

int generateRandomNumber(int num_ss)
{
	time_t t;
	int next_ss; /* line containg info for the next stepping stone */
	srand((unsigned) time(&t));
	/* Generate 100 random numbers to increase randomness -> probably not necessary */
	int i = 0;
	for (i; i < 100; i++) {
		next_ss = rand() % (num_ss);
	}
	return next_ss + 1;
}

char* getRandomSS()
{
	FILE *ifp;
	ifp = fopen("chainlist.txt", "r");
	char* contentsOfLine = calloc(100, sizeof(char));
	int numberOfLinesInFile = 0;
	int lineCounter = 1;
	char ch;
	int positionOfCharInLine = 0;
	int randomNumber = 0;

	/*Get the characters on the first line of the file which will be the number of lines in the file*/
	while((ch = fgetc(ifp) ) != '\n' )
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
	randomNumber = (generateRandomNumber(numberOfLinesInFile)) + 1;


	char* line = calloc(30, sizeof(char));
	while (fgets(line, 30, ifp)) {
		if(lineCounter == randomNumber) {
			fclose(ifp);
			return line;
		}
		lineCounter++;
		memset(line, 0, 30);
	}

}

uint32_t getFileLength(char * filename)
{
	FILE* file;
	file = fopen(filename, "r");
	fseek(file, 0L, SEEK_END);

	int fileSize = ftell(file);
	rewind(file);
	return fileSize;
}




int main(int argc, char** argv){
	if(argc > 1) server(atoi(argv[1]));
	else server(0);
	return 0;
}

