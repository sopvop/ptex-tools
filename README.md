ptex-tools
==========

Tools for working with ptex textures

Usage
-----

    > ptex-tool reverse input.ptx output.ptx

Reverse winding order in ptx texture. Useful when you need to render
alembic geometry with texture created from maya model.

    > ptex-tool merge input.ptx input2.ptx [input3.ptx ..] output.ptx
    0:input.ptx
    555:input2.ptx
    1337:input3.ptx

Merge ptex textures into one. Outputs offsets for accessing individual textures.
All textures should have same format.

Also includes `ptexutls` python module exposing this functionality. 

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

- `BIN_INSTALL_DIR` - where to install executable files, default `bin`.
- `PYTHON_INSTALL_DIR` - where to install python modules, default `python`.
- `PTEX_LOCATION` - ptex library installation root, by default is empty.
- `PTEX_INCLUDE_DIR` - path to PTex includes.
- `PTEX_LIBARY` - path to PTex library.

Options for finding python executable/libraries are from cmake FindPython*.cmake modules
in your installation.
Example for Autodesk Maya built-in interpreter:

```
cmake -DPYTHON_EXECUTABLE=${MAYA_LOCATION}/bin/mayapy \
      -DPYTHON_LIBRARY=${MAYA_LOCATION}/lib/libpython2.7.so \
      -DPYTHON_INCLUDE_DIR=${MAYA_LOCATION}/include/python2.7 ..
```

