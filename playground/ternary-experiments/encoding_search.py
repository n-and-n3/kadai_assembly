#!/usr/bin/env python3
"""Exhaustively compare 2-bit trit codes in a small Boolean-gate cost model.

This is a *screening* model, not an x86-64 optimality proof: it charges one
unit per NOT, AND, OR, or XOR, separately for each output bit. It ignores
shared subexpressions, extraction/packing, instruction latency, and registers.
"""

from itertools import permutations, product

TRITS = (-1, 0, 1)
PAIRS = tuple(product(TRITS, repeat=2))
FULL = (1 << len(PAIRS)) - 1
OPS = {
    "min": lambda a, b: min(a, b),
    "max": lambda a, b: max(a, b),
    "product": lambda a, b: a * b,
    "cycle": lambda a, b: {-1: 0, 0: 1, 1: -1}[a],
    "neg": lambda a, b: -a,
}


def sig(values):
    return sum((1 << index) for index, bit in enumerate(values) if bit)


def minimize(codes, max_cost=5):
    bitvars = {
        "a0": sig((codes[a] >> 0) & 1 for a, _ in PAIRS),
        "a1": sig((codes[a] >> 1) & 1 for a, _ in PAIRS),
        "b0": sig((codes[b] >> 0) & 1 for _, b in PAIRS),
        "b1": sig((codes[b] >> 1) & 1 for _, b in PAIRS),
    }
    levels = [{0: "0", FULL: "1", **{v: k for k, v in bitvars.items()}}]
    known = set(levels[0])
    for cost in range(1, max_cost + 1):
        current = {}

        def add(result, expr):
            if result not in known and result not in current:
                current[result] = expr

        for a, ea in levels[cost - 1].items():
            add(FULL ^ a, f"~({ea})")
        for left_cost in range(cost):
            right_cost = cost - left_cost - 1
            for a, ea in levels[left_cost].items():
                for b, eb in levels[right_cost].items():
                    add(a & b, f"({ea}&{eb})")
                    add(a | b, f"({ea}|{eb})")
                    add(a ^ b, f"({ea}^{eb})")
        levels.append(current)
        known.update(current)
        if len(known) == 1 << len(PAIRS):
            break
    best = {}
    for cost, level in enumerate(levels):
        for function, expr in level.items():
            best[function] = (cost, expr)

    results = {}
    for name, op in OPS.items():
        targets = [sig((codes[op(a, b)] >> bit) & 1 for a, b in PAIRS)
                   for bit in (0, 1)]
        if any(target not in best for target in targets):
            raise RuntimeError(f"search depth too small for {codes} / {name}")
        results[name] = tuple(best[target] for target in targets)
    return results


def main():
    ranked = []
    for assignment in permutations(range(4), 3):
        codes = dict(zip(TRITS, assignment))
        costs = minimize(codes)
        total = sum(cost for bits in costs.values() for cost, _ in bits)
        ranked.append((total, codes, costs))
    for total, codes, costs in sorted(ranked, key=lambda r: (r[0], r[1][0] != 0)):
        parts = " ".join(f"{name}={sum(item[0] for item in bits)}"
                         for name, bits in costs.items())
        print(f"score={total:2} -= {codes[-1]:02b} 0={codes[0]:02b} += {codes[1]:02b} {parts}")


if __name__ == "__main__":
    main()
