"""Benchmark helpers and CLI for measuring NES environment throughput."""
import argparse
import json
import platform as platform_module
import sys
import sysconfig
import time
from dataclasses import asdict
from dataclasses import dataclass
from statistics import median
from typing import Callable

import gymnasium as gym
import numpy as np
from tqdm import tqdm

from ._rom import ROM
from .nes_env import NESEnv
from .nes_env import OBSERVATION_MODE_GRAYSCALE
from .nes_env import OBSERVATION_MODE_RGB_ARRAY
from .nes_env import OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS
from .nes_env import SCREEN_SHAPE_24_BIT
from .nes_env import SCREEN_SHAPE_GRAYSCALE
from .ram import normalize_ram_read_specs
from .vector_env import VectorNESEmulator


ActionPolicy = Callable[[NESEnv, int, np.random.RandomState], int] | str
OBSERVATION_PROFILE_OPERATIONS = (
    'step',
    'step_copy',
    'step_contiguous',
    'step_python_grayscale',
    'step_native_rgb_contiguous',
    'step_native_grayscale',
)
RAM_PROFILE_OPERATIONS = (
    'python_indexing',
    'numpy_take',
    'native_batch',
    'step_python_indexing',
    'step_native_batch',
)
VECTOR_PROFILE_BACKENDS = (
    'scalar_loop',
    'gym_sync_vector_env',
    'gym_async_vector_env',
    'native_vector',
)
VECTOR_PROFILE_OBSERVATIONS = (
    'step_only',
    'native_rgb_contiguous',
    'native_grayscale',
    'ram_info',
)
DEFAULT_RAM_READ_SPECS = (
    0x0000,
    0x0001,
    (0x0002, 2, 'little'),
    (0x0004, 2, 'big'),
    (0x0006, 2, 'bcd'),
    (0x0008, 3, 'digits'),
)


class _NESEnvFactory:
    """Picklable factory used by Gymnasium vector baselines."""

    def __init__(self, rom):
        self.rom = rom

    def __call__(self):
        return NESEnv(self.rom)


@dataclass(frozen=True)
class BenchmarkConfig:
    """Configuration for a benchmark run."""

    rom: str
    steps: int = 5000
    warmup_steps: int = 0
    seed: int | None = None
    action_policy: ActionPolicy = 'random'
    render_mode: str | None = None
    progress: bool = True
    backup_interval: int | None = None
    restore_interval: int | None = None


@dataclass(frozen=True)
class BenchmarkResult:
    """Structured result from a benchmark run."""

    total_steps: int
    measured_steps: int
    warmup_steps: int
    resets: int
    backups: int
    restores: int
    frames_rendered: int
    elapsed_seconds: float
    steps_per_second: float
    frames_per_second: float
    seed: int | None
    action_policy: str
    render_mode: str | None
    backup_interval: int | None
    restore_interval: int | None

    def to_dict(self):
        """Return this result as a JSON-serializable dictionary."""
        return asdict(self)


@dataclass(frozen=True)
class MapperBenchmarkResult:
    """Structured result from a current-mapper benchmark operation."""

    environment: str
    compiler: str
    platform: str
    rom: str
    mapper: int
    operation: str
    total_steps: int
    warmup_steps: int
    elapsed_seconds: float
    steps_per_second: float

    def to_dict(self):
        """Return this result as a JSON-serializable dictionary."""
        return asdict(self)


@dataclass(frozen=True)
class ObservationBenchmarkResult:
    """Structured result from an observation-consumption benchmark."""

    environment: str
    compiler: str
    platform: str
    rom: str
    operation: str
    action_policy: str
    total_steps: int
    warmup_steps: int
    elapsed_seconds: float
    steps_per_second: float
    checksum: int

    def to_dict(self):
        """Return this result as a JSON-serializable dictionary."""
        return asdict(self)


@dataclass(frozen=True)
class RepeatedBenchmarkSummary:
    """Summary statistics for repeated benchmark measurements."""

    operation: str
    runs: int
    median_steps_per_second: float
    min_steps_per_second: float
    max_steps_per_second: float
    iqr_steps_per_second: float
    baseline_steps_per_second: float | None = None
    percent_change: float | None = None

    def to_dict(self):
        """Return this summary as a JSON-serializable dictionary."""
        return asdict(self)


@dataclass(frozen=True)
class RAMReadBenchmarkResult:
    """Structured result from a RAM/info read benchmark."""

    environment: str
    compiler: str
    platform: str
    rom: str
    operation: str
    action_policy: str
    total_steps: int
    warmup_steps: int
    ram_read_count: int
    elapsed_seconds: float
    steps_per_second: float
    checksum: int

    def to_dict(self):
        """Return this result as a JSON-serializable dictionary."""
        return asdict(self)


@dataclass(frozen=True)
class VectorBenchmarkResult:
    """Structured result from a vector throughput benchmark."""

    environment: str
    compiler: str
    platform: str
    rom: str
    backend: str
    env_count: int
    observation_mode: str
    action_policy: str
    total_steps: int
    warmup_steps: int
    measured_frames: int
    elapsed_seconds: float
    frames_per_second: float
    instrumentation_enabled: bool
    cpu_affinity: str
    setup_seconds: float = 0.0
    action_transfer_seconds: float = 0.0
    native_step_seconds: float = 0.0
    synchronization_seconds: float = 0.0
    observation_seconds: float = 0.0
    ram_info_seconds: float = 0.0
    python_overhead_seconds: float = 0.0
    teardown_seconds: float = 0.0
    worker_stats: tuple = ()
    checksum: int = 0

    def to_dict(self):
        """Return this result as a JSON-serializable dictionary."""
        return asdict(self)


def _validate_non_negative(name, value):
    """Validate a non-negative integer CLI/API value."""
    if value < 0:
        raise ValueError('{} must be non-negative'.format(name))


def _validate_interval(name, value):
    """Validate an optional positive integer interval."""
    if value is not None and value <= 0:
        raise ValueError('{} must be positive when provided'.format(name))


def _reset(env, seed=None):
    """Reset a Gymnasium environment."""
    if seed is None:
        return env.reset()
    else:
        return env.reset(seed=seed)


def _step(env, action):
    """Step a Gymnasium environment and return a combined done flag."""
    observation, reward, terminated, truncated, info = env.step(action)
    done = terminated or truncated
    return observation, reward, bool(done), info


def _mark_not_done(env):
    """Keep legacy NESEnv instances step-able after a private restore."""
    if hasattr(env, 'done'):
        env.done = False


def _resolve_action(policy):
    """Return a callable action policy and its display name."""
    if callable(policy):
        name = getattr(policy, '__name__', 'callable')
        return policy, name
    if policy == 'random':
        return (
            lambda env, step, rng: int(rng.randint(env.action_space.n)),
            'random',
        )
    if policy == 'noop':
        return lambda env, step, rng: 0, 'noop'
    raise ValueError('unknown action policy: {}'.format(policy))


def _iter_steps(total, progress):
    """Return an iterable over step numbers."""
    steps = range(1, total + 1)
    if progress:
        return tqdm(steps)
    return steps


def _environment_name():
    """Return a short identifier for the active Python runtime."""
    return '{} {}'.format(
        platform_module.python_implementation(),
        platform_module.python_version(),
    )


def _compiler_name():
    """Return the compiler configured for this Python runtime."""
    return sysconfig.get_config_var('CC') or 'unknown'


def _percent_change(value, baseline):
    """Return percent change from baseline, or None for a zero baseline."""
    if baseline in (None, 0):
        return None
    return 100.0 * (value - baseline) / baseline


def _iqr(values):
    """Return a small-sample interquartile range for benchmark values."""
    ordered = sorted(values)
    if len(ordered) < 4:
        return ordered[-1] - ordered[0] if ordered else 0.0
    q1_index = (len(ordered) - 1) // 4
    q3_index = (3 * (len(ordered) - 1)) // 4
    return ordered[q3_index] - ordered[q1_index]


def summarize_repeated_results(results, operation, baseline=None):
    """Summarize repeated results that expose steps or frames per second."""
    values = []
    for result in results:
        if hasattr(result, 'frames_per_second'):
            values.append(result.frames_per_second)
        else:
            values.append(result.steps_per_second)
    summary_median = median(values)
    return RepeatedBenchmarkSummary(
        operation=operation,
        runs=len(values),
        median_steps_per_second=summary_median,
        min_steps_per_second=min(values),
        max_steps_per_second=max(values),
        iqr_steps_per_second=_iqr(values),
        baseline_steps_per_second=baseline,
        percent_change=_percent_change(summary_median, baseline),
    )


def _measure_mapper_operation(env, operation, steps, warmup_steps):
    """Measure one mapper benchmark operation."""
    if operation == 'reset':
        action = lambda: env.reset()
    elif operation == 'step':
        env.reset()
        action = lambda: _step(env, 0)
    elif operation == 'render_rgb_array':
        env.reset()
        action = lambda: env.render()
    elif operation == 'backup_restore':
        env.reset()
        action = lambda: (env._backup(), env._restore(), _mark_not_done(env))
    else:
        raise ValueError('unknown mapper benchmark operation: {}'.format(
            operation
        ))

    for _ in range(warmup_steps):
        action()

    started_at = time.perf_counter()
    for _ in range(steps):
        action()
    elapsed = time.perf_counter() - started_at
    steps_per_second = steps / elapsed if elapsed > 0 else 0.0
    return elapsed, steps_per_second


def _python_grayscale(frame):
    """Return a NumPy-computed grayscale copy for baseline profiling."""
    return ((
        77 * frame[..., 0].astype(np.uint16) +
        150 * frame[..., 1].astype(np.uint16) +
        29 * frame[..., 2].astype(np.uint16)
    ) >> 8).astype(np.uint8)


def _consume_observation_output(output):
    """Consume one byte from an observation output for benchmark accounting."""
    return int(np.asarray(output).flat[0])


def _measure_observation_operation(
    env,
    operation,
    action,
    rng,
    steps,
    warmup_steps,
    seed,
):
    """Measure one step-plus-observation-consumption operation."""
    rgb_output = np.empty(SCREEN_SHAPE_24_BIT, dtype=np.uint8)
    gray_output = np.empty(SCREEN_SHAPE_GRAYSCALE, dtype=np.uint8)
    total = steps + warmup_steps
    done = True
    checksum = 0
    resets = 0
    started_at = None

    for step_number in range(1, total + 1):
        if done:
            _reset(env, seed if resets == 0 else None)
            resets += 1
            done = False
        if step_number == warmup_steps + 1:
            started_at = time.perf_counter()

        observation, _, done, _ = _step(env, action(env, step_number, rng))
        if operation == 'step':
            output = observation
        elif operation == 'step_copy':
            output = observation.copy()
        elif operation == 'step_contiguous':
            output = np.ascontiguousarray(observation)
        elif operation == 'step_python_grayscale':
            output = _python_grayscale(observation)
        elif operation == 'step_native_rgb_contiguous':
            output = env.observation(
                OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                output=rgb_output,
            )
        elif operation == 'step_native_grayscale':
            output = env.observation(
                OBSERVATION_MODE_GRAYSCALE,
                output=gray_output,
            )
        else:
            raise ValueError(
                'unknown observation benchmark operation: {}'.format(
                    operation
                )
            )
        checksum ^= _consume_observation_output(output)

    elapsed = time.perf_counter() - started_at
    steps_per_second = steps / elapsed if elapsed > 0 else 0.0
    return elapsed, steps_per_second, checksum


def run_mapper_profile(
    roms,
    steps=50,
    warmup_steps=10,
    operations=None,
):
    """
    Run a small benchmark profile across current mapper fixtures.

    The profile intentionally checks result shape and positive throughput only;
    callers should compare emitted JSON across revisions instead of treating
    machine-specific timing as a pass/fail threshold.
    """
    if steps <= 0:
        raise ValueError('steps must be positive')
    _validate_non_negative('warmup_steps', warmup_steps)
    if operations is None:
        operations = (
            'reset',
            'step',
            'render_rgb_array',
            'backup_restore',
        )

    results = []
    for rom in roms:
        mapper = ROM(rom).mapper
        for operation in operations:
            render_mode = 'rgb_array' if operation == 'render_rgb_array' else None
            env = NESEnv(rom, render_mode=render_mode)
            try:
                elapsed, steps_per_second = _measure_mapper_operation(
                    env,
                    operation,
                    steps,
                    warmup_steps,
                )
            finally:
                env.close()
            results.append(MapperBenchmarkResult(
                environment=_environment_name(),
                compiler=_compiler_name(),
                platform=platform_module.platform(),
                rom=rom,
                mapper=mapper,
                operation=operation,
                total_steps=steps + warmup_steps,
                warmup_steps=warmup_steps,
                elapsed_seconds=elapsed,
                steps_per_second=steps_per_second,
            ))

    return results


def run_observation_profile(
    rom,
    steps=5000,
    warmup_steps=0,
    seed=None,
    action_policy='random',
    operations=None,
):
    """
    Run step-plus-observation-consumption benchmarks for ML-style loops.

    The default operation set compares the public zero-copy step output,
    user-land copies, NumPy grayscale conversion, and native opt-in helpers.
    """
    if steps <= 0:
        raise ValueError('steps must be positive')
    _validate_non_negative('warmup_steps', warmup_steps)
    if operations is None:
        operations = OBSERVATION_PROFILE_OPERATIONS

    action, action_name = _resolve_action(action_policy)
    results = []
    for operation in operations:
        rng = np.random.RandomState(seed)
        env = NESEnv(rom)
        try:
            elapsed, steps_per_second, checksum = (
                _measure_observation_operation(
                    env,
                    operation,
                    action,
                    rng,
                    steps,
                    warmup_steps,
                    seed,
                )
            )
        finally:
            env.close()
        results.append(ObservationBenchmarkResult(
            environment=_environment_name(),
            compiler=_compiler_name(),
            platform=platform_module.platform(),
            rom=rom,
            operation=operation,
            action_policy=action_name,
            total_steps=steps + warmup_steps,
            warmup_steps=warmup_steps,
            elapsed_seconds=elapsed,
            steps_per_second=steps_per_second,
            checksum=checksum,
        ))

    return results


def _default_ram_read_specs():
    """Return a mutable copy of the synthetic wrapper-like RAM read set."""
    return tuple(DEFAULT_RAM_READ_SPECS)


def _python_ram_values(ram, addresses, sizes, encodings):
    """Decode RAM values with Python indexing for baseline profiling."""
    values = []
    for address, size, encoding in zip(addresses, sizes, encodings):
        address = int(address)
        size = int(size)
        encoding = int(encoding)
        if encoding == 0:
            values.append(int(ram[address]))
        elif encoding == 1:
            value = 0
            for offset in range(size):
                value |= int(ram[address + offset]) << (8 * offset)
            values.append(value)
        elif encoding == 2:
            value = 0
            for offset in range(size):
                value = (value << 8) | int(ram[address + offset])
            values.append(value)
        elif encoding == 3:
            value = 0
            for offset in range(size):
                byte = int(ram[address + offset])
                value = value * 100 + ((byte >> 4) & 0x0f) * 10 + (byte & 0x0f)
            values.append(value)
        else:
            value = 0
            for offset in range(size):
                value = value * 10 + int(ram[address + offset])
            values.append(value)
    return values


def _measure_ram_operation(
    env,
    operation,
    action,
    rng,
    steps,
    warmup_steps,
    seed,
    specs,
):
    """Measure one RAM/info collection operation."""
    addresses, sizes, encodings = normalize_ram_read_specs(specs)
    output = np.empty((len(addresses),), dtype=np.uint32)
    take_addresses = addresses.astype(np.intp, copy=False)
    total = steps + warmup_steps
    checksum = 0
    done = True
    resets = 0
    started_at = None

    for step_number in range(1, total + 1):
        if done:
            _reset(env, seed if resets == 0 else None)
            resets += 1
            done = False
        if step_number == warmup_steps + 1:
            started_at = time.perf_counter()

        if operation.startswith('step_'):
            _, _, done, _ = _step(env, action(env, step_number, rng))

        if operation in {'python_indexing', 'step_python_indexing'}:
            values = _python_ram_values(env.ram, addresses, sizes, encodings)
            checksum ^= int(values[0]) if values else 0
        elif operation == 'numpy_take':
            values = np.take(env.ram, take_addresses)
            checksum ^= int(values[0]) if len(values) else 0
        elif operation in {'native_batch', 'step_native_batch'}:
            values = env.ram_values(specs, output=output)
            checksum ^= int(values[0]) if len(values) else 0
        else:
            raise ValueError('unknown RAM benchmark operation: {}'.format(
                operation
            ))

    elapsed = time.perf_counter() - started_at
    steps_per_second = steps / elapsed if elapsed > 0 else 0.0
    return elapsed, steps_per_second, checksum, len(addresses)


def run_ram_profile(
    rom,
    steps=5000,
    warmup_steps=0,
    seed=None,
    action_policy='random',
    specs=None,
    operations=None,
):
    """
    Run RAM/info collection benchmarks for wrapper-style loops.

    The profile compares pure Python indexing, NumPy gathers for plain byte
    reads, and the native batch helper both in isolation and after stepping.
    """
    if steps <= 0:
        raise ValueError('steps must be positive')
    _validate_non_negative('warmup_steps', warmup_steps)
    if specs is None:
        specs = _default_ram_read_specs()
    if operations is None:
        operations = RAM_PROFILE_OPERATIONS

    action, action_name = _resolve_action(action_policy)
    results = []
    for operation in operations:
        rng = np.random.RandomState(seed)
        env = NESEnv(rom)
        try:
            elapsed, steps_per_second, checksum, read_count = (
                _measure_ram_operation(
                    env,
                    operation,
                    action,
                    rng,
                    steps,
                    warmup_steps,
                    seed,
                    specs,
                )
            )
        finally:
            env.close()
        results.append(RAMReadBenchmarkResult(
            environment=_environment_name(),
            compiler=_compiler_name(),
            platform=platform_module.platform(),
            rom=rom,
            operation=operation,
            action_policy=action_name,
            total_steps=steps + warmup_steps,
            warmup_steps=warmup_steps,
            ram_read_count=read_count,
            elapsed_seconds=elapsed,
            steps_per_second=steps_per_second,
            checksum=checksum,
        ))

    return results


def run_repeated_benchmark(config=None, runs=5, **kwargs):
    """Run scalar throughput benchmark repeatedly and summarize spread."""
    if runs <= 0:
        raise ValueError('runs must be positive')
    results = []
    for _ in range(runs):
        results.append(run_benchmark(config, **kwargs))
    return results, summarize_repeated_results(results, 'scalar')


def run_benchmark(config=None, **kwargs):
    """
    Run an NES throughput benchmark and return a structured result.

    Backup and restore intervals use explicit one-based multiples. For example,
    ``backup_interval=12`` creates a backup after steps 12, 24, 36, and so on.
    This corrects the old root-level script's truthy modulo checks, which ran
    on every non-multiple instead of at the named interval.
    """
    if config is None:
        config = BenchmarkConfig(**kwargs)
    elif kwargs:
        data = asdict(config)
        data.update(kwargs)
        config = BenchmarkConfig(**data)
    if config.steps <= 0:
        raise ValueError('steps must be positive')
    _validate_non_negative('warmup_steps', config.warmup_steps)
    _validate_interval('backup_interval', config.backup_interval)
    _validate_interval('restore_interval', config.restore_interval)

    action, action_name = _resolve_action(config.action_policy)
    rng = np.random.RandomState(config.seed)
    env = NESEnv(config.rom, render_mode=config.render_mode)
    elapsed = 0.0
    measured = 0
    resets = 0
    backups = 0
    restores = 0
    frames_rendered = 0
    done = True
    has_backup = False
    total = config.warmup_steps + config.steps

    try:
        started_at = None
        for step_number in _iter_steps(total, config.progress):
            if step_number == config.warmup_steps + 1:
                started_at = time.perf_counter()

            if done:
                _reset(env, config.seed if resets == 0 else None)
                resets += 1
                done = False

                if (
                    (config.backup_interval is not None or
                     config.restore_interval is not None) and
                    not has_backup
                ):
                    env._backup()
                    backups += 1
                    has_backup = True

            _, _, done, _ = _step(env, action(env, step_number, rng))
            measured_step = step_number > config.warmup_steps
            if measured_step:
                measured += 1

            if config.render_mode is not None:
                env.render()
                if measured_step:
                    frames_rendered += 1

            # Explicit interval semantics: run on exact one-based multiples.
            if (
                config.backup_interval is not None and
                step_number % config.backup_interval == 0
            ):
                env._backup()
                backups += 1
                has_backup = True
            if (
                config.restore_interval is not None and
                step_number % config.restore_interval == 0 and
                has_backup
            ):
                env._restore()
                restores += 1
                done = False
                _mark_not_done(env)

        elapsed = time.perf_counter() - started_at
    finally:
        env.close()

    steps_per_second = measured / elapsed if elapsed > 0 else 0.0
    fps_count = frames_rendered if frames_rendered else measured
    frames_per_second = fps_count / elapsed if elapsed > 0 else 0.0
    return BenchmarkResult(
        total_steps=total,
        measured_steps=measured,
        warmup_steps=config.warmup_steps,
        resets=resets,
        backups=backups,
        restores=restores,
        frames_rendered=frames_rendered,
        elapsed_seconds=elapsed,
        steps_per_second=steps_per_second,
        frames_per_second=frames_per_second,
        seed=config.seed,
        action_policy=action_name,
        render_mode=config.render_mode,
        backup_interval=config.backup_interval,
        restore_interval=config.restore_interval,
    )


def _vector_action_array(policy, env_count, step, rng):
    """Return one uint8 action per vector slot."""
    if policy == 'noop':
        return np.zeros((env_count,), dtype=np.uint8)
    if policy == 'random':
        return rng.randint(256, size=env_count).astype(np.uint8)
    raise ValueError('unknown action policy: {}'.format(policy))


def _consume_vector_observation(mode, observations, checksum):
    """Consume a vector observation array or tuple for benchmark accounting."""
    if mode == 'step_only':
        return checksum
    if isinstance(observations, tuple):
        if not observations:
            return checksum
        return checksum ^ int(np.asarray(observations[0]).flat[0])
    return checksum ^ _consume_observation_output(observations)


def _measure_scalar_vector_loop(
    rom,
    env_count,
    steps,
    warmup_steps,
    seed,
    action_policy,
    observation_mode,
    ram_specs,
    instrumentation_enabled,
    cpu_affinity,
):
    """Measure a Python loop over scalar NESEnv instances."""
    del instrumentation_enabled
    setup_started_at = time.perf_counter()
    envs = [NESEnv(rom) for _ in range(env_count)]
    for index, env in enumerate(envs):
        env.reset(seed=seed + index if seed is not None else None)
    setup_seconds = time.perf_counter() - setup_started_at

    rgb_outputs = [
        np.empty(SCREEN_SHAPE_24_BIT, dtype=np.uint8)
        for _ in range(env_count)
    ]
    gray_outputs = [
        np.empty(SCREEN_SHAPE_GRAYSCALE, dtype=np.uint8)
        for _ in range(env_count)
    ]
    ram_outputs = [
        np.empty((len(normalize_ram_read_specs(ram_specs)[0]),),
                 dtype=np.uint32)
        for _ in range(env_count)
    ]
    rng = np.random.RandomState(seed)
    total = steps + warmup_steps
    checksum = 0
    observation_seconds = 0.0
    ram_info_seconds = 0.0
    native_step_seconds = 0.0
    started_at = None
    teardown_seconds = 0.0

    try:
        for step_number in range(1, total + 1):
            if step_number == warmup_steps + 1:
                started_at = time.perf_counter()
            measured_step = step_number > warmup_steps
            actions = _vector_action_array(
                action_policy,
                env_count,
                step_number,
                rng,
            )
            step_started_at = time.perf_counter()
            for index, env in enumerate(envs):
                env.step(int(actions[index]))
            step_elapsed = time.perf_counter() - step_started_at
            if measured_step:
                native_step_seconds += step_elapsed

            if observation_mode == 'native_rgb_contiguous':
                observed_at = time.perf_counter()
                for index, env in enumerate(envs):
                    output = env.observation(
                        OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                        output=rgb_outputs[index],
                    )
                    if measured_step:
                        checksum ^= _consume_observation_output(output)
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    observation_seconds += observed_elapsed
            elif observation_mode == 'native_grayscale':
                observed_at = time.perf_counter()
                for index, env in enumerate(envs):
                    output = env.observation(
                        OBSERVATION_MODE_GRAYSCALE,
                        output=gray_outputs[index],
                    )
                    if measured_step:
                        checksum ^= _consume_observation_output(output)
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    observation_seconds += observed_elapsed
            elif observation_mode == 'ram_info':
                ram_at = time.perf_counter()
                for index, env in enumerate(envs):
                    output = env.ram_values(
                        ram_specs,
                        output=ram_outputs[index],
                    )
                    if measured_step:
                        checksum ^= int(output[0]) if len(output) else 0
                ram_elapsed = time.perf_counter() - ram_at
                if measured_step:
                    ram_info_seconds += ram_elapsed
    finally:
        teardown_started_at = time.perf_counter()
        for env in envs:
            env.close()
        teardown_seconds = time.perf_counter() - teardown_started_at

    elapsed = time.perf_counter() - started_at
    measured_frames = steps * env_count
    accounted = native_step_seconds + observation_seconds + ram_info_seconds
    return VectorBenchmarkResult(
        environment=_environment_name(),
        compiler=_compiler_name(),
        platform=platform_module.platform(),
        rom=rom,
        backend='scalar_loop',
        env_count=env_count,
        observation_mode=observation_mode,
        action_policy=action_policy,
        total_steps=total,
        warmup_steps=warmup_steps,
        measured_frames=measured_frames,
        elapsed_seconds=elapsed,
        frames_per_second=measured_frames / elapsed if elapsed > 0 else 0.0,
        instrumentation_enabled=False,
        cpu_affinity=cpu_affinity,
        setup_seconds=setup_seconds,
        native_step_seconds=native_step_seconds,
        observation_seconds=observation_seconds,
        ram_info_seconds=ram_info_seconds,
        python_overhead_seconds=max(0.0, elapsed - accounted),
        teardown_seconds=teardown_seconds,
        checksum=checksum,
    )


def _make_env_factory(rom):
    """Return a top-level-friendly factory for Gymnasium vector envs."""
    return _NESEnvFactory(rom)


def _measure_gym_vector_loop(
    rom,
    backend,
    env_count,
    steps,
    warmup_steps,
    seed,
    action_policy,
    observation_mode,
    cpu_affinity,
):
    """Measure Gymnasium SyncVectorEnv or AsyncVectorEnv throughput."""
    setup_started_at = time.perf_counter()
    factories = [_make_env_factory(rom) for _ in range(env_count)]
    vector_cls = (
        gym.vector.AsyncVectorEnv
        if backend == 'gym_async_vector_env'
        else gym.vector.SyncVectorEnv
    )
    env = vector_cls(factories)
    env.reset(seed=seed)
    setup_seconds = time.perf_counter() - setup_started_at

    rng = np.random.RandomState(seed)
    total = steps + warmup_steps
    checksum = 0
    observation_seconds = 0.0
    native_step_seconds = 0.0
    started_at = None
    teardown_seconds = 0.0

    try:
        for step_number in range(1, total + 1):
            if step_number == warmup_steps + 1:
                started_at = time.perf_counter()
            measured_step = step_number > warmup_steps
            actions = _vector_action_array(
                action_policy,
                env_count,
                step_number,
                rng,
            )
            step_started_at = time.perf_counter()
            observations, _, _, _, _ = env.step(actions)
            step_elapsed = time.perf_counter() - step_started_at
            if measured_step:
                native_step_seconds += step_elapsed
            if observation_mode == 'native_rgb_contiguous':
                observed_at = time.perf_counter()
                output = np.ascontiguousarray(observations)
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    checksum ^= _consume_observation_output(output)
                    observation_seconds += observed_elapsed
            elif observation_mode == 'native_grayscale':
                observed_at = time.perf_counter()
                output = _python_grayscale(observations)
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    checksum ^= _consume_observation_output(output)
                    observation_seconds += observed_elapsed
            else:
                if measured_step:
                    checksum = _consume_vector_observation(
                        observation_mode,
                        observations,
                        checksum,
                    )
    finally:
        teardown_started_at = time.perf_counter()
        env.close()
        teardown_seconds = time.perf_counter() - teardown_started_at

    elapsed = time.perf_counter() - started_at
    measured_frames = steps * env_count
    accounted = native_step_seconds + observation_seconds
    return VectorBenchmarkResult(
        environment=_environment_name(),
        compiler=_compiler_name(),
        platform=platform_module.platform(),
        rom=rom,
        backend=backend,
        env_count=env_count,
        observation_mode=observation_mode,
        action_policy=action_policy,
        total_steps=total,
        warmup_steps=warmup_steps,
        measured_frames=measured_frames,
        elapsed_seconds=elapsed,
        frames_per_second=measured_frames / elapsed if elapsed > 0 else 0.0,
        instrumentation_enabled=False,
        cpu_affinity=cpu_affinity,
        setup_seconds=setup_seconds,
        native_step_seconds=native_step_seconds,
        observation_seconds=observation_seconds,
        python_overhead_seconds=max(0.0, elapsed - accounted),
        teardown_seconds=teardown_seconds,
        checksum=checksum,
    )


def _measure_native_vector_loop(
    rom,
    env_count,
    steps,
    warmup_steps,
    seed,
    action_policy,
    observation_mode,
    ram_specs,
    instrumentation_enabled,
    cpu_affinity,
):
    """Measure the native vector emulator prototype."""
    setup_started_at = time.perf_counter()
    env = VectorNESEmulator(rom, env_count)
    env.reset(seed=seed)
    setup_seconds = time.perf_counter() - setup_started_at

    rgb_output = np.empty(
        (env_count, SCREEN_SHAPE_24_BIT[0], SCREEN_SHAPE_24_BIT[1], 3),
        dtype=np.uint8,
    )
    gray_output = np.empty(
        (env_count, SCREEN_SHAPE_GRAYSCALE[0], SCREEN_SHAPE_GRAYSCALE[1]),
        dtype=np.uint8,
    )
    ram_output = np.empty(
        (env_count, len(normalize_ram_read_specs(ram_specs)[0])),
        dtype=np.uint32,
    )
    rng = np.random.RandomState(seed)
    total = steps + warmup_steps
    checksum = 0
    action_transfer_seconds = 0.0
    native_step_seconds = 0.0
    observation_seconds = 0.0
    ram_info_seconds = 0.0
    python_overhead_seconds = 0.0
    started_at = None
    teardown_seconds = 0.0

    try:
        for step_number in range(1, total + 1):
            if step_number == warmup_steps + 1:
                started_at = time.perf_counter()
            measured_step = step_number > warmup_steps
            actions = _vector_action_array(
                action_policy,
                env_count,
                step_number,
                rng,
            )
            if instrumentation_enabled:
                timing = env.step_timed(actions)
                if measured_step:
                    action_transfer_seconds += timing.action_transfer_seconds
                    native_step_seconds += timing.native_step_seconds
                    python_overhead_seconds += timing.python_overhead_seconds
            else:
                step_started_at = time.perf_counter()
                env.step(actions)
                step_elapsed = time.perf_counter() - step_started_at
                if measured_step:
                    native_step_seconds += step_elapsed

            if observation_mode == 'native_rgb_contiguous':
                observed_at = time.perf_counter()
                output = env.observation(
                    OBSERVATION_MODE_RGB_ARRAY_CONTIGUOUS,
                    output=rgb_output,
                )
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    checksum ^= _consume_observation_output(output)
                    observation_seconds += observed_elapsed
            elif observation_mode == 'native_grayscale':
                observed_at = time.perf_counter()
                output = env.observation(
                    OBSERVATION_MODE_GRAYSCALE,
                    output=gray_output,
                )
                observed_elapsed = time.perf_counter() - observed_at
                if measured_step:
                    checksum ^= _consume_observation_output(output)
                    observation_seconds += observed_elapsed
            elif observation_mode == 'ram_info':
                ram_at = time.perf_counter()
                output = env.ram_values(ram_specs, output=ram_output)
                ram_elapsed = time.perf_counter() - ram_at
                if measured_step:
                    checksum ^= int(output[0, 0]) if output.size else 0
                    ram_info_seconds += ram_elapsed
            else:
                if measured_step:
                    checksum = _consume_vector_observation(
                        observation_mode,
                        env.screens,
                        checksum,
                    )
    finally:
        teardown_started_at = time.perf_counter()
        env.close()
        teardown_seconds = time.perf_counter() - teardown_started_at

    elapsed = time.perf_counter() - started_at
    measured_frames = steps * env_count
    accounted = (
        action_transfer_seconds +
        native_step_seconds +
        observation_seconds +
        ram_info_seconds +
        python_overhead_seconds
    )
    if not instrumentation_enabled:
        python_overhead_seconds = max(0.0, elapsed - accounted)
    return VectorBenchmarkResult(
        environment=_environment_name(),
        compiler=_compiler_name(),
        platform=platform_module.platform(),
        rom=rom,
        backend='native_vector',
        env_count=env_count,
        observation_mode=observation_mode,
        action_policy=action_policy,
        total_steps=total,
        warmup_steps=warmup_steps,
        measured_frames=measured_frames,
        elapsed_seconds=elapsed,
        frames_per_second=measured_frames / elapsed if elapsed > 0 else 0.0,
        instrumentation_enabled=instrumentation_enabled,
        cpu_affinity=cpu_affinity,
        setup_seconds=setup_seconds,
        action_transfer_seconds=action_transfer_seconds,
        native_step_seconds=native_step_seconds,
        synchronization_seconds=0.0,
        observation_seconds=observation_seconds,
        ram_info_seconds=ram_info_seconds,
        python_overhead_seconds=python_overhead_seconds,
        teardown_seconds=teardown_seconds,
        worker_stats=(),
        checksum=checksum,
    )


def run_vector_profile(
    rom,
    steps=5000,
    warmup_steps=0,
    seed=None,
    action_policy='random',
    env_counts=None,
    observation_modes=None,
    backends=None,
    ram_specs=None,
    runs=1,
    instrumentation=False,
    cpu_affinity='none',
):
    """Run repeated scalar, Gymnasium, and native vector throughput profiles."""
    if steps <= 0:
        raise ValueError('steps must be positive')
    _validate_non_negative('warmup_steps', warmup_steps)
    if runs <= 0:
        raise ValueError('runs must be positive')
    if cpu_affinity not in {'none', 'round_robin'}:
        raise ValueError('cpu_affinity must be none or round_robin')
    if env_counts is None:
        env_counts = (1, 2, 4, 8, 16)
    if observation_modes is None:
        observation_modes = VECTOR_PROFILE_OBSERVATIONS
    if backends is None:
        backends = VECTOR_PROFILE_BACKENDS
    if ram_specs is None:
        ram_specs = _default_ram_read_specs()

    results = []
    for env_count in env_counts:
        if env_count <= 0:
            raise ValueError('env counts must be positive')
        for observation_mode in observation_modes:
            if observation_mode not in VECTOR_PROFILE_OBSERVATIONS:
                raise ValueError('unknown vector observation mode: {}'.format(
                    observation_mode
                ))
            for backend in backends:
                if backend not in VECTOR_PROFILE_BACKENDS:
                    raise ValueError('unknown vector backend: {}'.format(
                        backend
                    ))
                if backend in {
                    'gym_sync_vector_env',
                    'gym_async_vector_env',
                } and observation_mode == 'ram_info':
                    continue
                for _ in range(runs):
                    try:
                        if backend == 'scalar_loop':
                            result = _measure_scalar_vector_loop(
                                rom,
                                env_count,
                                steps,
                                warmup_steps,
                                seed,
                                action_policy,
                                observation_mode,
                                ram_specs,
                                instrumentation,
                                cpu_affinity,
                            )
                        elif backend in {
                            'gym_sync_vector_env',
                            'gym_async_vector_env',
                        }:
                            result = _measure_gym_vector_loop(
                                rom,
                                backend,
                                env_count,
                                steps,
                                warmup_steps,
                                seed,
                                action_policy,
                                observation_mode,
                                cpu_affinity,
                            )
                        else:
                            result = _measure_native_vector_loop(
                                rom,
                                env_count,
                                steps,
                                warmup_steps,
                                seed,
                                action_policy,
                                observation_mode,
                                ram_specs,
                                instrumentation,
                                cpu_affinity,
                            )
                    except (AttributeError, RuntimeError, OSError):
                        if backend == 'gym_async_vector_env':
                            continue
                        raise
                    results.append(result)
    return results


def format_result(result):
    """Return human-readable benchmark output."""
    lines = [
        'NES benchmark',
        '  total steps: {}'.format(result.total_steps),
        '  measured steps: {}'.format(result.measured_steps),
        '  warmup steps: {}'.format(result.warmup_steps),
        '  resets: {}'.format(result.resets),
        '  backups: {}'.format(result.backups),
        '  restores: {}'.format(result.restores),
        '  elapsed seconds: {:.6f}'.format(result.elapsed_seconds),
        '  steps per second: {:.2f}'.format(result.steps_per_second),
        '  frames per second: {:.2f}'.format(result.frames_per_second),
    ]
    return '\n'.join(lines)


def format_mapper_profile(results):
    """Return human-readable mapper profile output."""
    lines = ['NES mapper benchmark profile']
    for result in results:
        lines.append(
            '  mapper {mapper} {operation}: {steps_per_second:.2f} steps/s '
            '({elapsed_seconds:.6f}s)'.format(**result.to_dict())
        )
    return '\n'.join(lines)


def format_observation_profile(results):
    """Return human-readable observation profile output."""
    lines = ['NES observation benchmark profile']
    for result in results:
        lines.append(
            '  {operation}: {steps_per_second:.2f} steps/s '
            '({elapsed_seconds:.6f}s, checksum={checksum})'.format(
                **result.to_dict()
            )
        )
    return '\n'.join(lines)


def format_ram_profile(results):
    """Return human-readable RAM profile output."""
    lines = ['NES RAM benchmark profile']
    for result in results:
        lines.append(
            '  {operation}: {steps_per_second:.2f} steps/s '
            '({elapsed_seconds:.6f}s, reads={ram_read_count}, '
            'checksum={checksum})'.format(**result.to_dict())
        )
    return '\n'.join(lines)


def format_vector_profile(results):
    """Return human-readable vector profile output."""
    lines = ['NES vector benchmark profile']
    for result in results:
        lines.append(
            '  {backend} envs={env_count} mode={observation_mode}: '
            '{frames_per_second:.2f} frames/s '
            '({elapsed_seconds:.6f}s, checksum={checksum})'.format(
                **result.to_dict()
            )
        )
    return '\n'.join(lines)


def _parser():
    """Build the command line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', help='path to a .nes ROM')
    parser.add_argument(
        '--profile-rom',
        action='append',
        default=[],
        help='path to a .nes ROM fixture to include in the mapper profile',
    )
    parser.add_argument('--steps', type=int, default=5000)
    parser.add_argument('--seed', type=int)
    parser.add_argument('--warmup-steps', type=int, default=0)
    parser.add_argument(
        '--action-policy',
        choices=('random', 'noop'),
        default='random',
    )
    parser.add_argument('--render-mode', choices=('rgb_array', 'human'))
    parser.add_argument(
        '--observation-profile',
        action='store_true',
        help='benchmark step plus ML-style observation consumption modes',
    )
    parser.add_argument(
        '--ram-profile',
        action='store_true',
        help='benchmark wrapper-style RAM/info collection modes',
    )
    parser.add_argument(
        '--vector-profile',
        action='store_true',
        help='benchmark scalar, Gymnasium, and native vector throughput',
    )
    parser.add_argument(
        '--runs',
        type=int,
        default=1,
        help='number of repeated benchmark runs to emit',
    )
    parser.add_argument(
        '--env-counts',
        default='1,2,4,8,16',
        help='comma-separated vector environment counts',
    )
    parser.add_argument(
        '--vector-backend',
        action='append',
        choices=VECTOR_PROFILE_BACKENDS,
        help='vector backend to include; may be repeated',
    )
    parser.add_argument(
        '--vector-observation',
        action='append',
        choices=VECTOR_PROFILE_OBSERVATIONS,
        help='vector observation/profile mode to include; may be repeated',
    )
    parser.add_argument(
        '--instrumentation',
        action='store_true',
        help='emit vector timing breakdowns when supported',
    )
    parser.add_argument(
        '--cpu-affinity',
        choices=('none', 'round_robin'),
        default='none',
        help='record an explicit CPU affinity experiment mode',
    )
    parser.add_argument('--json', action='store_true', dest='json_output')
    parser.add_argument('--no-progress', action='store_false', dest='progress')
    parser.add_argument('--backup-interval', type=int)
    parser.add_argument('--restore-interval', type=int)
    parser.set_defaults(progress=True)
    return parser


def main(argv=None):
    """Run the benchmark command line interface."""
    parser = _parser()
    args = parser.parse_args(argv)

    profile_count = sum((
        bool(args.observation_profile),
        bool(args.ram_profile),
        bool(args.vector_profile),
        bool(args.profile_rom),
    ))
    if profile_count > 1:
        parser.error(
            'choose only one of --observation-profile, --ram-profile, '
            '--vector-profile, or --profile-rom'
        )

    if args.observation_profile:
        if args.rom is None:
            parser.error(
                '--rom is required when --observation-profile is provided'
            )
        try:
            results = run_observation_profile(
                args.rom,
                steps=args.steps,
                warmup_steps=args.warmup_steps,
                seed=args.seed,
                action_policy=args.action_policy,
            )
        except KeyboardInterrupt:
            return 130

        if args.json_output:
            print(json.dumps(
                [result.to_dict() for result in results],
                sort_keys=True,
            ))
        else:
            print(format_observation_profile(results))
        return 0

    if args.ram_profile:
        if args.rom is None:
            parser.error('--rom is required when --ram-profile is provided')
        try:
            results = []
            for _ in range(args.runs):
                results.extend(run_ram_profile(
                    args.rom,
                    steps=args.steps,
                    warmup_steps=args.warmup_steps,
                    seed=args.seed,
                    action_policy=args.action_policy,
                ))
        except KeyboardInterrupt:
            return 130

        if args.json_output:
            print(json.dumps(
                [result.to_dict() for result in results],
                sort_keys=True,
            ))
        else:
            print(format_ram_profile(results))
        return 0

    if args.vector_profile:
        if args.rom is None:
            parser.error('--rom is required when --vector-profile is provided')
        try:
            env_counts = tuple(
                int(value)
                for value in args.env_counts.split(',')
                if value.strip()
            )
        except ValueError:
            parser.error('--env-counts must be comma-separated integers')
        try:
            results = run_vector_profile(
                args.rom,
                steps=args.steps,
                warmup_steps=args.warmup_steps,
                seed=args.seed,
                action_policy=args.action_policy,
                env_counts=env_counts,
                observation_modes=args.vector_observation,
                backends=args.vector_backend,
                runs=args.runs,
                instrumentation=args.instrumentation,
                cpu_affinity=args.cpu_affinity,
            )
        except KeyboardInterrupt:
            return 130

        if args.json_output:
            print(json.dumps(
                [result.to_dict() for result in results],
                sort_keys=True,
            ))
        else:
            print(format_vector_profile(results))
        return 0

    if args.profile_rom:
        try:
            results = run_mapper_profile(
                args.profile_rom,
                steps=args.steps,
                warmup_steps=args.warmup_steps,
            )
        except KeyboardInterrupt:
            return 130

        if args.json_output:
            print(json.dumps(
                [result.to_dict() for result in results],
                sort_keys=True,
            ))
        else:
            print(format_mapper_profile(results))
        return 0

    if args.rom is None:
        parser.error(
            '--rom is required unless --profile-rom is provided'
        )

    config = BenchmarkConfig(
        rom=args.rom,
        steps=args.steps,
        warmup_steps=args.warmup_steps,
        seed=args.seed,
        action_policy=args.action_policy,
        render_mode=args.render_mode,
        progress=args.progress,
        backup_interval=args.backup_interval,
        restore_interval=args.restore_interval,
    )
    try:
        result = run_benchmark(config)
    except KeyboardInterrupt:
        return 130

    if args.json_output:
        print(json.dumps(result.to_dict(), sort_keys=True))
    else:
        print(format_result(result))
    return 0


if __name__ == '__main__':
    sys.exit(main())
