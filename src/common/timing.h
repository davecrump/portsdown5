#ifndef __TIMING_H__
#define __TIMING_H__

#include <stdint.h>

uint64_t monotonic_ms(void);

uint64_t timestamp_ms(void);

void sleep_ms(uint32_t _duration);

uint64_t monotonic_ns(void);

uint64_t monotonic_us(void);

#endif /* __TIMING_H__ */

