# CCcamTester
CCcam C Line Tester is a small utility command line app and shared library useful to test validity of CCcam c lines.

Features:

* Test a single C line.
* Test a CCcam.cfg file(-c commmand line parameter).
* Test all the lines passed from STDIN.
* Write valid lines to a oscam.serer file(-o commmand line parameter).
* Can be compiled as a shared library(.so) and then used in an external project.

## Installation

Just build the application and the shared library with command `make release`, output will be generated in bin/ folder.
After running `make release` command, to install in `/usr/local/bin/` just run `make install`.
*Tested just on Linux and MacOS.*

### Requirements
C compiler compatible with C11 standard, make and the openssl library.

## Usage
The application can be used in interactive mode, or passing c lines from standard output, like:
```
cat cccam.txt | cccamtest
```

### Command line parameters 

* -o /tmp/oscam.server generates an Oscam file cotnaining valid clines 
* -c /tmp/CCcam.cfg import c lines from a CCcam.cfg file

If -o parameter passed without a following valid file name, a default oscam.server is generated in local directory.

## Development 
Project can be opened with both Xcode and VSCode.