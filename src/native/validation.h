#ifndef VKAETHER_VALIDATION_H
#define VKAETHER_VALIDATION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 when both VK identifiers are positive, otherwise 0. */
int vkaether_valid_identity(int64_t peer_id, int64_t user_id);

#ifdef __cplusplus
}
#endif

#endif
