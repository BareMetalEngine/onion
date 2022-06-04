# What is this?
This repository contains compiled binaries for Onion Build Tool

[![Build Onion Tool](https://github.com/BareMetalEngine/onion_tool/actions/workflows/push.yml/badge.svg?branch=main)](https://github.com/BareMetalEngine/onion_tool/actions/workflows/push.yml)

Original source code this tool could be found here: [Onion Build Tool](https://github.com/BareMetalEngine/onion_tool/).

*NOTE*: Since the onion.exe contains additional executables and other files packed inside it Windows Defender may complain about it. I'm working on code-signing the executable to alleviate this problem.

# How to use ONION?

Onion Build Tool is in essense package manager and solution generator. It can generate both Visual Studio project files and CMake files.

* Basic unit in Onion infrastructure is a Project, that consists of compilable files. 
* Projects are usually groupped in Modules that form standalone solutions.
* Modules can reference other Modules to allow for code sharing
* Modules are usually published as single repositories in GitHub

# Project definition

Basic onion project definition is an XML file named `project.onion`. 

```
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<Project>
	<Type>Application</Type>
</Project>
```

At minimum, the project XML must specify what is the output type of the compilation. Following types are supported:

* Application - project compiles to standalone executable file (main.cpp is generated if not specified)
* Library - project compiles to a library (static or shared, depends on compilation settings)
* DynamicLibrary - project always compiles to a dynamic library 
* StaticLibrary - project always compiles to a static library

Projects could reference other project via the `<Dependency>` array:

```
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<Project>
	<Type>Application</Type>
	<Dependency>other_project</Dependency>
</Project>
```

# Module definition

Basic module definition is also an XML file, named `module.onion`:

```
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<Module name="AppWithLib">
	<Project>app</Project>
	<Project>lib</Project>
</Module>
```

At minimum the module definition must specify the module name (globally unique) and the list of projects it consists of. Project are *NOT* enumerated automatically so each project must be manually added to the module manifest.

All projects must reside in the "code" sub-directory inside the module.

```
C:\projects\module\module.onion
C:\projects\module\code\math\project.onion
```

e.g:

```
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<Module name="SampleMathModule">
	<Project>math</Project>
</Module>
```

# Referencing other modules

Modules may reference other modules in GitHub. In such cases those modules are locally cloned and assembled together into one big solution effectivelly pulling that remote source code.

```
<?xml version="1.0" encoding="utf-8" standalone="no" ?>
<Module name="AppWithExternalStaticLib">
	<Project>app</Project>
	<Dependency repo="https://github.com/BareMetalEngine/onion_test.git" path="module_with_static_lib"/>	
</Module>
```

# Using Onion to build code

Building code with onion consists of 3 steps:
* Configuring the module - this downloads all the dependencies
* Generating solution/make file - this scans the downloaded modules and generates the build files
* Actual building - that can happen internally or via external tools

Syntax for all commands are similar:
```
onion configure [arguments]
onion make [arguments]
onion build [arguments]
```

For all commands most important argument is the `-module=<path>` arguments that should point the `module.onion` file we want to process.

# Configuring

```
onion configure [options]

Required arguments:
-module=<path to module to configure>\n";

Optional arguments:
-configPath=<path where the generated configuration should be written\n";
```

This tool scans the dependencies of the given module (especially other referenced repositories) and downloads them to internal cache. Manifest (XML file) for configured module is written to the internal location in the module or to the specified file.

# Make file generation

This is the most complex step. By design choice Onion Build Tool generates solution for only ONE specific configuration at a time. There are 5 things that are considered important when choosing which exact build configuration to genreate:

*Config* (`-config`) - Determines the primary build configuration. Allowed options are:
 * debug - Compiles with no optimizations, log and debugging and assertions enabled
 * checked - Compiles with full optimizations, non-verbose log and assertions enabled (default option)
 * release - Compiles with full optimizations, minimal log (errors only) and no assertions
 * final - Compiles with full optimizations, link-time code generation and no log and no assertions

*Platform* (`-platform`) - Determines the target OS we want to build for. Allowed options are:
 * windows - WinAPI windows solution, only configuration with full support for development tools (editor, etc)
 * uwp - Windows Unified Platform solution, game/runtime only, no tools
 * linux - Standard linux solutions, most tools and runtime will work but it's meant mostly for headless server
 * scarlett - Console platform, game runtime only. You need appropiate SDK to generate solution for this platform.
 * prospero - Console platform, game runtime only. You need appropiate SDK to generate solution for this platform.
 * ios - iOS platform, requires Mac to work
 * android - Android platform, works only on Windows

*Libs* (`-libs`) - Determines if static or shared libraries should be preferred. Allowed options are:
 * shared - All libraries are compiled as DLL/so if possible, better for dev work and incremental recompilation
 * static - All libraires are compiled as static libraries, better for more final and standalone builds 

*Generator* (`-generator`) - Determines the type of solution the tool should output
 * vs2019 - Outputs the Visual Studio 2019 solution (deprecated)
 * vs2022 - Outputs the Visual Studio 2022 solution - default on Windows
 * cmake - Outputs CMake build files - default on Linux/Mac

# Usage example

Assuming module in `C:\projects\test\module.onion`

```
onion configure -module="C:\projects\test\module.onion"
onion make -module="C:\projects\test\module.onion" -config=release
onion make -module="C:\projects\test\module.onion" -config=debug
onion build -module="C:\projects\test\module.onion"
```

