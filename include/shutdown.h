#ifndef SHUTDOWN_H
#define SHUTDOWN_H

#define INTERRUPTED_EXIT_CODE 130

void install_signal_handlers(void);
int is_interrupted(void);
void print_interrupted_message(void);

#endif
