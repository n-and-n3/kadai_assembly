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
        if (-LIMIT <= sum && sum <= LIMIT) {
            assert(trit_add_ripple(x, y) == encode(sum));
            assert(trit_add_table(x, y) == encode(sum));
            assert(trit_add_table_portable(x, y) == encode(sum));
            assert(trit_add_chunk4(x, y) == encode(sum));
        }
    }
    // Boundary cases: the maximal opposite-signed values cancel.
    assert(trit_add_ripple(encode(LIMIT), encode(-LIMIT)) == 0);
    assert(trit_add_ripple(encode(1), encode(-1)) == 0);
    assert(trit_add_ripple(encode(LIMIT - 1), encode(1)) == encode(LIMIT));
    for (int i = 0; i < 200000; ++i) {
        i64 a = static_cast<i64>(next_random(state) % 2000001) - 1000000;
        i64 b = static_cast<i64>(next_random(state) % 2000001) - 1000000;
        u64 x = encode(a), y = encode(b);
        assert(trit_mul_table(x, y) == encode(a * b));
        assert(trit_mul_table(x, y) == trit_mul_native(x, y));
        if (b != 0) {
            assert(trit_div_table(x, y) == encode(a / b));
            assert(trit_div_table(x, y) == trit_div_native(x, y));
        }
    }
    assert(trit_mul_table(encode(LIMIT), encode(1)) == encode(LIMIT));
    assert(trit_mul_table(encode(-LIMIT), encode(-1)) == encode(LIMIT));
    assert(trit_div_table(encode(-LIMIT), encode(-1)) == encode(LIMIT));
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
    report("trit_add_table", trit_add_table);
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
    report("trit_div_native", trit_div_native);
    report("trit_div_table", trit_div_table);
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
                     Add add, Sub sub) {
    Value result = 0;
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
    struct Operation { int index; int delta; bool query; };
    std::vector<Operation> operations;
    operations.reserve(OPS);
    u64 state = 0x8F03C263C928E13BULL;
    for (int i = 0; i < OPS; ++i) {
        int index = static_cast<int>(next_random(state) % N) + 1;
        int delta = static_cast<int>(next_random(state) % 21) - 10;
        operations.push_back({index, delta, static_cast<bool>(i & 1)});
    }
    auto native_add = [](i64 a, i64 b) { return a + b; };
    auto native_sub = [](i64 a, i64 b) { return a - b; };
    auto packed_add = [](u64 a, u64 b) { return trit_add_table(a, b); };
    auto packed_sub = [](u64 a, u64 b) { return trit_add_table(a, trit_neg(b)); };
    std::vector<i64> native(N + 1);
    std::vector<u64> packed(N + 1);
    auto start_native = std::chrono::steady_clock::now();
    i64 checksum_native = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_native += tit_cum(native, op.index, native_add, native_sub);
        else tit_update(native, op.index, static_cast<i64>(op.delta), native_add);
    }
    auto end_native = std::chrono::steady_clock::now();
    auto start_packed = std::chrono::steady_clock::now();
    i64 checksum_packed = 0;
    for (const auto &op : operations) {
        if (op.query) checksum_packed += decode_table(tit_cum(packed, op.index, packed_add, packed_sub));
        else tit_update(packed, op.index, encode_table(op.delta), packed_add);
    }
    auto end_packed = std::chrono::steady_clock::now();
    assert(checksum_native == checksum_packed);
    for (int i = 1; i <= N; ++i) assert(native[i] == decode_table(packed[i]));
    auto per_op = [](auto a, auto b) {
        return std::chrono::duration<double, std::nano>(b - a).count() / OPS;
    };
    std::printf("TIT native: %.2f ns/op, checksum=%lld\n",
                per_op(start_native, end_native), static_cast<long long>(checksum_native));
    std::printf("TIT packed: %.2f ns/op, checksum=%lld\n",
                per_op(start_packed, end_packed), static_cast<long long>(checksum_packed));
}

int main(int argc, char **argv) {
    verify();
    if (argc > 1 && argv[1][0] == 'b') benchmark();
    if (argc > 1 && argv[1][0] == 't') tit_benchmark();
}
