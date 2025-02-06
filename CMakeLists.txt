cmake_minimum_required(VERSION 3.16.0)

project(codebase_processor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt configuration
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Adjust this path to match your Qt installation
set(CMAKE_PREFIX_PATH "E:/QT/6.8.2/msvc2022_64")

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
)

# List all our source files
add_executable(codebase_processor WIN32
    main.cpp
    MainWindow.cpp
    MainWindow.h
    FileExtensionConfig.h
    FileProcessableUtils.h  # Add this line
    FileProcessableUtils.cpp # And this line
    FileProcessingWorker.cpp
    FileProcessingWorker.h
    FileSystemModelWithGitIgnore.cpp
    FileSystemModelWithGitIgnore.h
    ProcessingDialog.cpp 
    ProcessingDialog.h
    resources.qrc
)

configure_file(${CMAKE_SOURCE_DIR}/config/file_extensions.json
               ${CMAKE_BINARY_DIR}/config/file_extensions.json
               COPYONLY)

# Link against Qt libraries
target_link_libraries(codebase_processor PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Widgets
) 

# Deployment configuration for Windows
if(WIN32)
    # Find windeployqt executable
    get_target_property(_qmake_executable Qt::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    # Add deployment command
    add_custom_command(TARGET codebase_processor POST_BUILD
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        --release  # <-- Instead of --debug
        --no-compiler-runtime
        --no-translations
        --no-system-d3d-compiler
        "$<TARGET_FILE:codebase_processor>"
    COMMENT "Deploying Qt dependencies..."
)

    # Copy Visual C++ runtime
    include(InstallRequiredSystemLibraries)
    if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        add_custom_command(TARGET codebase_processor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} $<TARGET_FILE_DIR:codebase_processor>
            COMMENT "Copying Visual C++ runtime libraries..."
        )
    endif()
endif() 