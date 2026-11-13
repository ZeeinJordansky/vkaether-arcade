#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* All functions return explicit status/result codes; output pointers must be writable. */
int vkaether_rust_add(int64_t a, int64_t b, int64_t* out);
int vkaether_rust_mul(int64_t a, int64_t b, int64_t* out);
int vkaether_rust_div(int64_t a, int64_t b, int64_t* out);
int vkaether_rust_div_f64(int64_t a, int64_t b, double* out);
int vkaether_rust_pow(int64_t base, uint32_t exponent, int64_t* out);
int vkaether_rust_factorial(uint32_t n, uint64_t* out);
int vkaether_rust_is_prime(uint64_t n);
int vkaether_rust_rps(int player, int bot);

#ifdef __cplusplus
}
#endif
