ptex-tools
==========

Tools for working with ptex textures

Dependencies
------------

- cmake 2.8.10 or newer
- ptex 2.0 or newer
- python 2.7

Building
--------

```
> mkdir build
> cd build
> cmake ..
> make && make install
```

Configure options
-----------------

- `BIN_INSTALL_DIR` - where to install executable files, default `bin`
- `PYTHON_INSTALL_DIR` - where to install python modules, default `python`

Options for finding python executable/libraries are from cmake FindPython*.cmake modules
in your installation.
Example for Autodesk Maya built-in interpreter:

```
cmake -DPYTHON_EXECUTABLE=${MAYA_LOCATION}/bin/mayapy \
      -DPYTHON_LIBRARY=${MAYA_LOCATION}/lib/libpython2.7.so \
      -DPYTHON_INCLUDE_DIR=${MAYA_LOCATION}/include/python2.7 ..
```

