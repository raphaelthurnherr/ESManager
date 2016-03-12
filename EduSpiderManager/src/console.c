/*******************************************************************************
 *	Librairie pour communication protocol ALGOID
 *	N�c�ssite les source PAHO MQTT.
 *	MAJ: 04.01.2016 / Raphael Thurnherr
 *
 *
 *******************************************************************************/

#define TASKNAME "[CONSOLE] "

#include "console.h"
#include "ExternCom.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "lib_mqtt/MQTTClient.h"
#include "lib_crc.h"

pthread_t th_console;

// ------------------------------------------------------------------------------------
// MAIN: Point d'entreee programme, initialisation connexion MQTT, souscription et publication
// ------------------------------------------------------------------------------------

void *consoleTask (void * arg){
	printf ("# Demarrage tache console: OK\n");

// BOUCLE PRINCIPALE
	    while(!EndOfApp)
	    {
			int ch;

			ch = getchar();

			// TRAITEMENT DU PREMIER MESSAGE EN PILE
			if(ch=='d'){
				if(algo_getMessage(algoidMsgRX, algoidMsgRXStack)){
					printf("\nOK\n");
					diplay_algoMessage(algoidMsgRXStack[0]);
					diplay_algoMessage(algoidMsgRX);
				}

				else printf("\nPAS DE MESSAGE\n");
			}

			// AFFICHE L'ETAT DE LA PILE RX
			if(ch=='r') {
		    	printf("\n ---------------------ETAT DE LA PILE DE MESSAGE ALGOID RX ----------------------");
				diplay_algoStack(algoidMsgRXStack);
			}

			// AFFICHE L'ETAT DE LA PILE TX
			if(ch=='t'){
		    	printf("\n ---------------------ETAT DE LA PILE DE MESSAGE ALGOID TX ----------------------");
		    	diplay_algoStack(algoidMsgTXStack);
			}

			// PUBLIE UN MESSAGE DE TEST
			if(ch=='p'){
				messageTest();
				if(algo_setMessage(algoidMsgTX, algoidMsgTXStack)<0)
					printf("\n ERREUR: Impossible de transmettre le message, pile pleine");
			}

			// PUBLIE UN MESSAGE DE TEST
			if(ch=='o'){
				messageTest2();
				if(algo_setMessage(algoidMsgTX, algoidMsgTXStack)<0)
					printf("\n ERREUR: Impossible de transmettre le message, pile pleine");
			}

			if(ch=='n'){
				messageNegoc(NEGOC_ADD_RX_CHANNEL, "MQ2ES12");
				algo_setMessage(algoidMsgTX, algoidMsgTXStack);
			}

			if(ch=='m'){
				messageNegoc(NEGOC_REM_RX_CHANNEL, "MQ2ES12");
				algo_setMessage(algoidMsgTX, algoidMsgTXStack);
			}

			// FERME L'APPLICATION
			if(ch=='q')
				EndOfApp=1;

			if(ch=='g'){
				system ("shutdown -h 0");
				EndOfApp=1;
			}

	    }
 // FIN BOUUCLE PRINCIPAL
	    return 0;
  usleep(5000);

  printf( "# ARRET tache ALGOID\n");

  usleep(10000);
  pthread_exit (0);
}

	// ---------------------------------------------------------------------------
	// CREATION THREAD UART
	// ---------------------------------------------------------------------------
	int createConsoleTask(void)
	{
		return (pthread_create (&th_console, NULL, consoleTask, NULL));
	}

	// ---------------------------------------------------------------------------
	// DESTRUCTION THREAD UART
	// ---------------------------------------------------------------------------
	int killConsoleTask(void){
		return (pthread_cancel(&th_console));
	}
