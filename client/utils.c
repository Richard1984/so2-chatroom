#include <stdio.h>
#include <time.h>

FILE *open_file(char *name) {
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
  sprintf(path, "log-%s-%s.log.txt", name, date);

  return fopen(path, "a");
}