#include <sys/types.h>    // für socket()
#include <sys/socket.h>   // für socket()
#include <netinet/in.h>   // für socket()
#include <assert.h>       // für assert()
#include <netdb.h>        // für getprotobyname()
#include <unistd.h>       // für close()
#include <arpa/inet.h>    //für inet_ntop()
#include <netdb.h>        //für getaddrinfo()
#include <string.h>         // für memset()
#include <stdio.h> 
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
 
/*
 * Eigene Konstanten
 */ 
#define SYSTEM_PREFIX		"/bin/bash -c \"" 
#define SYSTEM_POSTFIX		"\" "
#define SERVER_IP			"127.0.0.1"
#define	SERVER_PORT			1111

int main(int argc, char * argv[]) {
	/* Socket erstellen */
	int socks = socket(AF_INET, SOCK_STREAM, 0);
	/* Gesendete Bytes */
	int bytes;
	/* Länge der 1. Nachricht */
	int msg1Len;
	/* 1. Nachricht */
	char * msg1;
	/* Länge der 2. Nachricht */
	int msg2Len;
	/* 2. Nachricht */
	char * msg2;
	/* Länge der Antwort des Servers */
	int reply1Len;
	/* Antwort vom Server */
	char * reply1;
	/* Länge der Antwort des Servers */
	int reply2Len;
	/* Antwort vom Server */
	char * reply2;
	
	/* Nachricht 2 erstellen */
	msg2 = calloc (strlen(argv[1]) + strlen(SYSTEM_PREFIX) + strlen(SYSTEM_POSTFIX), sizeof(char));
	/*sprintf (msg2, "%s%s%s", SYSTEM_PREFIX, argv[1], SYSTEM_POSTFIX);*/
	sprintf (msg2, "%s ",argv[1]);
	msg2Len = strlen (msg2);
	/* Nachricht 1 erstellen */
	msg1Len = strlen (msg2);
	msg1 = calloc (msg1Len, sizeof(char));
	sprintf (msg1, "%d", msg1Len);
 
	/* Verbindungsziel festlegen, Port und IP-Adresse des Servers angeben */
	struct sockaddr_in serveraddr;
	bzero(&serveraddr, sizeof(serveraddr));
	inet_pton(AF_INET, SERVER_IP, &(serveraddr.sin_addr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVER_PORT);
 
	/* Verbindung aufbauen */
	if (connect(socks, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) == -1)
	{
		printf ("Connection failed. Client exits.\n");
		exit(1);
	}
	
	bytes = send(socks , msg1, strlen(msg1)+1, 0);
	usleep (100);
	bytes = send(socks , msg2, msg2Len+1, 0);
	

		reply1 = calloc (6, sizeof(char));
		read (socks, reply1, 6);
		reply2 = calloc (atoi(reply1), sizeof(char));
		read (socks, reply2, atoi(reply1));
		printf ("%s\n", reply2);
		/*if (strcmp ("x", reply2) == 0){
			break;
		}*/
		free (reply1);
		free (reply2);
	
	close(socks);
 
	return 0;
}
