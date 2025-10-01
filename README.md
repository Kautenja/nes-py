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

## Features

### Core Functionality
- **NES Emulation**: Full NES emulator based on SimpleNES
- **Gymnasium Integration**: Compatible with modern Gymnasium API
- **Multi-platform Support**: Works on Windows, macOS, and Linux
- **Python 3.9-3.12**: Modern Python support with type hints

### Advanced Features
- **Multi-process Rendering**: Optimized for parallel training with Stable-Baselines3
- **Frame Buffer Access**: Direct access to screen and RAM buffers
- **Controller Support**: Full NES controller emulation
- **State Management**: Backup and restore emulator state

### Performance Optimizations
- **SubprocVecEnv Support**: Each environment runs in separate process
- **Memory Efficient**: Optimized memory usage for training
- **Fast Rendering**: Hardware-accelerated rendering when available

## API Reference

### NESEnv Class
```python
class NESEnv(gym.Env):
    def __init__(self, rom_path, render_mode=None)
    def reset(self, seed=None, options=None)
    def step(self, action)
    def render(self)
    def close(self)
```

### Key Methods
- `_screen_buffer()`: Get current screen buffer
- `_ram_buffer()`: Access NES RAM
- `_frame_advance(action)`: Advance single frame
- `_backup()` / `_restore()`: State management

## Integration Examples

### Stable-Baselines3 Integration
```python
from stable_baselines3 import PPO
from stable_baselines3.common.vec_env import SubprocVecEnv
from nes_py import NESEnv

# Create vectorized environment
env = make_vec_env(
    lambda: NESEnv('game.nes'),
    n_envs=8,
    vec_env_cls=SubprocVecEnv
)

### Custom Environment Development
```python
import nes_py

class CustomGameEnv(nes_py.NESEnv):
    def __init__(self):
        super().__init__('custom_rom.nes')
    
    def _get_reward(self):
        # Implement custom reward logic
        return self._read_mem_range(0x07, 1)[0]
    
    def _get_done(self):
        # Implement custom done condition
        return self._is_game_over()
```

## Contributing

This fork focuses on:
- Modern Python compatibility
- Gymnasium API support
- Performance optimizations
- Bug fixes and stability improvements

## License

This project is provided for educational purposes only. It is not affiliated with and has not been approved by Nintendo.
