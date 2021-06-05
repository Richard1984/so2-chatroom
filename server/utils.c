#include <netinet/in.h>
#include <stdio.h>

void str_overwrite_stdout() {
  printf("\r%s", "> ");
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

void print_client_addr(struct sockaddr_in addr) {
  printf("%d.%d.%d.%d", addr.sin_addr.s_addr & 0xff,
         (addr.sin_addr.s_addr & 0xff00) >> 8,
         (addr.sin_addr.s_addr & 0xff0000) >> 16,
         (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

#include <stdio.h>
#include <time.h>

FILE *open_file() {
  char date[10];
  int day, month, year;
  char path[50];

  // `time_t` is an arithmetic time type
  time_t now;

  // Obtain current time
  // `time()` returns the current time of the system as a `time_t` value
  time(&now);

  // localtime converts a `time_t` value to calendar time and
  // returns a pointer to a `tm` structure with its members
  // filled with the corresponding values
  struct tm *local = localtime(&now);

  day = local->tm_mday;          // get day of month (1 to 31)
  month = local->tm_mon + 1;     // get month of year (0 to 11)
  year = local->tm_year + 1900;  // get year since 1900

  sprintf(date, "%d-%d-%d", day, month, year);
  sprintf(path, "log-%s.txt", date);

  return fopen(path, "a");
}