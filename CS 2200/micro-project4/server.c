#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
// Mani Japra
// CS 2200 - Spring 2016

/* Some constants that you can use */
#define ROT 13
#define BUFFER_SIZE 256

/* The port number you must setup the TCP socket on */
const unsigned short PORT = 2200;

struct sockaddr_in socketInfo;

/* Function prototype for the scramble string method */
static void scramble_string(char *str);

/* 
 * The main method. You must fill this out as described in the project description PDF
 */
int main()
{
	// #### YOUR CODE GOES HERE ####
	socketInfo.sin_addr.s_addr = INADDR_ANY;
	socketInfo.sin_port = htons(PORT);
	socketInfo.sin_family = AF_INET;
	
	int sSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	bind(sSocket, (struct sockaddr *) &socketInfo, sizeof(socketInfo));
	listen(sSocket, 10);
	int sSize = sizeof(socketInfo);
	char message[BUFFER_SIZE];
	
	while(1)
	{
		bzero(message, BUFFER_SIZE);
		int cSocket = accept(sSocket, (struct sockaddr *) &socketInfo, (socklen_t *) &sSize);
		recv(cSocket, message, BUFFER_SIZE, 0);
		printf("Recieved Message: %s\n", message);
		scramble_string(message);
		printf("Sent Message: %s\n", message);
		send(cSocket, message, BUFFER_SIZE, 0);
		close(cSocket);
	}
	
	close(sSocket);
	
	return 0;
}

/*
 * This function takes in a NULL terminated ASCII, C string and scrambles it
 * using the popular ROT13 cipher. MODIFY AT YOUR OWN RISK
 *
 * @param str C-style string of maximum length 2^31 - 1
 */
static void scramble_string(char *str)
{
	int i;
	char t;
	for (i = 0; str[i]; i++) {
		t = str[i];
		if (t >= 'A' && t <= 'Z') {
			if ((t + ROT) <= 'Z') {
				str[i] += ROT;
			} else {
				str[i] -= ROT;
			}
		} else if (t >= 'a' && t <= 'z') {
			if ((t + ROT) <= 'z') {
				str[i] += ROT;
			} else {
				str[i] -= ROT;
			}
		}
	}
}
