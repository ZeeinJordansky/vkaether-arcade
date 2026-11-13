#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* NativeAOT C ABI: 0=valid, 1=malformed text, 2=invalid pointer/length. */
int aether_policy_validate_message(const unsigned char* data, int length);

#ifdef __cplusplus
}
#endif
