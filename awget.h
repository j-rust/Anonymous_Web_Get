//
// Created by zach on 10/20/15.
//

#ifndef ANONYMOUS_WEB_GET_AWGET_H
#define ANONYMOUS_WEB_GET_AWGET_H

#include <stdio.h>

/**
 * @args file - the file that you want the contents of printed to the terminal.
 * This is used only for debugging.
 */
void printFileContents(FILE * file);

/**
 * @args argc - the number of arguments supplied to the program
 * @args argv - the values of each of the arguments supplied to the program
 * @return - the name of the chaingang file supplied by the user or picked up from the local directory
 * Parses command line inputs to get the name of the stepping stone file if the user passed one through
 * the command line.
 */
char * getChainFileName(int argc, char *argv[]);

/**
 * @args filename - the name of the file to be read
 * @return - a file pointer that points to the contents of the file
 * Finds the correct file depending on if the user specified a file, or if it was found in the local directory.
 */
FILE * readFile(char * filename);

/**
 * Prints a help message to the user.  Used when there is an error in the input values.
 */
void help();

#endif //ANONYMOUS_WEB_GET_AWGET_H
