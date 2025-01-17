static void
aegis128l_init(const uint8_t *key, const uint8_t *nonce, aes_block_t *const state)
{
    static CRYPTO_ALIGN(16)
        const uint8_t c0_[] = { 0x00, 0x01, 0x01, 0x02, 0x03, 0x05, 0x08, 0x0d,
                                0x15, 0x22, 0x37, 0x59, 0x90, 0xe9, 0x79, 0x62 };
    static CRYPTO_ALIGN(16)
        const uint8_t c1_[] = { 0xdb, 0x3d, 0x18, 0x55, 0x6d, 0xc2, 0x2f, 0xf1,
                                0x20, 0x11, 0x31, 0x42, 0x73, 0xb5, 0x28, 0xdd };
    const aes_block_t c0    = AES_BLOCK_LOAD(c0_);
    const aes_block_t c1    = AES_BLOCK_LOAD(c1_);
    aes_block_t       k;
    aes_block_t       n;
    int               i;

    k = AES_BLOCK_LOAD(key);
    n = AES_BLOCK_LOAD(nonce);

    state[0] = AES_BLOCK_XOR(k, n);
    state[1] = c1;
    state[2] = c0;
    state[3] = c1;
    state[4] = AES_BLOCK_XOR(k, n);
    state[5] = AES_BLOCK_XOR(k, c0);
    state[6] = AES_BLOCK_XOR(k, c1);
    state[7] = AES_BLOCK_XOR(k, c0);
    for (i = 0; i < 10; i++) {
        aegis128l_update(state, n, k);
    }
}

static void
aegis128l_mac(uint8_t *mac, size_t maclen, size_t adlen, size_t mlen, aes_block_t *const state)
{
    aes_block_t tmp;
    int         i;

    tmp = AES_BLOCK_LOAD_64x2(((uint64_t) mlen) << 3, ((uint64_t) adlen) << 3);
    tmp = AES_BLOCK_XOR(tmp, state[2]);

    for (i = 0; i < 7; i++) {
        aegis128l_update(state, tmp, tmp);
    }

    if (maclen == 16) {
        tmp = AES_BLOCK_XOR(state[6], AES_BLOCK_XOR(state[5], state[4]));
        tmp = AES_BLOCK_XOR(tmp, AES_BLOCK_XOR(state[3], state[2]));
        tmp = AES_BLOCK_XOR(tmp, AES_BLOCK_XOR(state[1], state[0]));
        AES_BLOCK_STORE(mac, tmp);
    } else if (maclen == 32) {
        tmp = AES_BLOCK_XOR(state[3], state[2]);
        tmp = AES_BLOCK_XOR(tmp, AES_BLOCK_XOR(state[1], state[0]));
        AES_BLOCK_STORE(mac, tmp);
        tmp = AES_BLOCK_XOR(state[7], state[6]);
        tmp = AES_BLOCK_XOR(tmp, AES_BLOCK_XOR(state[5], state[4]));
        AES_BLOCK_STORE(mac + 16, tmp);
    } else {
        memset(mac, 0, maclen);
    }
}

static inline void
aegis128l_absorb(const uint8_t *const src, aes_block_t *const state)
{
    aes_block_t msg0, msg1;

    msg0 = AES_BLOCK_LOAD(src);
    msg1 = AES_BLOCK_LOAD(src + 16);
    aegis128l_update(state, msg0, msg1);
}

static void
aegis128l_enc(uint8_t *const dst, const uint8_t *const src, aes_block_t *const state)
{
    aes_block_t msg0, msg1;
    aes_block_t tmp0, tmp1;

    msg0 = AES_BLOCK_LOAD(src);
    msg1 = AES_BLOCK_LOAD(src + 16);
    tmp0 = AES_BLOCK_XOR(msg0, state[6]);
    tmp0 = AES_BLOCK_XOR(tmp0, state[1]);
    tmp1 = AES_BLOCK_XOR(msg1, state[5]);
    tmp1 = AES_BLOCK_XOR(tmp1, state[2]);
    tmp0 = AES_BLOCK_XOR(tmp0, AES_BLOCK_AND(state[2], state[3]));
    tmp1 = AES_BLOCK_XOR(tmp1, AES_BLOCK_AND(state[6], state[7]));
    AES_BLOCK_STORE(dst, tmp0);
    AES_BLOCK_STORE(dst + 16, tmp1);

    aegis128l_update(state, msg0, msg1);
}

static void
aegis128l_dec(uint8_t *const dst, const uint8_t *const src, aes_block_t *const state)
{
    aes_block_t msg0, msg1;

    msg0 = AES_BLOCK_LOAD(src);
    msg1 = AES_BLOCK_LOAD(src + 16);
    msg0 = AES_BLOCK_XOR(msg0, state[6]);
    msg0 = AES_BLOCK_XOR(msg0, state[1]);
    msg1 = AES_BLOCK_XOR(msg1, state[5]);
    msg1 = AES_BLOCK_XOR(msg1, state[2]);
    msg0 = AES_BLOCK_XOR(msg0, AES_BLOCK_AND(state[2], state[3]));
    msg1 = AES_BLOCK_XOR(msg1, AES_BLOCK_AND(state[6], state[7]));
    AES_BLOCK_STORE(dst, msg0);
    AES_BLOCK_STORE(dst + 16, msg1);

    aegis128l_update(state, msg0, msg1);
}

static void
aegis128l_declast(uint8_t *const dst, const uint8_t *const src, size_t len,
                  aes_block_t *const state)
{
    uint8_t     pad[32];
    aes_block_t msg0, msg1;

    memset(pad, 0, sizeof pad);
    memcpy(pad, src, len);

    msg0 = AES_BLOCK_LOAD(pad);
    msg1 = AES_BLOCK_LOAD(pad + 16);
    msg0 = AES_BLOCK_XOR(msg0, state[6]);
    msg0 = AES_BLOCK_XOR(msg0, state[1]);
    msg1 = AES_BLOCK_XOR(msg1, state[5]);
    msg1 = AES_BLOCK_XOR(msg1, state[2]);
    msg0 = AES_BLOCK_XOR(msg0, AES_BLOCK_AND(state[2], state[3]));
    msg1 = AES_BLOCK_XOR(msg1, AES_BLOCK_AND(state[6], state[7]));
    AES_BLOCK_STORE(pad, msg0);
    AES_BLOCK_STORE(pad + 16, msg1);

    memset(pad + len, 0, sizeof pad - len);
    memcpy(dst, pad, len);

    msg0 = AES_BLOCK_LOAD(pad);
    msg1 = AES_BLOCK_LOAD(pad + 16);

    aegis128l_update(state, msg0, msg1);
}

static int
encrypt_detached(uint8_t *c, uint8_t *mac, size_t maclen, const uint8_t *m, size_t mlen,
                 const uint8_t *ad, size_t adlen, const uint8_t *npub, const uint8_t *k)
{
    aes_block_t              state[8];
    CRYPTO_ALIGN(16) uint8_t src[32];
    CRYPTO_ALIGN(16) uint8_t dst[32];
    size_t                   i;

    aegis128l_init(k, npub, state);

    for (i = 0ULL; i + 32ULL <= adlen; i += 32ULL) {
        aegis128l_absorb(ad + i, state);
    }
    if (adlen & 0x1f) {
        memset(src, 0, 32);
        memcpy(src, ad + i, adlen & 0x1f);
        aegis128l_absorb(src, state);
    }
    for (i = 0ULL; i + 32ULL <= mlen; i += 32ULL) {
        aegis128l_enc(c + i, m + i, state);
    }
    if (mlen & 0x1f) {
        memset(src, 0, 32);
        memcpy(src, m + i, mlen & 0x1f);
        aegis128l_enc(dst, src, state);
        memcpy(c + i, dst, mlen & 0x1f);
    }

    aegis128l_mac(mac, maclen, adlen, mlen, state);

    return 0;
}

static int
decrypt_detached(uint8_t *m, const uint8_t *c, size_t clen, const uint8_t *mac, size_t maclen,
                 const uint8_t *ad, size_t adlen, const uint8_t *npub, const uint8_t *k)
{
    aes_block_t              state[8];
    CRYPTO_ALIGN(16) uint8_t src[32];
    CRYPTO_ALIGN(16) uint8_t dst[32];
    CRYPTO_ALIGN(16) uint8_t computed_mac[32];
    const size_t             mlen = clen;
    size_t                   i;
    int                      ret;

    aegis128l_init(k, npub, state);

    for (i = 0ULL; i + 32ULL <= adlen; i += 32ULL) {
        aegis128l_absorb(ad + i, state);
    }
    if (adlen & 0x1f) {
        memset(src, 0, 32);
        memcpy(src, ad + i, adlen & 0x1f);
        aegis128l_absorb(src, state);
    }
    if (m != NULL) {
        for (i = 0ULL; i + 32ULL <= mlen; i += 32ULL) {
            aegis128l_dec(m + i, c + i, state);
        }
    } else {
        for (i = 0ULL; i + 32ULL <= mlen; i += 32ULL) {
            aegis128l_dec(dst, c + i, state);
        }
    }
    if (mlen & 0x1f) {
        if (m != NULL) {
            aegis128l_declast(m + i, c + i, mlen & 0x1f, state);
        } else {
            aegis128l_declast(dst, c + i, mlen & 0x1f, state);
        }
    }

    COMPILER_ASSERT(sizeof computed_mac >= 32);
    aegis128l_mac(computed_mac, maclen, adlen, mlen, state);
    ret = -1;
    if (maclen == 16) {
        ret = aegis_verify_16(computed_mac, mac);
    } else if (maclen == 32) {
        ret = aegis_verify_32(computed_mac, mac);
    }
    if (ret != 0 && m != NULL) {
        memset(m, 0, mlen);
    }
    return ret;
}

typedef struct _aegis128l_state {
    aes_block_t state[8];
    uint8_t     buf[32];
    uint64_t    adlen;
    uint64_t    mlen;
    size_t      pos;
} _aegis128l_state;

static void
state_init(aegis128l_state *st_, const uint8_t *ad, size_t adlen, const uint8_t *npub,
           const uint8_t *k)
{
    _aegis128l_state *const st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    size_t i;

    COMPILER_ASSERT((sizeof *st) + 15 <= sizeof *st_);
    st->mlen = 0;
    st->pos  = 0;

    aegis128l_init(k, npub, st->state);
    for (i = 0ULL; i + 32ULL <= adlen; i += 32ULL) {
        aegis128l_absorb(ad + i, st->state);
    }
    if (adlen & 0x1f) {
        memset(st->buf, 0, 32);
        memcpy(st->buf, ad + i, adlen & 0x1f);
        aegis128l_absorb(st->buf, st->state);
    }
    st->adlen = adlen;
}

static int
state_encrypt_update(aegis128l_state *st_, uint8_t *c, size_t clen_max, size_t *written,
                     const uint8_t *m, size_t mlen)
{
    _aegis128l_state *const st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    size_t i = 0;
    size_t left;

    *written = 0;
    if (clen_max < (mlen & ~(size_t) 0x1f)) {
        errno = ERANGE;
        return -1;
    }
    st->mlen += mlen;
    if (st->pos != 0) {
        const size_t available = (sizeof st->buf) - st->pos;
        const size_t n         = mlen < available ? mlen : available;

        if (n != 0) {
            memcpy(st->buf + st->pos, m + i, n);
            mlen -= n;
            st->pos += n;
        }
        if (st->pos == sizeof st->buf) {
            aegis128l_enc(c, st->buf, st->state);
            *written += 32;
            c += 32;
            st->pos = 0;
        } else {
            return 0;
        }
    }
    for (i = 0; i + 32 < mlen; i += 32) {
        aegis128l_enc(c + i, m + i, st->state);
    }
    *written += mlen & ~(size_t) 0x1f;
    left = mlen & 0x1f;
    if (left != 0) {
        memcpy(st->buf, m + i, left);
        st->pos = left;
    }
    return 0;
}

static int
state_encrypt_detached_final(aegis128l_state *st_, uint8_t *c, size_t clen_max, size_t *written,
                             uint8_t *mac, size_t maclen)
{
    _aegis128l_state *const st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    CRYPTO_ALIGN(16) uint8_t src[32];
    CRYPTO_ALIGN(16) uint8_t dst[32];

    *written = 0;
    if (clen_max < st->pos) {
        errno = ERANGE;
        return -1;
    }
    if (st->pos != 0) {
        memset(src, 0, sizeof src);
        memcpy(src, st->buf, st->pos);
        aegis128l_enc(dst, src, st->state);
        memcpy(c, dst, st->pos);
    }
    aegis128l_mac(mac, maclen, st->adlen, st->mlen, st->state);

    *written = st->pos;

    return 0;
}

static int
state_encrypt_final(aegis128l_state *st_, uint8_t *c, size_t clen_max, size_t *written,
                    size_t maclen)
{
    _aegis128l_state *const st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    CRYPTO_ALIGN(16) uint8_t src[32];
    CRYPTO_ALIGN(16) uint8_t dst[32];

    *written = 0;
    if (clen_max < st->pos + maclen) {
        errno = ERANGE;
        return -1;
    }
    if (st->pos != 0) {
        memset(src, 0, sizeof src);
        memcpy(src, st->buf, st->pos);
        aegis128l_enc(dst, src, st->state);
        memcpy(c, dst, st->pos);
    }
    aegis128l_mac(c + st->pos, maclen, st->adlen, st->mlen, st->state);

    *written = st->pos + maclen;

    return 0;
}

static int
state_decrypt_detached_update(aegis128l_state *st_, uint8_t *m, size_t mlen_max, size_t *written,
                              const uint8_t *c, size_t clen)
{
    _aegis128l_state *const st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    CRYPTO_ALIGN(16) uint8_t dst[32];
    size_t                   i = 0;
    size_t                   left;
    const size_t             mlen = clen;

    *written = 0;
    if (mlen_max < (clen & ~(size_t) 0x1f)) {
        errno = ERANGE;
        return -1;
    }
    st->mlen += mlen;
    if (st->pos != 0) {
        const size_t available = (sizeof st->buf) - st->pos;
        const size_t n         = clen < available ? clen : available;

        if (n != 0) {
            memcpy(st->buf + st->pos, m + i, n);
            clen -= n;
            st->pos += n;
        }
        if (st->pos == (sizeof st->buf)) {
            if (m != NULL) {
                aegis128l_dec(m, st->buf, st->state);
            } else {
                aegis128l_dec(dst, st->buf, st->state);
            }
            *written += 32;
            c += 32;
            st->pos = 0;
        } else {
            return 0;
        }
    }
    if (m != NULL) {
        for (i = 0; i + 32 < clen; i += 32) {
            aegis128l_dec(m + i, c + i, st->state);
        }
    } else {
        for (i = 0; i + 32 < clen; i += 32) {
            aegis128l_dec(dst, c + i, st->state);
        }
    }
    *written += mlen & ~(size_t) 0x1f;
    left = mlen & 0x1f;
    if (left) {
        memcpy(st->buf, c + i, left);
        st->pos = left;
    }
    return 0;
}

static int
state_decrypt_detached_final(aegis128l_state *st_, uint8_t *m, size_t mlen_max, size_t *written,
                             const uint8_t *mac, size_t maclen)
{
    CRYPTO_ALIGN(16) uint8_t computed_mac[32];
    CRYPTO_ALIGN(16) uint8_t dst[32];
    _aegis128l_state *const  st =
        (_aegis128l_state *) ((((uintptr_t) &st_->opaque) + 15) & ~(uintptr_t) 15);
    int ret;

    *written = 0;
    if (mlen_max < st->pos) {
        errno = ERANGE;
        return -1;
    }
    if (st->pos != 0) {
        if (m != NULL) {
            aegis128l_declast(m, st->buf, st->pos, st->state);
        } else {
            aegis128l_declast(dst, st->buf, st->pos, st->state);
        }
    }
    aegis128l_mac(computed_mac, maclen, st->adlen, st->mlen, st->state);
    ret = -1;
    if (maclen == 16) {
        ret = aegis_verify_16(computed_mac, mac);
    } else if (maclen == 32) {
        ret = aegis_verify_32(computed_mac, mac);
    }
    if (ret == 0) {
        *written = st->pos;
    } else {
        memset(m, 0, st->pos);
    }
    return ret;
}
