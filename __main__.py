"""The main script for development testing."""
import sys
from tqdm import tqdm
from nes_py.nes_env import NesENV
import numpy as np

# get the path for the ROM from the command line
path = sys.argv[1]


# create the environment
env = NesENV(path)


# run through some random steps in the environment
try:
    done = True
    for _ in tqdm(range(500)):
        if done:
            _ = env.reset()
        action = env.action_space.sample()
        _, reward, done, _ = env.step(action)
        env.render()
except KeyboardInterrupt:
    pass


# close the environment
env.close()
