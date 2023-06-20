#ifndef SHA3_H
#define SHA3_H

#include <stdint.h>

using unsign64 = uint64_t;

typedef struct {
    int length;
    unsign64 S[25];
    int rate, len;
} sha3;

#define SHA3_HASH224 28
#define SHA3_HASH256 32
#define SHA3_HASH384 48
#define SHA3_HASH512 64

#define SHAKE128 16
#define SHAKE256 32

extern void  SHA3_init(sha3 *H, int t);
extern void  SHA3_process(sha3 *H, int b);
extern void  SHA3_hash(sha3 *H, char *h);
extern void  SHA3_continuing_hash(sha3 *H, char *h);
extern void  SHA3_shake(sha3 *H, char *h, int len);
extern void  SHA3_continuing_shake(sha3 *H, char *h, int len);
extern void  SHA3_squeeze(sha3 *H, char *h, int len);


#endif // SHA3_H
