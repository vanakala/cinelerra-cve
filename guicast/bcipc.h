#ifndef BCIPC_H
#define BCIPC_H

#include "arraylist.h"
#include "mutex.h"

#include "bcipc.h"
#include <signal.h>

// These must be atomic routines

int bc_init_ipc();
int bc_enter_shmem_id(int id);
int bc_remove_shmem_id(int id);
int bc_enter_sema_id(int id);
int bc_remove_sema_id(int id);
int bc_enter_msg_id(int id);
int bc_remove_msg_id(int id);

#endif
