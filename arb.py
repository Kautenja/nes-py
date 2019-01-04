from nes_py import NESEnv
env = NESEnv('./nes_py/tests/games/smb1.nes')
env.render('human')

done = False
while True:
    if done:
        env.reset()
    else:
        env.step(env.action_space.sample())
    env.render('human')
