#include <sys/types.h>    // für socket()
#include <sys/socket.h>   // für socket()
#include <netinet/in.h>   // für socket()
#include <assert.h>       // für assert()
#include <stdlib.h>       //für calloc()
#include <netdb.h>        // für getprotobyname()
#include <unistd.h>       // für close()
#include <arpa/inet.h>    //für inet_ntop()
#include <netinet/in.h>   //für INET_ADDRSTRLEN
#include <string.h>       // für memset()
#include <stdio.h>

/*
 * Eigene Konstanten
 */
#define PORT				1111
#define	OFFSET				48

/*
 * Eigene Typen
 */

/**
 * get: der Server tut nichts und schickt eine Nachricht mit einem 
 * 		Zustand zurück.
 * simple: Der Server startet einen Prozess auf dem Host und beendet die
 * 		Pipeline. (In den Prozess kann nicht weiter eingegriffen werden.)
 * interactive: Der Server führt einen Befehl auf dem Hostrechner aus
 * 		und schickt eine Nachricht mit dem Output des Befehls zurück.
 * extended_input: Der Server führt einen Befehl aus und hält die Pipeline
 * 		auch nach dem Beenden des Clients für weiteren Input offen.
 * 		(Z.B. Befehle dem Programm über StdIn geben.)
 */
enum e_Command {get, simple, interactive, extended_input};
typedef enum e_Command CmdMode;

/*
 * Globale Variablen
 */

/** 
 * 0 = Befehle werden alle an das System weitergegeben (neuer Prozess, der beendet 
 * 		wird nach Ausführung.
 * 1 = Befehl wird ans System weitergegeben, alle weiteren (bis zum Moduswechsel)
 * 		werden in die gleiche Shell geleitet.
 *  */
CmdMode g_mode = simple;
FILE * g_stream = NULL;

/**
 * Da der letzte Buchstabe immer ein Leerzeichen ist, löschen wir dieses.
 */
char * deleteLastCharacter (char * msg)
{
	char * res = calloc (strlen(msg) - 1, sizeof(char));
	memcpy (res, msg, strlen(msg) - 1);
	free(msg);
	return res;
}

void respondToClient (char * msg, int csd)
{
	char * msg1 = calloc (6, sizeof(char));
	sprintf (msg1, "%ld", strlen(msg));
	write (csd, msg1, strlen(msg1)+1);
	usleep(100);
	write (csd, msg, strlen(msg)+1);
}

void closeStream (void)
{
	if (g_stream) {
		pclose (g_stream);
		g_stream = NULL;
	}
}

/**
 * Parsed die hereinkommenden Befehle und führt diese aus.
 */
void parseCommands (char * msg, int csd)
{
	msg = deleteLastCharacter (msg);
	
	if (strlen(msg) == 1 && msg[0] == 'x') {
		closeStream();
		respondToClient ("Server shutdown. Bye.", csd);
		exit(0);
	}
	else
	{
		/* Wird ein neuer Modus zugewiesen ? */
		int command = (strcmp ("mode = ", strndup (msg, 7)) == 0);
		
		/* Ein Command kommt rein! */
		if (command) {
			
			int cmd = msg[7] - OFFSET;
			
			switch (cmd) {
				case get:
					g_mode = get;
					respondToClient ("Mode set to 'get'", csd);
					break;
				case simple:
					g_mode = simple;
					respondToClient ("Mode set to 'simple'", csd);
					break;
				case interactive:
					g_mode = interactive;
					respondToClient ("Mode set to 'interactive'", csd);
					break;
				case extended_input:
					g_mode = extended_input;
					respondToClient ("Mode set to 'extended_input'", csd);
					break;
				default:
					respondToClient ("Mode not recognized.", csd);
			}
			
		} else {
			switch (g_mode) {
				case get:
					{
						/* Egal, was reinkommt, der Server gibt den Modus an. */
						char * msg = calloc (strlen("Mode == ") + 2, sizeof(char));
						sprintf (msg, "%s%d", "Mode == ", g_mode);
						respondToClient (msg, csd);	
						usleep(200);
						respondToClient ("x", csd);
					}		
					break;
				case simple:
					/* Server leitet den Befehlt auf das System um. */
					closeStream();
					g_stream = popen (msg, "w");
					respondToClient ("x", csd);
					closeStream();
					break;
				case interactive:
					/* Server leitet Befehle ans System weiter und schickt eine Nachricht 
					 * an den Client mit der Antwort des Prozesses */
					{
						char * line = malloc (101 * sizeof(char));
						char * respond;
						int read;
						closeStream ();
						g_stream = popen (msg, "r");
						printf ("....\n");
						while ((line = fgets (line, 100, g_stream))) {
							printf ("%s\n", line);
							respond = realloc (respond, (strlen(respond)+100)*sizeof(char));
							respond = strcat (respond, line);
						}
						
						respondToClient(respond, csd);
					}	
					break;
				case extended_input:
					/* Server lässt den Stream offen, um den Prozess zu kontrollieren. */
					if (!g_stream) {
						g_stream = popen (msg, "w");
					}
					fputs (msg, g_stream);
					fputs ("\n", g_stream);
					fflush(g_stream);
					break;
				default:
					respondToClient ("Command not recognized.", csd);
			}
		
		}
	}	
	/*printf ("stream condition: %d \n", g_stream != NULL);*/
	
}

/**
 * Hauptprogramm des Servers. 
 */
int main(void) {
	/* Socket erstellen - TCP, IPv4, keine Optionen */
	int lsd = socket(AF_INET, SOCK_STREAM, 0);
	int bytes;
	int csd;
	
	/* 2 berücksichtigte Nachrichten */
	int msgLen;
 
	/* IPv4, Port: 1111, jede IP-Adresse akzeptieren */
	struct sockaddr_in saddr;
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT);
	saddr.sin_addr.s_addr = htons(INADDR_ANY);
 
	/* Socket an Port binden */
	bind(lsd, (struct sockaddr*) &saddr, sizeof(saddr));
 
	/* Auf Socket horchen (Listen) */
	listen(lsd, 10);
 
	while(1) {
		/* Puffer und Strukturen anlegen */
		struct sockaddr_in clientaddr;
		char * msg1 = calloc (6, sizeof(char));
		
		int msgCnt = 0;
 
		/* Auf Verbindung warten, bei Verbindung Connected-Socket erstellen */
		socklen_t clen = sizeof(clientaddr);
		csd = accept(lsd, (struct sockaddr*) &clientaddr, &clen);
		
		/*printf ("server prepared.\n");*/
		
		/* Vom Client lesen und ausgeben */
		bytes = recv(csd, msg1, 6, 0);
		/*printf ("%d bytes received. msg = \"%s\"\n", bytes, msg1);	*/
		
		char * msg2 = calloc (atoi (msg1), sizeof(char));
		
		bytes = recv(csd, msg2, atoi (msg1), 0);
		/*printf ("%d bytes received. msg = \"%s\"\n", bytes, msg2);	*/
		
		parseCommands (msg2, csd);
 
		/* Verbindung schließen */
		close(csd);
		msgCnt++;
	}
	
	close(lsd);
	
	return EXIT_SUCCESS;
 
}
