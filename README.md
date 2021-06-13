# Homework Sistemi Operativi (Modulo 2) A.A. 2020/2021
* ### Chatroom multithread in C
* ### Sviluppato da Riccardo Sangiorgio (matricola: 1957804)


## Descrizione del progetto

La chatroom è composta da due programmi distinti, il client e il server:
* Il client effettua la connessione al server, specificandone la porta come argomento, e può inviare messaggi agli altri utenti.
* Il server si mette in ascolto sulla porta specificata e inoltra i messaggi ricevuti. Può funzionare in due modalità.

## Server

Il server viene avviato con il seguente comando:

```bash
$ ./server <porta> <modalità>
```

La porta verrà utilizzata dal server per definire la socket.
La modalità definisce l'ordine con cui vengono inoltrati i messaggi ricevuti dai clienti. Le modalità sono due:

1. 0: Il server inoltra i messaggi nell'ordine in cui sono arrivati.
2. 1: Il server inoltra i messaggi ordinandoli in base al timestamp di invio del client.

In entrambe le modalità il server salva i messaggi in una priority queue, il cui valore di priorità è definito dal timestamp.
Il messsaggio con timestamp più alto ha priorità più bassa e quindi verrà inviato per ultimo.
Nel caso in cui lo modalità sia la 0, il timestamp viene generato sul server alla ricezione del messaggio, ma il meccanismo è lo stesso.
Nel caso in cui lo modalità sia la 1, viene creata una finestra di ricezione di 5 secondi, durante la quale vengono ricevuti i messaggi e mantenuti ordinati.

## Client
Il client si connette al server sulla porta specificata con il seguente comando:

```bash
$ ./client <porta>
```

Dopo aver effettuato la connessione gli viene chiesto di inserire un nickname che verrà utilizzato per tutto lo scambio di messaggi.
Il formato adoperato per l'invio del messaggio è il seguente:

```
<messaggioo>:<timestamp>
```

Il timestamp è espresso in millisecondi.
