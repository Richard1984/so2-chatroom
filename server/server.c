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

/**
 * volatile: "disattiva le ottimizzazioni del compilatore (utile per il multithreading e per i signal handler)"
 * sig_atomic_t: tipo intero da usare in un signal handler
 */
volatile sig_atomic_t flag = 0;
static _Atomic unsigned int cli_count = 0;
static int uid = 10;
static int mode = 0;  // 0: timestamp di ricezione, 1: timestamp di invio
int listenfd = 0;

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

FILE *log_fp;  // Log dei messaggi

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
    close(listenfd);
}

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
void send_message(char *message, int uid) {
    pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {                                                       // Se nella posizione il client non è NULL
            if (clients[i]->uid != uid) {                                       // Se l'uid non corrisponde al mittente
                if (write(clients[i]->sockfd, message, strlen(message)) < 0) {  // Invia il messaggio sulla socket del client
                    perror("[ERRORE]: Impossibile inviare il messaggio.");
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
    int leave_flag = 0;                                  // Flag di validità del client

    cli_count++;                  // Incrementa il contatore dei client
    client *cli = (client *)arg;  // Genera una nuova struttura client

    /**
   * Riceve il nickname dal client e lo rifiuta se c'è un errore o se il
   * nickname non è compreso tra i 2 e i NICKNAME_LENGTH caratteri
   */
    if (recv(cli->sockfd, name, NICKNAME_LENGTH, 0) <= 0 || strlen(name) < 2 || strlen(name) >= NICKNAME_LENGTH - 1) {
        printf("Il client non ha inserito un nickname.\n");
        leave_flag = 1;
    } else {
        cli->name = name;                                                 // Assegna il nome alla struttura client
        sprintf(buff_out, "%s è entrato nella chatoroom.\n", cli->name);  // Compone la stringa in un buffer
        printf("%s", buff_out);                                           // Stampa il messaggio sul server
        send_message(buff_out, cli->uid);                                 // Invia a tutti i client la comunicazione di un nuovo client
    }

    memset(buff_out, 0, BUFFER_SZ);  // Pulisce il buffer (imposta tutto a zero)

    while (1) {
        if (leave_flag || flag) break;  // Se si è verificato un errore il client viene scartato

        int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);  // Riceve un messaggio e lo memorizza nel buffer
        if (receive > 0) {                                        // Se non ci sono errori
            if (strlen(buff_out) > 0) {                           // Se il messaggio ha contenuto
                char *message = calloc(2048, sizeof(char));
                long int timestamp;

                sscanf(buff_out, "%[^':']:%ld", message, &timestamp);  // Estrae il messaggio e il timestamp

                pthread_mutex_lock(&messages_mutex);                  // Acquisisce la lock
                if (mode == 0) timestamp = get_current_time();        // Se la modalità è la zero viene utilizzato un timestamp generato dal server
                push(&messages, message, cli->uid, name, timestamp);  // Aggiunge il messaggio in coda
                printf("%s: %s\n", name, message);                    // Stampa il messaggio
                pthread_mutex_unlock(&messages_mutex);                // Rilascia la lock

                string_remove_newline(buff_out);
            }
        } else if (receive == 0 || strcmp(buff_out, "exit") == 0) {     // Si sono ricevuti zero byte o  è stato ricevuto il messaggio "exit"
            sprintf(buff_out, "%s ha lasciato la chat.\n", cli->name);  // Si formatta il messaggio di abbandono della chat
            printf("%s", buff_out);                                     // Si stampa il  messaggio
            send_message(buff_out, cli->uid);                           // Si invia il messaggio
            leave_flag = 1;                                             // Si interrompe il ciclo
        } else {                                                        // Si è verificato un errore
            printf("[ERRORE]: Si è verificato un errore.\n");           // Si stamoa l'errore
            leave_flag = 1;                                             // Si interrompo il ciclo
        }

        memset(buff_out, 0, BUFFER_SZ);  // Pulisce il buffer (imposta tutto a zero)
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
    log_fp = open_file();  // Apre il file per il log

    while (1) {
        if (flag) {
            send_message("close\n", -1);  // Se la flag è setta manda un messaggio close
            break;                        // Se la flag è settata esce dal loop
        }
        pthread_mutex_lock(&messages_mutex);                                       // Acquisisce la lock
        if (!isEmpty(&messages)) {                                                 // Se la coda è vuota non fa nulla
            memset(message, 0, BUFFER_SZ + NICKNAME_LENGTH + 3);                   // Pulisce il buffer
            sprintf(message, "%s: %s\n", messages->user_name, messages->message);  // Formatta il messaggio in: nickname: messaggio\n
            send_message(message, messages->uid);                                  // Inoltra il messaggio a tutti i client tranne che al mittente
            fprintf(log_fp, "%s: %s\n", messages->user_name, messages->message);   // Salva il messaggio nel log
            pop(&messages);                                                        // Rimuove il messaggio dalla coda
            memset(message, 0, BUFFER_SZ + NICKNAME_LENGTH + 3);                   // Pulisce il buffer (imposta tutto a zero)
        }
        pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock

        if (mode == 1) {
            sleep(*(int *)arg);  // Se il server deve inoltrare i messaggi in base al timestamp di invio (modalità 1) allora si crea una finestra di 5 secondi
        }
    }
    fclose(log_fp);                  // Chiude il file
    free(message);                   // Libera la memoria
    pthread_detach(pthread_self());  // Imposta il thread a detached

    return NULL;
}

int main(int argc, char **argv) {
    /* Se non sono forniti i parametri richiesti genera errore. */
    if (argc != 3 && argc != 4) {
        printf("Utilizzo: %s <port> <mode> <wait>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = "127.0.0.1";    // Indirizzo locale del server
    int port = atoi(argv[1]);  // Legge la porta e la converte in intero
    mode = atoi(argv[2]);      // Legga la modalità e la converte in intero
    int wait = 0;

    if (!argv[3] || argv[3] < 0) {
        wait = 5;
    } else {
        wait = atoi(argv[3]);  // Legga la durata in secondi della finestra la converte in intero
    }

    /* Se le modalità non sono 0 o 1 genera errore. */
    if (mode != 0 && mode != 1) {
        printf("MODALITÀ:\n0) Timestamp del server.\n1) Timestamp del client.\n");
        return EXIT_FAILURE;
    }

    int option = 1, connfd = 0;
    struct sockaddr_in serv_addr;  // Socket del server
    struct sockaddr_in cli_addr;   // Socket del client
    pthread_t tid;                 // Identificatore dei thread

    /* Impostazione della socket */
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    signal(SIGPIPE, SIG_IGN);               // Ignora i pipe signal
    signal(SIGINT, catch_ctrl_c_and_exit);  // Gestisce il Ctrl+C

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
   * Massimo 10 richieste dai client verranno messere in coda.
   */
    if (listen(listenfd, 10) < 0) {
        perror("[ERRORE] Impossibile effettuare il listening.");
        return EXIT_FAILURE;
    }

    /* Crea il thread per l'inoltro dei messaggi */
    if (pthread_create(&tid, NULL, &handle_send_message, &wait) != 0) {
        printf("[ERRORE]: Impossibile creare il thread per l'inoltro dei messaggi.\n");

        return EXIT_FAILURE;
    }

    printf("=== BENVENUTO NELLA CHATROOM ===\n");

    while (1) {
        socklen_t clilen = sizeof(cli_addr);
        connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);
        if (flag) break;
        /* Verifica se è stato raggiunto il numero massimo di client. */
        if ((cli_count + 1) == MAX_CLIENTS) {
            printf("Raggiunto il numero massimo di client. Rifiutato: ");
            close(connfd);  // Chiude la connessione
            continue;
        }

        /* Creazione del client */
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

    return EXIT_SUCCESS;
}