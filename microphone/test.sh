#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build/tests
python3 tests/test-lazy-start.py
xcrun clang++ -std=c++17 -O1 -g -fsanitize=address,undefined -Wno-unused-parameter tests/hp250-hal-test.cpp -framework CoreAudio -framework CoreFoundation -framework IOKit -o build/tests/hal-test
build/tests/hal-test
xcrun clang++ -std=c++17 -O1 -g -fsanitize=address,undefined tests/hp250-ipc-test.cpp -o build/tests/ipc-test
build/tests/ipc-test
