# resman

Cross-platform resource compiler and manager based on llvm/clang

| Linux | macOS | Windows |
| ----- | ----- | ------- |
| [![Linux build status](https://travis-matrix-badges.herokuapp.com/repos/nohajc/resman/branches/master/2)](https://travis-ci.org/nohajc/resman) | [![macOS build status](https://travis-matrix-badges.herokuapp.com/repos/nohajc/resman/branches/master/1)](https://travis-ci.org/nohajc/resman) | [![Windows build status](https://ci.appveyor.com/api/projects/status/github/nohajc/resman?branch=master&svg=true)](https://ci.appveyor.com/project/nohajc/resman) |

## Table of Contents

- [resman](#resman)
  * [About](#about)
  * [How to use](#how-to-use)
  * [Installation](#installation)
  * [Building from source](#building-from-source)
  * [How it works](#how-it-works)

## About

This is a cross-platform solution for embedding resource files into executables.<sup>[1](#footnote1)</sup>

### Goals
* No boilerplate
* Efficient intermediate representation
* Universal solution for all common platforms
* No runtime dependencies

### Existing approaches to this problem
* Using Windows resource management (only available for Windows PE files)
* Converting your files into C/C++ byte arrays which can be compiled (this can produce very large source files)
* Using utilities like objcopy (quite low-level, Linux-only AFAIK)
* Using the Qt Resource System (adds runtime dependency on Qt)

As you can see, none of the above can fulfill all the goals.
However, with the help of LLVM and Clang, we can do better.

---------

<sup><a name="footnote1">1</a></sup> Note that we're talking about embedding during linking phase. This tool is not able to modify existing executables.

## How to use

This project consists of a resource compiler __rescomp__ and a header file __include/resman.h__.

Instead of converting your files into C/C++ byte arrays, you just declare
which files you want to embed like this:

__resource_list.h__
```c++
#pragma once
#include "resman.h"

// Define a global variable for each file
// It will be used to refer to the resource
constexpr resman::Resource<1> gRes1("resource_file1.jpg"); // resource with ID 1
constexpr resman::Resource<2> gRes2("resource_file2.txt"); // resource with ID 2
constexpr resman::Resource<3> gRes3("resource_file3.mp3"); // resource with ID 3
// IDs can be arbitrary but they should be unique across all translation units in your project
...
```
This header will be used to access the resources from your application code. If you include it and compile your project now, you will get a linker error because the resources were just declared.

To actually embed all the files, run __rescomp__ on the same header file:
```sh
$ rescomp resource_list.h -o resource_bundle.o [-R resource_search_path] [-I resman_include_path]
```
This will generate a static library or an object file containing the resource contents (as if compiled from the textual byte array). Clang parser is used to extract resource IDs and file names (and to do some error checking), LLVM is responsible for emitting the correct native object format.

Now, if you want to use a resource, construct a __ResourceHandle__ from the global __Resource<_ID_>__ variable:
```c++
#include "resource_list.h"
...
resman::ResourceHandle handle{gRes1};

// ResourceHandle provides convenient interface to do things like:

// iterate over bytes
for (char c : handle) { ... }

// convert bytes to string
std::string str{handle.begin(), handle.end()};

// query size and id
unsigned size = handle.size();
unsigned id = handle.id();

```

Make sure the linker can find __rescomp__'s output and your project should build now.

__So, to summarise:__ Instead of generating byte arrays, you just write a header file with the list of resources.
Then you run the resource compiler to generate object files or static libraries directly from that list.
That same list will also be be used to access your embedded resources.

### Configuration
The __rescomp__ interface is quite simple. It just takes one or more header files and produces one object file or static library. All resources declared in these headers are packed into the output file.

#### Required parameters
<pre>
&lt;input_file&gt; [&lt;input_file&gt; ...]       Input header file(s)
-o &lt;output_file&gt;                      Output format is determined from the file extension you provide
</pre>
 
#### Optional parameters
<pre>
-I &lt;directory&gt; [-I &lt;directory&gt; ...]   Include search path
-R &lt;directory&gt; [-R &lt;directory&gt; ...]   Resource search path
</pre>
 
Some directories are always added into search path implicitly:
* Current working directory (resources and includes)
* Program directory (includes only)

The inclusion of program directory is just for convenience; i.e. if you have __resman.h__ saved next to __rescomp__, includes like ```<resman.h>``` or ```"resman.h"``` will be resolved without any additional ```-I``` parameters.

### Build system integration
For an example project that uses CMake, see the _examples_ directory.

Generally, any build system which supports custom targets can be used.

## Installation
Prebuilt executables for Windows/Linux/macOS are available in the release section. Just unpack anywhere and enjoy!

The Linux binaries are linked statically so that they can run on any distribution.

I used Void Linux with musl libc which supports true static linking [unlike glibc](https://www.musl-libc.org/intro.html).

[Download latest release](https://github.com/nohajc/resman/releases/latest)

## Building from source

### Dependencies
* LLVM/Clang 6

For __Linux__, there is a docker image prepared which has all the necessary dependencies installed. You can see how to invoke the build in _build_scripts/linux/travis_script.sh_.

On __macOS__, you can download LLVM/Clang from the [official website](http://releases.llvm.org/download.html#6.0.1) and then invoke the build (see _build_scripts/osx/_).

If you're building on __Windows__, there is a Visual Studio solution. LLVM/Clang prebuilt libraries are not available officially though. I use my own [build](https://github.com/nohajc/llvm-clang-static-libs-prebuilt/releases) to speed up AppVeyor jobs.

## How it works
_TODO_
