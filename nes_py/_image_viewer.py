"""A simple class for viewing images using pyglet."""
from pyglet.window import key
from pyglet.window import Window
from pyglet.image import ImageData


# a mapping from pyglet key identifiers to native identifiers
KEY_MAP = {
    key.ENTER: ord('\r'),
    key.SPACE: ord(' '),
}


class ImageViewer(object):
    """A simple class for viewing images using pyglet."""

    def __init__(self, caption, height, width,
        monitor_keyboard=False,
        relevant_keys=None
    ):
        """
        Initialize a new image viewer.

        Args:
            caption (str): the caption/title for the window
            height (int): the height of the window
            width (int): the width of the window
            monitor_keyboard: whether to monitor events from the keyboard
            relevant_keys: the relevant keys to monitor events from

        Returns:
            None
        """
        self.caption = caption
        self.height = height
        self.width = width
        self.monitor_keyboard = monitor_keyboard
        self.relevant_keys = relevant_keys
        self._window = None
        self._pressed_keys = []
        self._is_escape_pressed = False

    @property
    def is_open(self):
        """Return a boolean determining if this window is open."""
        return self._window is not None

    @property
    def is_escape_pressed(self):
        """Return True if the escape key is pressed."""
        return self._is_escape_pressed

    @property
    def pressed_keys(self):
        """Return a sorted list of the pressed keys."""
        return tuple(sorted(self._pressed_keys))

    def _handle_key_event(self, symbol, is_press):
        """
        Handle a key event.

        Args:
            symbol: the symbol in the event
            is_press: whether the event is a press or release

        Returns:
            None

        """
        # remap the key to the expected domain
        symbol = KEY_MAP.get(symbol, symbol)
        # check if the symbol is the escape key
        if symbol == key.ESCAPE:
            self._is_escape_pressed = is_press
            return
        # make sure the symbol is relevant
        if self.relevant_keys is not None and symbol not in self.relevant_keys:
            return
        # handle the press / release by appending / removing the key to pressed
        if is_press:
            self._pressed_keys.append(symbol)
        else:
            self._pressed_keys.remove(symbol)

    def on_key_press(self, symbol, modifiers):
        """Respond to a key press on the keyboard."""
        self._handle_key_event(symbol, True)

    def on_key_release(self, symbol, modifiers):
        """Respond to a key release on the keyboard."""
        self._handle_key_event(symbol, False)

    def open(self):
        """Open the window."""
        # create a window for this image viewer instance
        self._window = Window(
            caption=self.caption,
            height=self.height,
            width=self.width,
            vsync=False,
            resizable=True,
        )

        # add keyboard event monitors if enabled
        if self.monitor_keyboard:
            self._window.event(self.on_key_press)
            self._window.event(self.on_key_release)
            self._window.set_exclusive_keyboard()

    def close(self):
        """Close the window."""
        if self.is_open:
            self._window.close()
            self._window = None

    def show(self, frame):
        """
        Show an array of pixels on the window.

        Args:
            frame (numpy.ndarray): the frame to show on the window

        Returns:
            None
        """
        # check that the frame has the correct dimensions
        if len(frame.shape) != 3:
            raise ValueError('frame should have shape with only 3 dimensions')
        # open the window if it isn't open already
        if not self.is_open:
            self.open()
        # prepare the window for the next frame
        self._window.clear()
        self._window.switch_to()
        self._window.dispatch_events()
        # create an image data object
        image = ImageData(
            frame.shape[1],
            frame.shape[0],
            'RGB',
            frame.tobytes(),
            pitch=frame.shape[1]*-3
        )
        # send the image to the window
        image.blit(0, 0, width=self._window.width, height=self._window.height)
        self._window.flip()


# explicitly define the outward facing API of this module
__all__ = [ImageViewer.__name__]
