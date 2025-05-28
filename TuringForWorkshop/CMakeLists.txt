# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.13)

# Define the project name
project(TuringForWorkshop)

# Pull in the Pico SDK
include(pico_sdk_import.cmake)
pico_sdk_init()

# Create the executable from your source files
add_executable(${PROJECT_NAME}
    main.cpp
    MainApp.cpp
    # App.cpp
    Clock.cpp
    # Sequence.cpp
    # Config.cpp
    UI.cpp
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}        # local .h files
)

# Give oscillator more time to start - workshop system won't start if this isn't included 
target_compile_definitions(${PROJECT_NAME} PRIVATE PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf") 


# Link against the libraries you’re using (as per ComputerCard dependencies)
target_link_libraries(${PROJECT_NAME}
pico_unique_id pico_stdlib hardware_dma hardware_i2c hardware_pwm hardware_adc hardware_spi
)
 
# Generate UF2, bin, and other output formats
pico_add_extra_outputs(${PROJECT_NAME})
