// Compare native signed binary storage with packed balanced ternary storage.
// This is a performance experiment, not a final ABI or overflow policy.
#define main packed32_experiment_main
#include "packed32.cpp"
#undef main

#include <cstring>

constexpr u64 BASE4 = 81, BASE8 = 6561, MODULUS = 1853020188851841ULL;
constexpr i64 HALF4 = 40, HALF8 = 3280;
constexpr i64 HALF16 = 21523360;
constexpr unsigned pow3(unsigned n) {
    unsigned result = 1;
    for (unsigned i = 0; i < n; ++i) result *= 3;
    return result;
}

enum class DigitOp { Min, Max, Product };
enum class DigitCount { Plus, Zero, Minus };

template <DigitOp op>
static inline u64 packed_op(u64 a, u64 b) {
    if constexpr (op == DigitOp::Min) return (a & b) | ((a ^ b) & EVEN);
    if constexpr (op == DigitOp::Max) return (a & b) | ((a ^ b) & ODD);
    u64 same = a & b, any = a | b;
    return ((same | (same << 1)) & ODD) | ((any & (any >> 1)) & EVEN);
}

static inline u64 cycle_word(u64 x) {
    u64 p = (x >> 1) & EVEN, n = x & EVEN;
    return p | (((~(p | n)) & EVEN) << 1);
}

template <DigitOp op>
static NOINLINE i64 binary_op_convert(i64 x, i64 y) {
    return decode_table(packed_op<op>(encode_table(x), encode_table(y)));
}

static inline u64 encode_low16_binary(i64 x) {
    assert(-HALF16 <= x && x <= HALF16);
    const auto &table = encode8_table();
    u64 biased = static_cast<u64>(x + HALF16);
    return table[biased % BASE8] | (static_cast<u64>(table[biased / BASE8]) << 16);
}

template <DigitOp op>
static NOINLINE i64 binary_op_small16(i64 x, i64 y) {
    u64 result = packed_op<op>(encode_low16_binary(x), encode_low16_binary(y));
    return decode_low16(result);
}

template <DigitOp op>
static NOINLINE i64 binary_op_hybrid16(i64 x, i64 y) {
    if (-HALF16 <= x && x <= HALF16 && -HALF16 <= y && y <= HALF16)
        return binary_op_small16<op>(x,y);
    return binary_op_convert<op>(x,y);
}

template <DigitOp op>
static const std::array<std::int8_t, BASE4 * BASE4> &op4_table() {
    static const auto table = [] {
        std::array<std::int8_t, BASE4 * BASE4> out{};
        for (unsigned a = 0; a < BASE4; ++a)
            for (unsigned b = 0; b < BASE4; ++b) {
                u64 pa = encode(static_cast<i64>(a) - HALF4);
                u64 pb = encode(static_cast<i64>(b) - HALF4);
                out[a * BASE4 + b] = static_cast<std::int8_t>(decode(packed_op<op>(pa, pb)));
            }
        return out;
    }();
    return table;
}

template <DigitOp op>
static NOINLINE i64 binary_op_chunk4(i64 x, i64 y) {
    const auto &table = op4_table<op>();
    u64 a = static_cast<u64>(x + LIMIT), b = static_cast<u64>(y + LIMIT);
    i64 result = 0, weight = 1;
    for (int i = 0; i < 8; ++i) {
        u64 da = a % BASE4, db = b % BASE4;
        a /= BASE4; b /= BASE4;
        result += table[da * BASE4 + db] * weight;
        weight *= BASE4;
    }
    return result;
}

template <DigitOp op, unsigned WIDTH>
static const std::array<std::int16_t, pow3(WIDTH) * pow3(WIDTH)> &op_chunk_table() {
    constexpr unsigned base = pow3(WIDTH);
    static const auto table = [] {
        std::array<std::int16_t, base * base> out{};
        for (unsigned a = 0; a < base; ++a)
            for (unsigned b = 0; b < base; ++b) {
                unsigned va = a, vb = b, power = 1;
                int result = 0;
                for (unsigned k = 0; k < WIDTH; ++k) {
                    int da = static_cast<int>(va % 3) - 1;
                    int db = static_cast<int>(vb % 3) - 1;
                    va /= 3; vb /= 3;
                    int dc;
                    if constexpr (op == DigitOp::Min) dc = std::min(da,db);
                    else if constexpr (op == DigitOp::Max) dc = std::max(da,db);
                    else dc = da * db;
                    result += dc * static_cast<int>(power);
                    power *= 3;
                }
                out[a * base + b] = static_cast<std::int16_t>(result);
            }
        return out;
    }();
    return table;
}

template <DigitOp op, unsigned WIDTH>
static NOINLINE i64 binary_op_chunk(i64 x, i64 y) {
    constexpr unsigned base = pow3(WIDTH), full = 32 / WIDTH, rem = 32 % WIDTH;
    const auto &table = op_chunk_table<op,WIDTH>();
    u64 a = static_cast<u64>(x + LIMIT), b = static_cast<u64>(y + LIMIT);
    i64 result = 0, weight = 1;
    for (unsigned i = 0; i < full; ++i) {
        u64 da = a % base, db = b % base;
        a /= base; b /= base;
        result += table[da * base + db] * weight;
        weight *= base;
    }
    if constexpr (rem != 0) {
        constexpr unsigned topbase = pow3(rem);
        const auto &top = op_chunk_table<op,rem>();
        result += top[a * topbase + b] * weight;
    }
    return result;
}

static const std::array<std::int16_t, 65536> &decode8_packed_table() {
    static const auto table = [] {
        std::array<std::int16_t, 65536> out{};
        for (unsigned a = 0; a < BASE8; ++a) {
            unsigned p = encode8_table()[a];
            out[p] = static_cast<std::int16_t>(static_cast<i64>(a) - HALF8);
        }
        return out;
    }();
    return table;
}

template <DigitOp op>
static NOINLINE i64 binary_op_chunk8(i64 x, i64 y) {
    const auto &encode8 = encode8_table();
    const auto &decode8 = decode8_packed_table();
    u64 a = static_cast<u64>(x + LIMIT), b = static_cast<u64>(y + LIMIT);
    i64 result = 0, weight = 1;
    for (int i = 0; i < 4; ++i) {
        u64 da = a % BASE8, db = b % BASE8;
        a /= BASE8; b /= BASE8;
        unsigned pa = encode8[da], pb = encode8[db];
        unsigned pr = static_cast<unsigned>(packed_op<op>(pa, pb));
        result += decode8[pr] * weight;
        weight *= BASE8;
    }
    return result;
}

static NOINLINE i64 binary_cycle_convert(i64 x, i64) {
    return decode_table(cycle_word(encode_table(x)));
}

static const std::array<std::int16_t, BASE8> &cycle8_table() {
    static const auto table = [] {
        std::array<std::int16_t, BASE8> out{};
        for (unsigned a = 0; a < BASE8; ++a)
            out[a] = static_cast<std::int16_t>(decode(trit_cycle(encode(a - HALF8)) & 0xffffULL));
        return out;
    }();
    return table;
}

static NOINLINE i64 binary_cycle_chunk8(i64 x, i64) {
    const auto &table = cycle8_table();
    u64 biased = static_cast<u64>(x + LIMIT);
    i64 result = 0, weight = 1;
    for (int i = 0; i < 4; ++i) {
        unsigned digit = biased % BASE8;
        biased /= BASE8;
        result += table[digit] * weight;
        weight *= BASE8;
    }
    return result;
}

template <DigitCount count>
static const std::array<std::uint8_t, BASE8> &count8_table() {
    static const auto table = [] {
        std::array<std::uint8_t, BASE8> out{};
        for (unsigned a = 0; a < BASE8; ++a) {
            unsigned v = a, n = 0;
            for (int i = 0; i < 8; ++i) {
                unsigned d = v % 3;
                v /= 3;
                if constexpr (count == DigitCount::Plus) n += d == 2;
                if constexpr (count == DigitCount::Zero) n += d == 1;
                if constexpr (count == DigitCount::Minus) n += d == 0;
            }
            out[a] = n;
        }
        return out;
    }();
    return table;
}

template <DigitCount count>
static NOINLINE u64 binary_count_chunk8(i64 x, i64) {
    const auto &table = count8_table<count>();
    u64 biased = static_cast<u64>(x + LIMIT);
    u64 result = 0;
    for (int i = 0; i < 4; ++i) {
        result += table[biased % BASE8];
        biased /= BASE8;
    }
    return result;
}

template <DigitCount count>
static NOINLINE u64 binary_count_convert(i64 x, i64) {
    u64 packed = encode_table(x);
    if constexpr (count == DigitCount::Plus) return __builtin_popcountll(packed & ODD);
    if constexpr (count == DigitCount::Minus) return __builtin_popcountll(packed & EVEN);
    return 32 - __builtin_popcountll(packed);
}

template <DigitCount count>
static NOINLINE u64 binary_count_small16(i64 x, i64) {
    u64 packed = encode_low16_binary(x);
    if constexpr (count == DigitCount::Plus) return __builtin_popcountll(packed & ODD);
    if constexpr (count == DigitCount::Minus) return __builtin_popcountll(packed & EVEN);
    return 32 - __builtin_popcountll(packed);
}

static NOINLINE u64 packed_plus_count(u64 x, u64) { return __builtin_popcountll(x & ODD); }
static NOINLINE u64 packed_zero_count(u64 x, u64) { return 32 - __builtin_popcountll(x); }
static NOINLINE u64 packed_minus_count(u64 x, u64) { return __builtin_popcountll(x & EVEN); }
static NOINLINE u64 packed_cycle_direct(u64 x, u64) { return cycle_word(x); }

static NOINLINE i64 binary_digit_runtime(i64 x, i64 k) {
    // Balanced digit k = base-3 digit of x+LIMIT minus one.
    static const auto powers = [] {
        std::array<u64, 32> out{};
        u64 p = 1;
        for (u64 &v : out) { v = p; p *= 3; }
        return out;
    }();
    return static_cast<i64>((static_cast<u64>(x + LIMIT) / powers[k]) % 3) - 1;
}

static NOINLINE i64 binary_digit_fixed(i64 x, i64) {
    constexpr u64 pow = 43046721ULL; // k = 16
    return static_cast<i64>((static_cast<u64>(x + LIMIT) / pow) % 3) - 1;
}

static NOINLINE i64 packed_digit_runtime(u64 x, u64 k) {
    unsigned d = static_cast<unsigned>((x >> (2 * k)) & 3);
    return static_cast<i64>(d >> 1) - static_cast<i64>(d & 1);
}

static NOINLINE i64 packed_digit_fixed(u64 x, u64) { return packed_digit_runtime(x, 16); }

static NOINLINE i64 binary_shr1(i64 x, i64) {
    // Remove the balanced low trit; this is exact division after subtracting it.
    i64 digit = ((x + LIMIT) % 3) - 1;
    return (x - digit) / 3;
}

static NOINLINE u64 packed_shr1(u64 x, u64) { return x >> 2; }
static NOINLINE i64 binary_shl1(i64 x, i64) {
    i64 product = x * 3; // |3x| <= 3*LIMIT < INT64_MAX.
    if (product > LIMIT) product -= static_cast<i64>(MODULUS);
    if (product < -LIMIT) product += static_cast<i64>(MODULUS);
    return product;
}
static NOINLINE u64 packed_shl1(u64 x, u64) { return x << 2; }

static NOINLINE i64 binary_neg(i64 x, i64) { return -x; }
static NOINLINE i64 binary_less(i64 x, i64 y) { return x < y; }
static NOINLINE i64 binary_add(i64 x, i64 y) { return x + y; }
static NOINLINE i64 binary_mul(i64 x, i64 y) { return x * y; }
static NOINLINE i64 binary_div(i64 x, i64 y) { return x / y; }

static NOINLINE i64 binary_add_mod(i64 x, i64 y) {
    i64 sum = x + y;
    if (sum > LIMIT) sum -= static_cast<i64>(MODULUS);
    if (sum < -LIMIT) sum += static_cast<i64>(MODULUS);
    return sum;
}

static NOINLINE i64 binary_add_mod_branchless(i64 x, i64 y) {
    i64 sum = x + y;
    i64 correction = static_cast<i64>(sum > LIMIT) - static_cast<i64>(sum < -LIMIT);
    return sum - correction * static_cast<i64>(MODULUS);
}

static NOINLINE i64 binary_mul_mod(i64 x, i64 y) {
    __int128 product = static_cast<__int128>(x) * y;
    __int128 residue = product % static_cast<__int128>(MODULUS);
    if (residue > LIMIT) residue -= MODULUS;
    if (residue < -LIMIT) residue += MODULUS;
    return static_cast<i64>(residue);
}

static NOINLINE u64 packed_mul16_mod(u64 x, u64 y) {
    i64 x0 = decode_low16(x & 0xffffffffULL), x1 = decode_low16(x >> 32);
    i64 y0 = decode_low16(y & 0xffffffffULL), y1 = decode_low16(y >> 32);
    i64 lower = x0 * y0;
    i64 upper = x0 * y1 + x1 * y0;
    return trit_add_bct_fast(encode_table(lower), encode_table(upper) << 32);
}

static NOINLINE i64 binary_chain_naive(i64 x, i64 y) {
    i64 r = binary_op_convert<DigitOp::Min>(x,y);
    r = binary_cycle_convert(r,0);
    r = binary_op_convert<DigitOp::Max>(r,x);
    return binary_op_convert<DigitOp::Product>(r,y);
}

static NOINLINE i64 binary_chain_cached(i64 x, i64 y) {
    u64 px = encode_table(x), py = encode_table(y);
    u64 r = packed_op<DigitOp::Min>(px,py);
    r = cycle_word(r);
    r = packed_op<DigitOp::Max>(r,px);
    r = packed_op<DigitOp::Product>(r,py);
    return decode_table(r);
}

static NOINLINE i64 binary_chain_chunk8(i64 x, i64 y) {
    // Fuse four lane-local operations inside each base-3^8 block.
    const auto &encode8 = encode8_table();
    const auto &decode8 = decode8_packed_table();
    u64 a = static_cast<u64>(x + LIMIT), b = static_cast<u64>(y + LIMIT);
    i64 result = 0, weight = 1;
    for (int i = 0; i < 4; ++i) {
        unsigned pa = encode8[a % BASE8], pb = encode8[b % BASE8];
        a /= BASE8; b /= BASE8;
        u64 r = packed_op<DigitOp::Min>(pa,pb);
        r = cycle_word(r);
        r = packed_op<DigitOp::Max>(r,pa);
        r = packed_op<DigitOp::Product>(r,pb);
        result += decode8[r & 65535] * weight;
        weight *= BASE8;
    }
    return result;
}

static NOINLINE u64 packed_chain(u64 x, u64 y) {
    u64 r = packed_op<DigitOp::Min>(x,y);
    r = cycle_word(r);
    r = packed_op<DigitOp::Max>(r,x);
    return packed_op<DigitOp::Product>(r,y);
}

static void verify_binary_backend() {
    u64 state = 0x7c3e09a83d4b5021ULL;
    for (int n = 0; n < 100000; ++n) {
        i64 x = static_cast<i64>(next_random(state) % MODULUS) - LIMIT;
        i64 y = static_cast<i64>(next_random(state) % MODULUS) - LIMIT;
        u64 px = encode_table(x), py = encode_table(y);
        assert(binary_op_convert<DigitOp::Min>(x,y) == decode(trit_min_xor(px,py)));
        assert(binary_op_chunk4<DigitOp::Min>(x,y) == decode(trit_min_xor(px,py)));
        assert(binary_op_chunk8<DigitOp::Min>(x,y) == decode(trit_min_xor(px,py)));
        assert((binary_op_chunk<DigitOp::Min,5>(x,y) == decode(trit_min_xor(px,py))));
        assert((binary_op_chunk<DigitOp::Min,6>(x,y) == decode(trit_min_xor(px,py))));
        assert(binary_op_convert<DigitOp::Max>(x,y) == decode(trit_max_xor(px,py)));
        assert(binary_op_chunk4<DigitOp::Max>(x,y) == decode(trit_max_xor(px,py)));
        assert(binary_op_chunk8<DigitOp::Max>(x,y) == decode(trit_max_xor(px,py)));
        assert((binary_op_chunk<DigitOp::Max,5>(x,y) == decode(trit_max_xor(px,py))));
        assert((binary_op_chunk<DigitOp::Max,6>(x,y) == decode(trit_max_xor(px,py))));
        assert(binary_op_convert<DigitOp::Product>(x,y) == decode(trit_product_swar(px,py)));
        assert(binary_op_chunk4<DigitOp::Product>(x,y) == decode(trit_product_swar(px,py)));
        assert(binary_op_chunk8<DigitOp::Product>(x,y) == decode(trit_product_swar(px,py)));
        assert((binary_op_chunk<DigitOp::Product,5>(x,y) == decode(trit_product_swar(px,py))));
        assert((binary_op_chunk<DigitOp::Product,6>(x,y) == decode(trit_product_swar(px,py))));
        assert(binary_cycle_convert(x,0) == decode(trit_cycle(px)));
        assert(binary_cycle_chunk8(x,0) == decode(trit_cycle(px)));
        assert(binary_count_chunk8<DigitCount::Plus>(x,0) == plus_count(px));
        assert(binary_count_chunk8<DigitCount::Zero>(x,0) == zero_count(px));
        assert(binary_count_chunk8<DigitCount::Minus>(x,0) == __builtin_popcountll(px & EVEN));
        assert(binary_count_convert<DigitCount::Plus>(x,0) == plus_count(px));
        assert(binary_count_convert<DigitCount::Zero>(x,0) == zero_count(px));
        assert(binary_count_convert<DigitCount::Minus>(x,0) == __builtin_popcountll(px & EVEN));
        unsigned k = static_cast<unsigned>(next_random(state) % 32);
        assert(binary_digit_runtime(x,k) == packed_digit_runtime(px,k));
        assert(binary_digit_fixed(x,0) == packed_digit_fixed(px,0));
        assert(binary_shr1(x,0) == decode(px >> 2));
        assert(binary_shl1(x,0) == decode(px << 2));
        assert(binary_neg(x,0) == decode(trit_neg(px)));
        assert(binary_less(x,y) == static_cast<i64>(trit_less_highest_digit(px,py)));
        assert(binary_add_mod(x,y) == decode(trit_add_bct_fast(px,py)));
        assert(binary_add_mod_branchless(x,y) == decode(trit_add_bct_fast(px,py)));
        assert(binary_mul_mod(x,y) == decode(packed_mul16_mod(px,py)));
        assert(binary_chain_naive(x,y) == decode(packed_chain(px,py)));
        assert(binary_chain_cached(x,y) == decode(packed_chain(px,py)));
        assert(binary_chain_chunk8(x,y) == decode(packed_chain(px,py)));
    }
    for (i64 x : std::array<i64,5>{-LIMIT, -1, 0, 1, LIMIT}) {
        u64 px = encode_table(x);
        assert(binary_cycle_chunk8(x,0) == decode(cycle_word(px)));
        assert(binary_shl1(x,0) == decode(px << 2));
        assert(binary_shr1(x,0) == decode(px >> 2));
        for (i64 y : std::array<i64,5>{-LIMIT, -1, 0, 1, LIMIT}) {
            u64 py = encode_table(y);
            assert(binary_add_mod_branchless(x,y) == decode(trit_add_bct_fast(px,py)));
            assert(binary_mul_mod(x,y) == decode(packed_mul16_mod(px,py)));
            assert(binary_chain_cached(x,y) == decode(packed_chain(px,py)));
        }
    }
    for (int n = 0; n < 100000; ++n) {
        i64 x = static_cast<i64>(next_random(state) % 100001) - 50000;
        i64 y = static_cast<i64>(next_random(state) % 100001) - 50000;
        u64 px = encode_table(x), py = encode_table(y);
        assert(binary_op_small16<DigitOp::Min>(x,y) == decode(trit_min_xor(px,py)));
        assert(binary_op_small16<DigitOp::Max>(x,y) == decode(trit_max_xor(px,py)));
        assert(binary_op_small16<DigitOp::Product>(x,y) == decode(trit_product_swar(px,py)));
        assert(binary_op_hybrid16<DigitOp::Min>(x,y) == decode(trit_min_xor(px,py)));
        assert(binary_count_small16<DigitCount::Plus>(x,0) == plus_count(px));
        assert(binary_count_small16<DigitCount::Zero>(x,0) == zero_count(px));
        assert(binary_count_small16<DigitCount::Minus>(x,0) == __builtin_popcountll(px & EVEN));
    }
    std::puts("binary backend verification: OK");
}

static void benchmark_binary_backend() {
    constexpr std::size_t N = 1024;
    constexpr unsigned ROUNDS = 400;
    u64 state = 0x9247f4a12b697c40ULL;
    std::vector<u64> xs(N),ys(N),px(N),py(N),sx(N),sy(N),spx(N),spy(N),ks(N);
    for (std::size_t i = 0; i < N; ++i) {
        i64 x = static_cast<i64>(next_random(state) % MODULUS) - LIMIT;
        i64 y = static_cast<i64>(next_random(state) % MODULUS) - LIMIT;
        xs[i] = static_cast<u64>(x); ys[i] = static_cast<u64>(y);
        px[i] = encode_table(x); py[i] = encode_table(y);
        i64 a = static_cast<i64>(next_random(state) % 100001) - 50000;
        i64 b = static_cast<i64>(next_random(state) % 100001) - 50000;
        if (b == 0) b = 1;
        sx[i] = static_cast<u64>(a); sy[i] = static_cast<u64>(b);
        spx[i] = encode_table(a); spy[i] = encode_table(b);
        ks[i] = next_random(state) % 32;
    }
    volatile u64 sink = 0;
    auto report = [&](const char *group, const char *name,
                      const std::vector<u64> &a, const std::vector<u64> &b, auto fn) {
        std::array<double,3> samples{};
        for (auto &sample : samples) sample = bench(a,b,fn,ROUNDS,sink);
        std::sort(samples.begin(),samples.end());
        std::printf("%-8s %-27s %7.2f ns/op\n",group,name,samples[1]);
    };
    report("native","add",sx,sy,binary_add);
    report("packed","add",spx,spy,trit_add_bct_fast);
    report("native","add modulo 3^32",xs,ys,binary_add_mod);
    report("native","add modulo branchless",xs,ys,binary_add_mod_branchless);
    report("packed","add modulo 3^32",px,py,trit_add_bct_fast);
    report("native","multiply",sx,sy,binary_mul);
    report("packed","multiply",spx,spy,trit_mul_low16);
    report("native","multiply modulo 3^32",xs,ys,binary_mul_mod);
    report("packed","multiply modulo 3^32",px,py,packed_mul16_mod);
    report("native","divide",sx,sy,binary_div);
    report("packed","divide",spx,spy,trit_div_table);
    report("native","compare less",xs,ys,binary_less);
    report("packed","compare less",px,py,trit_less_highest_digit);
    report("native","negate",xs,ys,binary_neg);
    report("packed","negate",px,py,[](u64 x,u64){return trit_neg(x);});
    report("native","trit shift left 1",xs,ys,binary_shl1);
    report("packed","trit shift left 1",px,py,packed_shl1);
    report("native","trit shift right 1",xs,ys,binary_shr1);
    report("packed","trit shift right 1",px,py,packed_shr1);
    report("native","digit k=16",xs,ys,binary_digit_fixed);
    report("packed","digit k=16",px,py,packed_digit_fixed);
    report("native","digit runtime",xs,ks,binary_digit_runtime);
    report("packed","digit runtime",px,ks,packed_digit_runtime);
    report("bridge","binary to packed",xs,ys,[](u64 x,u64){return encode_table(static_cast<i64>(x));});
    report("bridge","packed to binary",px,py,[](u64 x,u64){return static_cast<u64>(decode_table(x));});

    auto report_op = [&]<DigitOp op>(const char *name) {
        report("native",name,xs,ys,binary_op_convert<op>);
        report("native","  chunk4",xs,ys,binary_op_chunk4<op>);
        report("native","  chunk5",xs,ys,binary_op_chunk<op,5>);
        report("native","  chunk6",xs,ys,binary_op_chunk<op,6>);
        report("native","  chunk8",xs,ys,binary_op_chunk8<op>);
        if constexpr (op == DigitOp::Min) report("packed",name,px,py,trit_min_xor);
        if constexpr (op == DigitOp::Max) report("packed",name,px,py,trit_max_xor);
        if constexpr (op == DigitOp::Product) report("packed",name,px,py,trit_product_swar);
    };
    report_op.template operator()<DigitOp::Min>("tritwise min");
    report_op.template operator()<DigitOp::Max>("tritwise max");
    report_op.template operator()<DigitOp::Product>("tritwise product");
    auto report_small_op = [&]<DigitOp op>(const char *name) {
        report("small",name,sx,sy,binary_op_convert<op>);
        report("small","  low16",sx,sy,binary_op_small16<op>);
        report("small","  hybrid16",sx,sy,binary_op_hybrid16<op>);
        if constexpr (op == DigitOp::Min) report("small","  packed",spx,spy,trit_min_xor);
        if constexpr (op == DigitOp::Max) report("small","  packed",spx,spy,trit_max_xor);
        if constexpr (op == DigitOp::Product) report("small","  packed",spx,spy,trit_product_swar);
    };
    report_small_op.template operator()<DigitOp::Min>("tritwise min");
    report_small_op.template operator()<DigitOp::Max>("tritwise max");
    report_small_op.template operator()<DigitOp::Product>("tritwise product");
    report("native","cycle convert",xs,ys,binary_cycle_convert);
    report("native","cycle chunk8",xs,ys,binary_cycle_chunk8);
    report("packed","cycle",px,py,packed_cycle_direct);

    report("native","plus_count convert",xs,ys,binary_count_convert<DigitCount::Plus>);
    report("native","plus_count chunk8",xs,ys,binary_count_chunk8<DigitCount::Plus>);
    report("packed","plus_count",px,py,packed_plus_count);
    report("native","zero_count convert",xs,ys,binary_count_convert<DigitCount::Zero>);
    report("native","zero_count chunk8",xs,ys,binary_count_chunk8<DigitCount::Zero>);
    report("packed","zero_count",px,py,packed_zero_count);
    report("native","minus_count convert",xs,ys,binary_count_convert<DigitCount::Minus>);
    report("native","minus_count chunk8",xs,ys,binary_count_chunk8<DigitCount::Minus>);
    report("packed","minus_count",px,py,packed_minus_count);
    report("small","plus_count convert",sx,sy,binary_count_convert<DigitCount::Plus>);
    report("small","plus_count low16",sx,sy,binary_count_small16<DigitCount::Plus>);
    report("small","plus_count packed",spx,spy,packed_plus_count);
    report("pipeline","binary convert each",xs,ys,binary_chain_naive);
    report("pipeline","binary cache packed",xs,ys,binary_chain_cached);
    report("pipeline","binary fused chunk8",xs,ys,binary_chain_chunk8);
    report("pipeline","packed throughout",px,py,packed_chain);
}

int main(int argc,char **argv) {
    if (argc > 1 && std::strcmp(argv[1],"bench-only") == 0) benchmark_binary_backend();
    else { verify_binary_backend(); if (argc > 1 && std::strcmp(argv[1],"bench") == 0) benchmark_binary_backend(); }
    return 0;
}
