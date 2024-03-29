#define _POSIX_C_SOURCE 200809L

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

/**
 * Apre il file di log (relativo all'utente e alla data) in modalità append
 */
FILE *open_file(char *name) {
    char date[10];
    int day, month, year;
    char path[50];
    time_t now;  // Tipo temporale, che consente di svolgeri calcoli aritmetici

    time(&now);  // Ritorna l'ora attuale del sistema e lal memorizza in now

    struct tm *local = localtime(&now);  // Legge l'ora e la converte in una struttura "calendario" locale

    day = local->tm_mday;          // restituisce il giorno del mese associato alla data (1 to 31)
    month = local->tm_mon + 1;     // restituisce il mese dell'anno associato alla data (0 to 11) + 1 (mesi da 1 a 12)
    year = local->tm_year + 1900;  // resituisce gli anni trascorsi dal 1900 + 1900 (anno assoluto)

    sprintf(date, "%d-%d-%d", day, month, year);     // Compone la data in formato giorno-mese-anno
    sprintf(path, "log-%s-%s.log.txt", name, date);  // Compone il nome del file di log

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

/* Rimuove il carattere newline da una stringa */
void string_remove_newline(char *string) {
    string[strcspn(string, "\n")] = '\0';
}

/* Predispone il "layout" per l'inserimento del messaggio */
void str_overwrite_stdout() {
    printf("%s", "> ");
    fflush(stdout);  // Pulisce il buffer
}