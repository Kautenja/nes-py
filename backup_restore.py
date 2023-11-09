from nes_py import NESEnv
import tqdm
env = NESEnv('./nes_py/tests/games/super-mario-bros-1.nes')

done = True

try:
    for i in tqdm.tqdm(range(5000)):
        if done:
            state = env.reset()
            done = False
        else:
            state, reward, terminated, truncated, info = env.step(env.action_space.sample())
            done = terminated or truncated
        if (i + 1) % 12:
            env._backup()
        if (i + 1) % 27:
            env._restore()
except KeyboardInterrupt:
    pass
