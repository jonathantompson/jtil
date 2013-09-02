**jtil - Jonathan's Utility Library**
---------
---------

**Overview**
--------

This library is a random collection of code snippets and useful algorithm implementations.  It is the base library for many of my projects and is a way for me to create reusable code.  It is supported on Windows 7 (64 bit) and Mac OS X (gcc 4.7 or greater).  Support for linux systems will come soon.  Features include:

- A _reasonably_ full featured Matrix and Vector math library
- An implementation of a few Optimization algorithms (PSO, Levenberg-Marquardt, BFGS)
- Lots of string utilities
- A collection of basic non-STL data structures (hash tables, vectors, heaps, etc)
- Cross-platform utility objects (such as clocks/timers, thread-pools, callback binding, etc)
- A simple csv parser
- Image processing utilities (not very clean, but a number of filters are implemented)
- A marching squares implementation
- An openGL, fully-featured deferred renderer.  See the PRenderer project for details.
- A mesh decimation engine
- A simple ICP implementation
- A simple "Surface Simplification Using Quadric Error Metrics" implementation
- Lots of other random stuff...

**Compilation**
---------------

Building jtil uses Visual Studio 2012 on Windows, and cmake + gcc 4.7 (or greater) on Mac OS X.  I have tried to keep the dependency list to a minimum, but unfortunately I don't have time to implement everything.  Dependencies that need to be built:

- Assimp V3.0.1270 for Mac and pulled from github 6/29/13 for Win7 (for mesh import)
- Freeimage V3.15.4 (image imports)
- GLFW V3.0.1 for Win7 and pulled from github July 10th for Mac (cross platform windows manager)
- LibRocket (pulled from github 8/12/13 for Mac & 6/29/13 for Win7) with Freetype V2.4.3 (for rss / css UI parsing)

Header libraries or dependancies included in the repo (you don't have to build these):

- Eigen (Matrix decomposition)
- NanoFlann (Fast KD-Tree search)
- UCL V1.03 (compression library)
- FastLZ (real-time compression library)
- Glew

For specific instructions on building each dependency see: 

DEPENDANCY\_COMPILE\_NOTES.txt

VS2012 and cmake expect a specific directory structure:

- \\include\\WIN\\
- \\include\\MAC\_OS\_X\\
- \\lib\\WIN\\
- \\lib\\MAC\_OS\_X\\
- \\jtil\\

So the dependancy headers and static libraries (.lib on Windows and .a on Mac OS X) are separated by OS and exist in directories at the same level as jtil.  I have pre-compiled the dependencies and put them in dropbox, let me know if you need the link.

**Style**
---------

This project follows the Google C++ style conventions: 

<http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml>

Some highlights of the style:

- class names are camal-caps
- class getter and setter methods have the same name as the variable
- other class methods start lower case then are camel caps after
- class private variables should have trailing "_"

You can integrate cpplint (google's python based style check into Visual studio): 

- Go to Tools -> External Tools -> Add
- Title: cpplint.py
- Command: C:\Python27\python.exe
- Arguments: C:\Users\HomeComputer\Documents\cpplint.py --output=vs7 --filter=-build/header_guard,-legal/copyright,-whitespace/end_of_line,-runtime/arrays,-readability/streams $(ItemPath) 
- Initial directory: $(ItemPath)
- Check Use Output window
- Move cpplint.py to the top using the button on the right
To bind cpplint to a keyboard shortcut:
- Tools -> Options -> Environment -> Keyboard
- Enter "Tools.ExternalCommand1"
- Change "Use new shortcut in" to "Text Editor"
- Press a shortcut key and assign it
- Press OK.

