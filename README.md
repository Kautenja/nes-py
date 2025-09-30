# nes-py

[![Build Status](https://github.com/Kautenja/nes-py/actions/workflows/test.yml/badge.svg)](https://github.com/Kautenja/nes-py/actions/workflows/test.yml)
[![Version](https://img.shields.io/badge/version-9.0.0-blue)](https://badge.fury.io/py/nes-py)
[![Python Version](https://img.shields.io/badge/python-3.9%20|%203.10%20|%203.11%20|%203.12-blue)](https://python.org)
[![License](https://img.shields.io/pypi/l/nes-py.svg)](/sajmon83/nes-py/blob/master/LICENSE)

`nes-py` is a NES emulator and Gymnasium interface for macOS, Linux, and Windows based on the [SimpleNES](https://github.com/amhndu/SimpleNES) emulator.

**Disclaimer:** This repository is a fork of the original [Kautenja/nes-py](https://github.com/Kautenja/nes-py) project. The modifications implemented here aim to update the library for compatibility with newer Python versions (3.9-3.12) and modern environments like Gymnasium.

## Installation

The preferred installation of `nes-py` is from `pip`:
```bash
pip install nes-py
```
Or, install this fork directly from GitHub for the latest updates:
```bash
pip install git+https://github.com/sajmon83/nes-py.git
```

## Usage

```python
import gymnasium as gym
import nes_py

# create the NES environment
env = nes_py.NESEnv('path/to/your/rom.nes')

# reset the environment to get the initial state
obs, info = env.reset()

# loop through the game
while True:
    # sample a random action
    action = env.action_space.sample()
    # perform the action
    obs, reward, terminated, truncated, info = env.step(action)
    # check if the episode is over
    if terminated or truncated:
        # reset the environment
        obs, info = env.reset()

# close the environment
env.close()
```

## Multi-Process Rendering with Stable-Baselines3

To render multiple environments simultaneously at full speed (e.g., during training), you must use `SubprocVecEnv` from `stable-baselines3`. This ensures that each environment runs in its own process, allowing independent rendering without performance degradation.

### Example Implementation:

```python
# train.py
from stable_baselines3.common.env_util import make_vec_env
from stable_baselines3.common.vec_env import SubprocVecEnv

def train_model(config):
    # Use SubprocVecEnv for multi-process rendering
    render_enabled = config.get('render_training', False)
    vec_env_class = SubprocVecEnv if render_enabled else None

    train_env = make_vec_env(
        lambda: create_mario_env(config), 
        n_envs=config['n_envs'], 
        seed=42,
        vec_env_cls=vec_env_class
    )
    
    # ... rest of the training script
```

By setting `vec_env_cls=SubprocVecEnv`, each of the `n_envs` will open its own window and render independently, leveraging multiple CPU cores.

## Controls

| Keyboard Key | NES Joypad |
| :----------: | :--------: |
|      W       |     Up     |
|      A       |    Left    |
|      S       |    Down    |
|      D       |   Right    |
|      O       |     A      |
|      P       |     B      |
|    Enter     |   Start    |
|    Space     |   Select   |

## Disclaimer

This project is provided for educational purposes only. It is not affiliated with and has not been approved by Nintendo.
