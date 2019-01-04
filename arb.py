from nes_py import NESEnv
env = NESEnv('./nes_py/tests/games/smb1.nes')
env.render('human')

done = True

try:
    while True:
        if done:
            state = env.reset()
            done = False
        else:
            state, reward, done, info = env.step(env.action_space.sample())
        env.render('human')
except KeyboardInterrupt:
    pass
