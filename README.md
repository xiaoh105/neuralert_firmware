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