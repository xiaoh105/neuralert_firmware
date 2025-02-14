# Quickstart:
- Clone the repository (however you like)
- Checkout the submodule containing the SDK: `git submodule update --init --recursive`
- Create a directory to store the build: `mkdir build`
- (From the build directory) Configure cmake: `cd build; cmake ..`
- (From the build directory) Build the project `cmake --build .`

This should go through the build process, including pre-build and post-build steps, resulting in 'FBOOT' and 'FRTOS' in an 'img' directory that is generated. 

# Code Structure:
- The SDK (clean from Renaeses, other than the addition of the CMake files) is located in `da16200_sdk`, which is a git submodule. Other than debugging, nothing should need to be changed in there
- The main application is in the `neuralert` folder. 
This is where edits should happen. The substructure of the project does not follow any rules until it has been reworked

Normal CMake practices are used (including some that are generally frowned upon, such as glob includes [for now]). 
As long as you do not rename or create a folders, rebuilding should be as simple as `cmake ..; cmake --build .`

# How to build it with LLVM

## Step 1: Install wllvm
First, install the tool **wllvm** for compilation.

```sh
pip install wllvm
```

## Step 2: Set Environment Variables
Configure the environment variables required for wllvm compilation. Also, Add the bin library of the Toolchain to the PATH:

```sh
export LLVM_COMPILER=clang 
export BINUTILS_TARGET_PREFIX=arm-none-eabi
export PATH=$PATH:/path/to/your/neuralert_firmware/arm-toolchain/linux-x86_64/gcc-arm-none-eabi-10.3-2021.10/bin
```

## Step 3: Run CMake and Make

Run the CMake command (you need to manually specify the CC compiler):

```sh
mkdir build && cd build
cmake ..
make 
```

## Step 4: Extract .bc from the .elf Executable File

Use extract-bc to extract the .bc file from the .elf file:

```sh
neuralert_firmware/arm-toolchain/linux-x86_64/gcc-arm-none-eabi-10.3-2021.10/bin
```

Use extract-bc to extract the .bc file from the .elf file:

```sh
extract-bc neuralert.elf
```

In the build directory, you should see neuralert.elf.bc, which is the extracted bitcode file.

### Convert the .bc File to a Readable .ll File

```sh
llvm-dis neuralert.elf.bc
```

In the build directory, you should find neuralert.elf.ll, which corresponds to the LLVM IR.