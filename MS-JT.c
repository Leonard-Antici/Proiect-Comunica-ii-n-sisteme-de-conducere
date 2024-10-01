#include <c8051F040.h>	// declaratii SFR
#include <wdt.h>
#include <osc.h>
#include <port.h>
#include <uart0.h>
#include <uart1.h>
#include <lcd.h>
#include <keyb.h>
#include <Protocol.h>
#include <UserIO.h>

nod retea[NR_NODURI];					// retea cu 5 noduri

extern unsigned char STARE_NOD = 0;		// starea initiala a FSA COM
extern unsigned char TIP_NOD	= 0;		// tip nod initial: Slave sau No JET
unsigned char STARE_IO 	= 0;		// stare interfata IO - asteptare comenzi
unsigned char ADR_MASTER;				// adresa nod master - numai MS

extern unsigned char AFISARE;		// flag permitere afisare

//***********************************************************************************************************
void TxMesaj(unsigned char i);	// transmisie mesaj destinat nodului i
unsigned char RxMesaj(unsigned char i);		// primire mesaj de la nodul i

//***********************************************************************************************************
void main (void) {
	unsigned char i, found;	// variabile locale
	
	WDT_Disable();												// dezactiveaza WDT
	SYSCLK_Init();											// initializeaza si selecteaza oscilatorul ales in osc.h
	UART1_Init(NINE_BIT, BAUDRATE_COM);		// initilizare UART1 - port comunicatie (TxD la P1.0 si RxD la P1.1)
	UART1_TxRxEN (1,1);									// validare Tx si Rx UART1
	
 	PORT_Init ();													// conecteaza perifericele la pini (UART0, UART1) si stabileste tipul pinilor

	LCD_Init();    												// 2 linii, display ON, cursor OFF, pozitia initiala (0,0)
	KEYB_Init();													// initializare driver tastatura matriciala locala
	UART0_Init(EIGHT_BIT, BAUDRATE_IO);		// initializare UART0  - conectata la USB-UART (P0.0 si P0.1)

	Timer0_Init();  								// initializare Timer 0

 	EA = 1;                         			// valideaza intreruperile
 	SFRPAGE = LEGACY_PAGE;          			// selecteaza pagina 0 SFR
	
	for(i = 0; i < NR_NODURI; i++){		// initializare structuri de date
		retea[i].full = 0;						// initializeaza buffer gol pentru toate nodurile
		//retea[i].bufasc[0] = ":";				// pune primul caracter in buffer-ele binare ":"
	}

	Afisare_meniu();			   				// Afiseaza meniul de comenzi
	
 	while(1){												// bucla infinita
		
		switch(STARE_NOD){
			case 0:																// nodul este slave, asteapta mesaj de la master	
				switch(RxMesaj(ADR_NOD)){						// asteapta un mesaj de la master
					case TMO:	Error("\n\rNod SLAVE -> MASTER!");    // anunta ca nodul curent devine master
									TIP_NOD = 1;							// nodul curent devine master
									STARE_NOD = 2;						// master cazut, nodul curent devine master
									i = ADR_NOD;							// primul slave va fi cel care urmeaza dupa noul master
									break;

					case ROK:	Afisare_mesaj();
						STARE_NOD = 1; break;			// a primit un mesaj de la master, trebuie sa raspunda
					case POK:   STARE_NOD = 1; break;
					case CAN:	Error("\n\rMesaj incomplet!"); break;				// afiseaza eroare Mesaj incomplet
					case TIP:	Error("\n\rTip mesaj necunoscut!"); break;	// afiseaza eroare Tip mesaj necunoscut
					case ESC:	Error("\n\rEroare SC!");							// afiseaza Eroare SC
					Delay(1000);
					break;
					default:		Error("\n\rEroare necunoscuta!"); break;	// afiseaza cod eroare necunoscut, asteapta 1 sec
				}
				break;

			case 1:											// nodul este slave, transmite mesaj catre master
				found = 0;								// cauta sa gaseasca un mesaj util de transmis
				for(i = 0; i < NR_NODURI; i++){
					if(retea[i].full == 1){
							found = 1;
							break;
					}
				}
				if(found){ 																						// daca gaseste un nod i
					retea[i].bufbin.adresa_hw_dest = ADR_MASTER;				// pune adresa HW dest este ADR_MASTER
					TxMesaj(i);																					// transmite mesajul catre nodul i
				}
				else{																												// daca nu gaseste, construieste un mesaj in bufferul ADR_MASTER
					retea[ADR_MASTER].bufbin.adresa_hw_dest = ADR_MASTER;			// adresa HW dest este ADR_MASTER
					retea[ADR_MASTER].bufbin.adresa_hw_src = ADR_NOD;					// adresa HW src este ADR_NOD
					retea[ADR_MASTER].bufbin.tipmes = POLL_MES;								// tip mesaj = POLL_MES
					TxMesaj(ADR_MASTER);																			// transmite mesajul din bufferul ADR_MASTER
				}
				STARE_NOD = 0;				// revine sa astepte un mesaj de la master
				break;
	
			case 2:											// tratare stare 2 si eventual comutare stare -- nodul este master
				do{
					i++;												// selecteaza urmatorul slave
					if(i == NR_NODURI) i = 0;
				}
				while(i == ADR_NOD);
	
				retea[i].bufbin.adresa_hw_dest = i;				// adresa HW dest este i
				if(retea[i].full == 1) TxMesaj(i);				// daca in bufferul i se afla un mesaj util, il transmite
				else{																			// altfel, construieste un mesaj de interogare in bufferul i
					retea[i].bufbin.adresa_hw_src = ADR_NOD;			// adresa HW src este ADR_NOD
					retea[i].bufbin.tipmes = POLL_MES;						// tip mesaj = POLL_MES
					TxMesaj(i);																		// transmite mesajul din bufferul i
				}
				STARE_NOD = 3;				// trece in starea 3, sa astepte raspunsul de la slave-ul i
				break;

			case 3:											// nodul este master, asteapta mesaj de la slave	
				switch(RxMesaj(i)){				// asteapta un raspuns de la slave i
					case TMO:	Error("\n\rTimeout nod ");					// afiseaza eroare Timeout nod i
								if(AFISARE){	
									UART0_Putch(i+'0');
									LCD_Putch(i + '0');
								}
								break;
					case ROK:	Afisare_mesaj(); 				// a primit un mesaj de la master, trebuie sa raspunda
					case POK:   break;
					case ERI:	Error("\n\rEroare incadrare!"); break;			// afiseaza Eroare incadrare
					case ERA:	Error("\n\rEroare adresa!"); break;					// afiseaza Eroare adresa
					case TIP:	Error("\n\rTip mesaj necunoscut!"); break;	// afiseaza Tip mesaj necunoscut
					case OVR:	Error("\n\rEroare suprapunere!"); break;		// afiseaza Eroare suprapunere
					case ESC:	Error("\n\rEroare SC!"); break;							// afiseaza Eroare SC
					default:		Error("\n\rEroare necunoscuta!"); break;	// afiseaza Eroare necunoscuta, apoi asteapta 1000ms
				}
				STARE_NOD = 2;				// (revine in starea 2) a primit sau nu un raspuns de la slave, trece la urmatorul slave
				break;
		}
		UserIO();
	}
}