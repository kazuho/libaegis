#ifndef implementations_H
#define implementations_H

#include <stddef.h>
#include <stdint.h>

#include "aegis128l.h"

typedef struct aegis128l_implementation {
    int (*encrypt_detached)(uint8_t *c, uint8_t *mac, size_t maclen, const uint8_t *m, size_t mlen,
                            const uint8_t *ad, size_t adlen, const uint8_t *npub, const uint8_t *k);
    int (*decrypt_detached)(uint8_t *m, const uint8_t *c, size_t clen, const uint8_t *mac,
                            size_t maclen, const uint8_t *ad, size_t adlen, const uint8_t *npub,
                            const uint8_t *k);
    void (*state_init)(aegis128l_state *st_, const uint8_t *ad, size_t adlen, const uint8_t *npub,
                       const uint8_t *k);
    size_t (*state_encrypt_update)(aegis128l_state *st_, uint8_t *c, const uint8_t *m, size_t mlen);
    size_t (*state_encrypt_detached_final)(aegis128l_state *st_, uint8_t *c, uint8_t *mac,
                                           size_t maclen);
    size_t (*state_encrypt_final)(aegis128l_state *st_, uint8_t *c, size_t maclen);
} aegis128l_implementation;

#endif