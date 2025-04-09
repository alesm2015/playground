# Introduction

This is a simple project which offers CLI interface available via Telent protocol. Program supports multiple simultaneously connections. Those connections can be opened via single port or multiple ports depending on the listening port configuration. Each listening port supports multiple connections. Program limits maximum number of all open connection at time in order to control system resources. Further more, application can run as a Linux daemon. Any and all feedback is greatly appreciated!

## Features
* CMake-based project management, including dependencies
* Telnet server fully supporting telnet protocol and VT100 terminal protocol
* Simultaneously listening multiple TCP ports
* Multithreading command processing
* Using advanced programming, like coroutines
* Cross independed application
* Application can be started up in daemon mode
* **Doxygen** support, actived if doxygen is properly installed on the system
* **Unit testing** support, using Boost Unit Testing tool
* **Ccache** integration, for speeding up rebuild times
* **Static analysis** integration. Activated by default or via `BUILD_STATIC_ANALYSIS` option
* **Valgrind** an instrumentation framework for building dynamic analysis tools

## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequsites
List of requrements up to Boost library are required in order to build application. Other prerequsites are required only if additional feature is required to be executed during build time.
* **CMake v3.15+** - found at [https://cmake.org/](https://cmake.org/)
* **C++ Compiler** - needs to support at least the **C++20** standard
* **Boost library** - found at [https://www.boost.org/](https://www.boost.org/), at least version 1.73+.x
* **Doxygen** - Installed [https://doxygen.nl/](https://doxygen.nl/) and [https://graphviz.org/](https://graphviz.org/)
* **Telnet client** - Any telnet client would be sufficient
* **Cppcheck** - Static analysis, [cppecheck](https://cppcheck.sourceforge.io/)
* **Valgrind** - Dynamic analysis, [https://valgrind.org/](https://valgrind.org/)

There are two additional external libraries required to build entire code. None of the libraries are used as a submodule, but they are both automatically downloaded by CMake when fresh build is started.
* **cli** - C++ CLI library with VT100 protocol [CLI](https://github.com/daniele77/cli)
* **libtelnet** Telnet protocol library in C. [libtelnet](https://github.com/osmocom/libtelnet)

#### Fedora package installation tutorial
In order to install all the source code and exeute all of the test, it is required to properly install all the dependices. Following commands shall be executed from terminal.

```plain
* sudo dnf install boost-devel          - Required
* sudo dnf install telnet               - Recommended
* sudo dnf install doxygen
* sudo dnf install graphviz
* sudo dnf install valgrind 
* sudo dnf install cppcheck
* sudo dnf install cppcheck-htmlreport
```

## Repository layout
The repository layout is pretty straightforward, including the CMake files to build the project.

```plain
-- .vscode                      - Standard visual code configuration
+- booker                       - **Main module**, build as static library
  | +- include                  - Include files
      | -- booker.h             - Simple header file use for booker unique identification
      | -- booking.h            - Header file with API definition, used for booking control
      | -- customcli.h          - C++ wraper so the external CLI ribrary fits to this design
      | -- parser.h             - Function definitions, which converts string to array and vice versa
      | -- server.h             - Header file of a class which keeps all sessions and listening ports
      | -- session.h            - Header file for controlling TCP socket and Telnet session overall.
  | -- booking.cpp              - Source file, ith API definition, used for booking control
  | -- CMakeLists.txt           - CMake configuration file, to build static library
  | -- parser.cpp               - Function definitions, which converts string to array and vice versa
  | -- server.cpp               - Source file of a class which keeps all sessions and listening ports
  | -- session.cpp              - Source file for controlling TCP socket and Telnet session overall.
+- build                        - Output directory
  | +- cppcheck                 - Cppcheck output directory
      | -- index.html           - Report html file
  | +- doc                      - Doxygen output directory
  |   | +- html                 - Html output directory
          | -- index.html       - Report html file
  | +- playd                    - Main application (daemon) location
      | -- playd                - Application itself
  | +- test                     - Unit test location
      | -- test_suite           - Unit test application
+- doc                          - Doxygen folder
  | +- doxy                     - Doxygen configuration folder
    | -- doxyfile.doxygen       - Doxygen configuration file
  | -- README.md                - us
+- playd                        - Main application source file
  | -- CMakeLists.txt           - CMake file to build application
  | -- main.cpp                 - Application entry function
  | -- version.h.in             - Version control header files
+- test                         - Unit test folder
  | -- booking_test.cpp         - Bookink unit test folder
  | -- CMakeLists.txt           - CMake file to build unit tests
  | -- parser_test.cpp          - Parser unit test folder
-- .gitignore                   - git configuration folder
-- CMakeLists.txt               - Main CMake file
-- cpc                          - Configuration script to execute cppcheck analysis
-- README.md                    - Just a file, which redirect us beck to us
```

### Available CMake Options
* BUILD_UNIT_TESTS      - Build unit test application
* BUILD_DOCUMENTATION   - Automatically set if doxygen application is installed on the system
* BUILD_STATIC_ANALYSIS - Perform static analysis after sucessfully completed compalation
* BUILD_DAEMON          - Build application as a Linux daemon

## Building
The project can be built using the following commands:

```shell
cd /path/to/this/project
mkdir -p build # md build (on Windows)
cd build
cmake -BUILD_UNIT_TESTS=TRUE -BUILD_STATIC_ANALYSIS=TRUE -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .
```
These commands will build the entire code and generate all supporting files, like documentation, etc.

## Running
Application is located in a folder: /path/to/this/project/build/playd/playd. If application gets executed without daemon support, application will not exit untill is not terminated.
Optionally is possible to run application via GDB to run and debug it.

## Testing
By default, the template uses Boost unit test framework. To run the tests, simply use path/to/this/project/build/test/unit_test --log_level=all.
Optionally is possible to run application via GDB to debug unit tests.

## Generating documentation
Documentation is autmatically generated and is located in the folder: path/to/this/project/build/doc/html/index.html

## Static analysis
Static analysis report is located in the folder path/to/this/project/build/cppcheck/index.html

## Runtime analysis
Runtime analysis requires application to run. Further more application needs to be shut down properly (not terminated) in order for sucessfull report generation.
Runtime analysis can be executed via bash script on root folder as: ./vg ./build

## Installing
Instalation is currently not supported.

# Usage
As already described, the interface is allowed only trough telnet connection. Standard connection port is 50000, unless is configured 
differently. Listening port is configured in the main.cpp function.

Example connecting to the application:
```shell
telnet localhost 50000
```

As soon as connection gets established, application generates hello message, something like:
```shell
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
Hello: 127.0.0.1:42678@1
cli> 
```

## help command
By typing **help** command, application lists all available commands.

## status command
Parameters for command are **status 0 0 0**. This command reports current status from all movies, from all theatres list of free and occupied seats.
```shell
Movie: GodFather
   Theater: Delhi
     Free seats: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
     Allocated seats: 
   Theater: MexicoCity
    ....
```

## <movie name>
This command selects the movie. By typing name of the movie (list of all movies is visible from help command). 
```shell
cli> Matrix
Matrix> help
Commands available:
 - help
	This help message
 - exit
	Quit the session
 - history
	Show the history
 - ! <history entry index>
    ....
```

Once proper movie is being selected, help command lists all available theatres, where selected movie is being played. By enering a theatre name, proper thatre is being selected.

## <theatre name>
This command select appropiate theatre. Command is unacessible until movie is not selected.
```shell
Matrix> Tokyo
Tokyo> help
Commands available:
 - help
	This help message
 - exit
	Quit the session
 - history
	Show the history
 - ! <history entry index>
	Exec a command by index in the history
 - seats <string> <unsigned long> <unsigned long>
	Show free seats
    ....
```

After chosen theatre is selected, it is possible select a seat.

## seats
Command list all the available seats per selected movie and theatre. 
**Note** Command requires three additional parameters <string> <int> <int>, these values don't care, but they can not be left out, otherwise command won't get executed.

```shell
Tokyo> seats 1,2,3 0 0
Free available seats: 0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19
```

## book
Seats can be selected by typing command book. First parameter is the filter of the selected seats. Seats can be selected as:
* Filter command    Selected seats
* 1,2,4,5       -> 1, 2, 4, 5
* 1, 2, 6-8, 10 -> 1, 2, 6, 7, 8, 10
* or any that kind of example

This command is strict. If even at least one seat from selected list is already occupied, none of the sits will get occupied. Command will als print out seat which caused that.
**Note** Command requires three additional parameters <int> <int>, these values don't care, but they can not be left out, otherwise command won't get executed.

```shell
Tokyo> book 1 2 3s
Currently reserved seats 1, 2, 3: 
```

## trybook
This book is very similar to the: book parameters
The only diffrence is, that all selected seats will get occupied exept those which are already occupied. Otherwise entire behaviour is similar to the book command.
**Note** Command requires two additional parameters <int> <int>, these values don't care, but they can not be left out, otherwise command won't get executed.
**Note** Seat selection filter has the same behaviour as in book command.

```shell
Tokyo> trybook 8,9,10 0 0
Currently reserved seats: 1, 2, 3, 8, 9, 10
```

## unbook
Release already selected seat. If input parameter is wong, function will fail.
**Note** Command requires two additional parameters <int> <int>, these values don't care, but they can not be left out, otherwise command won't get executed.
**Note** Seat selection filter has the same behaviour as in book command.

```shell
Currently reserved seats: 1, 2, 3, 8, 9, 10
Invalid seats: 1, 2, 3, 8, 9, 105
```

## status
Return currently booked list of seates from particular user.

```shell
Tokyo> status 0 0 0
Currently reserved seats: 1, 2, 3, 8, 9, 10

```

## <movie> 
This command return CLI back ok the parent directory

