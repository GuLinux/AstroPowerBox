try:
    from typing import Callable, Protocol, TYPE_CHECKING
except ImportError:
    class Protocol:
        pass

    class Callable:
        def __class_getitem__(cls, _):
            return object

    TYPE_CHECKING = False
