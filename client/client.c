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

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

FILE *fp;

void str_overwrite_stdout() {
  printf("%s", "> ");
  fflush(stdout);
}

void str_trim_lf(char *arr, int length) {
  for (int i = 0; i < length; i++) {  // trim \n
    if (arr[i] == '\n') {
      arr[i] = '\0';
      break;
    }
  }
}

void catch_ctrl_c_and_exit(int sig) { flag = 1; }

void send_msg_handler() {
  char message[LENGTH] = {};
  char buffer[LENGTH + sizeof(long int)] = {};

  fp = open_file(name);  // Apre il file di log

  while (1) {
    str_overwrite_stdout();
    fgets(message, LENGTH, stdin);  // Legge l'input
    system("clear");
    str_trim_lf(message, LENGTH);

    if (strcmp(message, "exit") ==
        0) {  // Se l'utente digita exit, la chat viene interrotta
      break;
    } else {
      sprintf(buffer, "%s:%ld", message,
              get_current_time());  // Formatta il messaggio testo:timestamp
      fprintf(fp, "%s: %s\n", name, message);   // Aggiunge il log nel file
      send(sockfd, buffer, strlen(buffer), 0);  // Invia il messaggio al server
    }

    bzero(message, LENGTH);
    bzero(buffer, LENGTH + sizeof(long int));
  }
  fclose(fp);
  catch_ctrl_c_and_exit(2);
}

void recv_msg_handler() {
  char message[LENGTH] = {};
  while (1) {
    int receive = recv(sockfd, message, LENGTH, 0);
    if (receive > 0) {
      printf("%s", message);
      str_overwrite_stdout();
    } else if (receive == 0) {
      break;
    } else {
      // Si e' verificato un errore
    }
    memset(message, 0, sizeof(message));
  }
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("Utilizzo: %s <port>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";
  int port = atoi(argv[1]);

  signal(SIGINT, catch_ctrl_c_and_exit);

  printf("Inserisci il tuo nome: ");
  fgets(name, 32, stdin);
  str_trim_lf(name, strlen(name));

  if (strlen(name) > 32 || strlen(name) < 2) {
    printf(
        "La lunghezza del nome deve essere compresa tra i 2 e i 30 "
        "caratteri.\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in server_addr;

  /* Socket settings */
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = inet_addr(ip);
  server_addr.sin_port = htons(port);

  // Connect to Server
  int err =
      connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (err == -1) {
    printf("[ERRORE]: Impossibile effettuare la connessione al server.\n");
    return EXIT_FAILURE;
  }

  // Invio del nome al server
  send(sockfd, name, 32, 0);

  printf("=== BENVENUTO NELLA CHATROOM ===\n");

  pthread_t send_msg_thread;
  if (pthread_create(&send_msg_thread, NULL, (void *)send_msg_handler, NULL) !=
      0) {
    printf(
        "[ERROR]: Impossibile creare il thread per l'invio' dei messaggi.\n");
    return EXIT_FAILURE;
  }

  pthread_t recv_msg_thread;
  if (pthread_create(&recv_msg_thread, NULL, (void *)recv_msg_handler, NULL) !=
      0) {
    printf(
        "[ERRORE]: Impossibile creare il thread per la ricezione dei "
        "messaggi.\n");
    return EXIT_FAILURE;
  }

  while (1) {
    if (flag) {
      printf("\nGrazie di aver chattato con noi!\n");
      break;
    }
  }

  close(sockfd);

  return EXIT_SUCCESS;
}