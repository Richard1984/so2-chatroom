#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Nodo */
typedef struct node {
    char* message;
    char* user_name;
    int uid;
    long int priority;  // Un valore più basso indica una priorità più alta
    struct node* next;
} Node;

/* Crea un nuovo nodo */
Node* newNode(char* message, int uid, char* user_name, long int priority) {
    Node* temp = malloc(sizeof(Node));  // Alloca la memoria per il nodo
    temp->uid = uid;
    temp->message = calloc(2048, sizeof(char));
    strcpy(temp->message, message);
    temp->user_name = calloc(32, sizeof(char));
    strcpy(temp->user_name, user_name);
    temp->priority = priority;
    temp->next = NULL;  // Imposta il successivo a NULL

    return temp;
}

/* Rimuove l'elemento con la priorità più alta dalla coda */
void pop(Node** head) {
    Node* temp = *head;       // Indirizzo dell'area puntata da head
    (*head) = (*head)->next;  // Sostituisce il primo con il successivo al primo
    free(temp->message);      // Libera la memoria del messaggio
    free(temp->user_name);    // Libera la memoria del nickname
    free(temp);               // Libera la memoria precedentemente puntata da head
}

/* Verifica se la coda sia vuota */
int isEmpty(Node** head) {
    return (*head) == NULL;
}

/* Inserisce un nodo nella coda in base alla priorità */
void push(Node** head, char* message, int uid, char* user_name, long int priority) {
    Node* start = (*head);

    // Crea un nuovo nodo
    Node* temp = newNode(message, uid, user_name, priority);

    if (isEmpty(head)) {
        (*head) = temp;  // Se la coda è vuota lo inserisci per primo
        return;
    }

    /** La "testa" della coda ha un valore di priorità più basso del nuovo nodo,
     * quindi il nuovo nodo viene inserito per primo e la "testa" viene usato 
     * come "next" nodo. 
     * */
    if ((*head)->priority > priority) {
        // Inserisce il nuovo nodo prima di head
        temp->next = *head;
        (*head) = temp;
    } else {
        // Attaraversa la coda e trova il post con la priorità adatta, altrimenti lo inserisci alla fine
        while (start->next != NULL && start->next->priority < priority) {
            start = start->next;
        }

        // Viene inserito il nuovo nella posizione trovata (eventualmente la fine della coda)
        temp->next = start->next;
        start->next = temp;
    }
}