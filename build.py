import os
import platform
import sys
import multiprocessing
import subprocess

try:
    import thirdparty.getCryptoTools as getCryptoTools
except ImportError:
    print("Error: Could not find 'thirdparty.getCryptoTools'. Ensure the file structure is correct.")
    sys.exit(1)


def getParallel(args):
    """Determine the number of parallel threads to use for building."""
    par = multiprocessing.cpu_count()
    for x in args:
        if x.startswith("--par="):
            val = x.split("=", 1)[1]
            try:
                par = int(val)
                if par < 1:
                    par = 1
            except ValueError:
                print("Error: Invalid value for --par argument. Must be an integer.")
                sys.exit(1)
    return par


def createDirectories(path_list):
    for path in path_list:
        try:
            # Create a new directory if it does not exist
            if not os.path.exists(path):
                os.makedirs(path)
                print(f"Created directory: {path}")
            else:
                print(f"Directory already exists: {path}")
        except Exception as e:
            print(
                f"An error occurred while creating directory {path}: {str(e)}")


def Setup(install, prefix, par):
    """
    Setup all dependencies for the project and create necessary directories.

    Args:
        install (bool): Whether to install dependencies.
        prefix (str): Installation directory.
        par (int): Number of parallel build threads.
    """
    dir_path = os.path.dirname(os.path.realpath(__file__))
    thirdparty_dir = os.path.join(dir_path, "thirdparty")
    os.makedirs(thirdparty_dir, exist_ok=True)

    os.chdir(thirdparty_dir)

    # Always setup cryptoTools
    print("Setting up CryptoTools and Boost...")
    getCryptoTools.getCryptoTools(install, prefix, par, True, True, False)

    os.chdir(dir_path)
    print("Setup completed.")

    paths_to_create = [
        "./data/test/utils",
        "./data/test/ss",
        "./data/test/ss3",
        "./data/test/protocol",
        "./data/test/wm",
        "./data/test/fmi",
        "./data/bench/fmi",
        "./data/bench/os",
        "./data/bench/pir",
        "./data/bench/wm",
        "./data/log"
    ]
    current_file_path = os.path.dirname(os.path.abspath(__file__))
    paths_to_create = [current_file_path +
                       '/' + path for path in paths_to_create]
    createDirectories(paths_to_create)


def Build(projectName, mainArgs, cmakeArgs, install, prefix, par):
    """
    Build the main project.

    Args:
        projectName (str): Name of the project.
        mainArgs (list): Arguments passed to the build script.
        cmakeArgs (list): Arguments passed to CMake.
        install (bool): Whether to install the project.
        prefix (str): Installation directory.
        par (int): Number of parallel build threads.
    """
    if "--Debug" in mainArgs:
        buildType = "Debug"
    elif "--Profile" in mainArgs:
        buildType = "Profile"
    else:
        buildType = "Release"
    osStr = platform.system()
    if osStr == "Windows":
        print("Windows is not supported.")
        sys.exit(1)
    else:
        buildDir = "out/build/linux"

    cmakeArgs.append(f"-DCMAKE_BUILD_TYPE={buildType}")

    mkDirCmd = ["mkdir", "-p", buildDir]
    CMakeCmd = ["cmake", "-S", ".", "-B", buildDir] + cmakeArgs
    CMakeCmd += ["-G", "Ninja",
                 "-DCMAKE_CXX_FLAGS=-fdiagnostics-color=always"]
    BuildCmd = ["cmake", "--build", buildDir]

    if par != 1:
        BuildCmd += ["--parallel", str(par)]

    print("\n======= build.py ("+projectName+") ==========")
    print(' '.join(mkDirCmd))
    print(' '.join(CMakeCmd))
    print(' '.join(BuildCmd))
    print("vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv\n")

    try:
        subprocess.run(mkDirCmd, check=True)
        subprocess.run(CMakeCmd, check=True)
        subprocess.run(BuildCmd, check=True)
    # if install:
    #     InstallCmd = ["cmake", "--install", buildDir]
    #     print("Installing project:", " ".join(InstallCmd))
    #     subprocess.run(InstallCmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error during build: {e}")
        sys.exit(1)


def getInstallArgs(args):
    """Determine whether to install the built libraries and the installation directory"""
    prefix = ""
    for x in args:
        if x.startswith("--install="):
            prefix = x.split("=", 1)[1]
            prefix = os.path.abspath(os.path.expanduser(prefix))
            return (True, prefix)
        if x == "--install":
            return (True, "")
    return (False, os.getcwd())


def parseArgs():
    """Parse main and CMake-specific arguments."""
    hasCmakeArgs = "--" in sys.argv
    if hasCmakeArgs:
        idx = sys.argv.index("--")
        mainArgs = sys.argv[:idx]
        cmakeArgs = sys.argv[idx + 1:]
    else:
        mainArgs = sys.argv
        cmakeArgs = []
    return mainArgs, cmakeArgs


def help():
    """Display usage information."""
    print("""
        Usage:
            python build.py [options] [-- cmake_args]

        Options:
            --setup          Fetch, build, and optionally install dependencies.
            --install        Install the built libraries to the default location.
            --install=path   Install to the specified directory.
            --sudo           Use sudo when installing.
            --par=n          Use n threads for parallel builds (default: number of cores).
            --help           Show this help message.
            --               Pass arguments after '--' directly to cmake.

        Examples:
            Fetch dependencies without installation:
                python build.py --setup
            Install the built libraries to a specific directory:
                python build.py --setup --install=/path/to/install
            Build the main library with parallelism:
                python build.py --par=4
            Build with custom cmake configurations:
                python build.py -- -DCMAKE_BUILD_TYPE=Debug
        """)


def main(projectName):
    mainArgs, cmake = parseArgs()

    if "--help" in mainArgs:
        help()
        return

    setup = "--setup" in mainArgs
    install, prefix = getInstallArgs(mainArgs)
    par = getParallel(mainArgs)
    print("Setup:", setup)
    print("Install:", install)
    print("Prefix:", prefix)
    print("Parallel threads:", par)
    print("CMake args:", cmake)

    if setup:
        Setup(install, prefix, par)
    else:
        Build(projectName, mainArgs, cmake, install, prefix, par)


if __name__ == "__main__":
    main("RingOA")
