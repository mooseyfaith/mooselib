# mooselib

This is a simple OpenGL rendering framework with very little dependencies to the c-runtime (basically printf for console output) and other 3rd-party libraries. It uses the FreeType library to generate font textures for text rendering, which could be easly replaced if necessary. It also uses OpenGL header files from https://www.khronos.org/.
It is implemented in a procedural way (not in object oriented way) and requires C++11 for some widely used features like initializer lists and maybe auto. The only higher level construct used are in the defer macro, wich uses templates and lambdas to enable simple unwind mechanics. This is to make cleanup easier in the case of an error or failed load. There are no exceptions here and certainly no RAII!

This code is heavily inspired by Handmade Hero and Jonathan Blows streams on twitch (can't wait for Jai!).
I want to thank both Jonathan and Casey for reigniting my love for programming and bringing me back to the intuitive way of programming I learned myself as kind, but lost during many years of false education. (This code probably doesn't do them justice, but i'm trying :D)

## Features:

- immidiate mode ui rendering, and debug rendering (wich I will rewrite, since I'm not very happy with there current state)
- Mesh loading from custom mesh format using a custom blender plugin (also sampled mesh animations, but both are worked on)
- simple .tga texture loading
- live-code-editing, run your programm, do logical changes (not data layout), recompile and see the changes without restarting the program
- OpenGL 3.2-ish rendering
- load OpenGL shaders, set vertex attributes and retrive uniforms (still rather boilerplate code and maybe there needs to be a more high-level material system)
- full vector math implementaions for 2-4 dimensinal vectors, quaternions, 4x4 and 4x3 matrices
- string implementation WITHOUT 0-terminated c-strings, with parsing and formatting functions
- uses custom memory allocators; functions that allocate memory require allocators to be passed explicitly (curse and blessing; they may still contain bugs)
- c-style (NOT C++!) template files for commen container types array/buffer/list for now (i am not shure if this is a good practice yet, but they are way better to debug then plain macro functions)
- mostly clean border between platform and application to encurage portability (right now only windows is targeted, so the api will change if required, SDL should be a good target i think)
- immidiate mode platform window management (enables simple control flow for native platform windows; gl calls render to last opened window; if you want to close the window, just skipp the call to the platform_api->window function) also fullscreen support (fullscreen borderless window)
- Complex sample game to checkout how it works see: Asteroids repository. Keep in mind, that the game is not MIT licensed as I don't want people to just copy the game and release it as their work. You may use it as a reference to see how the single parts can work together.
 
## Getting started:

1. Since the codebase is under development, the best way to get started is to also checkout the Asteroids project and start with it as a basis.

2. In build.bat you can change the exe_name and dll_name however you want, also set moose_dir to point to mooselib directory.

3. Setup Visual Studio environment (call any Visual Studio command prompt) and navigate to the project directory (containing build.bat).

4. Run build.bat.

5. A Visual Studio solution will be loaded. Change the working directory from "xxxx\build" to "xxxx\data".
You should close the solution and save the changes. Or save the solution without closing. In the "Solution Explorer" select the solution called "Solution '<exe_name>' (1 project)", (NOT the .exe called <exe_name>) and now you can save the solution under File-> Save <exe_name>.sln.
!! If you don't have a saved solution file named <exe_name>.sln, build.bat will allways open a new solution.!!
  
If you want to debug your application, you can now use this solution. Set break points add files and debug or what ever.

Now build.bat should have created a build directory and a data directory.

your project directory should now look like this:
  * \build
    * <exe_name>.exe
    * <dll_name>.dll
    * <exe_name>.sln
    * ..
  * build.bat

The build directory conains an .exe (windows platform code) and several .dlls (your actual application logic) and some Visual Studio solution files.

If you change the exe_name or dll_name, you should delete the build directory to force build.bat to create a new Visual Studio solution.

If you have any questions, please ask me. I would love to hear peoples opinion on the project.
(Sorry for the spelling errors!)
