"""Test that the multiprocessing package works with the env."""
from multiprocessing import Process
from threading import Thread
from unittest import TestCase
from .rom_file_abs_path import rom_file_abs_path
from nes_py.nes_env import NESEnv


def play(steps):
    """
    Play the environment making uniformly random decisions.

    Args:
        steps (int): the number of steps to take

    Returns:
        None

    """
    # create an NES environment with Super Mario Bros.
    path = rom_file_abs_path('super-mario-bros-1.nes')
    env = NESEnv(path)
    # step the environment for some arbitrary number of steps
    done = True
    for _ in range(steps):
        if done:
            _, _ = env.reset()
        action = env.action_space.sample()
        _, _, done, _, _ = env.step(action)
    # close the environment
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
        args = (self.steps,)
        # spawn the parallel instances
        for idx in range(self.num_execs):
            procs[idx] = self.parallel_initializer(target=play, args=args)
            procs[idx].start()
        # join the parallel instances
        for proc in procs:
            proc.join()


class ProcessTest(ShouldMakeMultipleEnvironmentsParallel, TestCase):
    """Test that processes (true multi-threading) work."""
    parallel_initializer = Process


class ThreadTest(ShouldMakeMultipleEnvironmentsParallel, TestCase):
    """Test that threads (internal parallelism) work"""
    parallel_initializer = Thread


class ShouldMakeMultipleEnvironmentsSingleThread(TestCase):
    """Test making 4 environments in a single code stream."""

    # the number of environments to spawn
    num_envs = 4

    # the number of steps to take per environment
    steps = 10

    def test(self):
        path = rom_file_abs_path('super-mario-bros-1.nes')
        envs = [NESEnv(path) for _ in range(self.num_envs)]
        dones = [True] * self.num_envs

        for _ in range(self.steps):
            for idx in range(self.num_envs):
                if dones[idx]:
                    _, _ = envs[idx].reset()
                action = envs[idx].action_space.sample()
                _, _, dones[idx], _, _ = envs[idx].step(action)
