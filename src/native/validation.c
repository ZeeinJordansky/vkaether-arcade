#include "validation.h"

int vkaether_valid_identity(int64_t peer_id, int64_t user_id) {
    return peer_id > 0 && user_id > 0;
}

int vkaether_validate_event(int64_t peer_id, int64_t user_id, int64_t text_length) {
    return vkaether_valid_identity(peer_id, user_id) && text_length > 0 && text_length <= 4096;
}
