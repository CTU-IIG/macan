#include <stdint.h>
#include <string.h>

#include "macan.h"

#include "klee.h"

/**
 * Simplifies cmac -> message is automagically going to have both the correct and the incorrect cmac anyway.
 */
void macan_aes_cmac(const struct macan_key *key, size_t length, uint8_t *dst, uint8_t *src)
{
    (void)key, (void)length, (void)dst, (void)src;
    memset(dst, 0, 16);
}

/*
 * macan_aes_encrypt() - encrypt block of data
 */
void macan_aes_encrypt(const struct macan_key *key, size_t len, uint8_t *dst, const uint8_t *src)
{
    (void)key, (void)src;
    klee_make_symbolic(dst, len, "aes encryption");
}

/*
 * macan_aes_decrypt() - decrypt block of data
 */
void macan_aes_decrypt(const struct macan_key *key, size_t len, uint8_t *dst, const uint8_t *src)
{
    (void)key, (void)src;
    klee_make_symbolic(dst, len, "aes decryption");
}

