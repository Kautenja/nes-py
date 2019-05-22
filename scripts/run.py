from nes_py import NESEnv
import tqdm
env = NESEnv('./nes_py/tests/games/super-mario-bros-1.nes')

done = True

try:
    for _ in tqdm.tqdm(range(5000)):
        if done:
            state = env.reset()
            done = False
        else:
            state, reward, done, info = env.step(env.action_space.sample())
except KeyboardInterrupt:
    pass
