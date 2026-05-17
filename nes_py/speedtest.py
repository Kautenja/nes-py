"""Benchmark helpers and CLI for measuring NES environment throughput."""
import argparse
import json
import sys
import time
from dataclasses import asdict
from dataclasses import dataclass
from typing import Callable

import numpy as np
from tqdm import tqdm

from .nes_env import NESEnv


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


def _parser():
    """Build the command line parser."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, help='path to a .nes ROM')
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
    args = _parser().parse_args(argv)
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
