#include <c8051F040.h>	// declaratii SFR
#include <uart1.h>
#include <Protocol.h>
#include <UserIO.h>

extern nod retea[];						// reteaua Master-Slave, cu 5 noduri

extern unsigned char TIP_NOD;			// tip nod
extern unsigned char ADR_MASTER;	// adresa nodului master

extern unsigned char timeout;		// variabila globala care indica expirare timp de asteptare eveniment
//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i);				// primire mesaj de la nodul i

//***********************************************************************************************************
unsigned char RxMesaj(unsigned char i){					// receptie mesaj															   
unsigned char j, ch, sc, adresa_hw_src, screc, src, dest, lng, tipmes;

UART1_TxRxEN(0,1);										// dezactivare Tx, validare RX UART1
UART1_RS485_XCVR(0,1);																			// dezactivare Tx, validare RX RS485
UART1_MultiprocMode(MULTIPROC_ADRESA);																			// receptie doar octeti de adresa


if(TIP_NOD==MASTER){																		// Daca nodul este master sau detine jetonul ...
	ch=UART1_Getch_TMO(WAIT);																			// M: asteapta cu timeout raspunsul de la slave
	if(timeout == 1)																	// M: timeout, terminare receptie,  devine master sau regenerare jeton
	return TMO;
	retea[i].full=0;																	// M: raspunsul de la slave a venit, considera ca mesajul anterior a fost transmis cu succes	
	if(ch != ADR_NOD)																	// M: adresa HW gresita, terminare receptie
		return ERA;													
}															
else{
	do{				
		ch = UART1_Getch_TMO(WAIT + ADR_NOD*WAIT);			// // S: asteapta inceput mesaj nou
		if(timeout == 1)																// slave-ul nu raspunde
			return TMO;
	}
	while(ch!= ADR_NOD)	;															// S: iese doar atunci cand mesajul ii este adresat acestui nod	
}
UART1_MultiprocMode(MULTIPROC_DATA);																// receptie octeti de date (mod MULTIPROC_DATA)

screc = ADR_NOD;																// M+S: initializeaza screc cu adresa HW dest (ADR_NOD)

adresa_hw_src = UART1_Getch_TMO(5);						// M+S: Asteapta cu timeout receptia adresei HW sursa
if(timeout == 1)															// mesaj incomplet
	return CAN;
screc ^= adresa_hw_src;															// M+S: ia in calcul in screc adresa hw src
if(TIP_NOD == SLAVE)
	ADR_MASTER = adresa_hw_src;														// actualizeaza adresa master
tipmes = UART1_Getch_TMO(5);
if(timeout == 1)															// M+S: Asteapta cu timeout receptie tip mesaj
	return CAN;														// M+S: mesaj incomplet
if(tipmes > 1)
	return TIP;														
																// M+S: ignora restul mesajului
																
																// M+S: tip mesaj eronat, terminare receptie
															
screc ^= tipmes;															// M+S: ia in calcul in screc tipul mesajului

if(tipmes != USER_MES){															// M+S: Daca mesajul este unul de date (USER_MES)
	retea[ADR_NOD].bufbin.adresa_hw_src = adresa_hw_src;															// M+S: asteapta cu timeout adresa nodului sursa
																
	sc = UART1_Getch_TMO(5);														// M+S: ia in calcul in screc adresa src

	if(timeout == 1)															// M+S: asteapta cu timeout adresa nodului destinatie
		return CAN;	

	if(sc==screc)
		return POK;
	else
		return ESC;
}
else{
	src = UART1_Getch_TMO(5);
	if(timeout == 1)														
		return CAN;

	screc ^= src;
//////////////////////////
		dest = UART1_Getch_TMO(5);
		if(timeout == 1)
			return CAN;
		screc ^= dest;
																// M+S: ia in calcul in screc adresa dest

	if(TIP_NOD == MASTER)															// Daca nodul este master...
		if(retea[dest].full == 1)
				return OVR;															// M: bufferul destinatie este deja plin, terminare receptie
																

		lng = UART1_Getch_TMO(5);														// M+S: asteapta cu timeout receptia lng
		if(timeout == 1)
			return CAN;
		screc ^= lng;									// M+S: ia in calcul in screc lungimea datelor

		if(TIP_NOD == MASTER){														// Daca nodul este master...
		retea[dest].bufbin.adresa_hw_src = adresa_hw_src;															// M: stocheaza adresa HW sursa	(ADR_NOD)
		retea[dest].bufbin.tipmes = tipmes;															// M: stocheaza tipul mesajului	
		retea[dest].bufbin.src = src;															// M: stocheaza la src codul nodului sursa al mesajului	
		retea[dest].bufbin.dest = dest;															// M: stocheaza la dest codul nodului destinatie a mesajului	
		retea[dest].bufbin.lng = lng;															// M: stocheaza lng
																	
		for(j=0; j<retea[dest].bufbin.lng; j++){
				retea[dest].bufbin.date[j] = UART1_Getch_TMO(5);
				if(timeout == 1)
					return CAN;																							// M: asteapta cu timeout un octet de date
				retea[dest].bufbin.sc ^= retea[dest].bufbin.date[j];
			}														
																																	// M: ia in calcul in screc datele
																		
																	
			if(timeout == 1)
				return CAN;														// M: Asteapta cu timeout receptia sumei de control
																	

																	
			if(sc == screc){																// M: mesaj corect, marcare buffer plin
				retea[dest].full =1;
				return ROK;
				}															
			else 
				return ESC;															
		}														// M: eroare SC, terminare receptie
																											
		else{														// nodul este slave
			retea[ADR_NOD].bufbin.src = src;														// S: stocheaza la destsrc codul nodului sursa al mesajului	
			retea[ADR_NOD].bufbin.lng = lng;														// S: stocheaza lng
																	
		for(j=0; j<retea[ADR_NOD].bufbin.lng; j++){																// S: asteapta cu timeout un octet de date
			retea[ADR_NOD].bufbin.date[j] = UART1_Getch_TMO(5);
				if(timeout == 1) return CAN;															
					retea[ADR_NOD].bufbin.sc ^= retea[ADR_NOD].bufbin.date[j];																				// S: ia in calcul in screc octetul de date
		}															
																	
		sc = UART1_Getch_TMO(5);															// S: Asteapta cu timeout receptia sumei de control
		if(timeout == 1)
				return CAN;															

		if(sc == screc){															
			retea[ADR_NOD].full = 1;
				return ROK;
			}															// S: mesaj corect, marcare buffer plin
																		
														
		else
				return ESC;								// S: eroare SC, terminare receptie
		}	
		//break;

}														
																// este un mesaj POLL_MES sau JET_MES
																	// M+S:memoreaza de la cine a primit jetonul
																	// M+S: Asteapta cu timeout receptia sumei de control
																	
																	
																	// M+S: eroare SC, terminare receptie
//return TMO; 		// simuleaza timeout receptie																

}


//***********************************************************************************************************
unsigned char ascii2bin(unsigned char *ptr){			// converteste doua caractere ASCII HEX de la adresa ptr
unsigned char bin;

if(*ptr > '9') bin = (*ptr++ - 'A' + 10) << 4;	// contributia primului caracter daca este litera
else bin = (*ptr++ - '0') << 4;									// contributia primului caracter daca este cifra
if(*ptr > '9') bin  += (*ptr++ - 'A' + 10);			// contributia celui de-al doilea caracter daca este litera
else bin += (*ptr++ - '0');											// contributia celui de-al doilea caracter daca este cifra
return bin;
}


