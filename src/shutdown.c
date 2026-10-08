#define _POSIX_C_SOURCE 200112L

#include "shutdown.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>

static volatile sig_atomic_t interrupted = 0;

static void handle_shutdown_signal(int signal_number) {
    (void)signal_number;
    interrupted = 1;
}

void install_signal_handlers(void) {
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = handle_shutdown_signal;
    sigemptyset(&action.sa_mask);
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

int is_interrupted(void) {
    return interrupted != 0;
}

void print_interrupted_message(void) {
    printf("\nInterrupted, closing GPIB connection...\n");
}
