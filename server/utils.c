#include <inttypes.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <time.h>

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

/**
 * Apre il file di log (relativo alla data) in modalita' append
 */
FILE *open_file() {
    char date[10];
    int day, month, year;
    char path[50];
    time_t now;  // tipo temporale, che consente di svolgeri calcoli aritmetici

    time(&now);  // Ritorna l'ora attuale del sistema e lal memorizza in now

    // localtime converts a `time_t` value to calendar time and
    // returns a pointer to a `tm` structure with its members
    // filled with the corresponding values
    struct tm *local = localtime(&now);

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