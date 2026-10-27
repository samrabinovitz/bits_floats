#include "assignment.h"

#include <math.h>
#include <stddef.h>

unsigned bitset_symmetric_difference_count(uint32_t a, uint32_t b) {
    uint32_t diff = a^b;
    unsigned count = 0;
    while (diff != 0) {
        diff &= diff - 1u;
        count++;
    }
    return count;
}

uint32_t bit_range_mask(unsigned low, unsigned high) {
    uint32_t ones = 0xFFFFFFFFu >> (31u - (high - low));
    return ones << low;
}

uint32_t splice_bits(
    uint32_t destination,
    uint32_t source,
    unsigned low,
    unsigned high
) {
    uint32_t mask = bit_range_mask(low,high);
    uint32_t cleared = destination & ~mask;
    uint32_t piece = (source << low) & mask;
    return cleared | piece;
}

int32_t decode_twos_complement(uint32_t bits, unsigned width) {
    uint32_t mask = bit_range_mask(0, width - 1u);
    uint32_t value = bits & mask;
    uint32_t sign = (value >> (width - 1u)) & 1u;
    if (sign == 0) {
        return (int32_t) value;
    }
    uint32_t magnitude = (~value + 1u) & mask;
    if (magnitude == 0x80000000u) {
        return INT32_MIN;
    }
    return -(int32_t) magnitude;
}

bool add_signed_width(
    uint32_t a_bits,
    uint32_t b_bits,
    unsigned width,
    uint32_t *sum_bits
) {
    uint32_t mask = bit_range_mask(0, width - 1u);
    uint32_t a = a_bits & mask;
    uint32_t b = b_bits & mask;
    uint32_t sum = (a + b) & mask;
    uint32_t sign_bit = 1u << (width - 1u);
    *sum_bits = sum;
    return ((a ^ sum) & (b ^ sum) & sign_bit) != 0;
}

int compare_signed_unsigned_math(
    int32_t signed_value,
    uint32_t unsigned_value
) {
    if (signed_value < 0) {
        return -1;
    }
    uint32_t s = (uint32_t)signed_value;
    if (s < unsigned_value) {
        return -1;
    }
    if (s > unsigned_value) {
        return 1;
    }
    return 0;
}

uint32_t multiply_by_24(uint32_t x) {
    return (x << 5) - (x << 3);
}

fp32_info_t fp32_decode(uint32_t bits) {
    fp32_info_t info;
    uint32_t exp = (bits >> 23) & 0xFFu;
    uint32_t frac = bits & 0x7FFFFFu;
    info.negative = (bits >> 31) != 0;
    info.exponent_bits = (uint8_t)exp;
    info.fraction_bits = frac;
    if (exp == 0) {
        info.kind = (frac == 0) ? FP32_ZERO : FP32_SUBNORMAL;
        info.unbiased_exponent = -126;
    } else if (exp == 255) {
        info.kind = (frac == 0) ? FP32_INFINITY : FP32_NAN;
        info.unbiased_exponent = 0;
    } else {
        info.kind = FP32_NORMAL;
        info.unbiased_exponent = (int)exp - 127;
    }
    return info;
}

double fp32_exact_value(uint32_t bits) {
    fp32_info_t info = fp32_decode(bits);
    double value;
    switch (info.kind) {
        case FP32_NAN:
            return NAN;
        case FP32_INFINITY:
            value = INFINITY;
            break;
        case FP32_ZERO:
            value = 0.0;
            break;
        case FP32_SUBNORMAL:
            value = ldexp((double)info.fraction_bits, -149);
            break;
        default:
        value = ldexp((double)(info.fraction_bits | 0x800000u), info.unbiased_exponent - 23);
        break;
    }
    return info.negative ? -value : value;
}

uint32_t fp32_next_up_bits(uint32_t bits) {
    uint32_t magnitude = bits & 0x7FFFFFFFu;
    if (magnitude > 0x7F800000u) {
        return bits;
    }
    if (bits == 0x7F800000u) {
        return bits;
    }
    if (magnitude == 0) {
        return 0x00000001u;
    }
    if (bits & 0x80000000u) {
        return bits - 1u;
    }
    return bits + 1u;
}

bool fp32_truncate_to_int(uint32_t bits, int32_t *out) {
    bool negative = (bits >> 31) != 0;
    uint32_t exp = (bits >> 23) & 0xFFu;
    uint32_t frac = bits & 0x7FFFFFu;
    if (exp == 255) {
        return false;
    }
    if (exp < 127) {
        *out = 0;
        return true;
    }
    int e = (int)exp - 127;
    if (e >= 31) {
        if (negative && e == 31 && frac == 0) {
            *out = INT32_MIN;
            return true;
        }
        return false;
    }
    uint32_t significand = frac | 0x800000u;
    uint32_t magnitude;
    if (e >= 23) {
        magnitude = significand << (e - 23);
    } else {
        magnitude = significand >> (23 - e);
    }
    *out = negative ? -(int32_t)magnitude : (int32_t)magnitude;
    return true;
}
