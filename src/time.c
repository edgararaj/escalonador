#include "time.h"

#define NANOSECONDS_IN_SECOND 1000000000L
#define NANOSECONDS_IN_MILLISECOND 1000000L

long get_delta_ms(struct timespec ts_start, struct timespec ts_end)
{
    // Calculate time difference
    long seconds = ts_end.tv_sec - ts_start.tv_sec;
    long nanoseconds = ts_end.tv_nsec - ts_start.tv_nsec;
    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += NANOSECONDS_IN_SECOND;
    }

    // Convert nanoseconds to milliseconds
    long milliseconds = nanoseconds / NANOSECONDS_IN_MILLISECOND;

    return seconds * 1000 + milliseconds;
}