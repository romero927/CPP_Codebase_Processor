# Codebase Processor

A **Qt-based C++** application for processing codebases into a single text file. It allows you to select a folder, choose which files to process based on customizable filters, and then either copy the processed content to your clipboard or save it to a single output file.

**Note:** This written through a combination of hand-coding, Claude 3.5 Sonnet, ChatGPT o1/4o, and Cline. After initial version was up and running, it was then run against itself and pushed back into AI to iteratively improve itself.

## Features

- Recursively scans a selected directory and its subdirectories for code files
- Supports filtering files based on customizable patterns, including a `.gitignore` file if present
- Provides a tree view UI to select individual files to include/exclude from processing
- Concatenates the contents of selected files into a single output, with file paths as headers
- Option to copy the output directly to clipboard or save to a file
- Provides progress updates and statistics during processing
- Cross-platform support (Windows, macOS, Linux)

## Prerequisites

- **Qt 6.8** or higher (adjust `CMAKE_PREFIX_PATH` in `CMakeLists.txt` if needed)
- **CMake 3.16** or higher
- A **C++17** compatible compiler (MSVC, GCC, Clang)
- On Windows, a **Visual Studio Developer Command Prompt** to properly configure the build environment

## Build Instructions

### Automatic Build (Windows)

1. Open a **Developer Command Prompt for Visual Studio**
2. Run either `build_deploy_release.bat` for a Release build or `build_deploy_debug.bat` for a Debug build

The batch file will:
- Set up the Visual Studio build environment 
- Create a `build` directory
- Configure and build the project with CMake
- Deploy the necessary Qt dependencies next to the executable

You can find the final executable in `build\Release` or `build\Debug` depending on the selected build type.

### Manual Build

1. **Create a build directory**: `mkdir build` (or use your preferred build directory setup)
2. **Configure the project with CMake**:
   ```
   cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```
   (Replace `Release` with `Debug` if you want a debug build)
3. **Build the project**:
   ```
   cmake --build . --config Release
   ```
4. The compiled executable will be in the `Release` or `Debug` subfolder, depending on your build type.

On Windows, you'll also need to run `windeployqt` to copy the necessary Qt dependencies next to the executable:
```
<path_to_qt>\bin\windeployqt.exe --release --no-translations --no-system-d3d-compiler Release\codebase_processor.exe  
```
(Adjust the path to `windeployqt.exe` and the executable as needed)

## Code Structure
```CPP_Codebase_Processor/
├── CMakeLists.txt               # Build configuration file
├── main.cpp                     # Application entry point
├── MainWindow                   # Main application window
│   ├── MainWindow.h            # Main window header
│   └── MainWindow.cpp          # Main window implementation
├── FileProcessingWorker         # Background processing worker
│   ├── FileProcessingWorker.h    
│   └── FileProcessingWorker.cpp
├── FileSystemModelWithGitIgnore # Custom file system model
│   ├── FileSystemModelWithGitIgnore.h
│   └── FileSystemModelWithGitIgnore.cpp
├── ProcessingDialog            # Progress dialog
│   ├── ProcessingDialog.h
│   └── ProcessingDialog.cpp
├── resources.qrc              # Qt resource file
├── README.md                  # Documentation
└── Build Scripts
    ├── build_deploy_debug.bat    # Debug build script
    └── build_deploy_release.bat  # Release build script

Class Hierarchy:
MainWindow (QMainWindow)
├── FileSystemModelWithGitIgnore (QFileSystemModel)
├── FileProcessingWorker (QObject)
└── ProcessingDialog (QDialog)
The application is structured around these main components:

MainWindow: The main UI that handles file selection and user interaction
FileSystemModelWithGitIgnore: Manages the file system view with .gitignore support
FileProcessingWorker: Handles file processing in a background thread
ProcessingDialog: Shows progress during file processing

The build system uses CMake with Qt 6.8 and requires C++17, with separate batch scripts for debug and release builds on Windows.
```
## Usage

1. Launch the `codebase_processor` executable
2. Click the **"Select Codebase Folder"** button and choose the root directory of the codebase you want to process
3. The file tree will populate with all the code files found in the selected directory and its subdirectories
   - Files ignored based on patterns in a `.gitignore` file (if present) or common binary/build output patterns will be unchecked by default
   - You can manually check/uncheck files to include or exclude them from processing
4. Click **"Copy to Clipboard"** to concatenate the contents of all checked files and copy the result to the clipboard
   - OR click **"Save to File"** to save the concatenated content to a file instead
5. A progress dialog will show the current processing status and statistics
6. Once complete, the processed content will be in your clipboard or saved file

## License

This project is open-source and available under the [MIT License](LICENSE).
