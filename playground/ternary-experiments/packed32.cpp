// Experiments for 32 balanced trits packed in one uint64_t.
// Encoding: -1=01, 0=00, +1=10, low trit in low two bits.
// This is an experiment, not the submitted runtime ABI.
#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <vector>
#include <immintrin.h>

using u64 = std::uint64_t;
using i64 = std::int64_t;
using u128 = unsigned __int128;
#define NOINLINE __attribute__((noinline))
constexpr u64 EVEN = 0x5555555555555555ULL;
constexpr u64 ODD = 0xAAAAAAAAAAAAAAAAULL;
constexpr i64 LIMIT = 926510094425920LL; // (3^32 - 1) / 2
static i64 decode_table(u64 x);

#ifdef WITH_ASM
extern "C" u64 asm_trit_neg(u64);
extern "C" u64 asm_trit_min(u64, u64);
extern "C" u64 asm_trit_max(u64, u64);
extern "C" u64 asm_trit_product(u64, u64);
extern "C" u64 asm_trit_cycle_inverse(u64);
extern "C" u64 asm_trit_plus_count_raw(u64);
extern "C" u64 asm_trit_add_bct(u64, u64);
#endif

static NOINLINE u64 trit_neg(u64 x) {
    return ((x & EVEN) << 1) | ((x & ODD) >> 1);
}

static NOINLINE u64 trit_min(u64 x, u64 y) {
    return ((x & y) & ODD) | ((x | y) & EVEN);
}

static NOINLINE u64 trit_max(u64 x, u64 y) {
    return ((x | y) & ODD) | ((x & y) & EVEN);
}

static NOINLINE u64 trit_min_xor(u64 x, u64 y) {
    return (x & y) | ((x ^ y) & EVEN);
}

static NOINLINE u64 trit_max_xor(u64 x, u64 y) {
    return (x & y) | ((x ^ y) & ODD);
}

static NOINLINE u64 trit_cycle(u64 x) {
    // -1 -> 0, 0 -> +1, +1 -> -1.
    u64 p = (x >> 1) & EVEN;
    u64 n = x & EVEN;
    return p | (((~(p | n)) & EVEN) << 1);
}

static NOINLINE u64 trit_product(u64 x, u64 y) {
    u64 xp = (x >> 1) & EVEN, xn = x & EVEN;
    u64 yp = (y >> 1) & EVEN, yn = y & EVEN;
    u64 positive = (xp & yp) | (xn & yn);
    u64 negative = (xp & yn) | (xn & yp);
    return (positive << 1) | negative;
}

static NOINLINE u64 trit_product_swar(u64 x, u64 y) {
    // A same-sign pair appears in x&y; an opposite-sign pair fills both
    // bits of a lane in x|y. This avoids unpacking four bit planes.
    u64 same = x & y;
    u64 any = x | y;
    u64 positive = (same | (same << 1)) & ODD;
    u64 negative = (any & (any >> 1)) & EVEN;
    return positive | negative;
}

static NOINLINE bool trit_less_highest_digit(u64 x, u64 y) {
    u64 differing = x ^ y;
    if (!differing) return false;
    unsigned shift = (63U - static_cast<unsigned>(__builtin_clzll(differing))) & ~1U;
    unsigned a = static_cast<unsigned>(x >> shift) & 3U;
    unsigned b = static_cast<unsigned>(y >> shift) & 3U;
    int da = static_cast<int>(a >> 1) - static_cast<int>(a & 1);
    int db = static_cast<int>(b >> 1) - static_cast<int>(b & 1);
    return da < db;
}

static NOINLINE bool trit_less_decode(u64 x, u64 y) {
    return decode_table(x) < decode_table(y);
}

static NOINLINE void min_batch_avx2(const u64 *x, const u64 *y, u64 *out,
                                    std::size_t count) {
    const __m256i even = _mm256_set1_epi64x(static_cast<i64>(EVEN));
    for (std::size_t i = 0; i < count; i += 4) {
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(x + i));
        __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(y + i));
        __m256i same = _mm256_and_si256(a, b);
        __m256i diff = _mm256_xor_si256(a, b);
        __m256i value = _mm256_or_si256(same, _mm256_and_si256(diff, even));
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i), value);
    }
}

static NOINLINE void product_batch_avx2(const u64 *x, const u64 *y, u64 *out,
                                        std::size_t count) {
    const __m256i odd = _mm256_set1_epi64x(static_cast<i64>(ODD));
    const __m256i even = _mm256_set1_epi64x(static_cast<i64>(EVEN));
    for (std::size_t i = 0; i < count; i += 4) {
        __m256i a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(x + i));
        __m256i b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(y + i));
        __m256i same = _mm256_and_si256(a, b);
        __m256i any = _mm256_or_si256(a, b);
        __m256i pos = _mm256_and_si256(_mm256_or_si256(same, _mm256_slli_epi64(same, 1)), odd);
        __m256i neg = _mm256_and_si256(_mm256_and_si256(any, _mm256_srli_epi64(any, 1)), even);
        _mm256_storeu_si256(reinterpret_cast<__m256i *>(out + i), _mm256_or_si256(pos, neg));
    }
}

static inline unsigned plus_count(u64 x) {
    return __builtin_popcountll(x & ODD);
}

static inline unsigned minus_count(u64 x) {
    return __builtin_popcountll(x & EVEN);
}

static inline unsigned zero_count(u64 x) {
    // Each valid nonzero trit contributes exactly one set bit.
    return 32 - __builtin_popcountll(x);
}

static NOINLINE u64 trit_cycle_user_inverse(u64 x) {
    // User's complement, subtract 01 in every lane, then swap sign bits.
    // This is the reverse cycle: -1 -> +1, 0 -> -1, +1 -> 0.
    return trit_neg(~x - EVEN);
}

static NOINLINE u64 trit_cycle_inverse(u64 x) {
    u64 n = x & EVEN;
    u64 p = (x >> 1) & EVEN;
    return (n << 1) | (~(n | p) & EVEN);
}

static inline u64 trit_shift_left(u64 x, unsigned k) {
    return k >= 32 ? 0 : x << (2 * k);
}

static inline u64 trit_shift_right(u64 x, unsigned k) {
    return k >= 32 ? 0 : x >> (2 * k);
}

static i64 decode(u64 x) {
    i64 value = 0, power = 1;
    for (int i = 0; i < 32; ++i) {
        u64 d = (x >> (2 * i)) & 3;
        assert(d != 3);
        value += (d == 2 ? 1 : d == 1 ? -1 : 0) * power;
        power *= 3;
    }
    return value;
}

static u64 encode(i64 value) {
    assert(-LIMIT <= value && value <= LIMIT);
    u64 biased = static_cast<u64>(value + LIMIT), result = 0;
    for (int i = 0; i < 32; ++i) {
        u64 d = biased % 3;
        biased /= 3;
        result |= (d == 0 ? 1ULL : d == 2 ? 2ULL : 0ULL) << (2 * i);
    }
    assert(biased == 0);
    return result;
}

static u64 trit_add_ripple(u64 x, u64 y) {
    u64 result = 0;
    int carry = 0;
    for (int i = 0; i < 32; ++i) {
        u64 dx = (x >> (2 * i)) & 3;
        u64 dy = (y >> (2 * i)) & 3;
        int sum = (dx == 2 ? 1 : dx == 1 ? -1 : 0)
                + (dy == 2 ? 1 : dy == 1 ? -1 : 0) + carry;
        carry = (sum > 1) - (sum < -1);
        int digit = sum - 3 * carry;
        result |= (digit > 0 ? 2ULL : digit < 0 ? 1ULL : 0ULL) << (2 * i);
    }
    return result;
}

static inline u64 to_biased_bct(u64 x) {
    // one-hot: -=01, 0=00, +=10; BCT: -=00, 0=01, +=10.
    return x ^ (~(x >> 1) & EVEN);
}

static inline u64 add_unsigned_bct(u64 a, u64 b) {
    // Jones' BCT correction scheme, extended with a 65th carry bit so that
    // the highest of 32 trits receives its correction too.
    u128 c = static_cast<u128>(a) + EVEN;
    u128 d = static_cast<u128>(b) + c;
    u128 no_carry = ~(static_cast<u128>(b) ^ c ^ d)
                  & ((static_cast<u128>(1) << 64) | (EVEN & ~1ULL));
    return static_cast<u64>(d - (no_carry >> 2));
}

static inline u64 add_unsigned_bct_fast(u64 a, u64 b) {
    u64 c = a + EVEN;
    u64 d = b + c;
    u64 no_carry_low = (~(b ^ c ^ d) & (EVEN & ~1ULL)) >> 2;
    u64 no_carry_top = static_cast<u64>(d >= b) << 62;
    return d - (no_carry_low | no_carry_top);
}

static NOINLINE u64 trit_add_bct(u64 x, u64 y) {
    constexpr u64 NEG_BIAS = 0x5555555555555556ULL;
    u64 a = to_biased_bct(x);
    u64 b = to_biased_bct(y);
    u64 sum = add_unsigned_bct(add_unsigned_bct(a, b), NEG_BIAS);
    return to_biased_bct(sum);
}

static NOINLINE u64 trit_add_bct_fast(u64 x, u64 y) {
    constexpr u64 NEG_BIAS = 0x5555555555555556ULL;
    u64 a = to_biased_bct(x);
    u64 b = to_biased_bct(y);
    u64 sum = add_unsigned_bct_fast(add_unsigned_bct_fast(a, b), NEG_BIAS);
    return to_biased_bct(sum);
}

static NOINLINE u64 bct_add_direct(u64 a, u64 b) {
    constexpr u64 NEG_BIAS = 0x5555555555555556ULL;
    return add_unsigned_bct_fast(add_unsigned_bct_fast(a, b), NEG_BIAS);
}

static NOINLINE u64 trit_sub_bct_fast(u64 x, u64 y) {
    return trit_add_bct_fast(x, trit_neg(y));
}

static NOINLINE u64 trit_sub_bct_negated(u64 x, u64 y) {
    constexpr u64 NEG_BIAS = 0x5555555555555556ULL;
    u64 a = to_biased_bct(x);
    u64 b = ODD - to_biased_bct(y);
    return to_biased_bct(add_unsigned_bct_fast(add_unsigned_bct_fast(a, b), NEG_BIAS));
}

static NOINLINE u64 bct_sub_direct(u64 a, u64 b) {
    return bct_add_direct(a, ODD - b);
}

static NOINLINE u64 trit_mul_pow3(u64 x, unsigned exponent, bool negative) {
    assert(exponent < 32);
    u64 shifted = x << (2 * exponent); // Product modulo 3^32.
    return negative ? trit_neg(shifted) : shifted;
}

static NOINLINE u64 trit_mul_sparse2_mod(u64 x, u64 sparse) {
    assert(__builtin_popcountll(sparse) == 2);
    unsigned first = static_cast<unsigned>(__builtin_ctzll(sparse));
    sparse &= sparse - 1;
    unsigned second = static_cast<unsigned>(__builtin_ctzll(sparse));
    u64 a = trit_mul_pow3(x, first / 2, (first & 1U) == 0);
    u64 b = trit_mul_pow3(x, second / 2, (second & 1U) == 0);
    return trit_add_bct_fast(a, b); // Product modulo 3^32.
}

static int trit_sign(u64 x) {
    if (!x) return 0;
    unsigned highest_bit = 63U - static_cast<unsigned>(__builtin_clzll(x));
    return (highest_bit & 1U) ? 1 : -1;
}

static NOINLINE u64 trit_div_pow3(u64 x, unsigned exponent, bool negative) {
    assert(exponent < 32);
    if (exponent == 0) return negative ? trit_neg(x) : x;
    unsigned shift = 2 * exponent;
    u64 quotient = x >> shift;
    u64 remainder = x & ((1ULL << shift) - 1);
    if (remainder == 0) return negative ? trit_neg(quotient) : quotient;
    // x = 3^k * quotient + remainder, with a balanced remainder.
    // Correct the quotient only when truncation toward zero requires it.
    int qs = trit_sign(quotient), rs = trit_sign(remainder);
    if (qs && rs && qs != rs)
        quotient = trit_add_bct_fast(quotient, qs > 0 ? 1ULL : 2ULL);
    return negative ? trit_neg(quotient) : quotient;
}

static NOINLINE u64 bct_neg(u64 x) {
    return ODD - x; // Per-lane 2 - digit; no inter-lane borrow.
}

static inline u64 bct_logic(u64 x) {
    return x ^ ((x & ODD) >> 1);
}

static NOINLINE u64 bct_min(u64 x, u64 y) {
    return bct_logic(bct_logic(x) & bct_logic(y));
}

static NOINLINE u64 bct_max(u64 x, u64 y) {
    return bct_logic(bct_logic(x) | bct_logic(y));
}

static NOINLINE u64 bct_product(u64 x, u64 y) {
    u64 xp = (x >> 1) & EVEN, xz = x & EVEN;
    u64 yp = (y >> 1) & EVEN, yz = y & EVEN;
    u64 xn = ~(xp | xz) & EVEN, yn = ~(yp | yz) & EVEN;
    u64 positive = (xp & yp) | (xn & yn);
    u64 negative = (xp & yn) | (xn & yp);
    u64 zero = ~(positive | negative) & EVEN;
    return (positive << 1) | zero;
}

static NOINLINE u64 bct_cycle(u64 x) {
    u64 zero = x & EVEN;
    u64 negative = ~(x | (x >> 1)) & EVEN;
    return (zero << 1) | negative;
}

static const std::array<std::uint16_t, 3 * 65536> &add4_table() {
    static const auto table = [] {
        std::array<std::uint16_t, 3 * 65536> result{};
        for (int carry = -1; carry <= 1; ++carry) {
            for (unsigned x = 0; x < 256; ++x) {
                for (unsigned y = 0; y < 256; ++y) {
                    bool valid = true;
                    int c = carry;
                    unsigned out = 0;
                    for (int i = 0; i < 4; ++i) {
                        unsigned a = (x >> (2 * i)) & 3;
                        unsigned b = (y >> (2 * i)) & 3;
                        if (a == 3 || b == 3) { valid = false; break; }
                        int sum = (a == 2 ? 1 : a == 1 ? -1 : 0)
                                + (b == 2 ? 1 : b == 1 ? -1 : 0) + c;
                        c = (sum > 1) - (sum < -1);
                        int digit = sum - 3 * c;
                        out |= (digit > 0 ? 2U : digit < 0 ? 1U : 0U) << (2 * i);
                    }
                    if (valid) result[(carry + 1) * 65536 + x * 256 + y] =
                        static_cast<std::uint16_t>(out | ((c + 1) << 8));
                }
            }
        }
        return result;
    }();
    return table;
}

static NOINLINE u64 trit_add_chunk4(u64 x, u64 y) {
    const auto &table = add4_table();
    u64 result = 0;
    unsigned carry = 1;
    for (unsigned i = 0; i < 8; ++i) {
        unsigned a = static_cast<unsigned>(x >> (8 * i)) & 255;
        unsigned b = static_cast<unsigned>(y >> (8 * i)) & 255;
        std::uint16_t entry = table[carry * 65536 + a * 256 + b];
        result |= static_cast<u64>(entry & 255) << (8 * i);
        carry = entry >> 8;
    }
    return result;
}

static const std::array<std::uint16_t, 6561> &encode8_table() {
    static const auto table = [] {
        std::array<std::uint16_t, 6561> result{};
        for (unsigned k = 0; k < result.size(); ++k) {
            unsigned value = k, packed = 0;
            for (int j = 0; j < 8; ++j) {
                unsigned d = value % 3;
                value /= 3;
                packed |= (d == 0 ? 1U : d == 2 ? 2U : 0U) << (2 * j);
            }
            result[k] = static_cast<std::uint16_t>(packed);
        }
        return result;
    }();
    return table;
}

static const std::array<std::uint16_t, 6561> &encode8_bct_table() {
    static const auto table = [] {
        std::array<std::uint16_t, 6561> result{};
        for (unsigned k = 0; k < result.size(); ++k) {
            unsigned value = k, packed = 0;
            for (int j = 0; j < 8; ++j) {
                unsigned digit = value % 3;
                value /= 3;
                packed |= digit << (2 * j);
            }
            result[k] = static_cast<std::uint16_t>(packed);
        }
        return result;
    }();
    return table;
}

static const std::array<std::int16_t, 256> &decode8_weights() {
    static const auto table = [] {
        std::array<std::int16_t, 256> result{};
        for (unsigned k = 0; k < result.size(); ++k) {
            unsigned power = 1, value = 0;
            for (int j = 0; j < 8; ++j) {
                if ((k >> j) & 1) value += power;
                power *= 3;
            }
            result[k] = static_cast<std::int16_t>(value);
        }
        return result;
    }();
    return table;
}

static const std::array<std::int8_t, 256> &decode4_table() {
    static const auto table = [] {
        std::array<std::int8_t, 256> result{};
        for (unsigned packed = 0; packed < 256; ++packed) {
            int value = 0, power = 1;
            for (int i = 0; i < 4; ++i) {
                unsigned digit = (packed >> (2 * i)) & 3;
                value += (digit == 2 ? 1 : digit == 1 ? -1 : 0) * power;
                power *= 3;
            }
            result[packed] = static_cast<std::int8_t>(value);
        }
        return result;
    }();
    return table;
}

static i64 decode_table_portable(u64 x) {
    const auto &table = decode4_table();
    i64 result = 0;
    for (int i = 7; i >= 0; --i)
        result = result * 81 + table[(x >> (8 * i)) & 255];
    return result;
}

static u64 encode_table(i64 value) {
    assert(-LIMIT <= value && value <= LIMIT);
    const auto &table = encode8_table();
    u64 biased = static_cast<u64>(value + LIMIT), result = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned digit = static_cast<unsigned>(biased % 6561);
        biased /= 6561;
        result |= static_cast<u64>(table[digit]) << (16 * i);
    }
    return result;
}

static u64 encode_bct_table(i64 value) {
    assert(-LIMIT <= value && value <= LIMIT);
    const auto &table = encode8_bct_table();
    u64 biased = static_cast<u64>(value + LIMIT), result = 0;
    for (int i = 0; i < 4; ++i) {
        unsigned digit = static_cast<unsigned>(biased % 6561);
        biased /= 6561;
        result |= static_cast<u64>(table[digit]) << (16 * i);
    }
    return result;
}

static i64 decode_table(u64 x) {
    const auto &weights = decode8_weights();
    u64 positive = _pext_u64(x, ODD);
    u64 negative = _pext_u64(x, EVEN);
    i64 result = 0;
    for (int i = 3; i >= 0; --i) {
        unsigned shift = 8 * i;
        result = result * 6561
               + weights[(positive >> shift) & 255]
               - weights[(negative >> shift) & 255];
    }
    return result;
}

static inline i64 decode_low16(u64 x) {
    assert((x >> 32) == 0);
    const auto &weights = decode8_weights();
    unsigned positive = static_cast<unsigned>(_pext_u64(x, ODD));
    unsigned negative = static_cast<unsigned>(_pext_u64(x, EVEN));
    return 6561LL * (weights[positive >> 8] - weights[negative >> 8])
         + weights[positive & 255] - weights[negative & 255];
}

static i64 decode_bct_table(u64 x) {
    const auto &weights = decode8_weights();
    u64 high = _pext_u64(x, ODD);
    u64 low = _pext_u64(x, EVEN);
    i64 result = 0;
    for (int i = 3; i >= 0; --i) {
        unsigned shift = 8 * i;
        result = result * 6561
               + 2 * weights[(high >> shift) & 255]
               + weights[(low >> shift) & 255];
    }
    return result - LIMIT;
}

static u64 trit_add_reference(u64 x, u64 y) {
    i64 result = decode(x) + decode(y);
    // This benchmark uses only in-range sums. Overflow policy remains open.
    assert(-LIMIT <= result && result <= LIMIT);
    return encode(result);
}

static u64 trit_add_table(u64 x, u64 y) {
    i64 result = decode_table(x) + decode_table(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode_table(result);
}

static u64 trit_sub_table(u64 x, u64 y) {
    i64 result = decode_table(x) - decode_table(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode_table(result);
}

static u64 trit_add_table_portable(u64 x, u64 y) {
    i64 result = decode_table_portable(x) + decode_table_portable(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode_table(result);
}

static u64 trit_mul_native(u64 x, u64 y) {
    __int128 result = static_cast<__int128>(decode(x)) * decode(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode(static_cast<i64>(result));
}

static u64 trit_mul_table(u64 x, u64 y) {
    __int128 result = static_cast<__int128>(decode_table(x)) * decode_table(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode_table(static_cast<i64>(result));
}

static NOINLINE u64 trit_mul_low16(u64 x, u64 y) {
    i64 a = decode_low16(x), b = decode_low16(y);
    return encode_table(a * b); // Both inputs <= (3^16-1)/2, so product fits.
}

static NOINLINE u64 trit_mul_low16_hybrid(u64 x, u64 y) {
    if (((x | y) >> 32) == 0) return trit_mul_low16(x, y);
    return trit_mul_table(x, y);
}

static NOINLINE u64 trit_mul_low8_kronecker(u64 x, u64 y) {
    // Proof of concept: pack eight sign-bit coefficients four bits apart.
    // Each product coefficient is <=8, so 4-bit lanes do not carry.
    assert(((x | y) >> 16) == 0);
    auto spread = [](u64 bits) { return _pdep_u64(bits, 0x11111111ULL); };
    u64 xp = spread(_pext_u64(x, 0xAAAAULL));
    u64 xn = spread(_pext_u64(x, 0x5555ULL));
    u64 yp = spread(_pext_u64(y, 0xAAAAULL));
    u64 yn = spread(_pext_u64(y, 0x5555ULL));
    u64 pp = xp * yp, nn = xn * yn, pn = xp * yn, np = xn * yp;
    int carry = 0;
    u64 out = 0;
    for (int i = 0; i < 16; ++i) {
        unsigned shift = 4 * i;
        int coefficient = static_cast<int>((pp >> shift) & 15)
                        + static_cast<int>((nn >> shift) & 15)
                        - static_cast<int>((pn >> shift) & 15)
                        - static_cast<int>((np >> shift) & 15) + carry;
        int digit = ((coefficient + 1) % 3 + 3) % 3 - 1;
        carry = (coefficient - digit) / 3;
        out |= static_cast<u64>(digit > 0 ? 2 : digit < 0 ? 1 : 0) << (2 * i);
    }
    assert(carry == 0);
    return out;
}

static NOINLINE u64 trit_mul_hybrid(u64 x, u64 y) {
    if (x == 0 || y == 0) return 0;
    if (y == 2) return x;
    if (y == 1) return trit_neg(x);
    u64 sparse = y, other = x;
    unsigned count = static_cast<unsigned>(__builtin_popcountll(sparse));
    if (count > 2) {
        sparse = x;
        other = y;
        count = static_cast<unsigned>(__builtin_popcountll(sparse));
    }
    if (count == 1) {
        unsigned bit = static_cast<unsigned>(__builtin_ctzll(sparse));
        unsigned exponent = bit / 2;
        assert(exponent == 0 || (other >> (64 - 2 * exponent)) == 0);
        return trit_mul_pow3(other, exponent, (bit & 1U) == 0);
    }
    if (count == 2) {
        if (other == 0) return 0;
        unsigned highest_x = (63U - static_cast<unsigned>(__builtin_clzll(other))) / 2;
        unsigned highest_y = (63U - static_cast<unsigned>(__builtin_clzll(sparse))) / 2;
        // Conservative bound: no overflow is possible when h(x)+h(y)<=30.
        if (highest_x + highest_y <= 30)
            return trit_mul_sparse2_mod(other, sparse);
    }
    if (((x | y) >> 32) == 0) return trit_mul_low16(x, y);
    return trit_mul_table(x, y);
}

static u64 bct_mul_table(u64 x, u64 y) {
    __int128 result = static_cast<__int128>(decode_bct_table(x)) * decode_bct_table(y);
    assert(-LIMIT <= result && result <= LIMIT);
    return encode_bct_table(static_cast<i64>(result));
}

static u64 trit_div_native(u64 x, u64 y) {
    i64 divisor = decode(y);
    assert(divisor != 0);
    return encode(decode(x) / divisor); // Truncates toward zero for now.
}

static u64 trit_div_table(u64 x, u64 y) {
    i64 divisor = decode_table(y);
    assert(divisor != 0);
    return encode_table(decode_table(x) / divisor);
}

static inline i64 div_i64_fp_checked(i64 x, i64 y) {
    assert(y != 0);
    // |x|,|y| <= LIMIT < 2^50: both inputs are exact in binary64.
    u64 ax = static_cast<u64>(x < 0 ? -x : x);
    u64 ay = static_cast<u64>(y < 0 ? -y : y);
    u64 q = static_cast<u64>(static_cast<double>(ax) / static_cast<double>(ay));
    u64 product = q * ay; // < 2*LIMIT, so it fits in uint64_t.
    if (product > ax) --q;
    else if (ax - product >= ay) ++q;
    return ((x < 0) != (y < 0)) ? -static_cast<i64>(q) : static_cast<i64>(q);
}

static NOINLINE u64 trit_div_fp_checked(u64 x, u64 y) {
    i64 divisor = decode_table(y);
    assert(divisor != 0);
    return encode_table(div_i64_fp_checked(decode_table(x), divisor));
}

static NOINLINE u64 trit_div_hybrid(u64 x, u64 y) {
    assert(y != 0);
    if (x == 0) return 0;
    if (y == 2) return x;
    if (y == 1) return trit_neg(x);
    if (__builtin_popcountll(y) == 1) {
        unsigned bit = static_cast<unsigned>(__builtin_ctzll(y));
        unsigned shift = bit & ~1U;
        if ((x & ((1ULL << shift) - 1)) == 0) {
            u64 quotient = x >> shift;
            return (bit & 1U) == 0 ? trit_neg(quotient) : quotient;
        }
    }
    return trit_div_table(x, y);
}

static u64 bct_div_table(u64 x, u64 y) {
    i64 divisor = decode_bct_table(y);
    assert(divisor != 0);
    return encode_bct_table(decode_bct_table(x) / divisor);
}

static u64 next_random(u64 &state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

static u64 random_valid(u64 &state) {
    u64 p = next_random(state) & EVEN;
    u64 n = next_random(state) & EVEN & ~p;
    return (p << 1) | n;
}

static void verify() {
    constexpr std::array<int, 3> digits{-1, 0, 1};
    for (int a : digits) for (int b : digits) {
        u64 x = encode(a), y = encode(b);
        assert(decode(trit_min(x, y)) == std::min(a, b));
        assert(decode(trit_max(x, y)) == std::max(a, b));
        // Product applies to all 32 trits, so a*b is exact for these inputs.
        assert(decode(trit_product(x, y)) == a * b);
    }
    u64 state = 0x9E3779B97F4A7C15ULL;
    for (int i = 0; i < 200000; ++i) {
        u64 x = random_valid(state), y = random_valid(state);
        assert(encode_table(decode(x)) == x);
        assert(decode_table(x) == decode(x));
        assert(decode_table_portable(x) == decode(x));
        assert(trit_neg(trit_neg(x)) == x);
        u64 bx = to_biased_bct(x), by = to_biased_bct(y);
        assert(to_biased_bct(bct_neg(bx)) == trit_neg(x));
        assert(to_biased_bct(bct_min(bx, by)) == trit_min_xor(x, y));
        assert(to_biased_bct(bct_max(bx, by)) == trit_max_xor(x, y));
        assert(to_biased_bct(bct_product(bx, by)) == trit_product_swar(x, y));
        assert(to_biased_bct(bct_cycle(bx)) == trit_cycle(x));
        assert(decode_bct_table(bx) == decode(x));
        assert(encode_bct_table(decode(x)) == bx);
        assert((bx < by) == (decode(x) < decode(y)));
#ifdef WITH_ASM
        assert(asm_trit_neg(x) == trit_neg(x));
        assert(asm_trit_min(x, y) == trit_min_xor(x, y));
        assert(asm_trit_max(x, y) == trit_max_xor(x, y));
        assert(asm_trit_product(x, y) == trit_product_swar(x, y));
        assert(asm_trit_cycle_inverse(x) == trit_cycle_inverse(x));
        assert(asm_trit_plus_count_raw(x) == plus_count(x));
#endif
        assert(trit_min(x, y) == trit_min_xor(x, y));
        assert(trit_max(x, y) == trit_max_xor(x, y));
        assert(trit_product(x, y) == trit_product_swar(x, y));
        assert(trit_less_highest_digit(x, y) == (decode(x) < decode(y)));
        assert(trit_less_decode(x, y) == (decode(x) < decode(y)));
        assert(trit_cycle(trit_cycle(trit_cycle(x))) == x);
        assert(trit_cycle_user_inverse(x) == trit_cycle_inverse(x));
        assert(trit_cycle(trit_cycle_inverse(x)) == x);
        assert(plus_count(x) + minus_count(x) + zero_count(x) == 32);
        assert(trit_shift_right(trit_shift_left(x, 1), 1) == (x & 0x3FFFFFFFFFFFFFFFULL));
        i64 sum = decode(x) + decode(y);
        i64 wrapped = sum;
        if (wrapped > LIMIT) wrapped -= 2 * LIMIT + 1;
        if (wrapped < -LIMIT) wrapped += 2 * LIMIT + 1;
        assert(trit_add_bct(x, y) == encode(wrapped));
        assert(trit_add_bct_fast(x, y) == encode(wrapped));
        assert(to_biased_bct(bct_add_direct(bx, by)) == encode(wrapped));
        i64 difference = decode(x) - decode(y);
        i64 wrapped_difference = difference;
        if (wrapped_difference > LIMIT) wrapped_difference -= 2 * LIMIT + 1;
        if (wrapped_difference < -LIMIT) wrapped_difference += 2 * LIMIT + 1;
        assert(trit_sub_bct_fast(x, y) == encode(wrapped_difference));
        assert(trit_sub_bct_negated(x, y) == encode(wrapped_difference));
        assert(to_biased_bct(bct_sub_direct(bx, by)) == encode(wrapped_difference));
#ifdef WITH_ASM
        assert(asm_trit_add_bct(x, y) == encode(wrapped));
#endif
        if (-LIMIT <= sum && sum <= LIMIT) {
            assert(trit_add_ripple(x, y) == encode(sum));
            assert(trit_add_table(x, y) == encode(sum));
            assert(trit_add_table_portable(x, y) == encode(sum));
            assert(trit_add_chunk4(x, y) == encode(sum));
        }
        if (-LIMIT <= difference && difference <= LIMIT)
            assert(trit_sub_table(x, y) == encode(difference));
    }
    // Boundary cases: the maximal opposite-signed values cancel.
    assert(trit_add_ripple(encode(LIMIT), encode(-LIMIT)) == 0);
    assert(trit_add_ripple(encode(1), encode(-1)) == 0);
    assert(trit_add_ripple(encode(LIMIT - 1), encode(1)) == encode(LIMIT));
    for (int start : {0, 28}) {
        i64 scale = 1;
        for (int i = 0; i < start; ++i) scale *= 3;
        // The upper test reaches the top trit and tests lost carry handling.
        for (int a = -40; a <= 40; ++a) {
            for (int b = -40; b <= 40; ++b) {
                i64 value = static_cast<i64>(a + b) * scale;
                if (value > LIMIT) value -= 2 * LIMIT + 1;
                if (value < -LIMIT) value += 2 * LIMIT + 1;
                u64 x = encode(static_cast<i64>(a) * scale);
                u64 y = encode(static_cast<i64>(b) * scale);
                assert(trit_add_bct(x, y) == encode(value));
                assert(trit_add_bct_fast(x, y) == encode(value));
#ifdef WITH_ASM
                assert(asm_trit_add_bct(x, y) == encode(value));
#endif
            }
        }
    }
    for (int i = 0; i < 200000; ++i) {
        i64 a = static_cast<i64>(next_random(state) % 2000001) - 1000000;
        i64 b = static_cast<i64>(next_random(state) % 2000001) - 1000000;
        u64 x = encode(a), y = encode(b);
        assert(trit_mul_table(x, y) == encode(a * b));
        assert(trit_mul_low16(x, y) == encode(a * b));
        assert(trit_mul_low16_hybrid(x, y) == encode(a * b));
        assert(trit_mul_hybrid(x, y) == encode(a * b));
        assert(bct_mul_table(to_biased_bct(x), to_biased_bct(y)) == to_biased_bct(encode(a * b)));
        assert(trit_mul_table(x, y) == trit_mul_native(x, y));
        if (b != 0) {
            assert(trit_div_table(x, y) == encode(a / b));
            assert(trit_div_fp_checked(x, y) == encode(a / b));
            assert(trit_div_hybrid(x, y) == encode(a / b));
            assert(bct_div_table(to_biased_bct(x), to_biased_bct(y)) == to_biased_bct(encode(a / b)));
            assert(trit_div_table(x, y) == trit_div_native(x, y));
        }
    }
    assert(trit_mul_table(encode(LIMIT), encode(1)) == encode(LIMIT));
    assert(trit_mul_table(encode(-LIMIT), encode(-1)) == encode(LIMIT));
    assert(trit_div_table(encode(-LIMIT), encode(-1)) == encode(LIMIT));
    for (int i = 0; i < 200000; ++i) {
        i64 a = decode(random_valid(state));
        i64 b = decode(random_valid(state));
        if (b == 0) b = 1;
        assert(div_i64_fp_checked(a, b) == a / b);
        assert(div_i64_fp_checked(a, 1) == a);
        assert(div_i64_fp_checked(a, -1) == -a);
        if (a != -1 && a < LIMIT)
            assert(div_i64_fp_checked(a, a + 1) == a / (a + 1));
    }
    {
        i64 power = 1;
        for (unsigned k = 0; k < 32; ++k, power *= 3) {
            for (i64 a = -100; a <= 100; ++a) {
                u64 x = encode(a);
                assert(trit_div_pow3(x, k, false) == encode(a / power));
                assert(trit_div_pow3(x, k, true) == encode(a / -power));
                if (static_cast<__int128>(a) * power <= LIMIT &&
                    static_cast<__int128>(a) * power >= -LIMIT) {
                    assert(trit_mul_pow3(x, k, false) == encode(a * power));
                    assert(trit_mul_pow3(x, k, true) == encode(-a * power));
                }
            }
            for (int i = 0; i < 1000; ++i) {
                u64 x = random_valid(state);
                i64 a = decode(x);
                assert(trit_div_pow3(x, k, false) == encode(a / power));
                assert(trit_div_pow3(x, k, true) == encode(a / -power));
            }
        }
    }
    for (int i = 0; i < 20000; ++i) {
        i64 a = static_cast<i64>(next_random(state) % 20001) - 10000;
        unsigned k = static_cast<unsigned>(next_random(state) % 8) + 1;
        i64 power = 1;
        for (unsigned j = 0; j < k; ++j) power *= 3;
        i64 b = ((i & 1) ? -1 : 1) + ((i & 2) ? -power : power);
        assert(trit_mul_sparse2_mod(encode(a), encode(b)) == encode(a * b));
        assert(trit_mul_hybrid(encode(a), encode(b)) == encode(a * b));
    }
    for (int i = 0; i < 20000; ++i) {
        u64 x = random_valid(state);
        unsigned high = static_cast<unsigned>(next_random(state) % 31) + 1;
        unsigned low = static_cast<unsigned>(next_random(state) % high);
        u64 sparse = (((i & 1) ? 1ULL : 2ULL) << (2 * high)) |
                     (((i & 2) ? 1ULL : 2ULL) << (2 * low));
        i64 y = decode(sparse);
        __int128 product = static_cast<__int128>(decode(x)) * y;
        __int128 modulus = static_cast<__int128>(2 * LIMIT + 1);
        __int128 residue = (product + LIMIT) % modulus;
        if (residue < 0) residue += modulus;
        assert(trit_mul_sparse2_mod(x, sparse) == encode(static_cast<i64>(residue - LIMIT)));
    }
    for (int i = 0; i < 20000; ++i) {
        i64 a = static_cast<i64>(next_random(state) % 6561) - 3280;
        i64 b = static_cast<i64>(next_random(state) % 6561) - 3280;
        assert(trit_mul_low8_kronecker(encode(a), encode(b)) == encode(a * b));
    }
    {
        std::array<u64, 4> a{}, b{}, out{};
        for (int i = 0; i < 4; ++i) {
            a[i] = random_valid(state);
            b[i] = random_valid(state);
        }
        min_batch_avx2(a.data(), b.data(), out.data(), 4);
        for (int i = 0; i < 4; ++i) assert(out[i] == trit_min_xor(a[i], b[i]));
        product_batch_avx2(a.data(), b.data(), out.data(), 4);
        for (int i = 0; i < 4; ++i) assert(out[i] == trit_product_swar(a[i], b[i]));
    }
    std::puts("verification: OK");
}

template <typename Fn>
static double bench(const std::vector<u64> &xs, const std::vector<u64> &ys,
                    Fn fn, unsigned rounds, volatile u64 &sink) {
    auto start = std::chrono::steady_clock::now();
    u64 acc0 = 0, acc1 = 0, acc2 = 0, acc3 = 0;
    for (unsigned r = 0; r < rounds; ++r) {
        for (std::size_t i = 0; i < xs.size(); i += 4) {
            std::size_t xoff = (i + r) & (xs.size() - 1);
            std::size_t yoff = (i + 3 * r) & (xs.size() - 1);
            acc0 += fn(xs[xoff], ys[yoff]);
            acc1 += fn(xs[(xoff+1) & (xs.size()-1)], ys[(yoff+1) & (xs.size()-1)]);
            acc2 += fn(xs[(xoff+2) & (xs.size()-1)], ys[(yoff+2) & (xs.size()-1)]);
            acc3 += fn(xs[(xoff+3) & (xs.size()-1)], ys[(yoff+3) & (xs.size()-1)]);
        }
    }
    auto end = std::chrono::steady_clock::now();
    sink = acc0 + 3 * acc1 + 5 * acc2 + 7 * acc3;
    return std::chrono::duration<double, std::nano>(end - start).count() /
           (static_cast<double>(rounds) * xs.size());
}

static void benchmark() {
    constexpr std::size_t N = 1024;
    constexpr unsigned ROUNDS = 500;
    std::vector<u64> xs(N), ys(N);
    u64 state = 0xD1B54A32D192ED03ULL;
    // Small values avoid most overflows and fit a typical contest workload.
    for (std::size_t i = 0; i < N; ++i) {
        xs[i] = encode(static_cast<i64>(next_random(state) % 100001) - 50000);
        ys[i] = encode(static_cast<i64>(next_random(state) % 100001) - 50000);
    }
    volatile u64 sink = 0;
    auto report = [&](const char *name, auto fn) {
        std::array<double, 3> samples{};
        for (auto &sample : samples) sample = bench(xs, ys, fn, ROUNDS, sink);
        std::sort(samples.begin(), samples.end());
        std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                    static_cast<unsigned long long>(sink));
    };
    report("empty", [](u64 x, u64) { return x; });
    report("trit_add_ripple", trit_add_ripple);
    report("trit_add_chunk4", trit_add_chunk4);
    report("trit_add_bct", trit_add_bct);
    report("trit_add_bct_fast", trit_add_bct_fast);
    report("trit_sub_bct_fast", trit_sub_bct_fast);
    report("trit_sub_bct_neg", trit_sub_bct_negated);
    {
        std::vector<u64> bx(N), by(N);
        for (std::size_t i = 0; i < N; ++i) {
            bx[i] = to_biased_bct(xs[i]);
            by[i] = to_biased_bct(ys[i]);
        }
        auto report_bct = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (auto &sample : samples) sample = bench(bx, by, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                        static_cast<unsigned long long>(sink));
        };
        report_bct("bct_add_direct", bct_add_direct);
        report_bct("bct_sub_direct", bct_sub_direct);
        report_bct("bct_neg", [](u64 x, u64) { return bct_neg(x); });
        report_bct("bct_min", bct_min);
        report_bct("bct_max", bct_max);
        report_bct("bct_product", bct_product);
        report_bct("bct_cycle", [](u64 x, u64) { return bct_cycle(x); });
        report_bct("bct_zero_count", [](u64 x, u64) { return __builtin_popcountll(x & EVEN); });
        report_bct("bct_less", [](u64 x, u64 y) { return x < y; });
        report_bct("bct_mul_table", bct_mul_table);
        report_bct("bct_div_table", bct_div_table);
    }
#ifdef WITH_ASM
    report("asm_trit_add_bct", asm_trit_add_bct);
#endif
    report("trit_add_table", trit_add_table);
    report("trit_sub_table", trit_sub_table);
    report("add_table_portable", trit_add_table_portable);
    report("trit_add_reference", trit_add_reference);
    report("trit_min", trit_min);
    report("trit_min_xor", trit_min_xor);
    report("trit_max", trit_max);
    report("trit_max_xor", trit_max_xor);
    report("trit_product", trit_product);
    report("trit_product_swar", trit_product_swar);
    report("less_highest_digit", trit_less_highest_digit);
    report("less_decode", trit_less_decode);
    report("trit_cycle", [](u64 x, u64) { return trit_cycle(x); });
    report("cycle_inverse", [](u64 x, u64) { return trit_cycle_inverse(x); });
    report("cycle_user_inverse", [](u64 x, u64) { return trit_cycle_user_inverse(x); });
    report("trit_neg", [](u64 x, u64) { return trit_neg(x); });
    report("plus_count", [](u64 x, u64) { return plus_count(x); });
    report("zero_count", [](u64 x, u64) { return zero_count(x); });
    report("trit_mul_native", trit_mul_native);
    report("trit_mul_table", trit_mul_table);
    report("trit_mul_low16", trit_mul_low16);
    report("trit_mul_low16_hyb", trit_mul_low16_hybrid);
    {
        std::vector<u64> x8(N), y8(N);
        for (std::size_t i = 0; i < N; ++i) {
            x8[i] = encode(static_cast<i64>(next_random(state) % 6561) - 3280);
            y8[i] = encode(static_cast<i64>(next_random(state) % 6561) - 3280);
        }
        auto report8 = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (auto &sample : samples) sample = bench(x8, y8, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                        static_cast<unsigned long long>(sink));
        };
        report8("mul_low8_kronecker", trit_mul_low8_kronecker);
        report8("mul_low8_table", trit_mul_table);
    }
    report("trit_mul_hybrid", trit_mul_hybrid);
    report("trit_div_native", trit_div_native);
    report("trit_div_table", trit_div_table);
    report("trit_div_fp_check", trit_div_fp_checked);
    report("trit_div_hybrid", trit_div_hybrid);
    {
        std::vector<u64> powers(N);
        for (std::size_t i = 0; i < N; ++i) {
            i64 value = 1;
            for (std::size_t j = 0; j < i % 10; ++j) value *= 3;
            powers[i] = encode((i & 1) ? -value : value);
        }
        auto report_power = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (auto &sample : samples) sample = bench(xs, powers, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                        static_cast<unsigned long long>(sink));
        };
        report_power("mul_pow3_hybrid", trit_mul_hybrid);
        report_power("mul_pow3_table", trit_mul_table);
        std::vector<u64> sparse2(N);
        for (std::size_t i = 0; i < N; ++i) {
            i64 power = 1;
            for (std::size_t j = 0; j < 1 + i % 9; ++j) power *= 3;
            sparse2[i] = encode(((i & 1) ? -1 : 1) + ((i & 2) ? -power : power));
        }
        auto report_sparse = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (auto &sample : samples) sample = bench(xs, sparse2, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                        static_cast<unsigned long long>(sink));
        };
        report_sparse("mul_sparse2_direct", trit_mul_sparse2_mod);
        report_sparse("mul_sparse2_hybrid", trit_mul_hybrid);
        report_sparse("mul_sparse2_table", trit_mul_table);
        report_power("div_pow3_hybrid", trit_div_hybrid);
        report_power("div_pow3_table", trit_div_table);
        std::vector<u64> exact(N), units(N);
        for (std::size_t i = 0; i < N; ++i) {
            i64 small = static_cast<i64>(i % 201) - 100;
            exact[i] = trit_mul_pow3(encode(small), 9, false);
            units[i] = (i & 1) ? 1ULL : 2ULL;
        }
        auto report_exact = [&](const char *name, const std::vector<u64> &dividends,
                                const std::vector<u64> &divisors, auto fn) {
            std::array<double, 3> samples{};
            for (auto &sample : samples) sample = bench(dividends, divisors, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                        static_cast<unsigned long long>(sink));
        };
        report_exact("div_pow3_exact_hyb", exact, powers, trit_div_hybrid);
        report_exact("div_pow3_exact_tbl", exact, powers, trit_div_table);
        report_exact("div_unit_hybrid", xs, units, trit_div_hybrid);
        report_exact("div_unit_table", xs, units, trit_div_table);
    }
#ifdef WITH_ASM
    report("asm_trit_min", asm_trit_min);
    report("asm_trit_max", asm_trit_max);
    report("asm_trit_product", asm_trit_product);
    report("asm_trit_neg", [](u64 x, u64) { return asm_trit_neg(x); });
    report("asm_cycle_inverse", [](u64 x, u64) { return asm_trit_cycle_inverse(x); });
    report("asm_plus_count", [](u64 x, u64) { return asm_trit_plus_count_raw(x); });
#endif
    std::vector<u64> out(N);
    auto report_batch = [&](const char *name, auto fn) {
        std::array<double, 3> samples{};
        for (double &sample : samples) {
            auto start = std::chrono::steady_clock::now();
            for (unsigned r = 0; r < ROUNDS * 8; ++r) {
                fn(xs.data(), ys.data(), out.data(), N);
                sink = out[r & (N - 1)];
            }
            auto end = std::chrono::steady_clock::now();
            sample = std::chrono::duration<double, std::nano>(end - start).count() /
                     (static_cast<double>(ROUNDS) * 8 * N);
        }
        std::sort(samples.begin(), samples.end());
        std::printf("%-22s %.3f ns/op (median), sink=%llu\n", name, samples[1],
                    static_cast<unsigned long long>(sink));
    };
    report_batch("min_scalar_batch", [](const u64 *x, const u64 *y, u64 *out, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) out[i] = trit_min_xor(x[i], y[i]);
    });
    report_batch("min_avx2_batch", min_batch_avx2);
    report_batch("product_scalar_batch", [](const u64 *x, const u64 *y, u64 *out, std::size_t n) {
        for (std::size_t i = 0; i < n; ++i) out[i] = trit_product_swar(x[i], y[i]);
    });
    report_batch("product_avx2_batch", product_batch_avx2);
}

template <typename Value, typename Add, typename Sub>
static Value tit_cum(const std::vector<Value> &array, int index,
                     Add add, Sub sub, Value zero) {
    Value result = zero;
    int p = 1;
    while (index > 0) {
        if (index % 3 == 1) {
            result = add(result, array[index * p - (p - 1) / 2]);
            --index;
        } else if (index % 3 == 2) {
            ++index;
            result = sub(result, array[index * p - (p - 1) / 2]);
        }
        index /= 3;
        p *= 3;
    }
    return result;
}

template <typename Value, typename Add>
static void tit_update(std::vector<Value> &array, int index, Value delta,
                       Add add) {
    int p = 1;
    int size = static_cast<int>(array.size()) - 1;
    while (index * p <= size) {
        if (index % 3 != 2) {
            int pos = index * p - (p - 1) / 2;
            array[pos] = add(array[pos], delta);
        }
        index = (index - 1) / 3 + 1;
        p *= 3;
    }
}

static void tit_benchmark() {
    // Mirrors the scalar update / prefix query loops in the user's TIT article.
    constexpr int N = 9841; // (3^9 - 1) / 2
    constexpr int OPS = 100000;
    struct Operation { int index; int delta; bool query; u64 packed_delta; u64 bct_delta; };
    std::vector<Operation> operations;
    operations.reserve(OPS);
    u64 state = 0x8F03C263C928E13BULL;
    for (int i = 0; i < OPS; ++i) {
        int index = static_cast<int>(next_random(state) % N) + 1;
        int delta = static_cast<int>(next_random(state) % 21) - 10;
        u64 packed_delta = encode_table(delta);
        operations.push_back({index, delta, static_cast<bool>(i & 1),
                              packed_delta, to_biased_bct(packed_delta)});
    }
    auto native_add = [](i64 a, i64 b) { return a + b; };
    auto native_sub = [](i64 a, i64 b) { return a - b; };
    auto packed_add = [](u64 a, u64 b) { return trit_add_table(a, b); };
    auto packed_sub = [](u64 a, u64 b) { return trit_add_table(a, trit_neg(b)); };
    auto packed_bct_add = [](u64 a, u64 b) { return trit_add_bct_fast(a, b); };
    auto packed_bct_sub = [](u64 a, u64 b) { return trit_add_bct_fast(a, trit_neg(b)); };
    auto direct_bct_add = [](u64 a, u64 b) { return bct_add_direct(a, b); };
    auto direct_bct_sub = [](u64 a, u64 b) { return bct_add_direct(a, bct_neg(b)); };
    std::vector<i64> native(N + 1);
    std::vector<u64> packed(N + 1);
    auto start_native = std::chrono::steady_clock::now();
    i64 checksum_native = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_native += tit_cum(native, op.index, native_add, native_sub, static_cast<i64>(0));
        else tit_update(native, op.index, static_cast<i64>(op.delta), native_add);
    }
    auto end_native = std::chrono::steady_clock::now();
    auto start_packed = std::chrono::steady_clock::now();
    i64 checksum_packed = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_packed += decode_table(tit_cum(packed, op.index, packed_add, packed_sub, static_cast<u64>(0)));
        else tit_update(packed, op.index, op.packed_delta, packed_add);
    }
    auto end_packed = std::chrono::steady_clock::now();
    std::vector<u64> packed_bct(N + 1);
    auto start_bct = std::chrono::steady_clock::now();
    i64 checksum_bct = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_bct += decode_table(tit_cum(packed_bct, op.index, packed_bct_add, packed_bct_sub, static_cast<u64>(0)));
        else tit_update(packed_bct, op.index, op.packed_delta, packed_bct_add);
    }
    auto end_bct = std::chrono::steady_clock::now();
    std::vector<u64> direct_bct(N + 1, EVEN);
    auto start_direct = std::chrono::steady_clock::now();
    i64 checksum_direct = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_direct += decode_bct_table(tit_cum(direct_bct, op.index, direct_bct_add, direct_bct_sub, EVEN));
        else tit_update(direct_bct, op.index, op.bct_delta, direct_bct_add);
    }
    auto end_direct = std::chrono::steady_clock::now();
    assert(checksum_native == checksum_packed);
    assert(checksum_native == checksum_bct);
    assert(checksum_native == checksum_direct);
    for (int i = 1; i <= N; ++i) assert(native[i] == decode_table(packed[i]));
    for (int i = 1; i <= N; ++i) assert(native[i] == decode_table(packed_bct[i]));
    for (int i = 1; i <= N; ++i) assert(native[i] == decode_bct_table(direct_bct[i]));
    auto per_op = [](auto a, auto b) {
        return std::chrono::duration<double, std::nano>(b - a).count() / OPS;
    };
    std::printf("TIT native: %.2f ns/op, checksum=%lld\n",
                per_op(start_native, end_native), static_cast<long long>(checksum_native));
    std::printf("TIT packed: %.2f ns/op, checksum=%lld\n",
                per_op(start_packed, end_packed), static_cast<long long>(checksum_packed));
    std::printf("TIT packed BCT: %.2f ns/op, checksum=%lld\n",
                per_op(start_bct, end_bct), static_cast<long long>(checksum_bct));
    std::printf("TIT BCT native: %.2f ns/op, checksum=%lld\n",
                per_op(start_direct, end_direct), static_cast<long long>(checksum_direct));
}

int main(int argc, char **argv) {
    verify();
    if (argc > 1 && argv[1][0] == 'b') benchmark();
    if (argc > 1 && argv[1][0] == 't') tit_benchmark();
    return 0;
}
