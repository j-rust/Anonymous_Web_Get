//
// Created by Jonathan Rust on 10/20/15.
//

#ifndef ANONYMOUS_WEB_GET_SS_H
#define ANONYMOUS_WEB_GET_SS_H

/**
 * @args port - Port to connect to. If no port is passed as a command line
 * argument, 0 will be passed.
 */
void server(unsigned short port);

/**
 * The IP address and port for the server to connect to is determined by "host_list.txt"
 * If there are no remaining hosts to connect to, the client will initiate file retrieval
 */
void client();

/**
 * Reads host_list.txt
 * @return - The line containing the IP address and port of the host to connect to.
 * If there are no other remaining hosts int the file, NULL is returned.
 */
char* getNextSteppingStone();


#endif //ANONYMOUS_WEB_GET_SS_H
