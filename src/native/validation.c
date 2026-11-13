#include "validation.h"

int vkaether_valid_identity(int64_t peer_id, int64_t user_id) {
    return peer_id > 0 && user_id > 0;
}
