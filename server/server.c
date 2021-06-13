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

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
static int mode = 0;  // 0: timestamp di invio, 1: timestamp di ricezione

/* Struttura dati per memorizzare un client */
typedef struct {
  int uid;                     // ID del client
  char *name;                  // Nickname del client
  struct sockaddr_in address;  // Indirizzo del client
  int sockfd;                  // Socket del client
} client;

// typedef struct {
//   char message[BUFFER_SZ];
//   // char client_name[32];
//   int client_uid;
//   int sender_timestamp;
//   int receiver_timestamp;
// } message;

client *clients[MAX_CLIENTS];  // Lista dei client
// message *messages[MESSAGES_QUEUE_SIZE];  // Coda dei messaggi
Node *messages = NULL;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t messages_mutex = PTHREAD_MUTEX_INITIALIZER;

FILE *fp;

/* Aggiunge un client alla coda */
void clients_queue_add(client *cl) {
  pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (!clients[i]) {
      clients[i] = cl;  // Aggiunge un cliente in coda
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
      clients[i] = NULL;           // Rimuove il client dalla coda
      break;
    }
  }

  pthread_mutex_unlock(&clients_mutex);  // Rilascia la lock
}

/* Invia un messaggio a tutti i client ad eccezione di quello con uid
 * specificato */
void send_message(char *s, int uid) {
  pthread_mutex_lock(&clients_mutex);  // Acquisisce la lock

  fprintf(fp, "%s\n", s);

  for (int i = 0; i < MAX_CLIENTS; ++i) {
    if (clients[i]) {
      if (clients[i]->uid != uid) {  // Se l'uid non corrisponde al mittente
        if (write(clients[i]->sockfd, s, strlen(s)) <
            0) {  // Invia il messaggio sulla socket del client
          perror("ERROR: write to descriptor failed");
          break;
        }
      }
    }
  }

  pthread_mutex_unlock(&clients_mutex);  // Rilascia la lock
}

// /* Aggiunge un messaggio alla coda */
// void messages_queue_add(message *message) {
//   pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock

//   for (int i = 0; i < MAX_CLIENTS; ++i) {
//     if (!messages[i]) {
//       messages[i] = message;  // Aggiunge un cliente in coda
//       break;
//     }
//   }

//   pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock
// }

// /* Rimuove un messaggio dalla coda */
// void messages_queue_remove() {
//   pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock
//   int max_timestamp = 0, max_index = -1;

//   for (int i = 0; i < MESSAGES_QUEUE_SIZE; ++i) {
//     if (messages[i] && messages[i]->receiver_timestamp > max_timestamp) {
//       max_timestamp = messages[i]->receiver_timestamp;
//       max_index = i;
//     }
//   }

//   if (max_index != -1) {
//     send_message(messages[max_index]->message,
//     messages[max_index]->client_uid); free(messages[max_index]);
//     messages[max_index] = NULL;
//   }

//   pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock
// }

// /* Rimuove un messaggio dalla coda */
// void messages_queue_new(message *message) {
//   pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock
//   int max_timestamp = 0, max_index = -1;

//   for (int i = 0; i < MESSAGES_QUEUE_SIZE; ++i) {
//     if (messages[i] && messages[i]->receiver_timestamp > max_timestamp) {
//       max_timestamp = messages[i]->receiver_timestamp;
//       max_index = i;
//     }
//   }

//   if (max_index != -1 &&
//       messages[max_index]->receiver_timestamp > message->receiver_timestamp)
//       {
//     send_message(messages[max_index]->message,
//     messages[max_index]->client_uid); free(messages[max_index]);
//     messages[max_index] = message;
//   } else {
//     send_message(message->message, message->client_uid);
//     free(message);
//   }

//   pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock
// }

/* La funzione gestisce le interazione del server con il client */
void *handle_client(void *arg) {
  char buff_out[BUFFER_SZ];               // Buffer di output sul server
  char *name = calloc(32, sizeof(char));  // Nickname temporaneo del client
  int leave_flag = 0;                     // Flag di validita' del client

  cli_count++;
  client *cli = (client *)arg;

  // Name
  if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 ||
      strlen(name) >= 32 - 1) {
    printf("Il client non ha inserito un nickname.\n");
    leave_flag = 1;
  } else {
    // strcpy(cli->name, name);  // Copia il nickname nella struttura client
    cli->name = name;
    sprintf(buff_out, "%s e' entrato nella chatoroom.\n",
            cli->name);                // Compone la stringa in un buffer
    printf("%s", buff_out);            // Stampa il messaggio sul server
    send_message(buff_out, cli->uid);  // Invia a tutti i client la
                                       // comunicazione di un nuovo client
  }

  bzero(buff_out, BUFFER_SZ);  // Svuota il buffer

  while (1) {
    if (leave_flag) {  // Se si e' verificato un errore il client viene scartato
      break;
    }

    int receive = recv(cli->sockfd, buff_out, BUFFER_SZ,
                       0);  // Riceve un messaggio e lo memorizza nel buffer
    if (receive > 0) {
      if (strlen(buff_out) > 0) {
        // send_message(buff_out, cli->uid);  // Inoltra il messaggio tranne che
        // al mittente
        char *message = calloc(2048, sizeof(char));
        long int timestamp;

        sscanf(buff_out, "%[^':']:%ld", message, &timestamp);

        pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock

        if (mode == 0) {
          timestamp = get_current_time();
        }

        push(&messages, message, cli->uid, name, timestamp);

        pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock

        // message *msg = malloc(sizeof(message));

        // char subbuff[5];
        // memcpy(msg->message, &buff_out[6], BUFFER_SZ - 6);
        // subbuff[4] = '\0';

        // strcpy(msg->message, buff_out);
        // msg->receiver_timestamp = (int)time(NULL);
        // msg->client_uid = cli->uid;
        // messages_queue_new(msg);

        str_trim_lf(buff_out, strlen(buff_out));
        printf("%s: %s\n", name, message);
      }
    } else if (receive == 0 || strcmp(buff_out, "exit") == 0) {
      sprintf(buff_out, "%s ha lasciato la chat.\n", cli->name);
      printf("%s", buff_out);
      send_message(buff_out, cli->uid);
      leave_flag = 1;
    } else {
      printf("ERROR: -1\n");
      leave_flag = 1;
    }

    bzero(buff_out, BUFFER_SZ);
  }

  /* Delete client from queue and yield thread */
  close(cli->sockfd);
  clients_queue_remove(cli->uid);
  free(cli);
  free(name);
  cli_count--;
  pthread_detach(pthread_self());

  return NULL;
}

// Gestisce l'invio dei messaggi
void *handle_send_message(void *arg) {
  char *message = calloc(BUFFER_SZ + 35, sizeof(char));
  while (1) {
    pthread_mutex_lock(&messages_mutex);  // Acquisisce la lock
    if (!isEmpty(&messages)) {
      bzero(message, BUFFER_SZ + 35);
      sprintf(message, "%s> %s\n", messages->user_name, messages->message);
      send_message(message, messages->uid);
      pop(&messages);
    }
    pthread_mutex_unlock(&messages_mutex);  // Rilascia la lock
    if (mode == 1) {
      sleep(5);
    }
  }
  free(message);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("Usage: %s <port> <mode>\n", argv[0]);
    return EXIT_FAILURE;
  }

  char *ip = "127.0.0.1";   // Indirizzo locale del server
  int port = atoi(argv[1]); /* Viene convertito l'argomento stringa in intero
                             * (valore della porta) */
  mode = atoi(argv[2]);

  if (mode != 0 && mode != 1) {
    printf(
        "MODALITÀ: \n 0) Timestamp del server.\n1) Timebstamp del client.\n");
    return EXIT_FAILURE;
  }
  int option = 1, listenfd = 0, connfd = 0;
  struct sockaddr_in serv_addr;
  struct sockaddr_in cli_addr;
  pthread_t tid;

  /* Socket settings */
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = inet_addr(ip);
  serv_addr.sin_port = htons(port);

  /* Ignore pipe signals */
  signal(SIGPIPE, SIG_IGN);

  if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR),
                 (char *)&option, sizeof(option)) < 0) {
    perror("ERROR: setsockopt failed");
    return EXIT_FAILURE;
  }

  /* Si effettua il bind dell'indirizzo locale alla socket.  */
  if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("[ERRORE] Impossibile effettuare il binding.");
    return EXIT_FAILURE;
  }

  /*
   * Viene messo in ascolto il server.
   * 10 richieste dai cliente verranno messere in coda
   */
  if (listen(listenfd, 10) < 0) {
    perror("[ERRORE] Impossibile effettuare il listening.");
    return EXIT_FAILURE;
  }

  fp = open_file();

  /* Aggiunge il client alla coda e crea il trhead */
  pthread_create(&tid, NULL, &handle_send_message, NULL);

  printf("=== BENVENUTO NELLA CHATROOM ===\n");

  while (1) {
    socklen_t clilen = sizeof(cli_addr);
    connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

    /* Verifica se e' stato raggiunto il numero massimo di client. */
    if ((cli_count + 1) == MAX_CLIENTS) {
      printf("Raggiunto il numero massimo di client. Rifiutato: ");
      print_client_addr(cli_addr);
      printf(":%d\n", cli_addr.sin_port);
      close(connfd);
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