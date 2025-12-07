### ReactorMQ Fuzzing (libFuzzer)

This directory contains [libFuzzer](https://llvm.org/docs/LibFuzzer.html)-based fuzz targets for ReactorMQ. The fuzzers focus on MQTT parsing and codec logic and are **opt-in** via the `REACTORMQ_BUILD_FUZZERS` CMake option.

#### Requirements

- Compiler: Clang  with `-fsanitize=fuzzer` support

#### Enabling and Building Fuzzers

```bash
cmake -S . -B cmake-build-fuzz \
  -DREACTORMQ_BUILD_TESTS=ON \
  -DREACTORMQ_BUILD_FUZZERS=ON \
  -DENABLE_ASAN=ON \
  -DENABLE_UBSAN=ON

cmake --build cmake-build-fuzz --target \
  fuzz_mqtt_codec fuzz_fixed_header fuzz_variable_byte_integer \
  fuzz_publish_packet fuzz_connect_packet
```

The helper scripts under `scripts/` provide shortcuts for this (see below).

#### Running Fuzzers

By default, executables are emitted into the `tests/` subdirectory of the build tree. From the build directory:

```bash
cd cmake-build-fuzz

# Basic runs (no corpus)
tests/fuzz_mqtt_codec -max_total_time=10
tests/fuzz_fixed_header -max_total_time=10

# With a corpus directory (created automatically by CMake for each target)
tests/fuzz_mqtt_codec corpus/fuzz_mqtt_codec

# Run a fixed number of iterations
tests/fuzz_mqtt_codec -runs=100000
```

Common useful options (can be combined):

- `-max_total_time=<seconds>` – total wall-clock time
- `-runs=<N>` – stop after N executions
- `-rss_limit_mb=<MB>` – memory limit
- `-jobs=<N> -workers=<N>` – parallel fuzzing
- `-print_final_stats=1` – print coverage statistics

#### Corpus Layout

Source-tree corpus directories live under `tests/fuzz/corpus/` and are intended for **seed inputs** only:

```text
tests/fuzz/corpus/
  fuzz_variable_byte_integer/
  fuzz_mqtt_codec/
  fuzz_fixed_header/
  fuzz_publish_packet/
  fuzz_connect_packet/
```

At build time, a corresponding corpus directory is also created in the build tree: `cmake-build-*/tests/fuzz/corpus/<fuzzer_name>/`.

To create initial seeds (examples):

```bash
mkdir -p tests/fuzz/corpus/fuzz_variable_byte_integer
printf '\x7f'        > tests/fuzz/corpus/fuzz_variable_byte_integer/vbi_1byte.bin
printf '\xff\x7f'    > tests/fuzz/corpus/fuzz_variable_byte_integer/vbi_2byte.bin
printf '\xff\xff\x7f'  > tests/fuzz/corpus/fuzz_variable_byte_integer/vbi_3byte.bin
printf '\xff\xff\xff\x7f' > tests/fuzz/corpus/fuzz_variable_byte_integer/vbi_4byte.bin
```

You can then point a fuzzer at this corpus, and libFuzzer will augment it with new inputs that increase coverage.

#### MQTT Dictionary (Optional)

The optional `mqtt.dict` file in this directory contains MQTT wire-level tokens to help libFuzzer construct protocol-like inputs more quickly. Use it with:

```bash
tests/fuzz_mqtt_codec -dict=tests/fuzz/mqtt.dict corpus/fuzz_mqtt_codec
```

#### Helper Scripts

Two convenience scripts are provided under `scripts/`:

- `scripts/fuzz-build.sh` – configure and build all fuzz targets in `cmake-build-fuzz`
- `scripts/fuzz-run.sh` – run a selected fuzzer with a couple of common option sets

Windows users have analogous batch files (`scripts/fuzz-build.bat`, `scripts/fuzz-run.bat`), but note that libFuzzer is not supported with MSVC in this project; use Clang/LLVM toolchains where available.
