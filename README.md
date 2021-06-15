# Homework Sistemi Operativi (Modulo 2) A.A. 2020/2021
* ### Chatroom multithread in C
* ### Sviluppato da Riccardo Sangiorgio (matricola: 1957804)


## Descrizione del progetto

La chatroom è composta da due programmi distinti, il client e il server:
* Il client effettua la connessione al server, specificandone l'indirizzo e la porta come argomento> Può inviare messaggi agli altri utenti.
* Il server si mette in ascolto sulla porta specificata e inoltra i messaggi ricevuti. Può funzionare in due modalità.

## Server

Il server viene avviato con il seguente comando:

```bash
$ ./server <porta> <modalità> <wait>
```

* La porta verrà utilizzata dal server per definire la socket.
* La modalità definisce l'ordine con cui vengono inoltrati i messaggi ricevuti dai client.
* Il wait definisce la durata della finestra. Se la modalità è zero viene ignorata. Se la modalita è uno ma il wait non viene fornito, viene impostato a 5 secondi.

Le modalità sono due:
 
0. [ZERO] Il server inoltra i messaggi nell'ordine in cui sono arrivati.
1. [UNO] Il server inoltra i messaggi ordinandoli in base al timestamp di invio del client.

In entrambe le modalità il server salva i messaggi in una priority queue, il cui valore di priorità è definito dal timestamp.
Il messsaggio con timestamp più alto ha priorità più bassa e quindi verrà inviato per ultimo.
Un timestamp più basso indica un messaggio più vecchio, che quindi verrà inviato per primo.
Nel caso in cui lo modalità sia la 0, il timestamp viene generato sul server alla ricezione del messaggio, ma il meccanismo è lo stesso.
Nel caso in cui lo modalità sia la 1, viene creata una finestra di ricezione di 5 secondi, durante la quale vengono ricevuti i messaggi e mantenuti ordinati.
Al termine dei cinque secondi il messaggio con timestamp più basso viene inviato.

### Log
Quando si avvia una chat, viene generato un file con il seguente formato: 
```
log-<giorno>-<mese>-<anno>.log.txt
```

Il log identifica una sessione di chat in uno specifico giorno. Quindi per più sessioni di chat nella stessa giornata i messaggi verranno salvati sul medesimo file.

## Client
Il client si connette al server sulll'indirizzo e sulla porta specificata con il seguente comando:

```bash
$ ./client <address> <porta>
```
Nel caso in cui l'indirizzo non sia valido viene restituito un errore.

Dopo aver effettuato la connessione viene chiesto all'utente di inserire un nickname che verrà utilizzato durante tutta la sessione di chat.
Il formato adoperato dal client per l'invio del messaggio è il seguente:

```
<messaggio>:<timestamp>
```

Il timestamp è espresso in millisecondi (e non in secondi per garantire un ordinamento più preciso).

### Log
Quando si avvia una chat, viene generato un file con il seguente formato: 
```
log-<nickname>-<giorno>-<mese>-<anno>.log.txt
```

Il log identifica una sessione di chat di uno specifico utente in uno specifico giorno. Quindi se si utilizza lo stesso nickname durante la stessa giornata, i messaggi verranno salvati sul medesimo file.

## Test

### Ordinamento dei messaggi
Per verificare il corretto funzionamento dell'ordinamento dei messaggi sono stati effettuati i seguenti test:
1. Nel codice sono stati inseriti dei messaggi preconfezionati con timestamp non ordinato e se ne è verificata la corretta ricezione nell'ordine desiderato.
2. Sono stati inseriti manualemente dei messaggi in ordine sparso (di tempo) e da diversi utenti e se ne è verificata la corretta ricezione nell'ordine desiderato.

I test sono stati effettuati con una finestra superiore ai 5 secondi, per poterne verificare il corretto funzionamento.

### Log dei messaggi
Per verificare il corretto funzionamento dei log è stata avviata una sessione di chat con diversi utenti, al termine della quale sono stati effettivamente creati i log server e client .

## Struttura del codice, compilazione ed esecuzione

Il progetto è monorepo, organizzato in due cartelle: *client* e *server*.
Nella cartella principale è presente un Makefile globale che permette di compilare sia client che server (un Makefile per ogni cartella).

```bash
$ make
```

Avviare il server (esempio):

```bash
$ ./server 4000 0
```

Avviare il server con wait (esempio):

```bash
$ ./server 4000 1 2
```

Avviare il client (esempio):

```bash
$ ./client 127.0.0.1 4000
```
