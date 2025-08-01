# CMake file for Raspberry Pi Pico project

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()

set(sdkVersion 2.1.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.1.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Configuração específica para Pico W
set(PICO_BOARD pico_w CACHE STRING "Board type")
set(PICO_CYW43_ARCH_COUNTRY "WW")
set(PICO_BOARD pico_w)

# === Configuração do SDK ===
include(pico_sdk_import.cmake)

project(Tarefa7_JOAOPONTES C CXX ASM)

# Inicialização do SDK
pico_sdk_init()

# === Configuração do executável ===
add_executable(Tarefa7_JOAOPONTES 
    Tarefa7_JOAOPONTES.c 
    libs/SSD1306.c
)

# === Configurações do programa ===
pico_set_program_name(Tarefa7_JOAOPONTES "Tarefa7_JOAOPONTES")
pico_set_program_version(Tarefa7_JOAOPONTES "0.1")


# === Habilitar UART/USB ===
pico_enable_stdio_uart(Tarefa7_JOAOPONTES 1)
pico_enable_stdio_usb(Tarefa7_JOAOPONTES 1)

# === Diretórios de include ===
# Diretórios de include atualizados
target_include_directories(Tarefa7_JOAOPONTES PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/libs
    ${PICO_SDK_PATH}/lib/lwip/src/include
    ${PICO_SDK_PATH}/lib/cyw43-driver/include
)
# === Bibliotecas para linking ===
target_link_libraries(Tarefa7_JOAOPONTES
    pico_stdlib
    hardware_uart
    hardware_pwm
    hardware_i2c
    pico_cyw43_arch_lwip_poll
    pico_lwip
    pico_cyw43_driver
)

# Definições essenciais
target_compile_definitions(Tarefa7_JOAOPONTES PRIVATE
    PICO_CYW43_ARCH_LWIP=1
    PICO_CYW43_ARCH_POLL=1
    CYW43_LWIP=1
)

pico_add_extra_outputs(Tarefa7_JOAOPONTES)