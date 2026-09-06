![](https://github.com/OpenCppCoverage/OpenCppCoverage/workflows/Unit%20tests/badge.svg)
# OpenCppCoverage

OpenCppCoverage is an open source code coverage tool for C++ under Windows.

The main usage is for unit testing coverage, but you can also use it to know the executed lines in a program for debugging purpose.

---------------------
## Project status: archived and no longer maintained

This project is no longer actively maintained.

I stopped active development and maintenance approximately seven years ago after moving away from C++ in my professional work. At first, I expected this to be temporary, but I am now formally retiring the project.

The existing source code and releases will remain available for historical use. However:

* no further releases, bug fixes, or compatibility updates are planned;
* issues and pull requests will not be reviewed;
* support questions may not receive a response;
* security fixes should not be expected;
* users should evaluate the software carefully before continuing to depend on it.

Forks and independent continuation of the project are welcome, subject to the existing licence.

### Community-maintained forks

**There is currently no designated successor or recommended fork.**

The following independent fork has been brought to my attention:

[bmarques1995/OpenCppCoverage](https://github.com/bmarques1995/OpenCppCoverage)

**Note: I have not reviewed or tested this fork.**

If you actively maintain a fork of this project and would like it to be considered for inclusion here, please contact me through OpenCppCoverage@gmail.com.

I intend to link from this page to the most active community-maintained fork I am aware of. The selected fork may change over time as development activity changes.

Any fork listed here is maintained independently. Its inclusion does not constitute a transfer of ownership, and I cannot provide support or guarantees for it.

### Existing issues and pull requests

Existing issues and pull requests have been closed as part of retiring the project.

Their closure does not necessarily mean that an issue was resolved, that a proposed change was rejected, or that the contribution lacked value. It only means that the original repository is no longer being maintained.

The history and discussion will remain available for reference.

### Licence

The project remains available under the terms described in the [LICENSE](LICENSE) file.

Archiving the project does not change its licence. You may use, modify, fork, and redistribute the code according to those terms.

### Thank you

Thank you to everyone who used the project, reported issues, contributed code or documentation, reviewed changes, answered questions, or otherwise helped improve it.

Maintaining this project and working with its community was a valuable experience. I am grateful for all the time and effort contributed over the years.

---------------------
## Features:
- **Visual Studio support**: Support compiler with program database file (.pdb).
- **Non intrusive**: Just run your program with OpenCppCoverage, no need to recompile your application.
- **HTML reporting**
- **Line coverage**.
- **Run as Visual Studio Plugin**: See [here](https://github.com/OpenCppCoverage/OpenCppCoveragePlugin) for more information.
- **Jenkins support**: See [here](https://github.com/OpenCppCoverage/OpenCppCoverage/wiki/Jenkins) for more information.
- **Support optimized build**.
- **Exclude a line based on a regular expression**.
- **Child processes coverage**.
- **Coverage aggregation**: Run several code coverages and merge them into a single report.
 
## Requirements
- Windows Vista or higher.
- Microsoft Visual Studio 2008 or higher all editions **including Express edition**. It should also work with previous version of Visual Studio.

## Download
OpenCppCoverage can be downloaded from [here](../../releases).

## Usage
You can simply run the following command:

```OpenCppCoverage.exe --sources MySourcePath* -- YourProgram.exe arg1 arg2```

For example, *MySourcePath* can be *MyProject*, if your sources are located in *C:\Dev\MyProject*.

See [Getting Started](https://github.com/OpenCppCoverage/OpenCppCoverage/wiki) for more information about the usage.
You can also have a look at [Command-line reference](https://github.com/OpenCppCoverage/OpenCppCoverage/wiki/Command-line-reference).
