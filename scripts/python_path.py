import sys
import os

class PythonPath:
    def __init__(self, path, relative_to=None):
        if relative_to:
            self.path = os.path.abspath(os.path.join(os.path.dirname(relative_to), path))
        else:
            self.path = os.path.abspath(path)

    def __enter__(self):
        sys.path.insert(0, self.path)

    def __exit__(self, exc_type, exc_val, exc_tb):
        sys.path.pop(0)
