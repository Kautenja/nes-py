"""Benchmark helpers and CLI for measuring NES environment throughput."""
import argparse
import json
import platform as platform_module
import sys
import sysconfig
import time
from dataclasses import asdict
from dataclasses import dataclass
from typing import Callable

import numpy as np
from tqdm import tqdm

from ._rom import ROM
from .nes_env import NESEnv
from .nes_env import _native_mapper_hook_smoke_results


ActionPolicy = Callable[[NESEnv, int, np.random.RandomState], int] | str


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
class MapperHookBenchmarkResult:
    """Structured result from the native mapper hook smoke benchmark."""

    environment: str
    compiler: str
    platform: str
    operation: str
    measured_iterations: int
    warmup_steps: int
    elapsed_seconds: float
    iterations_per_second: float
    results: dict[str, bool]

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
    """Reset an environment and normalize Gym/Gymnasium return values."""
    if seed is None:
        result = env.reset()
    else:
        try:
            result = env.reset(seed=seed)
        except TypeError:
            env.seed(seed)
            result = env.reset()
    if isinstance(result, tuple) and len(result) == 2:
        return result
    return result, {}


def _step(env, action):
    """Step an environment and normalize Gym/Gymnasium return values."""
    result = env.step(action)
    if len(result) == 5:
        observation, reward, terminated, truncated, info = result
        return observation, reward, bool(terminated or truncated), info
    observation, reward, done, info = result
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


def _measure_mapper_operation(env, operation, steps, warmup_steps):
    """Measure one mapper benchmark operation."""
    if operation == 'reset':
        action = lambda: env.reset()
    elif operation == 'step':
        env.reset()
        action = lambda: _step(env, 0)
    elif operation == 'render_rgb_array':
        env.reset()
        action = lambda: env.render('rgb_array')
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
            env = NESEnv(rom)
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


def run_mapper_hook_profile(iterations=5000, warmup_steps=100):
    """
    Run a portable benchmark profile for native mapper timing/IRQ hooks.

    This benchmark intentionally repeats the focused C++ smoke checks from the
    mapper lifecycle tests. It is informational, not a CI timing threshold.
    """
    if iterations <= 0:
        raise ValueError('iterations must be positive')
    _validate_non_negative('warmup_steps', warmup_steps)

    for _ in range(warmup_steps):
        _native_mapper_hook_smoke_results()

    results = {}
    started_at = time.perf_counter()
    for _ in range(iterations):
        results = _native_mapper_hook_smoke_results()
    elapsed = time.perf_counter() - started_at
    iterations_per_second = iterations / elapsed if elapsed > 0 else 0.0
    return MapperHookBenchmarkResult(
        environment=_environment_name(),
        compiler=_compiler_name(),
        platform=platform_module.platform(),
        operation='mapper_hook_smoke',
        measured_iterations=iterations,
        warmup_steps=warmup_steps,
        elapsed_seconds=elapsed,
        iterations_per_second=iterations_per_second,
        results=results,
    )


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
    env = NESEnv(config.rom)
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
                env.render(config.render_mode)
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


def format_mapper_hook_profile(result):
    """Return human-readable native mapper hook benchmark output."""
    return (
        'NES mapper hook benchmark profile\n'
        '  {operation}: {iterations_per_second:.2f} iterations/s '
        '({elapsed_seconds:.6f}s)'
    ).format(**result.to_dict())


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
    parser.add_argument(
        '--mapper-hook-profile',
        action='store_true',
        help='run the native mapper timing/IRQ hook smoke benchmark',
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
    if args.mapper_hook_profile and args.profile_rom:
        parser.error(
            '--mapper-hook-profile cannot be combined with --profile-rom'
        )

    if args.mapper_hook_profile:
        try:
            result = run_mapper_hook_profile(
                iterations=args.steps,
                warmup_steps=args.warmup_steps,
            )
        except KeyboardInterrupt:
            return 130

        if args.json_output:
            print(json.dumps(result.to_dict(), sort_keys=True))
        else:
            print(format_mapper_hook_profile(result))
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
            '--rom is required unless --profile-rom or '
            '--mapper-hook-profile is provided'
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
