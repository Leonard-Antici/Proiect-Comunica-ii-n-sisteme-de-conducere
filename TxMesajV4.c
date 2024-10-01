 #include <c8051F040.h> // declaratii SFR

 #include <uart1.h>

 #include <Protocol.h>

 #include <UserIO.h>

 extern unsigned char STARE_NOD; // starea initiala a nodului curent
 extern unsigned char TIP_NOD; // tip nod initial: Nu Master, Nu Jeton

 extern nod retea[];

 extern unsigned char timeout; // variabila globala care indica expirare timp de asteptare eveniment
 //***********************************************************************************************************
 void TxMesaj(unsigned char i); // transmisie mesaj destinat nodului i
 void bin2ascii(unsigned char ch, unsigned char * ptr); // functie de conversie octet din binar in ASCII HEX

 //***********************************************************************************************************
 void TxMesaj(unsigned char i) { // transmite mesajul din buffer-ul i
   unsigned char sc, j;

   if (retea[i].bufbin.tipmes == POLL_MES) // daca este un mesaj de interogare (POLL=0)
   {
     sc = retea[i].bufbin.adresa_hw_dest ^ retea[i].bufbin.adresa_hw_src; // initializeaza SC cu adresa HW a nodului destinatie
     retea[i].bufbin.sc = sc; // calculeaza direct sc // altfel...
   } else {
     sc = retea[i].bufbin.adresa_hw_dest; // initializeaza SC cu adresa HW a nodului destinatie
     sc ^= retea[i].bufbin.adresa_hw_src; // ia in adresa_hw_src
     sc ^= retea[i].bufbin.tipmes; // ia in calcul tipul mesajului
     sc ^= retea[i].bufbin.src; // ia in calcul adresa nodului sursa al mesajului
     sc ^= retea[i].bufbin.dest; // ia in calcul adresa nodului destinatie al mesajului
     sc ^= retea[i].bufbin.lng; // ia in calcul lungimea datelor
     for (j = 0; j < retea[i].bufbin.lng; j++) {
       sc ^= retea[i].bufbin.date[j]; // ia in calcul datele
     }

     retea[i].bufbin.sc = sc; // stocheaza suma de control
   }

   UART1_MultiprocMode(MULTIPROC_ADRESA); // urmeaza transmisia octetului de adresa
   UART1_TxRxEN(1, 1);
   UART1_RS485_XCVR(1, 1); // validare Tx si Rx RS485

	UART1_Putch(retea[i].bufbin.adresa_hw_dest);// transmite adresa HW dest
  

   if (UART1_Getch_TMO(2) != retea[i].bufbin.adresa_hw_dest || timeout) // daca caracterul primit e diferit de cel transmis ...
   {
     UART1_TxRxEN(0, 0);
     UART1_RS485_XCVR(0, 0); // dezactivare Tx RS485
     Error("Detectie coliziune!"); // afiseaza Eroare coliziune
     Delay(1000); // asteapta 1 secunda
     return; // termina transmisia (revine)
   }
   UART1_MultiprocMode(MULTIPROC_DATA); // urmeaza tranmisia octetilor de date
   UART1_TxRxEN(1, 0);
	UART1_Putch(retea[i].bufbin.adresa_hw_src);
   UART1_Putch(retea[i].bufbin.tipmes);
   if(retea[i].bufbin.tipmes==USER_MES) { 
     UART1_Putch(retea[i].bufbin.src); // transmite restul caracterelor din buffer
	 UART1_Putch(retea[i].bufbin.dest); 
	 UART1_Putch(retea[i].bufbin.lng); 	
	 for (j = 0; j < retea[i].bufbin.lng; j++) {
        UART1_Putch(retea[i].bufbin.date[j]);
     }	   	   
   } 
    UART1_Putch(retea[i].bufbin.sc);
	UART1_TxRxEN(1, 1);
   
   if (TIP_NOD != MASTER) {
     retea[i].full = 0; // slave-ul considera acum ca a transmis mesajul
   }
//   Delay(1); // asteapta terminarea transmisie ultimului caracter
   UART1_Getch(0);
   UART1_TxRxEN(0, 0);
   UART1_RS485_XCVR(0, 0); // dezactivare Tx RS485
   return;
 }

 //***********************************************************************************************************
 void bin2ascii(unsigned char ch, unsigned char * ptr) { // converteste octetul ch in doua caractere ASCII HEX puse la adresa ptr
   unsigned char first, second;
   first = (ch & 0xF0) >> 4; // extrage din ch primul digit
   second = ch & 0x0F; // extrage din ch al doilea digit
   if (first > 9) * ptr++ = first - 10 + 'A'; // converteste primul digit daca este litera
   else * ptr++ = first + '0'; // converteste primul digit daca este cifra
   if (second > 9) * ptr++ = second - 10 + 'A'; // converteste al doilea digit daca este litera
   else * ptr++ = second + '0'; // converteste al doilea digit daca este cifra
 }