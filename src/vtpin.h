#pragma once

void handle_termination(int isignal, siginfo_t *pssiginfo, void *psContext);

__attribute__((destructor)) static void print_vtpin_stats(void);

/* Used to check whether vtpin is actively loaded or not */
static bool volatile vtpin_system_in_place = false;