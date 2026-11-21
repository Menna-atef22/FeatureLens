#!/usr/bin/env bash
# No Qt needed: compile only the headless tool with g++/clang++ and generate every result image.
set -e
cd "$(dirname "$0")/.."
python3 tools/run_all.py
