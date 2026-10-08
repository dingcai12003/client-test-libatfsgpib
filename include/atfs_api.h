#ifndef ATFS_API_H
#define ATFS_API_H

#include <stddef.h>
#include <sys/types.h>

#include <UTSC_Common.h>
#include <UTSC_Gpib.h>
#include <UTSC_Rm.h>

typedef int (*utsc_gpib_open_fn)(char *, UTSC_Gpib *);
typedef int (*utsc_gpib_close_fn)(UTSC_Gpib);
typedef int (*utsc_gpib_send_data_fn)(UTSC_Gpib, void *, UTSC_size_t);
typedef int (*utsc_gpib_recv_data_fn)(UTSC_Gpib, void *, UTSC_size_t, UTSC_size_t *);
typedef int (*utsc_gpib_recv_stb_fn)(UTSC_Gpib, int *);
typedef int (*utsc_gpib_recv_stb_all_fn)(UTSC_Gpib, int *);
typedef int (*utsc_gpib_wait_srq_fn)(UTSC_Gpib, long);
typedef int (*utsc_gpib_clear_device_fn)(UTSC_Gpib);
typedef char *(*utsc_gpib_get_error_message_fn)(void);
typedef int (*utsc_rm_lock_fn)(char *, char *);
typedef int (*utsc_rm_trylock_fn)(char *, char *);
typedef int (*utsc_rm_unlock_fn)(char *);
typedef char *(*utsc_rm_get_error_message_fn)(void);
typedef void (*utsc_program_name_set_fn)(const char *);

#endif
