#include <inttypes.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

void str_overwrite_stdout() {
    printf("\r%s", "> ");
    fflush(stdout);
}

/* Rimuove il carattere newline da una stringa */
void string_remove_newline(char *string) {
    string[strcspn(string, "\n")] = '\0';
}

/**
 * Apre il file di log (relativo alla data) in modalità append
 */
FILE *open_file() {
    char date[10];
    int day, month, year;
    char path[50];
    time_t now;  // tipo temporale, che consente di svolgeri calcoli aritmetici

    time(&now);  // Ritorna l'ora attuale del sistema e lal memorizza in now

    struct tm *local = localtime(&now);  // Legge l'ora e la converte in una struttura "calendario" locale

    day = local->tm_mday;          // get day of month (1 to 31)
    month = local->tm_mon + 1;     // get month of year (0 to 11)
    year = local->tm_year + 1900;  // get year since 1900

    sprintf(date, "%d-%d-%d", day, month, year);
    sprintf(path, "log-%s.log.txt", date);

    return fopen(path, "a");
}

/**
 * Restituisce l'ora attuale in millisecondi
 */
long get_current_time(void) {
    long milliseconds;     // Millisecondi
    time_t seconds;        // Secondi
    struct timespec spec;  // Struttura timestamp con nanosecondi

    clock_gettime(CLOCK_REALTIME, &spec);  // Legge il tempo corrente

    seconds = spec.tv_sec;
    milliseconds = round(spec.tv_nsec / 1.0e6);  // Converte i nanosecondi in millisecondi

    return (long)seconds * 1000 + milliseconds;
}