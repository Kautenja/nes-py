"""Test that the multiprocessing package works with the env."""
import os
from multiprocessing import Process
from threading import Thread
from unittest import TestCase
from ..nes_env import NESEnv


def play(steps):
    """
    Play the environment making uniformly random decisions.

    Args:
        steps (int): the number of steps to take

    Returns:
        None

    """
    # create an NES environment with Super Mario Bros.
    path =  os.path.join(os.path.dirname(__file__), 'games/smb1.nes')
    env = NESEnv(path)
    # step the environment for some arbitrary number of steps
    done = True
    for step in range(steps):
        if done:
            state = env.reset()
        action = env.action_space.sample()
        state, reward, done, info = env.step(action)
    # close the environment
    env.close()


class ShouldMakeMultipleEnvironemntsParallel(object):

    # the class to the parallel initializer (Thread, Process, etc.)
    parallel_initializer = None

    # the number of parallel executions
    num_execs = 4

    # the number of steps to take per environment
    steps = 1000

    def test(self):
        from ..nes_env import NESEnv
        procs = [None] * self.num_execs
        args = (self.steps, )
        # spawn the parallel instances
        for idx in range(self.num_execs):
            procs[idx] = self.parallel_initializer(target=play, args=args)
            procs[idx].start()
        # join the parallel instances
        for proc in procs:
            proc.join()


class ProcessTest(ShouldMakeMultipleEnvironemntsParallel, TestCase):
    parallel_initializer = Process


class ThreadTest(ShouldMakeMultipleEnvironemntsParallel, TestCase):
    parallel_initializer = Thread


# class ShouldMakeMultipleEnvironmentsSingleThread(TestCase):

#     # the number of environments to spawn
#     num_envs = 4

#     # the number of steps to take per environment
#     steps = 1000

#     def test(self):
#         from ..nes_env import NESEnv
#         path =  os.path.join(os.path.dirname(__file__), 'games/smb1.nes')
#         envs = [NESEnv(path) for _ in range(self.num_envs)]
#         dones = [False] * self.num_envs

#         for step in range(self.steps):
#             for idx in range(self.num_envs):
#                 if dones[idx]:
#                     state = envs[idx].reset()
#                 action = envs[idx].action_space.sample()
#                 state, reward, done, info = envs[idx].step(action)
