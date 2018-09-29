# resman

Cross-platform resource compiler and manager based on llvm/clang

| Linux | macOS | Windows |
| ----- | ----- | ------- |
| [![Linux build status](https://travis-matrix-badges.herokuapp.com/repos/nohajc/resman/branches/master/2)](https://travis-ci.org/nohajc/resman) | [![macOS build status](https://travis-matrix-badges.herokuapp.com/repos/nohajc/resman/branches/master/1)](https://travis-ci.org/nohajc/resman) | [![Windows build status](https://ci.appveyor.com/api/projects/status/github/nohajc/resman?branch=master&svg=true)](https://ci.appveyor.com/project/nohajc/resman) |

## About

This is a cross-platform solution for embedding resource files into executables.<sup>[1](#footnote1)</sup>

### Goals
* No boilerplate
* Efficient intermediate representation
* Unversal solution for all common platforms
* No runtime dependencies

#### Existing approaches to this problem include
* Using Windows resource management (only available for Windows PE files)
* Converting your files into C/C++ byte arrays which can be compiled (this can produce very large source files)
* Using utilities like objcopy (quite low-level, Linux-only AFAIK)
* Using the Qt Resource System (adds runtime dependency on Qt)

As you can see, none of the above can fulfill all the goals.
However, with the help of LLVM and Clang, we can do better.

<sup><a name="footnote1">1</a></sup> Note that we're talking about embedding during linking phase. This tool is not able to modify existing executables.

### What resman does differently

This project consist of a resource compiler __rescomp__ and a header file __include/resman.h__.

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

#### So, to summarise
Instead of generating byte arrays, you just write a header file with the list of resources.
Then you run the resource compiler to generate object files or static libraries directly from that list.
That same list will also be be used to access your embedded resources.

## More information
_TODO_
