 -----------------------------
 | Marin Ioana-Valentina 325 |
 -----------------------------
===============================================================================
===============================================================================
====================>>> PROTOCOALE DE COMUNICATIE <<<==========================
=============>>>Tema #2 Aplicatie client-server TCP si UDP <<<=================
===================>>> pentru gestionarea mesajelor <<<========================
===============================================================================
===============================================================================

   ---> 1. Obiectivele temei de casa:
	Scopul temei este realizarea unei aplicatii care respecta modelul
client-server pentru gestiunea mesajelor.
	--> Obiectivele temei sunt:
		• intelegerea mecanismelor folosite pentru dezvoltarea aplicatiilor
		  de retea folosind protocoale TCP si UDP;
		• multiplexarea conexiunilor TCP si UDP;
		• definirea unui tip d date folosit de aplicatia propusa intr-un
		  protocol UDP predefinit;
		• dezvoltarea unei aplicatii practice de tip client-server ce
		  foloseste socketi.

  ---> 2. Descrierea modului de implementare (idee generala):
	-----------------------------------------------------------------------
	------------
======= |  SERVER: | ==========================================================
	------------
	  Serverul are rol de broker (componenta de intermediere) in platforma
de gestionare a mesajelor. Acesta va deschide 2 socketi (unul TCP si unul UDP)
pe un port primit ca parametru in linia de comanda si va astepta conexiuni/da-
tagrame pe toate adresele IP disponibile local. Pornirea serverului se va face 
folosind comanda:
		       ------------------------
		       |./server <PORT_DORIT> |
		       ------------------------
	-----------------------------------------------------------------------

	  -> Comenzi acceptate:

	-----------------------------------------------------------------------

		Serverul va accepta, de la tastatura, doar comanda "exit" ce va
	    avea ca efect inchiderea simultana a serverului si a tuturor
	    clientilor TCP conectati in acel moment.

	-----------------------------------------------------------------------

	  -> Comunicarea cu clientii UDP:

	-----------------------------------------------------------------------

	    	Comunicarea dintre clientii UDP, pe care eu am ales sa ii numesc
	"provider-ii de stiri" :), se va face in modul urmator:
	
	• Serverul verifica daca s-au primit noi stiri de la clientii UDP, pe
socket-ul rezervat clientilor UDP, unul singur, si folosind apelul de sistem
"recvfrom(...)" va primi intr-o structura de tip "sockaddr_in" IP-ul si port-ul
clientului UDP, iar mesajul propriu-zis trimis de clientul UDP va fi primit
intr-o structura de tip "udp_message" ce contine campurile topic, data_type si
payload.
	In cazul in care primim un mesaj de clientul UDP ce va contine in 
payload un STRING, atunci vom adauga la finalul acestuia terminatorul de sir.
	Pentru a trimite mai departe mesajul de la clientul UDP catre clientii
TCP, se vor completa campurile unui mesaj de tip "to_client_udp_message", 
structura ce va contine IP-ul clientului UDP de unde s-a primit mesajul, 
port-ul acestuia si mesajul de tip "udp_message" primit.
	Trimiterea mesajului se va face parcurgand toata lista de clienti 
conectati in momentul respectiv, dupa care parcurgand toata lista de topicuri
a fiecarui client si daca este abonat la topic-ul aferent atunci i se trimite
mesajul instant. In cazul in care clientul este deconectat, dar este abonat
la topic-ul primit de la clientul UDP si are activata optiunea SF (store &
forward) atunci se pastreaza mesajele primite, stirile, in coada sa de mesaje,
urmand ca in caz de reconectare, acesta sa fie notificat si sa i se trimita
mesajele stocate in coada. Acest lucru se realizeaza folosind functia 
"put_in_queue(...)". Aceasta adauga fiecare mesaj primit ca parametru in coada
de mesaje a clientului. 

	-----------------------------------------------------------------------

	  -> Comenzile primite de la tastura:

	----------------------------------------------------------------------

	Comanda "exit" primita de la tastura va inchide server-ul si in acelasi 
timp toti clientii ce sunt conectati la acesta.
	Inchiderea server-ului se va face folosind functia "server_closing()",
ce verifica daca comanda citita de la tastatura a fost "exit", in caz afirmativ
returneaza 1, afiseaza un mesaj la stdout corespunzator "Server shuts down 
correctly.", dupa care iesind din bucla infinita, elibereaza memoria alocata
dinamic si inchide socket-ul pasiv de TCP si socket-ul UDP, iar in cazul in
care returneaza 0 trece la urmatoarea citire de la tastatura.

	-----------------------------------------------------------------------

	  -> Conectarea unui nou client TCP/ reconectarea unui client TCP:

	-----------------------------------------------------------------------

	In momentul in care se conecteaza un client, acesta trimite catre server
un mesaj de tip "client_tcp_message" ce contine ID-ul sau. Server-ul primeste 
mesajul pe socket-ul pasiv mesajul si verifica daca este un caz de reconectare a
unui client, daca este un caz de conectare a unui client nou sau daca un client
incearca sa se conecteze cu un ID existent deja in baza de date fiind al unui
user conectat la momentul respectiv.
	Folosind variabila "isReconnected" contorizam cazul in care un client 
se reconecteaza sau daca este un client nou, iar folosind variabila "error" 
contorizam aparitia unei erori de genul: cazul in care un client incearca sa se
conecteze cu un ID deja folosit. 
	In cazul in care se atinge numarul maxim de clienti, atunci se va 
realoca memorie pentru lista de clienti folosind functia "realloc_clients(...)", 
ce va realoca spatiu pentru lista de clienti si va dubla numarul maxim de clienti.
	In cazul in care un client doreste sa se reconecteze la server, atunci
se va verifica daca are mesaje stocate in coada sa si daca da, atunci i se vor
trimite mesajele folosind functia "send_updates(..)". Aceasta functie parcurge 
coada de mesaje si va extrage din aceasta fiecare update de stiri urmand sa le
trimita clientului.
	In cazul in care un client se conecteaza pentru prima data la server 
atunci se completeaza campurile unei structuri de tip "tcp_client" si se va
aloca spatiu pentru lista sa de topicuri si de asemenea se vor actualiza faptul
ca avem un client nou in baza de date si de asemenea ca avem un client activ, 
adica variabilele "clients_number" si "active_clients".

	-----------------------------------------------------------------------
	
	  -> Primirea comenzilor de abonare/dezbonare a unui client TCP:

	-----------------------------------------------------------------------

	In momentul in care un client TCP doreste sa se aboneze la un anumit
topic acesta trimite serverului un mesaj de tip "client_tcp_message" ce contine
campurile : command_type, topic, id_client si SF. 
	In momentul in care serverul primeste mesajul, acesta parcurge lista de
clienti si verifica pe care dintre socketii clientilor a primit mesaj urmand 
sa il primeasca intr-o structura de tip "client_tcp_message".
	Serverul verifica intai daca file descriptorul returnat de functia "recv"
este 0, adica daca clientul s-a deconectat de la server. In caz afirmativ,
atunci elibereaza socket-ul clientului stergandu-l din multimea de citire si
actualizeaza numarul de clienti activi si in acelasi timp actualizeaza in structura
de client ("tcp_client") campul "isConnected" ca fiind 0.

	• SUBSCRIBE:
	------------

	In cazul in care un client doreste sa se aboneze la un topic si trimite
catre server un mesaj ce contine comanda "subscribe", serverul parcurge lista 
de topicuri ale clientului respectiv, verifica cazurile de eroare si in caz 
ca nu exista niciun caz de eroare atunci aboneaza clientul respectiv la topicul
pentru care a fost data comanda "subscribe". In acest caz, intai se verifica
daca nu s-a atins numarul maxim de topicuri alocate deja si in caz ca este nevoie
de redimensionarea listei de topicuri, acest lucru este facut in paralel
actualizandu-se si numarul maxim de topicuri din acel moment. Dupa aceasta, 
se adauga in lista de topicuri topicul nou la care trebuie abonat clientul, 
completandu-se o structura de tip "topic" ce contine campurile: SF, title si 
isSubscribed.

	-> Exista doua cazuri de eroare in ceea ce priveste aceasta comanda:
	-------------------------------------------------------------------
	
	• Un client doreste sa se aboneze inca o data la un topic la care este
deja abonat cu acelasi SF. In acest caz serverul trimite un mesaj catre client 
ce contine in payload eroarea "User is already subscribing this channel.".
	• Un client doreste sa aboneze la un topic la care este deja abonat
dar cu un alt parametru SF. In aceasta situatie este actualizat campul SF din
structura topicului respectiv si este trimis catre client un mesaj corespunzator:
"Updating SF...SF updated for this topic.".

	
	• UNSUBSCRIBE:
	--------------
	
	In cazul in care un client doreste sa se dezaboneze de la un topic si
trimite catre server un mesaj ce contine comanda "unsubscribe", serverul 
parcurge lista de topicuri ale clientului respectiv, verifica cazurile de eroare
si in caz ca nu exista niciun caz de eroare atunci dezaboneaza clientul de la 
topicul mentionat actualizand campul "isSubscribed" din structura topicului 
respectiv si campul "active_topics_number" din structura de client corespunzatoare
clientului in cauza.
	
	-> Exista doua cazuri de eroare in ceea ce priveste aceasta comanda:
	-------------------------------------------------------------------
	
	• Un client doreste sa se dezaboneze de la un topic la care nu s-a 
abonat niciodata. In acest caz serverul trimite un mesaj catre client ce contine
in payload eroarea "Cannot unsubscribe. User has never subscribed this topic.".
	• Un client doreste sa se dezaboneze de la un topic la care s-a abonat
in prealabil si s-a si dezabonat. In aceasta situatie serverul trimite un mesaj
catre client ce contine in payload eroarea "User is not subscribing anymore 
this topic.".
	

	-----------------------------------------------------------------------
	
	  -> Tratarea cazurilor de eroare:
	
	-----------------------------------------------------------------------

	Cazurile de eroare sunt tratate folosind functia "error_handler(...)" 
ce primeste ca parametrii socketul pe care trebuie trimis mesajul si mesajul
de eroare ce trebuie trimis catre client.

	-----------------------------------------------------------------------

	  -> Dezactivarea algoritmului Neagle:

	-----------------------------------------------------------------------

	Dezactivarea algoritmului Neagle se face in momentul in care se creeaza
un socket nou pentru un client, dezactivand pentru fiecare socket creat aceasta
componenta. Dezactivarea este facuta de catre functia "tcp_nodelay(...)" ce 
primeste ca parametru socketul pentru care se doreste a fi facuta aceasta
dezactivare, folosind de asemenea apelul de sistem "setsockop(...)".


	----------------
======= | CLIENT TCP : | ======================================================
	----------------

	Clientii de TCP pot fi in orice numar, la fel ca cei UDP, si vor fi
rulati folosind comanda urmatoare :
	
		 -----------------------------------------------------
		 |./subscriber <ID_Client> <IP_Server> <Port_Server> |
		 -----------------------------------------------------
unde:
	• ID_Client este un sir de caracatere ce reprezeinta ID-ul clientului.
(maxim 10 caractere ASCII);
	• IP_Server reprezinta adresa IPv4 a serverului reprezentata folosind 
notatia dotted-decimal (exemplu 1.2.3.4);
	• Port_Server reprezinta portul pe care serverul asteapta conexiuni.


	-----------------------------------------------------------------------

	  -> Comenzi acceptate de la tastatura:

	-----------------------------------------------------------------------

		Clientii de TCP pot primi de la tastaura una dintre urmatoarele 
	comenzi:

	-------------------------
	• subscribe <topic> <SF>
	       ------------------------------------------------------------------
	       | <topic> = topicul la care clientul urmeaza sa se aboneze;      |
	       | <SF> = poate avea valoarea 0 sau 1. (componenta store&forward).|
	       ------------------------------------------------------------------

		Pentru acest tip de comanda afiseaza inainte sa o trimita catre
	server o linie de feedback de tipul "subscribe topic".

	---------------------
	• unsubscribe <topic>
		---------------------------------------------------------------
	        | <topic> = topicul la care clientul urmeaza sa se dezaboneze |
		---------------------------------------------------------------
		Pentru acest tip de comanda afiseaza inainte sa o trimita catre
	server o linie de feedback de tipul "unsubscribe topic".

	-------
	• exit
		Comanda va fi folosita pentru a realiza inchiderea clientului.

	-----------------------------------------------------------------------
	
	      (*) Clientii de TCP pot primi de la server mesaje de tip "update"
	pentru topicurile la care acestia sunt abonati. Pentru fiecare mesaj
	de acest tip se va afisa imediat un mesaj de forma:
		
		---------------------------------------------------------
		| IP:PORT client_UDP - topic - tip_date - valoare mesaj |
		---------------------------------------------------------

	-----------------------------------------------------------------------

		-> Comenzile primite de la tastatura:	
	
	-----------------------------------------------------------------------
		
		In cazul in care clientul primeste o comanda de la tasatura,
clientul verfica daca este una dintre cele mentionate si permise de mai sus:
"exit", "subscribe" sau "unscribe". In cazul in care nu este niciuna dintre 
comenzile mentionate mai sus atunci afiseaza la "stderr" un mesaj de eroare
de genul "Invalid operation. Type unknown." si forteaza clientul sa treaca la
urmatoarea comanda de la tastatura. 
		
	• EXIT :
	--------
		
	In momentul in care aceasta comanda este primita de catre client
acesta inchide conexiunea cu serverul si acest lucru este remarcat in server 
de catre file descriptorul intors de functia "recv(...)" care va fi setat cu 0, 
astfel serverul sa observe faptul ca un client s-a deconectat.


	• SUBSCRIBE :
	-------------
	
	In momentul in care aceasta comanda este primita de catre client de la
tastatura, este apelata functia "subscribe_topic(...)" ce va realiza trimiterea 
catre server a unui mesaj de tip "client_tcp_message" ce contine campurile
command_type, topic, id_client si SF. 
	Functia "subscribe_topic(...)" primeste ca parametrii intreaga comanda
citita de la tasatura si stocata intr-un buffer si socketul serverului. Aceasta
extrage fiecare camp asociat structurii de mesaj si le seteaza pe acestea in
cazul in care nu exista niciun caz de eroare, urmand sa trimita mesajul catre
server si sa afiseze linia de feedback la stdout dupa trimitere.
	
	
	-> Exista doua cazuri de eroare in ceea ce priveste aceasta comanda:
	--------------------------------------------------------------------
	
	• Comanda "subscribe" este primita de catre client fara niciun parametru,
adica fara un topic. In acest caz este printat de catre client la stdout mesajul
de eroare "Invalid subscribe operation. No topic."
	• Comanda "subscribe" primeste un SF diferit de 0 sau 1, ceea ce este 
un SF invalid. In acest caz clientul va printa la stdout "Invalid subscribe
operation. SF incorrect.".

	
	• UNSUBSCRIBE :
	---------------

	In momentul in care aceasta comanda este primita de catre client de la 
tasatura, se vor extrage din buffer-ul in care este stocata tipul de operatie
si topicul si seteaza campurile asociate acestora intr-un tip de mesaj 
"client_tcp_message". Campul corespunzator SF-ului este setat cu valoarea -1,
pentru ca nu ne intereseaza valoarea sa si nu vrem oricum sa modificam 
aceasta valoare pentru topicul prezent in lista de topicuri ale clientului
respectiv, in server nemodificandu-se in momentul in care se primeste comanda
de "unsubscribe" campul "SF" din structura unui topic.
	Dupa acestea, mesajul este trimis catre server si clientul afiseaza 
linia de feedback la stdout.


	-> Exista un caz de eroare in ceea ce priveste aceasta comanda:
	--------------------------------------------------------------------

	• Comanda "unsubscribe" este primita de catre client fara niciun 
parametru aditional, adica fara niciun topic. In aceasta situatie este afisat
la stdout un mesaj de eroare corespunzator "Invalid unsubscribe operation. No
topic.". 

	
	-----------------------------------------------------------------------

	  -> Comunicarea cu serverul :

	-----------------------------------------------------------------------

	In cazul in care clientul primeste un mesaj de la server, acesta primeste 
mesajul intr-o structura de tip "to_client_udp_message".

	-----------------------------------------------------------------------

	• Inchiderea serverului:
	------------------------
	
	In cazul in care serverul este inchis, acesta trebuie sa inchida si toti
clientii. Astfel in acest caz, clientul verifica file descriptorul returnat de
catre functia "recv(...)" si in cazul in care valoarea sa este 0 iese din 
bucla infinita si inchide socketul. Astfel se inchide conexiunea.


	• Erori semnalate de catre server:
	----------------------------------

	In cazul in care clientul primeste un mesaj de eroare se la server, acesta
verifica acest caz folosind functia "error_handler(...)" ce primeste ca parametru 
mesajul de tip "to_client_udp_message" primit. 
	Aceasta functie verifica ce tip de eroare este semnalata de catre server,
primita in payload-ul mesajului, si o afiseaza la stderr dupa care returneaza 
1, 0 sau -1 depinzand de ce trebuie sa faca clientul dupa: sa opreasca programul 
si sa inchida socketul (1), sa treaca la urmatoarea comanda (0) sau in caz ca nu
este nicio eroare sa continue normal executia (-1).


	• Afisarea mesajelor primite de tip "update" pentru topic-uri:
	--------------------------------------------------------------

	In cazul in care nu exista nicio eroare si serverul nu este in situatia
de a fi in curs de inchidere, atunci mesajul primit de catre client de la server
este unul de tip "update a stirilor" primit de la clientii de UDP. 
	In aceasta situatie se verifica campul "data_type" a structurii de mesaj
si in functie de acesta sunt apelate functii corespunzatoare pentru a face 
conversia tipului de date gasit in payload si afisarea sa la stdout.

	• INT :
	-------
	
	In cazul in care data_type este setat cu valoarea 0 atunci se va apela
functia "convert_to_int(...)" ce va primi ca parametru mesajul primit de la 
server.
	Aceasta functie converteste payload-ul primit la o variabila de tip
intreg si afiseaza la tasatura datele primite in formatul specificat mai sus,
in cadrul sectiunii (*) referitoare la mesajele de "update" pentru clienti.
	Conversia se realizeaza luand fiecare dintre cei 4 octeti, formand 
numarul folosind operatii pe biti si verificand daca primul octet 
(payload[0]) este 1 (negativ) sau 0 (pozitiv), acesta semnificand semnul 
numarului intreg.
	Dupa aceasta se afiseaza la tastatura un mesaj corespunzator.


	• SHORT_REAL :
	--------------

	In cazul in care data_type este setat cu valoarea 1 atunci se va apela
functia "convert_to_float(...)" ce va primi ca parametru mesajul primit de la 
server.
	Aceasta functie converteste payload-ul primit la o variabila de tip
float si afiseaza la afiseaza la tasatura datele primite in formatul specificat 
mai sus, in cadrul sectiunii (*) referitoare la mesajele de "update" pentru 
clienti.
	Conversia se realizeaza luand cei doi octeti ai numarului, formandu-l 
folosind operatii pe biti si impartindu-l pe acesta la 100, impartirea fiind
de tip real.
	Dupa aceasta se afiseaza la tastatura un mesaj corepunzator, numarul
real fiind afisat cu 3 zecimale.


	• FLOAT :
	---------

	In cazul in care data_type este setat cu valoarea 2 atunci se va apela
functia "convert_to_double(...)" ce va primi ca parametru mesajul primit de la 
server.
	Aceasta functie converteste payload-ul primit la o variabila de tip
double si afiseaza la tasatura datele primite in formatul specificat mai sus,
in cadrul sectiunii (*) referitoare la mesajele de "update" pentru clienti.
	Conversia se realizeaza luand fiecare dintre cei 4 octeti, formand 
numarul folosind operatii pe biti si verificand daca primul octet 
(payload[0]) este 1 (negativ) sau 0 (pozitiv), acesta semnificand semnul 
numarului intreg, dupa care urmand sa se calculeze puterea lui 10 la care
trebuie impartit folosind functia "pow(...)", 10 avand ca exponent unsigned
char-ul din al 6-lea octet din payload (payload[5]). 
	Ultimul pas este impartirea celor doua valori calculate si stocarea
rezultatului intr-o variabila de tip double, rezultatul intors de functia
"pow(...)" fiind unul de tip double.
	Dupa aceasta se afiseaza la tastatura un mesaj corepunzator, numarul
real nefiind afisat cu un numar de zecimale exact.


	• STRING :
	----------

	In cazul in care data_type este setat cu valoarea 3 atunci se va afisa
la tastatura un mesaj corespunzator, afisand string-ul stocat in payload asa
cum a fost primit.

	----------------------------------------------------------------------
	-----------------------------------------------------------------------

	  -> Precizari suplimentare:
		
	-----------------------------------------------------------------------
	
	• Coada folosita pentru stocarea mesajelor cat timp clientul este 
deconectat este preluata din resursele TEMEI 1, implementarea procesului de 
dirijare a pachetelo dintr-un router, puse la dispozitie de catre echipa de PC.

	• Aplicatia nu are memory leak-uri, dezalocand absolut tot ce este alocat
la sfarsitul functiei "main(...)" inainte de inchiderea socketilor TCP si UDP si
de asemena dezalocand si ce (char *) folosesc in functiile suplimentare.

	• Am folosit scheletul de laborator din "Laboratorul 8 - TCP si multiplexare
I/O" atat pentru partea de cod din server cat si pentru codul prezent in fisierul
"subscriber.c". 
	
	• Am folosit functia "DIE" prezenta in fisierul "helpers.h" pentru
verificarea rezultatelor returnate de apelurile de sistem, punand conditia ca 
acestea sa fie mai mari sau egale cu zero, in cazul in care nu erau se afisa
un mesaj de eroare.
	
	• Am verificat rezultatul alocarilor de memorie, respectiv ca pointerii
alocati dinamic sa nu fie NULL dupa alocare, in caz ca erau sa printeze la "stderr"
un mesaj sa eroare si dupa aceasta sa iasa din program cu "exit(1)". De asemenea
intr-un mod similar am verificat si numarul argumentelor date in linia de comanda
in momentul rularii executabilelor "server" si "subscriber", pentru a fi sigura ca
ambele executabile sunt rulate corect din punct de vedere al parametrilor dati.

	• Structura specifica unui client TCP are urmatoarele campuri:
		-> socket : socketul clientului
		-> id_client : id-ul clientului
		-> *topics : lista de topicuri la care este abonat clientul 
		-> active_topics_number : numarul de topicuri la care clientul este 
								abonat in momentul prezent 
		-> topics_number : numarul total de topicuri
		-> max_topics : numarul maxim de topicuri la care se poate abona un client
		-> isConnected : variabila setata cu 1 daca clientul este conectat,
						0 daca clientul nu este conectat.
		-> message_queue : coada de mesaje pastrate in cazul algoritmului SF.
	
	-----------------------------------------------------------------------

===============================================================================

