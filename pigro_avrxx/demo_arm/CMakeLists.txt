# I target a recent cmake, it shouldn't be a problem on a dev machine
cmake_minimum_required(VERSION 3.11)
# Project name
project("demo_arm")

# Product filename
set(PRODUCT_NAME demo_arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(MCU "cortex-m3")
set(LD_SCRIPT "STM32F100XB_FLASH.ld")

add_compile_definitions(STM32F100xB)

set(triplet arm-none-eabi)

# Use AVR GCC toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_CXX_COMPILER ${triplet}-g++)
set(CMAKE_C_COMPILER ${triplet}-gcc)

include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/../libtiny)

# mmcu MUST be passed to bot the compiler and linker, this handle the linker
set(CMAKE_C_FLAGS "-mcpu=${MCU} -mthumb -ffreestanding")
set(CMAKE_CXX_FLAGS "-mcpu=${MCU} -mthumb -ffreestanding")
set(CMAKE_EXE_LINKER_FLAGS "-Wl,-T${CMAKE_SOURCE_DIR}/${LD_SCRIPT},--gc-sections")

add_compile_options(
#    -mmcu=${MCU} # MCU
    -std=gnu++17
    -nostdlib
    -O2 # optimize
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
        armxx/bitband.h
        armxx/pin.h
        armxx/static_ptr.h
        isr.h
        uart.h
        armxx/bitband.cpp
        armxx/pin.cpp
        armxx/static_ptr.cpp
        isr.cpp
        isr_vector.cpp
        main.cpp
        uart.cpp
    )
# Create one target
add_executable(${PRODUCT_NAME} ${SRC_FILES})

# Rename the output to .elf as we will create multiple files
set_target_properties(${PRODUCT_NAME} PROPERTIES OUTPUT_NAME ${PRODUCT_NAME}.elf)

# Strip binary for upload
add_custom_target(strip ALL ${triplet}-strip ${PRODUCT_NAME}.elf DEPENDS ${PRODUCT_NAME})

# Transform binary into hex file, we ignore the eeprom segments in the step
add_custom_target(hex ALL ${triplet}-objcopy -R .eeprom -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.hex DEPENDS strip)
# Transform binary into hex file, this is the eeprom part (empty if you don't
# use eeprom static variables)
add_custom_target(eeprom ${triplet}-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load"  --change-section-lma .eeprom=0 -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.eep DEPENDS strip)

# Clean extra files
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")
