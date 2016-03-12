
#define TASKNAME "[MAIN] "
#define UDPPORT 53530  //The port on which to send data
#define UDPBUFLEN 512	// Size of UDP Buffer
#define CLIENID_PREFIX "ESMGR_"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

// MAC ADRESS
#include <net/if.h>   //ifreq
#include <sys/ioctl.h>

//UDP
#include<arpa/inet.h>

#include "ExternCom.h"
#include "console.h"
#include "mainapp.h"

unsigned char EndOfApp;
ALGOID algoidNegociationRX;


struct sockaddr_in si_other;
static int s, slen=sizeof(si_other);
char buf[UDPBUFLEN];
char message[UDPBUFLEN];
int InitMultiTasking (void);

char searchNegocMsg(void);

void messageTest(void);
void messageTest2(void);
void messageNegoc(unsigned char cmd, char *str);
void diplay_algoMessage(ALGOID msgStack);

// Variable id^^ID du manager
char myID[50]="ESMGR_";

// Traitement de la commande de négociation avec le manager
char processNegociation(ALGOID message);

// Get MAC Adresse function
char* getMACaddr(void);

// Initialisation UDP
void initUDP();

//
void sendUDPHeartBit(void);

// ***************************************************************************
// ---------------------------------------------------------------------------
// STARTUP
// ---------------------------------------------------------------------------
// ***************************************************************************

int main(void) {
	int counter15s=0;

	system("clear");
	printf("\nESMANAGER - 15/02/2016\n");
	printf("------------------------------------------------\n\n");

	printf ("# Demarrage du gestionnaire des taches...\n");

	// DEMARRAGE DES TACHES
	InitMultiTasking();

	// Creation du ID manager compose de l'adresse mac
		char* myMACaddr;
		myMACaddr=getMACaddr();
		sprintf(&myID[6], "%02x%02x%02x%02x%02x%02x",myMACaddr[0], myMACaddr[1], myMACaddr[2], myMACaddr[3], myMACaddr[4],myMACaddr[5]);
	    printf("%sSet MAC addresse as ID : %s\n",TASKNAME, myID);

	// Préparation Connexion  UDP brodacast
    initUDP();

    // Préparation Connexion ALgoid sur Brocker local MQTT
   algoComInit(myID);

	// ---------------------------------------------------------------------------
	// MAIN LOOP
	// ---------------------------------------------------------------------------
	while(!EndOfApp){

		// RECHERCHE ET TRAITE D'EVENTUELS MESSAGE DE NEGOCIATION ALGOID
		if(searchNegocMsg())
			processNegociation(algoidNegociationRX);



		// Envoie periodiquement un Heartbit UDP
		if(counter15s++>=150){
			sendUDPHeartBit();
			counter15s=0;
		}
        /*
        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            die("recvfrom()");
        }
        puts(buf);
        */

        usleep(100000);
	}
 /*Ralentit le thread d'ecriture... */

	close(s);
	usleep(10000);
		printf("\n# Fin d' application, bye bye !\n");
		return (0);
}

// ---------------------------------------------------------------------------
// RECHERCHE DE MESSAGE DE NEGOCIATION DANS LA PILE DE RECEPTION
// ---------------------------------------------------------------------------
char searchNegocMsg(void){
	char j, prtFindNegocMsg=0;
	char result=0;

	// Recherche d'éventuels message de negociation recus sur topic "ESMGR"
	for(prtFindNegocMsg=0;(prtFindNegocMsg<RXTXSTACK_SIZE); prtFindNegocMsg++){
		if((algoidMsgRXStack[prtFindNegocMsg].msg_type==T_NEGOC) && (!strcmp(algoidMsgRXStack[prtFindNegocMsg].topic, TOPIC_MGR))){

			// Récupère le message de la pile
			algoidNegociationRX=algoidMsgRXStack[prtFindNegocMsg];

			// Verification que le message est bien destiné à ce spider ([0] = destinataire du message, [1]= Expediteur)
			if(!strcmp(algoidNegociationRX.msg_string_array[0], myID)){
				result =1;
			}
			else
			{
				printf("\n MESSAGE NEGOCIATION NON TRAITE, MAUVAIS DESTINATAIRE \n");
				result =0;
			}

			// Liberation de l'espace dans la pile
			for(j=prtFindNegocMsg;j<RXTXSTACK_SIZE-1;j++)
				algoidMsgRXStack[j]=algoidMsgRXStack[j+1];

		}
	}
	return result;
}

// ---------------------------------------------------------------------------
// TRAITEMENT DU MESSAGE DE NEGOCIATION AVEC LE MANAGER
// ---------------------------------------------------------------------------
char processNegociation(ALGOID message){

	printf("\n%sMessage negociation en traitement \n", TASKNAME);
	switch(message.msg_type_value){
		case NEGOC_ES_RENAME : printf("\nrename ES\n"); break;
		case NEGOC_MGR_JOIN : printf("\njoin group\n"); break;
		case NEGOC_MGR_LEAVE : printf("\nleave group\n"); break;
		case NEGOC_TX_CHANNEL : algoSetTXChannel(message.msg_string_array[1]);
									break;
		case NEGOC_ADD_RX_CHANNEL : algoAddRXChannel(message.msg_string_array[1]);
									printf("\n - Ajout canal d écoute sur topic: %s", message.msg_string_array[1]);
									break;
		case NEGOC_REM_RX_CHANNEL : if(strcmp(message.msg_string_array[1], TOPIC_MGR)){			 // Ne desincrit pas le topic manager
										algoRemoveRXChannel(message.msg_string_array[1]);
										printf("\n - Suppression canal d'écoute: %s", message.msg_string_array[1]);
									}else printf(" - Suppression canal d'écoute %s impossible", message.msg_string_array[1]);
									break;
		default : break;
	}

	return 0;
}

// ***************************************************************************
// ---------------------------------------------------------------------------
// INIT MULTITASKING
// ---------------------------------------------------------------------------
// ***************************************************************************

int InitMultiTasking (void)
{
  //void *ret;
	// CREATION TACHE HWCTRL
	  if (createAlgoidTask()< 0) {
		printf ("# Creation tache console : ERREUR\n");
		exit (1);
	  }

	// CREATION TACHE HWCTRL
	  if (createConsoleTask()< 0) {
		printf ("# Creation tache console : ERREUR\n");
		exit (1);
	  }
/*
(void)pthread_join (th_hwctrl, &ret);		//
*/

return(1);
}


// ---------------------------------------------------------------------------
// MESSAGE DE TEST MQTT -> DEBUG
// ---------------------------------------------------------------------------
void messageTest2(void){
	 // TEST - CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID
				strcpy(algoidMsgTX.topic, "MQ2ES12");

				algoidMsgTX.msg_type=T_ERROR;
				algoidMsgTX.msg_type_value=0x03;

				algoidMsgTX.msg_param[0]=PS_1;
				algoidMsgTX.msg_param_value[0]=((rand()<<16)+rand())&0x00FFFFFF;
				algoidMsgTX.msg_param[1]=0;
	// TEST - FIN CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID

}





// ---------------------------------------------------------------------------
// MESSAGE DE TEST MQTT -> DEBUG
// ---------------------------------------------------------------------------
void messageTest(void){
unsigned char i;
	 // TEST - CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID
				strcpy(algoidMsgTX.topic, "MQ2ES12");

				//algoidMsgTX.msg_id = rand()&0x00FFFFFF;
				//algoidMsgTX.msg_id |= HOSTSENDERID<<24;

				algoidMsgTX.msg_type=T_ERROR;
				algoidMsgTX.msg_type_value=0x03;

				algoidMsgTX.msg_param[1]=PSA_1;
				algoidMsgTX.msg_param_count[1]=rand()&0x000000F;
				for(i=0;i<algoidMsgTX.msg_param_count[1];i++)
					algoidMsgTX.msg_param_array[1][i]=rand()&0x0000FFFF;

				algoidMsgTX.msg_param[0]=PS_1;
				algoidMsgTX.msg_param_value[0]=((rand()<<16)+rand())&0x00FFFFFF;


				algoidMsgTX.msg_param[2]=PS_1;
					algoidMsgTX.msg_param_value[2]=((rand()<<16)+rand())&0x00FFFFFF;

				algoidMsgTX.msg_param[3]=PCA_1;
				strcpy(algoidMsgTX.msg_string_array[3], "BONJOUR L'AMI");
				algoidMsgTX.msg_param_count[3]=strlen(algoidMsgTX.msg_string_array[3]);

				algoidMsgTX.msg_param[4]=PSA_1;
				algoidMsgTX.msg_param_count[4]=rand()&0x000000F;
				for(i=0;i<algoidMsgTX.msg_param_count[4];i++)
				algoidMsgTX.msg_param_array[4][i]=rand()&0x0000FFFF;

				algoidMsgTX.msg_param[5]=PCA_1;
				strcpy(algoidMsgTX.msg_string_array[5], "SALUT COCOLET, J'ESP�RE QUE TU VAS BIEN ALLER");
				algoidMsgTX.msg_param_count[5]=strlen(algoidMsgTX.msg_string_array[5]);
				algoidMsgTX.msg_param[6]=0;
	// TEST - FIN CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID

}


// ---------------------------------------------------------------------------
// MESSAGE DE TEST DE NEGOCIATION
// ---------------------------------------------------------------------------
void messageNegoc(unsigned char cmd, char *str){
	 // TEST - CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID
				// Destinataire
				strcpy(algoidMsgTX.topic, TOPIC_MGR);

				algoidMsgTX.msg_id = rand()&0xFFFFFFFF;

				algoidMsgTX.msg_type=T_NEGOC;
				algoidMsgTX.msg_type_value=cmd;

				// Destinataire
				algoidMsgTX.msg_param[0]=PCA_1;
				strcpy(algoidMsgTX.msg_string_array[0], "ES12");
				algoidMsgTX.msg_param_count[0]=strlen(algoidMsgTX.msg_string_array[0]);

				// Topic d'ecoute a ajouter
				algoidMsgTX.msg_param[1]=PCA_1;
				strcpy(algoidMsgTX.msg_string_array[1], str);
				algoidMsgTX.msg_param_count[1]=strlen(algoidMsgTX.msg_string_array[1]);

				algoidMsgTX.msg_param[2]=0;

				//algoidMsgTX.msg_param[2]=0;

	// TEST - FIN CHARGEMENT DE LA STRUCTURE DU MESSAGE ALGOID

}

// -------------------------------------------------------------------
	// DISPLAY_ALGOMESAGE, affichage des data du message
	// -------------------------------------------------------------------
		void diplay_algoMessage(ALGOID msgStack){
			unsigned char j;
			unsigned char ptrParam;

			printf("\n#MESSAGE:     Algo Topic: %s, message ID: %d, command: %d, cmd value: %d", msgStack.topic, msgStack.msg_id, msgStack.msg_type,
										msgStack.msg_type_value);
		for(ptrParam=0;msgStack.msg_param[ptrParam]!=0;ptrParam++){
		//		for(ptrParam=0;ptrParam<10;ptrParam++){

			 if(msgStack.msg_param[ptrParam]==PS_1)
				 printf("\n        value: %d",msgStack.msg_param_value[ptrParam]);

			 if(msgStack.msg_param[ptrParam]==PSA_1){
				 printf("\n        array: ");
				 for(j=0;j<msgStack.msg_param_count[ptrParam];j++) printf("%d ", msgStack.msg_param_array[ptrParam][j]);
			 }
			 if(msgStack.msg_param[ptrParam]==PCA_1){
				 printf("\n        string: ");
				 printf("%s ", msgStack.msg_string_array[ptrParam]);
			 }
			}
		    	printf("\n");
		    printf("\n");
		}

		// -------------------------------------------------------------------
		//GETMACADDR, r�cuperation de l'adresse MAC sur ETH0
		// -------------------------------------------------------------------
		char* getMACaddr(void){
		    int fd;
		    struct ifreq ifr;
		    char *iface = "eth0";
		    unsigned char *mac;

		    fd = socket(AF_INET, SOCK_DGRAM, 0);

		    ifr.ifr_addr.sa_family = AF_INET;
		    strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);

		    ioctl(fd, SIOCGIFHWADDR, &ifr);

		    close(fd);

		    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

		    char adrMac[15];

		    return(mac);
		}

		// -------------------------------------------------------------------
		// INITUDP, Configuration pour envoie de data broadcast sur UDP
		// -------------------------------------------------------------------
		void initUDP(){
			if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
			    {
			       // die("socket");
			    }else{
				    // Activation Broadcast
				    int broadcastEnable=1;
				    int ret=setsockopt(s, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));
				     // Fin activation broadcast
			    }

			    memset((char *) &si_other, 0, sizeof(si_other));
			    si_other.sin_family = AF_INET;
			    si_other.sin_port = htons(UDPPORT);

			    /*
			    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
			    {
			        fprintf(stderr, "inet_aton() failed\n");
			        exit(1);
			    }
			    */
			    si_other.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		}

		// -------------------------------------------------------------------
		// SEND UDP HEARTBIT, Publie sur UDP le nom de serveur
		// -------------------------------------------------------------------
		void sendUDPHeartBit(void){
			memset(message,'\0', UDPBUFLEN);
			sprintf(message, "|%s| test", myID);

	        if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1){
	            die("sendto()");
	        }
		}


		void die(char *s)
		{
		    perror(s);
		}
