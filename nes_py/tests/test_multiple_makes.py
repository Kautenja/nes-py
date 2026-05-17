"""Test multiple NESEnv instances in one process and across workers."""
from multiprocessing import Process
from threading import Thread
from unittest import TestCase

from nes_py.tests.rom_file_abs_path import rom_file_abs_path
from nes_py.nes_env import NESEnv


def play(steps):
    """Play the environment with deterministic actions for a few steps."""
    path = rom_file_abs_path('super-mario-bros-1.nes')
    env = NESEnv(path)
    try:
        done = True
        for step in range(steps):
            if done:
                _ = env.reset(seed=step)
            _, _, terminated, truncated, _ = env.step(
                step % env.action_space.n
            )
            done = terminated or truncated
    finally:
        env.close()


class ShouldMakeMultipleEnvironmentsParallel(object):
    """An abstract test case to make environments in parallel."""

    # the class to the parallel initializer (Thread, Process, etc.)
    parallel_initializer = lambda target, args: None

    # the number of parallel executions
    num_execs = 4

    # the number of steps to take per environment
    steps = 10

    def test(self):
        procs = [None] * self.num_execs
        args = (self.steps, )
        # spawn the parallel instances
        for idx in range(self.num_execs):
            procs[idx] = self.parallel_initializer(target=play, args=args)
            procs[idx].start()
        # join the parallel instances
        for proc in procs:
            proc.join()
            self.assertFalse(proc.is_alive())
            exitcode = getattr(proc, 'exitcode', 0)
            self.assertEqual(0, exitcode)


class ProcessTest(ShouldMakeMultipleEnvironmentsParallel, TestCase):
    """Test that processes (true multi-threading) work."""
    parallel_initializer = Process


class ThreadTest(ShouldMakeMultipleEnvironmentsParallel, TestCase):
    """Test that threads (internal parallelism) work"""
    parallel_initializer = Thread


class ShouldMakeMultipleEnvironmentsSingleThread(TestCase):
    """Test multiple environments in one process keep public state separate."""

    # the number of environments to spawn
    num_envs = 4

    # the number of steps to take per environment
    steps = 10

    def test(self):
        path = rom_file_abs_path('super-mario-bros-1.nes')
        envs = [NESEnv(path) for _ in range(self.num_envs)]
        try:
            dones = [True] * self.num_envs

            for step in range(self.steps):
                for idx, env in enumerate(envs):
                    if dones[idx]:
                        _ = env.reset(seed=idx)
                    action = (step + idx) % env.action_space.n
                    _, _, terminated, truncated, _ = env.step(action)
                    dones[idx] = terminated or truncated

            envs[0].ram[0] = 0x2a
            envs[1].ram[0] = 0x11
            envs[0].controllers[0][0] = 0x80
            envs[1].controllers[0][0] = 0x01
            self.assertEqual(0x2a, envs[0].ram[0])
            self.assertEqual(0x11, envs[1].ram[0])
            self.assertEqual(0x80, envs[0].controllers[0][0])
            self.assertEqual(0x01, envs[1].controllers[0][0])

            envs[0].close()
            _, _, _, _, _ = envs[1].step(0)
        finally:
            for env in envs:
                try:
                    env.close()
                except ValueError:
                    pass
