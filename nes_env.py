"""A CTypes interface to the C++ NES environment."""
import ctypes
# from ctypes import cdll, c_wchar_p
from gym import Env


# load the library from the shared object file
_LIB = ctypes.cdll.LoadLibrary('build/lib_nes_env.so')


class NesENV(Env):
    """An NES environment based on the LaiNES emulator."""

    # relevant metadata about the environment
    metadata = {
        'render.modes': ['rgb_array', 'human']
    }

    def __init__(self, rom_path):
        """
        Create a new NES environment.

        Args:
            path (str): the path to the ROM for the environment

        Returns:
            None

        """
        self._rom_path = rom_path
        # initialize the C++ object for running the environment
        self._env = _LIB.NESEnv_init(ctypes.c_wchar_p(self._rom_path))

    def step(self, action):
        """
        Run one frame of the NES and return the relevant observation data.

        Args:
            action (byte): the bitmap determining which buttons to press

        Returns:
            a tuple of:
            - state (np.ndarray): next frame as a result of the given action
            - reward (float) : amount of reward returned after given action
            - done (boolean): whether the episode has ended
            - info (dict): contains auxiliary diagnostic information

        """
        # pass the action to the emulator
        _LIB.NESEnv_step(self._env, ctypes.c_ubyte(action))
        # TODO: call method for getting frame
        # TODO: call method for reward
        # TODO: call method for done
        return None, 0, False, {}

    def reset(self):
        """
        Reset the state of the environment and returns an initial observation.

        Returns:
            state (np.ndarray): next frame as a result of the given action

        """
        # call reset on the emulator
        _LIB.NESEnv_reset(self._env)
        # TODO: call method for getting frame and return it
        return None

    def render(self, mode='human'):
        """
        Render the environment.

        Args:
            mode (str): the mode to render with:
            - human: render to the current display
            - rgb_array: Return an numpy.ndarray with shape (x, y, 3),
              representing RGB values for an x-by-y pixel image

        Returns:
            a numpy array if mode is 'rgb_array', None otherwise

        """
        if mode == 'human':
            raise NotImplementedError('TODO: human mode')
        elif mode == 'rgb_array':
            raise NotImplementedError('TODO: rgb_array mode')
        else:
            render_modes = self.metadata['render.modes']
            msg = 'valid render modes are {}'.format(render_modes)
            raise NotImplementedError(msg)

    def close(self):
        """Close the environment."""
        # make sure the environment hasn't already been closed
        if self._env is None:
            raise ValueError('env has already been closed.')
        # purge the environment from C++ memory
        _LIB.NESEnv_close(self._env)
        # deallocate the object locally
        self._env = None


# explicitly define the outward facing API of this module
__all__ = [NesENV.__name__]


# handle running this environment as the main script
if __name__ == '__main__':
    import sys
    path = sys.argv[1]

    # create the environment
    env = NesENV(path)
    # run through some random steps in the environment
    try:
        done = True
        for _ in range(500):
            if done:
                _ = env.reset()
            action = 8 # env.action_space.sample()
            _, reward, done, _ = env.step(action)
            # env.render()
    except KeyboardInterrupt:
        pass
    # close the environment
    env.close()
