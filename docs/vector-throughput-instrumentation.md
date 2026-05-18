# Vector Throughput Instrumentation

Spec 052 extends `nes_py.speedtest` with repeated vector-oriented throughput
profiles instead of creating an unrelated script.

## What It Reports

`run_vector_profile` and `python -m nes_py.speedtest --vector-profile` can emit
structured rows with:

- host metadata, compiler, platform, ROM, action policy, warmup steps,
  measured steps, env count, backend, and observation/profile mode;
- total frames/second with repeated rows suitable for median and spread
  summaries;
- setup and teardown seconds;
- action transfer, native step, synchronization wait, observation access,
  RAM/info readback, and remaining Python overhead seconds;
- per-worker stats, currently an empty tuple because the kept prototype has no
  worker threads;
- instrumentation enabled/disabled and CPU affinity experiment labels.

Backends are `scalar_loop`, `gym_sync_vector_env`, `gym_async_vector_env`, and
`native_vector`. Observation/profile modes are `step_only`,
`native_rgb_contiguous`, `native_grayscale`, and `ram_info`. Gymnasium baselines
skip `ram_info` because generic wrapper-owned RAM reads are not available
through `AsyncVectorEnv` workers.

## CPU Usage and Affinity

The kept native vector prototype is serial. It does not busy-wait, does not pin
threads, and does not create worker threads. CPU usage therefore tracks the
single-thread scalar emulator path plus benchmark-side Python work.

`--cpu-affinity round_robin` is accepted as an explicit experiment label, but
no production affinity is applied. Future threaded prototypes can use this
field to compare pinning without silently changing runtime behavior.

## Instrumentation Overhead

Host: macOS-26.3 arm64, CPython 3.14.2, clang, editable release-extension
build. ROM: `nes_py/tests/games/super-mario-bros-1.nes`. Native vector,
4 envs, step-only, noop policy, 30 measured steps, 5 warmups, 5 runs:

| Instrumentation | Median frames/s | Min | Max |
| --- | ---: | ---: | ---: |
| disabled | 2281.40 | 2278.43 | 2285.35 |
| enabled | 2274.39 | 2265.42 | 2279.62 |

The measured overhead was -0.31% median throughput, inside the 1% disabled
overhead budget for this benchmark.

## Example CLI

```sh
.venv/bin/python -m nes_py.speedtest \
  --rom nes_py/tests/games/super-mario-bros-1.nes \
  --vector-profile \
  --steps 1000 \
  --warmup-steps 100 \
  --runs 5 \
  --env-counts 1,2,4,8,16 \
  --vector-backend scalar_loop \
  --vector-backend gym_sync_vector_env \
  --vector-backend native_vector \
  --vector-observation step_only \
  --vector-observation native_rgb_contiguous \
  --vector-observation native_grayscale \
  --vector-observation ram_info \
  --instrumentation \
  --json \
  --no-progress
```

No generated benchmark artifacts are required or committed.
