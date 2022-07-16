# I target a recent cmake, it shouldn't be a problem on a dev machine
cmake_minimum_required(VERSION 3.11)

set(CMAKE_VERBOSE_MAKEFILE ON)

# Project name
project("demo")

# Product filename
set(PRODUCT_NAME demo)

# CPU, you can find the list here:
# https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
set(MCU atmega16a)

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_CXX_COMPILER avr-g++)
set(CMAKE_C_COMPILER avr-gcc)

include_directories(../avrxx ../libtiny)

# mmcu MUST be passed to bot the compiler and linker, this handle the linker
set(CMAKE_EXE_LINKER_FLAGS -mmcu=${MCU})

add_compile_options(
    -mmcu=${MCU} # MCU
    -std=gnu++20
    -fmodules-ts
    -Os # optimize
#    -I../avrxx
#    -I../libtiny

#    -Wall # enable warnings
#    -Wno-main
#    -Wundef
#    -pedantic
#    -Werror
#    -Wfatal-errors
#    -Wl,--relax,--gc-sections
#    -g
#    -gdwarf-2
#    -funsigned-char # a few optimizations
#    -funsigned-bitfields
#    -fpack-struct
#    -fshort-enums
    -ffunction-sections
    -fdata-sections
#    -fno-split-wide-types
#    -fno-tree-scev-cprop
)
file(GLOB SRC_FILES
    PigroProto.h
    PigroService.h
    PigroTimer.h
    jtag.h
    stm32.h
    main.cpp
    foo.cpp
    bar.cpp
    )
# Create one target
add_executable(${PRODUCT_NAME} ${SRC_FILES})

# Rename the output to .elf as we will create multiple files
set_target_properties(${PRODUCT_NAME} PROPERTIES OUTPUT_NAME ${PRODUCT_NAME}.elf)

# Strip binary for upload
add_custom_target(strip ALL avr-strip ${PRODUCT_NAME}.elf DEPENDS ${PRODUCT_NAME})

# Transform binary into hex file, we ignore the eeprom segments in the step
add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.hex DEPENDS strip)
# Transform binary into hex file, this is the eeprom part (empty if you don't
# use eeprom static variables)
add_custom_target(eeprom avr-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load"  --change-section-lma .eeprom=0 -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.eep DEPENDS strip)

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")
