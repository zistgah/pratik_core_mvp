#!/usr/bin/env bash
# build.sh — build + run the PRATIK MVP.
#   ./build.sh          CPU-only (g++, no cmake required)
#   ./build.sh cuda     CPU + CUDA backend (needs nvcc, sm_80+)
# © 1993-2026 Abhishek Choudhary. All rights reserved. · AyeAI
set -euo pipefail
cd "$(dirname "$0")"

if [ "${1:-}" = "cuda" ]; then
  command -v nvcc >/dev/null || { echo "nvcc not found"; exit 1; }
  echo "== building CPU + CUDA =="
  nvcc -std=c++20 -O2 -arch=sm_80 --expt-relaxed-constexpr -Iinclude \
       -DPRATIK_ENABLE_CUDA \
       cuda/cuda_backend.cu src/cpu_backend.cpp src/main.cpp -o pratik_mvp
else
  echo "== building CPU-only =="
  g++ -std=c++20 -O2 -Wall -Wextra -Iinclude \
      src/cpu_backend.cpp src/main.cpp -o pratik_mvp -pthread
fi
echo "== running =="
./pratik_mvp
