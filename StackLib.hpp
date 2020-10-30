#pragma once
#include <cstdint>
#include <cstddef>
#define RELEASE_LEVEL 0
#define DUMP_LEVEL 1
#define CANARY_LEVEL 2
#define HASH_LEVEL 3

#if DEBUG_LEVEL >= DUMP_LEVEL
#include "logs.hpp"
#endif

typedef unsigned long long canary_t;
typedef unsigned long long hash_t;
const canary_t cuckoo_standard = 0x9DEADBEEFBADF00D;

static char stack_fail_code[][80] = 
/* 0*/{"ok",
/* 1*/ "FAIL: NULL pointer passed",
/* 2*/ "FAIL: Stack has been attacked from left (cuckoo_low departed)",
/* 3*/ "FAIL: Stack has been attacked from right (cuckoo_high departed)",
/* 4*/ "FAIL: Stack has been corrupted (hashsum not valid)",
/* 5*/ "FAIL: Stack size less than zero (possibly destructed before)",
/* 6*/ "FAIL: Stack capacity less than zero (possibly destructed before)",
/* 7*/ "FAIL: Stack size is more than capacity",
/* 8*/ "FAIL: Stack pointer is not valid",
/* 9*/ "FAIL: Stack buffer has been attacked from left (cuckoo_low departed)",
/*10*/ "FAIL: Stack buffer has been attacked from left (cuckoo_high departed)",
/*11*/ "FAIL: Stack buffer has been corrupted (hashsum not valid)"};

///--------------------------------------------------------------------
/// Calculate polinomial hash of byte string from in [begin, end) range
/// Doesn't check any boundaries, cause I don't know how.
/// Anyway, it shouldn't ruin any memory contents.
/// 
/// @param begin    begining of the range
/// @param end      end of the range
/// 
/// @return     polinomial hash.
///--------------------------------------------------------------------
hash_t calc_hash(char *begin, char *end) {
   const uint64_t prime = 353; //Just a radom prime number
   uint64_t multiplicator = 1;
   hash_t hash = 0;
   for (char *ptr = begin; ptr < end; ptr++) {
        hash += *ptr * multiplicator;
        multiplicator *= prime;
   }
   return hash;
}

static char ptr_poisons[][50] = {"Correct pointer", "Destructed by stack_destruct()"};

int pointer_poison_name(void * p) {
    uint64_t code = (uint64_t)p;
    if (1 <= code && code <= sizeof(ptr_poisons) / sizeof(ptr_poisons[0])) {
        return code;
    }
    return 0;
}


#if DEBUG_LEVEL >= DUMP_LEVEL
void to_str(int x, char *buf) {
    sprintf(buf, "%d", x);
}

void to_str(unsigned int x, char *buf) {
    sprintf(buf, "%u", x);
}

void to_str(long long x, char *buf) {
    sprintf(buf, "%lld", x);
}

void to_str(unsigned long long x, char *buf) {
    sprintf(buf, "%llu", x);
}

void to_str(float x, char *buf) {
    sprintf(buf, "%f", x);
}

void to_str(double x, char *buf) {
    sprintf(buf, "%lf", x);
}

void to_str(long double x, char *buf) {
    sprintf(buf, "%Lf", x);
}

#endif
