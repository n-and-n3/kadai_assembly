// Follow-up multiplication experiments. Reuse the exact encoding/reference
// routines from packed32.cpp, while keeping these exploratory kernels separate.
#define main packed32_experiment_main
#include "packed32.cpp"
#undef main

#include <cstring>

static const std::array<std::uint8_t, 256> &mul2_table() {
    static const auto table = [] {
        std::array<std::uint8_t, 256> result{};
        for (unsigned a = 0; a < 16; ++a)
            for (unsigned b = 0; b < 16; ++b) {
                if ((a & 3) == 3 || (a >> 2) == 3 ||
                    (b & 3) == 3 || (b >> 2) == 3) continue;
                result[(a << 4) | b] = static_cast<std::uint8_t>(encode(decode(a) * decode(b)));
            }
        return result;
    }();
    return table;
}

static NOINLINE u64 mul2_blocks(u64 x, u64 y, unsigned trits) {
    const auto &table = mul2_table();
    u64 out = 0;
    bool first = true;
    unsigned blocks = trits / 2;
    for (unsigned i = 0; i < blocks; ++i) {
        unsigned a = static_cast<unsigned>((x >> (4 * i)) & 15);
        for (unsigned j = 0; j < blocks && i + j < 16; ++j) {
            unsigned b = static_cast<unsigned>((y >> (4 * j)) & 15);
            u64 part = static_cast<u64>(table[(a << 4) | b]) << (4 * (i + j));
            if (first) { out = part; first = false; }
            else out = trit_add_bct_fast(out, part);
        }
    }
    return out;
}

static inline u64 encode_low16(i64 value) {
    // Valid when |value| <= (3^16-1)/2; two table accesses, no high chunks.
    constexpr u64 half = 21523360ULL;
    assert(-static_cast<i64>(half) <= value && value <= static_cast<i64>(half));
    u64 biased = static_cast<u64>(value + half);
    const auto &table = encode8_table();
    return table[biased % 6561] | (static_cast<u64>(table[biased / 6561]) << 16);
}

static inline i64 decode_low8(u64 value) {
    const auto &weights = decode8_weights();
    unsigned p = static_cast<unsigned>(_pext_u64(value, 0xAAAAULL));
    unsigned n = static_cast<unsigned>(_pext_u64(value, 0x5555ULL));
    return weights[p] - weights[n];
}

static inline i64 decode_low8_bytes(u64 value) {
    const auto &table = decode4_table();
    return table[value & 255] + 81LL * table[(value >> 8) & 255];
}

static const std::array<std::uint16_t, 65536> &mul4_table() {
    static const auto table = [] {
        std::array<std::uint16_t, 65536> result{};
        for (unsigned a = 0; a < 256; ++a) {
            for (unsigned b = 0; b < 256; ++b) {
                auto valid = [](unsigned v) {
                    for (int i = 0; i < 4; ++i) if (((v >> (2 * i)) & 3) == 3) return false;
                    return true;
                };
                if (valid(a) && valid(b)) result[(a << 8) | b] = static_cast<std::uint16_t>(encode(decode(a) * decode(b)));
            }
        }
        return result;
    }();
    return table;
}

static NOINLINE u64 mul4_blocks(u64 x, u64 y, unsigned trits) {
    // Modulo 3^32. All source block products fit eight trits.
    const auto &table = mul4_table();
    u64 out = 0;
    bool first = true;
    unsigned blocks = trits / 4;
    for (unsigned i = 0; i < blocks; ++i) {
        unsigned a = static_cast<unsigned>((x >> (8 * i)) & 255);
        for (unsigned j = 0; j < blocks && i + j < 8; ++j) {
            unsigned b = static_cast<unsigned>((y >> (8 * j)) & 255);
            u64 part = static_cast<u64>(table[(a << 8) | b]) << (8 * (i + j));
            if (first) { out = part; first = false; }
            else out = trit_add_bct_fast(out, part);
        }
    }
    return out;
}

static NOINLINE u64 mul8_blocks(u64 x, u64 y, unsigned trits) {
    // Decode each eight-trit block separately, multiply as small binary ints,
    // reencode each 16-trit partial product, then use packed ternary additions.
    u64 out = 0;
    bool first = true;
    unsigned blocks = trits / 8;
    for (unsigned i = 0; i < blocks; ++i) {
        i64 a = decode_low8((x >> (16 * i)) & 65535);
        for (unsigned j = 0; j < blocks && i + j < 4; ++j) {
            i64 b = decode_low8((y >> (16 * j)) & 65535);
            u64 part = encode_low16(a * b) << (16 * (i + j));
            if (first) { out = part; first = false; }
            else out = trit_add_bct_fast(out, part);
        }
    }
    return out;
}

static NOINLINE u64 mul16_blocks(u64 x, u64 y, unsigned trits) {
    // Two 16-trit blocks per input for a full-word modulo product.
    // A 16x16-trit partial product always fits the signed 64-bit range.
    u64 out = 0;
    bool first = true;
    unsigned blocks = trits / 16;
    for (unsigned i = 0; i < blocks; ++i) {
        i64 a = decode_low16((x >> (32 * i)) & 0xffffffffULL);
        for (unsigned j = 0; j < blocks && i + j < 2; ++j) {
            i64 b = decode_low16((y >> (32 * j)) & 0xffffffffULL);
            u64 part = encode_table(a * b) << (32 * (i + j));
            if (first) { out = part; first = false; }
            else out = trit_add_bct_fast(out, part);
        }
    }
    return out;
}

static NOINLINE u64 mul16_cross_mod(u64 x, u64 y) {
    // x = x0 + 3^16*x1, y = y0 + 3^16*y1.
    // The x1*y1 term vanishes modulo 3^32, while the two cross terms can be
    // summed as signed 64-bit integers before encoding just their low 16 trits.
    i64 x0 = decode_low16(x & 0xffffffffULL);
    i64 x1 = decode_low16(x >> 32);
    i64 y0 = decode_low16(y & 0xffffffffULL);
    i64 y1 = decode_low16(y >> 32);
    i64 low = x0 * y0;
    i64 cross = x0 * y1 + x1 * y0;
    constexpr i64 modulus = 43046721LL; // 3^16
    constexpr i64 half = (modulus - 1) / 2;
    cross %= modulus;
    if (cross > half) cross -= modulus;
    if (cross < -half) cross += modulus;
    u64 lower = encode_table(low);
    u64 upper = encode_low16(cross) << 32;
    return trit_add_bct_fast(lower, upper);
}

static NOINLINE u64 mul16_cross_encode(u64 x, u64 y) {
    // The cross sum is always within the signed 32-trit range. Encoding it
    // completely and shifting discards its upper 16 trits automatically.
    i64 x0 = decode_low16(x & 0xffffffffULL);
    i64 x1 = decode_low16(x >> 32);
    i64 y0 = decode_low16(y & 0xffffffffULL);
    i64 y1 = decode_low16(y >> 32);
    i64 lower = x0 * y0;
    i64 upper = x0 * y1 + x1 * y0;
    return trit_add_bct_fast(encode_table(lower), encode_table(upper) << 32);
}

static NOINLINE u64 mul16_carry_fold(u64 x, u64 y) {
    // Normalize at the 16-trit block boundary in integer arithmetic.
    // p = low + 3^16 * carry, then the upper block is cross + carry.
    i64 x0 = decode_low16(x & 0xffffffffULL);
    i64 x1 = decode_low16(x >> 32);
    i64 y0 = decode_low16(y & 0xffffffffULL);
    i64 y1 = decode_low16(y >> 32);
    constexpr i64 modulus = 43046721LL, half = (modulus - 1) / 2;
    i64 product = x0 * y0;
    i64 low = product % modulus;
    if (low > half) low -= modulus;
    if (low < -half) low += modulus;
    i64 carry = (product - low) / modulus;
    i64 high = (x0 * y1 + x1 * y0 + carry) % modulus;
    if (high > half) high -= modulus;
    if (high < -half) high += modulus;
    return encode_low16(low) | (encode_low16(high) << 32);
}

static NOINLINE u64 mul_full_mod(u64 x, u64 y) {
    constexpr __int128 modulus = 2 * static_cast<__int128>(LIMIT) + 1;
    __int128 product = static_cast<__int128>(decode_table(x)) * decode_table(y);
    __int128 residue = product % modulus;
    if (residue > LIMIT) residue -= modulus;
    if (residue < -LIMIT) residue += modulus;
    return encode_table(static_cast<i64>(residue));
}

static NOINLINE u64 mul_shift_add(u64 x, u64 y, unsigned trits) {
    u64 out = 0;
    for (unsigned i = 0; i < trits; ++i) {
        unsigned d = static_cast<unsigned>((y >> (2 * i)) & 3);
        if (d) out = trit_add_bct_fast(out, (d == 2 ? x : trit_neg(x)) << (2 * i));
    }
    return out;
}

static NOINLINE u64 mul_low8_short_decode(u64 x, u64 y) {
    // One four-trit decode per half and input. Product fits 16 trits.
    return encode_low16(decode_low8(x) * decode_low8(y));
}

static NOINLINE u64 mul_low8_bytes(u64 x, u64 y) {
    return encode_low16(decode_low8_bytes(x) * decode_low8_bytes(y));
}

static NOINLINE u64 mul_low8_kronecker_weighted(u64 x, u64 y) {
    // Same four products as the original Kronecker trial. Replace its 16-step
    // carry-normalization loop by evaluating the coefficient polynomial at 3.
    // The existing four-chunk encoder then normalizes the resulting integer.
    auto spread = [](u64 bits) { return _pdep_u64(bits, 0x11111111ULL); };
    u64 xp = spread(_pext_u64(x, 0xAAAAULL));
    u64 xn = spread(_pext_u64(x, 0x5555ULL));
    u64 yp = spread(_pext_u64(y, 0xAAAAULL));
    u64 yn = spread(_pext_u64(y, 0x5555ULL));
    u64 pp = xp * yp, nn = xn * yn, pn = xp * yn, np = xn * yp;
    i64 value = 0, power = 1;
    for (int i = 0; i < 16; ++i) {
        unsigned shift = 4 * i;
        int coefficient = static_cast<int>((pp >> shift) & 15)
                        + static_cast<int>((nn >> shift) & 15)
                        - static_cast<int>((pn >> shift) & 15)
                        - static_cast<int>((np >> shift) & 15);
        value += coefficient * power;
        power *= 3;
    }
    return encode_low16(value);
}

static const std::array<std::uint16_t, 65536> &nibble4_weight_table() {
    static const auto table = [] {
        std::array<std::uint16_t, 65536> result{};
        for (unsigned x = 0; x < result.size(); ++x)
            result[x] = (x & 15) + 3 * ((x >> 4) & 15)
                      + 9 * ((x >> 8) & 15) + 27 * (x >> 12);
        return result;
    }();
    return table;
}

static NOINLINE u64 mul_low8_kronecker_table(u64 x, u64 y) {
    // Four coefficients at a time: evaluate each four-nibble field at base 3.
    auto spread = [](u64 bits) { return _pdep_u64(bits, 0x11111111ULL); };
    u64 xp = spread(_pext_u64(x, 0xAAAAULL));
    u64 xn = spread(_pext_u64(x, 0x5555ULL));
    u64 yp = spread(_pext_u64(y, 0xAAAAULL));
    u64 yn = spread(_pext_u64(y, 0x5555ULL));
    u64 pp = xp * yp, nn = xn * yn, pn = xp * yn, np = xn * yp;
    const auto &table = nibble4_weight_table();
    i64 value = 0, power = 1;
    for (int i = 0; i < 4; ++i) {
        unsigned shift = 16 * i;
        int chunk = table[(pp >> shift) & 65535]
                  + table[(nn >> shift) & 65535]
                  - table[(pn >> shift) & 65535]
                  - table[(np >> shift) & 65535];
        value += chunk * power;
        power *= 81;
    }
    return encode_low16(value);
}

static NOINLINE u64 mul_low8_clmul_parity(u64 x, u64 y) {
    // Correct only when each output degree receives at most one monomial.
    // CLMUL is polynomial multiplication over GF(2), hence XOR not sum.
    u64 xp = _pext_u64(x, ODD), xn = _pext_u64(x, EVEN);
    u64 yp = _pext_u64(y, ODD), yn = _pext_u64(y, EVEN);
    auto cmul = [](u64 a, u64 b) {
        __m128i va = _mm_cvtsi64_si128(static_cast<i64>(a));
        __m128i vb = _mm_cvtsi64_si128(static_cast<i64>(b));
        return static_cast<u64>(_mm_cvtsi128_si64(_mm_clmulepi64_si128(va, vb, 0)));
    };
    u64 positive = cmul(xp, yp) ^ cmul(xn, yn);
    u64 negative = cmul(xp, yn) ^ cmul(xn, yp);
    return (_pdep_u64(positive, ODD) | _pdep_u64(negative, EVEN));
}

static inline u64 cmul64(u64 a, u64 b) {
    __m128i va = _mm_cvtsi64_si128(static_cast<i64>(a));
    __m128i vb = _mm_cvtsi64_si128(static_cast<i64>(b));
    return static_cast<u64>(_mm_cvtsi128_si64(_mm_clmulepi64_si128(va, vb, 0)));
}

static NOINLINE u64 mul_low8_clmul_guarded(u64 x, u64 y) {
    u64 support_x = _pext_u64(x, 0xAAAAULL) | _pext_u64(x, 0x5555ULL);
    u64 support_y = _pext_u64(y, 0xAAAAULL) | _pext_u64(y, 0x5555ULL);
    // For 0/1 coefficients, binary MUL == CLMUL iff no output degree has
    // more than one monomial (the excess causes binary carries).
    if (support_x * support_y == cmul64(support_x, support_y))
        return mul_low8_clmul_parity(x, y);
    return mul_low8_short_decode(x, y);
}

static NOINLINE u64 mul_low8_sparse_unique_clmul(u64 x, u64 y) {
    // Caller guarantees coefficient supports do not collide. Otherwise this
    // loses multiplicities and may produce 11 lanes.
    return mul_low8_clmul_parity(x, y);
}

static NOINLINE u64 mul_sparse_known_shift_add(u64 x, u64 y) {
    // Same fixed support as the CLMUL benchmark; fully inlined two shifts
    // and one packed add, avoiding generic sparse dispatch/call overhead.
    auto neg = [](u64 v) { return ((v & EVEN) << 1) | ((v & ODD) >> 1); };
    u64 a = ((y >> 2) & 3) == 2 ? x : neg(x);
    u64 b = ((y >> 8) & 3) == 2 ? x : neg(x);
    a <<= 2;
    b <<= 8;
    constexpr u64 NEG_BIAS = 0x5555555555555556ULL;
    return to_biased_bct(add_unsigned_bct_fast(
        add_unsigned_bct_fast(to_biased_bct(a), to_biased_bct(b)), NEG_BIAS));
}

static void verify_blocks() {
    u64 state = 0x9565e37021b63d69ULL;
    for (unsigned trits : {8U, 16U, 32U}) {
        for (int i = 0; i < 20000; ++i) {
            u64 x = 0, y = 0;
            for (unsigned d = 0; d < trits; ++d) {
                x |= (next_random(state) % 3) << (2 * d);
                y |= (next_random(state) % 3) << (2 * d);
            }
            // Random generator gives 0,1,2, exactly the three valid codes.
            __int128 product = static_cast<__int128>(decode(x)) * decode(y);
            constexpr __int128 modulus = 2 * static_cast<__int128>(LIMIT) + 1;
            __int128 residue = (product + LIMIT) % modulus;
            if (residue < 0) residue += modulus;
            u64 expected = encode(static_cast<i64>(residue - LIMIT));
            assert(mul_full_mod(x, y) == expected);
            assert(mul4_blocks(x, y, trits) == expected);
            if (i < 2000) assert(mul2_blocks(x, y, trits) == expected);
            assert(mul8_blocks(x, y, trits) == expected);
            if (trits >= 16) assert(mul16_blocks(x, y, trits) == expected);
            if (trits == 32) assert(mul16_cross_mod(x, y) == expected);
            if (trits == 32) assert(mul16_cross_encode(x, y) == expected);
            if (trits == 32) assert(mul16_carry_fold(x, y) == expected);
            assert(mul_shift_add(x, y, trits) == expected);
            if (trits == 8) {
                assert(mul_low8_short_decode(x, y) == expected);
                assert(mul_low8_bytes(x, y) == expected);
                assert(mul_low8_clmul_guarded(x, y) == expected);
                assert(mul_low8_kronecker_weighted(x, y) == expected);
                assert(mul_low8_kronecker_table(x, y) == expected);
            }
        }
    }
    // 1+1 at the degree-one coefficient demonstrates why XOR loses carries.
    u64 a = encode(1 + 3), b = a;
    assert(mul_low8_clmul_parity(a, b) != encode(16));
    // Disjoint supports: all degrees 0,2,4,6 occur once, hence CLMUL works.
    a = encode(1 + 9); b = encode(1 + 81);
    assert(mul_low8_sparse_unique_clmul(a, b) == encode(820));
    for (int i = 0; i < 20000; ++i) {
        u64 x = ((next_random(state) & 1) ? 1ULL : 2ULL)
              | (((next_random(state) & 1) ? 1ULL : 2ULL) << 4);
        u64 y = (((next_random(state) & 1) ? 1ULL : 2ULL) << 2)
              | (((next_random(state) & 1) ? 1ULL : 2ULL) << 8);
        assert(mul_low8_sparse_unique_clmul(x, y) == encode(decode(x) * decode(y)));
        assert(mul_sparse_known_shift_add(x, y) == encode(decode(x) * decode(y)));
        assert(mul_low8_clmul_guarded(x, y) == encode(decode(x) * decode(y)));
    }
    std::puts("block multiplication verification: OK");
}

static void bench_blocks() {
    constexpr std::size_t N = 1024;
    constexpr unsigned ROUNDS = 150;
    volatile u64 sink = 0;
    u64 state = 0x1205d7c90e33a691ULL;
    for (unsigned trits : {8U, 16U, 32U}) {
        std::vector<u64> xs(N), ys(N);
        for (std::size_t i = 0; i < N; ++i) {
            if (trits == 32) {
                // A sparse operand avoids overflow for the baseline encoder;
                // block methods still handle arbitrary modulo-3^32 inputs.
                xs[i] = encode(static_cast<i64>(next_random(state) % 100001) - 50000);
                ys[i] = encode(static_cast<i64>(next_random(state) % 100001) - 50000);
            } else {
                unsigned mask = trits == 8 ? 0xffffU : 0xffffffffU;
                auto make = [&]() {
                    u64 v = 0;
                    for (unsigned d = 0; d < trits; ++d) v |= (next_random(state) % 3) << (2 * d);
                    return v & mask;
                };
                xs[i] = make(); ys[i] = make();
            }
        }
        auto report = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (double &sample : samples) sample = bench(xs, ys, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("%2u trits %-23s %8.2f ns/op\n", trits, name, samples[1]);
        };
        report("2-trit LUT blocks", [=](u64 x, u64 y) { return mul2_blocks(x, y, trits); });
        report("4-trit LUT blocks", [=](u64 x, u64 y) { return mul4_blocks(x, y, trits); });
        report("8-trit numeric blocks", [=](u64 x, u64 y) { return mul8_blocks(x, y, trits); });
        if (trits >= 16) report("16-trit numeric blocks", [=](u64 x, u64 y) { return mul16_blocks(x, y, trits); });
        report("trit shift/add", [=](u64 x, u64 y) { return mul_shift_add(x, y, trits); });
        if (trits == 8) {
            report("short decode/mul", mul_low8_short_decode);
            report("byte table decode/mul", mul_low8_bytes);
            report("CLMUL guarded", mul_low8_clmul_guarded);
            report("Kronecker carry loop", trit_mul_low8_kronecker);
            report("Kronecker eval at 3", mul_low8_kronecker_weighted);
            report("Kronecker 4-digit LUT", mul_low8_kronecker_table);
        }
        if (trits == 16) report("half-width decode/mul", trit_mul_low16);
        report("full decode/mul", trit_mul_table);
    }
    {
        std::vector<u64> xs(N), ys(N);
        for (std::size_t i = 0; i < N; ++i) {
            for (unsigned d = 0; d < 32; ++d) {
                xs[i] |= (next_random(state) % 3) << (2 * d);
                ys[i] |= (next_random(state) % 3) << (2 * d);
            }
        }
        auto report = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (double &sample : samples) sample = bench(xs, ys, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("full 32  %-23s %8.2f ns/op\n", name, samples[1]);
        };
        report("16-trit blocks mod", [](u64 x, u64 y) { return mul16_blocks(x, y, 32); });
        report("16-trit cross combine", mul16_cross_mod);
        report("16-trit cross full enc", mul16_cross_encode);
        report("16-trit carry fold", mul16_carry_fold);
        report("8-trit blocks mod", [](u64 x, u64 y) { return mul8_blocks(x, y, 32); });
        report("decode/mul/mod/recode", mul_full_mod);
    }
    {
        std::vector<u64> xs(N), ys(N);
        for (std::size_t i = 0; i < N; ++i) {
            xs[i] = ((next_random(state) & 1) ? 1ULL : 2ULL)
                  | (((next_random(state) & 1) ? 1ULL : 2ULL) << 4);
            ys[i] = (((next_random(state) & 1) ? 1ULL : 2ULL) << 2)
                  | (((next_random(state) & 1) ? 1ULL : 2ULL) << 8);
        }
        auto report = [&](const char *name, auto fn) {
            std::array<double, 3> samples{};
            for (double &sample : samples) sample = bench(xs, ys, fn, ROUNDS, sink);
            std::sort(samples.begin(), samples.end());
            std::printf("sparse   %-23s %8.2f ns/op\n", name, samples[1]);
        };
        report("CLMUL parity (unique)", mul_low8_sparse_unique_clmul);
        report("CLMUL guarded", mul_low8_clmul_guarded);
        report("two shifts + add", trit_mul_sparse2_mod);
        report("two shifts + add inline", mul_sparse_known_shift_add);
        report("short decode/mul", mul_low8_short_decode);
    }
}

int main(int argc, char **argv) {
    verify_blocks();
    if (argc > 1 && std::strcmp(argv[1], "bench") == 0) bench_blocks();
}
