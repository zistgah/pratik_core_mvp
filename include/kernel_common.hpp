// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
//
// This file is part of PRATIK Kernel Core (pratik_core_mvp), free software:
// redistribute and/or modify it under the terms of the GNU General Public
// License, version 3 or (at your option) any later version. Distributed
// WITHOUT ANY WARRANTY. See the LICENSE file for the full text.

// kernel_common.hpp — shared types + the unified backend interface.
// © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
#pragma once
#include "trit.hpp"
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace pratik {

// A point-event: an asynchronous impulse routed by its semantic token,
// carrying a ternary drive applied to the targeted state coordinate.
struct Event {
    std::uint32_t token = 0;   // semantic token (routes via grammar G)
    trit_t        drive = 0;   // ternary payload in {-1,0,+1}
};

// One recorded state transition: (dimension index, resulting trit).
using Step       = std::pair<int, trit_t>;
using Trajectory = std::vector<Step>;

struct KernelConfig {
    int    D_max          = 512;   // hard physical boundary of the substrate
    double gamma          = 0.5;   // persistence dissipation constant  (Axiom III)
    double theta0         = 0.4;   // base crystallization threshold    (Axiom IV)
    double Pi0            = 1.0;   // persistence assigned on spawn
    double Pi_floor       = 0.1;   // below this an unconsolidated dim evaporates
    double dt             = 1.0;   // physical tick interval
    int    frust_threshold= 3;     // sign-frustration cycles before spawning (Axiom II)
    int    ledger_cap     = 4096;  // ring-buffer capacity for I
};

// Unified abstract backend interface (CPU and CUDA implement this).
class IKernel {
public:
    virtual ~IKernel() = default;
    virtual const char* name() const = 0;

    // core loop
    virtual Step  ingest(const Event& e) = 0;   // route + apply ⊞/⊙; may flag frustration
    virtual void  tick()                 = 0;    // decay Π, then consolidate/evaporate (Axiom III/IV)
    virtual bool  maybe_spawn()          = 0;    // fire * if frustration exceeded (Axiom II); true if spawned

    // introspection / control
    virtual int    D_active()  const = 0;
    virtual trit_t state(int d) const = 0;
    virtual bool   provisional(int d) const = 0;
    virtual void   set_resources(double battery, double memory) = 0;
    virtual void   reset_baseline(const std::vector<trit_t>& s0) = 0; // restore states for recollection
    virtual std::vector<trit_t> snapshot_states() const = 0;

    // deep copy for independent sub-scenarios (crystallize vs evaporate)
    virtual std::unique_ptr<IKernel> clone() const = 0;
};

} // namespace pratik
