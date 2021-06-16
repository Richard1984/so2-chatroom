#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "utils.c"

#define LENGTH 2048
#define NICKNAME_LENGTH 32

/**
 * volatile: "disattiva le ottimizzazioni del compilatore (utile per il multithreading e per i signal handler)"
 * sig_atomic_t: tipo intero da usare in un signal handler
 */
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

FILE *log_fp;  // Log dei messaggi

void catch_ctrl_c_and_exit(int sig) { flag = 1; }

void send_msg_handler() {
    char message[LENGTH] = {};
    char buffer[LENGTH + sizeof(long int) + 1] = {};

    log_fp = open_file(name);  // Apre il file di log

    while (1) {
        str_overwrite_stdout();
        fgets(message, LENGTH, stdin);  // Legge l'input
        // system("clear"); // Utile nella modalità 1 per evitare che il messaggio inviato si mischi con quelli ricevuti
        string_remove_newline(message);

        if (strcmp(message, "exit") == 0) {  // Se l'utente digita exit, la chat viene interrotta
            break;
        } else {
            sprintf(buffer, "%s:%ld", message, get_current_time());  // Formatta il messaggio testo:timestamp
            fprintf(log_fp, "%s: %s\n", name, message);              // Aggiunge il log nel file
            send(sockfd, buffer, strlen(buffer), 0);                 // Invia il messaggio al server
        }

        memset(message, 0, LENGTH);         // Pulisce il buffer (imposta tutto a zero)
        memset(buffer, 0, sizeof(buffer));  // Pulisce il buffer (imposta tutto a zero)
    }
    fclose(log_fp);            // Chiude il file
    catch_ctrl_c_and_exit(2);  // Interrompe il programma
}

void recv_msg_handler() {
    char message[LENGTH + NICKNAME_LENGTH + 3] = {};
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);  // Riceve il messaggio
        if (receive > 0) {                               // Se il messaggio non è vuoto
            printf("%s", message);                       // Stampa il messaggio
            str_overwrite_stdout();                      // Predispone il layout "> "
        } else if (receive == 0) {
            // 0 bytes letti
            catch_ctrl_c_and_exit(2);
            break;
        } else {
            // Si è verificato un errore
        }
        memset(message, 0, sizeof(message));
    }
}

int main(int argc, char **argv) {
    /* Se non sono forniti i parametri richiesti genera errore. */
    if (argc != 3) {
        printf("Utilizzo: %s <address> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];        // Legge l'indirizzo del server
    int port = atoi(argv[2]);  // Legge la porta e la converte in intero

    if (inet_addr(ip) == -1) {
        printf("[ERRORE]: L'indirizzo del server non è valido.\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, catch_ctrl_c_and_exit);  // Viene catturato il Ctrl+C

    printf("Inserisci il tuo nome: ");
    fgets(name, NICKNAME_LENGTH, stdin);  // Si legge il nickname
    string_remove_newline(name);          // Viene rimosso il \n

    if (strlen(name) > NICKNAME_LENGTH || strlen(name) < 2) {
        printf("La lunghezza del nome deve essere compresa tra i 2 e i 30 caratteri.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;  // Socket del server

    /* Impostazione della socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ip);
    server_addr.sin_port = htons(port);

    // Connessione al server
    int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));

    if (err == -1) {
        printf("[ERRORE]: Impossibile effettuare la connessione al server.\n");
        return EXIT_FAILURE;
    }

    err = send(sockfd, name, 32, 0);  // Invio del nome al server

    if (err == -1) {
        printf("[ERRORE]: Impossibile inviare il nickname.\n");
        return EXIT_FAILURE;
    }

    printf("=== BENVENUTO NELLA CHATROOM ===\n");

    pthread_t send_msg_thread;

    if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) != 0) {
        printf("[ERRORE]: Impossibile creare il thread per l'invio dei messaggi.\n");

        return EXIT_FAILURE;
    }

    pthread_t recv_msg_thread;

    if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) != 0) {
        printf("[ERRORE]: Impossibile creare il thread per la ricezione dei messaggi.\n");

        return EXIT_FAILURE;
    }

    while (1) {
        if (flag) {
            printf("\nGrazie di aver chattato con noi!\n");
            break;
        }
    }

    close(sockfd);  // Chiude la connessione

    return EXIT_SUCCESS;
}