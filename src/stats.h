#ifndef _STATS_H
#define _STATS_H

#include <stdint.h>
#include "fping.h"

int calc_pct(int64_t part, int64_t total);
int calc_loss_pct(int64_t sent, int64_t recv);
int64_t calc_outage_ms(int64_t sent, int64_t recv, int64_t perhost_interval_ns);

void print_per_system_stats(void);
void print_per_system_stats_json(void);
void print_global_stats(void);
void print_global_stats_json(void);
#endif
