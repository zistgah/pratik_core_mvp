// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// trit.hpp — Balanced ternary primitives for the PRATIK kernel core.
//
// Alphabet  T = {-1, 0, +1}:
//   +1  positive activation
//   -1  negative inhibition
//    0  poised buffer / high-impedance (structural noise dissipation)
//
// Operators (NO floating point in the core path):
//   boxplus (a ⊞ b) : dominance + cancellation  = sat-add clamped to [-1,+1]
//                      +1 ⊞ -1 = 0 ; +1 ⊞ +1 = +1 ; 0 ⊞ x = x
//   gate    (a ⊙ b) : multiplicative routing gate = a*b
//                      multiply-by-0 => high-impedance, freezes the path
//
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#pragma once
#include <cstdint>
#include <array>

namespace pratik {

using trit_t = std::int8_t;   // holds exactly one of {-1,0,+1}

// branchless sign() — the balanced-ternary saturating add for a+b in [-2,+2]
constexpr trit_t sat_sign(int v) noexcept {
    return static_cast<trit_t>((v > 0) - (v < 0));
}

// a ⊞ b : dominance/cancellation. Verified against the spec table below.
constexpr trit_t boxplus(trit_t a, trit_t b) noexcept {
    return sat_sign(static_cast<int>(a) + static_cast<int>(b));
}

// a ⊙ b : gating. product stays inside {-1,0,+1}; *0 = high-impedance.
constexpr trit_t gate(trit_t a, trit_t b) noexcept {
    return static_cast<trit_t>(static_cast<int>(a) * static_cast<int>(b));
}

// Optional packed 9-entry lookup tables (word-aligned), index = (a+1)*3 + (b+1)
inline constexpr std::array<trit_t, 9> BOXPLUS_LUT = {
    /* -1,-1 */ -1, /* -1,0 */ -1, /* -1,+1 */ 0,
    /*  0,-1 */ -1, /*  0,0 */  0, /*  0,+1 */ 1,
    /* +1,-1 */  0, /* +1,0 */  1, /* +1,+1 */ 1
};
inline constexpr std::array<trit_t, 9> GATE_LUT = {
     1,  0, -1,
     0,  0,  0,
    -1,  0,  1
};
constexpr int lut_index(trit_t a, trit_t b) noexcept { return (a + 1) * 3 + (b + 1); }

// compile-time proof that the two implementations agree with the spec
static_assert(boxplus(+1, -1) == 0,  "cancellation");
static_assert(boxplus(+1, +1) == +1, "dominance (saturates, not +2)");
static_assert(boxplus(-1, -1) == -1, "dominance negative");
static_assert(boxplus( 0, +1) == +1, "identity 0 ⊞ x = x");
static_assert(gate(+1, 0) == 0,      "gate-by-zero => high impedance");
static_assert(gate(-1, +1) == -1,    "gate sign");
static_assert(BOXPLUS_LUT[lut_index(+1,-1)] == boxplus(+1,-1), "LUT matches op");
static_assert(GATE_LUT[lut_index(-1,+1)]    == gate(-1,+1),    "LUT matches op");

} // namespace pratik
