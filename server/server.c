#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "queue.c"
#include "utils.c"

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048
#define MESSAGES_QUEUE_SIZE 5
#define NICKNAME_LENGTH 32

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
static int mode = 0;  // 0: timestamp di ricezione, 1: timestamp di invio

/* Struttura dati per memorizzare un client */
typedef struct {
    int uid;                     // ID del client
    char *name;                  // Nickname del client
    struct sockaddr_in address;  // Indirizzo del client
    int sockfd;                  // Socket del client
} client;

client *clients[MAX_CLIENTS];  // Lista dei client
Node *messages = NULL;         // Coda dei messaggi

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex per la lista dei client
pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex per la coda dei messaggi

FILE *fp;  // Log dei messaggi

/* Aggiunge un client alla coda */
void clients_queue_add(client *cl) {
    pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (!clients[i]) {
            clients[i] = cl;  // Aggiunge un client nel primo post libero
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);  // Rilascia la lock
}

/* Rimuove un client dalla coda */
void clients_queue_remove(int uid) {
    pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] &&
            clients[i]->uid == uid) {  // Se l'uid del client corrisponde
            clients[i] = NULL;         // Rimuove il client dalla coda
            break;
        }
    }

    pthread_mutex_unlock(&clients_mutex);  // Rilascia la lock
}

// Invia un messaggio a tutti i client ad eccezione di quello con uid specificato
void send_message(char *s, int uid) {
    pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

    fprintf(fp, "%s\n", s);  // Salva il messaggio nel log

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {                                           // Se nella posizione il client non e' NULL
            if (clients[i]->uid != uid) {                           // Se l'uid non corrisponde al mittente
                if (write(clients[i]->sockfd, s, strlen(s)) < 0) {  // Invia il messaggio sulla socket del client
                    perror("[ERROR]: Impossibile inviare il messaggio.");
                    break;
                }
            }
        }
    }

    pthread_mutex_unlock(&clients_mutex);  // Rilascia la lock
}

/* Gestisce le interazione del server con il client */
void *handle_client(void *arg) {
    char buff_out[BUFFER_SZ];                            // Buffer di output sul server
    char *name = calloc(NICKNAME_LENGTH, sizeof(char));  // Nickname temporaneo del client
    int leave_flag = 0;                                  // Flag di validita' del client

    cli_count++;                  // Incrementa il contatore dei client
    client *cli = (client *)arg;  // Genera una nuova struttura client

    /**
   * Riceve il nickname dal client e lo rifiuta se c'e' un errore o se il
   * nickname non e' compreso tra i 2 e i NICKNAME_LENGTH caratteri
   */
    if (recv(cli->sockfd, name, NICKNAME_LENGTH, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NICKNAME_LENGTH - 1) {
        printf("Il client non ha inserito un nickname.\n");
        leave_flag = 1;
    } else {
        cli->name = name;                                                  // Assegna il nome alla struttura client
        sprintf(buff_out, "%s e' entrato nella chatoroom.\n", cli->name);  // Compone la stringa in un buffer
        printf("%s", buff_out);                                            // Stampa il messaggio sul server
        send_message(buff_out, cli->uid);                                  // Invia a tutti i client la comunicazione di un nuovo client
    }

    bzero(buff_out, BUFFER_SZ);  // Pulisce il buffer (imposta tutto a zero)

    while (1) {
        if (leave_flag) break;  // Se si e' verificato un errore il client viene scartato

        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);  // Riceve un messaggio e lo memorizza nel buffer
        if (receive > 0) {                                        // Se non ci sono errori
            if (strlen(buff_out) > 0) {                           // Se il messaggio ha contenuto
                char *message = calloc(2048, sizeof(char));
                long int timestamp;

                sscanf(buff_out, "%[^':']:%ld", message, &timestamp);  // Estrae il messaggio e il timestamp

                pthread_mutex_lock(&messages_mutex);                  // Acquisisce la lock
                if (mode == 0) timestamp = get_current_time();        // Se la modalita' e' la zero viene utilizzato un timestamp generato dal server
                push(&messages, message, cli->uid, name, timestamp);  // Aggiunge il messaggio in coda
                pthread_mutex_unlock(&messages_mutex);                // Rilascia la lock

                str_trim_lf(buff_out, strlen(buff_out));
                printf("%s: %s\n", name, message);
            }
        } else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
            sprintf(buff_out, "%s ha lasciato la chat.\n", cli->name);
            printf("%s", buff_out);
            send_message(buff_out, cli->uid);
            leave_flag = 1;
        } else {
            printf("[ERRORE]: Si è verificato un errore.\n");
            leave_flag = 1;
        }

        bzero(buff_out, BUFFER_SZ);  // Pulisce il buffer (imposta tutto a zero)
    }

    close(cli->sockfd);              // Chiude la connessione
    clients_queue_remove(cli->uid);  // Rimuove il client dalla lista dei client
    free(cli);                       // Libera la memoria associata al client
    free(name);                      // Libera la memoria associata al nickname temporaneo
    cli_count--;                     // Decrementa il contatore dei client
    pthread_detach(pthread_self());  // Imposta il thread a detached

    return NULL;
}

/* Gestisce l'inoltro dei messaggi */
void *handle_send_message(void *arg) {
    char *message = calloc(BUFFER_SZ + NICKNAME_LENGTH + 3, sizeof(char));
    while (1) {
        pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock
        if (!isEmpty(&messages)) {            // Se la coda e' vuota non fa nulla
            bzero(message, BUFFER_SZ + NICKNAME_LENGTH + 3);
            sprintf(message, "%s: %s\n", messages->user_name, messages->message);  // Formatta il messaggio in: nickname: messaggio\n
            send_message(message, messages->uid);                                  // Inoltra il messaggio a tutti i client tranne che al mittente
            pop(&messages);                                                        // Rimuove il messaggio dalla coda
        }
        pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock
        if (mode == 1) {
            sleep(5);  // Se il server delle inoltrati i messaggi in base al timestamp di invio (modalita' 1) allora si crea una finestra di 5 secondi
        }
    }
    free(message);
}

int main(int argc, char **argv) {
    /* Se non sono forniti i parametri richiesti genera errore. */
    if (argc != 3) {
        printf("Utilizzo: %s <port> <mode>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";    // Indirizzo locale del server
    int port = atoi(argv[1]);  // Legge la porta e la converte in intero
    mode = atoi(argv[2]);      // Legga la modalita' e la converte in intero

    /* Se le modalita' non sono 0 o 1 genera errore. */
    if (mode != 0 && mode != 1) {
        printf("MODALITÀ:\n0) Timestamp del server.\n1) Timestamp del client.\n");
        return EXIT_FAILURE;
    }

    int option = 1, listenfd = 0, connfd = 0;
    struct sockaddr_in serv_addr;  // Socket del server
    struct sockaddr_in cli_addr;   // Socket del client
    pthread_t tid;                 // Identificatore dei thread

    /* Impostazione della socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    /* Ignore pipe signals */
    signal(SIGPIPE, SIG_IGN);

    if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char *)&option, sizeof(option)) < 0) {
        perror("[ERRORE]: Impossibile impostare le opzioni della socket.");
        return EXIT_FAILURE;
    }

    /* Si effettua il bind dell'indirizzo locale alla socket.  */
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("[ERRORE] Impossibile effettuare il binding.");
        return EXIT_FAILURE;
    }

    /*
   * Viene messo in ascolto il server.
   * Masssimo 10 richieste dai client verranno messere in coda.
   */
    if (listen(listenfd, 10) < 0) {
        perror("[ERRORE] Impossibile effettuare il listening.");
        return EXIT_FAILURE;
    }

    fp = open_file();  // Apre il file per il log

    /* Crea il thread per l'inoltro dei messaggi */
    pthread_create(&tid, NULL, &handle_send_message, NULL);

    printf("=== BENVENUTO NELLA CHATROOM ===\n");

    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

        /* Verifica se e' stato raggiunto il numero massimo di client. */
        if ((cli_count + 1) == MAX_CLIENTS) {
            printf("Raggiunto il numero massimo di client. Rifiutato: ");
            close(connfd);  // Chiude la connessione
            continue;
        }

        /* Client settings */
        client *cli = (client *)malloc(sizeof(client));
        cli->address = cli_addr;
        cli->sockfd = connfd;
        cli->uid = uid++;

        /* Aggiunge il client alla coda e crea il trhead */
        clients_queue_add(cli);
        pthread_create(&tid, NULL, &handle_client, (void *)cli);

        /* Riduce l'uso della CPU */
        sleep(1);
    }

    fclose(fp);

    return EXIT_SUCCESS;
}