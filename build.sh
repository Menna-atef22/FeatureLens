#!/usr/bin/env bash
# Builds FeatureLens (GUI, needs Qt) + FeatureLensBatch, optionally generates all result images.
#   ./build.sh [results]     (set QT_PREFIX=/path/to/Qt/6.x/gcc_64 if CMake can't find Qt)
set -e
cd "$(dirname "$0")"
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release ${QT_PREFIX:+-DCMAKE_PREFIX_PATH=$QT_PREFIX}
cmake --build build -j
if [ "$1" = "results" ]; then python3 tools/run_all.py --exe build/FeatureLensBatch; fi
