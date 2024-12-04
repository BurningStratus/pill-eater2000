cmake_minimum_required(VERSION 3.5)

include(pico-sdk/pico_sdk_init.cmake)

set(PICO_BOARD pico_w)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

project(main ASM C CXX)
pico_sdk_init()

add_executable(${PROJECT_NAME}
        main.c
)

target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        hardware_gpio
        hardware_pwm
        hardware_i2c
)

#target_link_options(${PROJECT_NAME} PRIVATE "LINKER:--wrap=malloc")

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -j8
)

#add_subdirectory(modules)

# pull in common dependencies
#target_link_libraries(${PROJECT_NAME} pico_stdlib)


# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

