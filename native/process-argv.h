#ifndef FL_FRONT_PROCESS_ARGV_H
#define FL_FRONT_PROCESS_ARGV_H
#include "runtime.h"
/* Literal process boundary, not a shell or frontend policy. Timeout is in ms;
 * output limit applies separately to each stream, bounded by Native file IO. */
FLValue fl_front_run_argv(FLValue arguments, FLValue timeout, FLValue output_limit);
#endif
