
// C code to implement Priority Queue
// using Linked List
#include <stdio.h>
#include <stdlib.h>

// Node
typedef struct node {
  char* message;
  char* user_name;
  int uid;

  // Lower values indicate higher priority
  long int priority;

  struct node* next;

} Node;

// Function to Create A New Node
Node* newNode(char* message, int uid, char* user_name, long int priority) {
  Node* temp = malloc(sizeof(Node));
  temp->uid = uid;
  temp->message = message;
  temp->user_name = user_name;
  temp->priority = priority;
  temp->next = NULL;

  return temp;
}

// Return the value at head
char* peek(Node** head) {
  return strcat((*head)->user_name, strcat("> ", (*head)->message));
}

// Removes the element with the
// highest priority form the list
void pop(Node** head) {
  Node* temp = *head;
  (*head) = (*head)->next;
  free(temp);
}

// Function to check is list is empty
int isEmpty(Node** head) { return (*head) == NULL; }

// Function to push according to priority
void push(Node** head, char* message, int uid, char* user_name,
          long int priority) {
  Node* start = (*head);

  // Create new Node
  Node* temp = newNode(message, uid, user_name, priority);

  if (isEmpty(head)) {
    (*head) = temp;
    return;
  }

  // Special Case: The head of list has lesser
  // priority than new node. So insert new
  // node before head node and change head node.
  if ((*head)->priority > priority) {
    // Insert New Node before head
    temp->next = *head;
    (*head) = temp;
  } else {
    // Traverse the list and find a
    // position to insert new node
    while (start->next != NULL && start->next->priority < priority) {
      start = start->next;
    }

    // Either at the ends of the list
    // or at required position
    temp->next = start->next;
    start->next = temp;
  }
}

// Driver code
// int main() {
//   // Create a Priority Queue
//   // 7->4->5->6
//   Node* pq = newNode(4, 1);
//   push(&pq, 5, 2);
//   push(&pq, 6, 3);
//   push(&pq, 7, 0);

//   while (!isEmpty(&pq)) {
//     printf("%d ", peek(&pq));
//     pop(&pq);
//   }

//   return 0;
// }