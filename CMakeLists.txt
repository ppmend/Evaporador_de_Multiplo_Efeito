cmake_minimum_required(VERSION 3.15)
project(evaporador_multiplo_efeito VERSION 2.0.0 LANGUAGES C)

# Padrão C11 obrigatório
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Configurações de diretórios de inclusão
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Opções de avisos rigorosos do compilador
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wno-unused-parameter)
endif()

# Fontes da biblioteca central (Core)
set(CORE_SOURCES
    src/core/thermodynamics.c
    src/core/validation.c
    src/core/solver.c
    src/core/types.c
)

# Biblioteca estática do núcleo numérico
add_library(evap_core STATIC ${CORE_SOURCES})
target_include_directories(evap_core PUBLIC include)

if (NOT MSVC)
    target_link_libraries(evap_core PUBLIC m)
endif()

# Executável CLI
add_executable(evap_cli src/cli/main.c)
target_link_libraries(evap_cli PRIVATE evap_core)

# Suporte a WebAssembly via Emscripten
if (EMSCRIPTEN)
    add_executable(evap_wasm
        src/wasm/wasm_api.c
        ${CORE_SOURCES}
    )
    target_include_directories(evap_wasm PUBLIC include)
    set_target_properties(evap_wasm PROPERTIES
        OUTPUT_NAME "evap_solver"
        SUFFIX ".js"
        LINK_FLAGS "-s WASM=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s EXPORTED_FUNCTIONS=['_wasm_solve_direct','_wasm_get_steam_rate','_wasm_get_vapor_economy','_wasm_get_area_per_effect','_wasm_get_total_area','_wasm_get_total_dt','_wasm_get_total_bpe','_wasm_get_outer_iter','_wasm_get_inner_iter','_wasm_get_effect_vapor','_wasm_get_effect_liquid','_wasm_get_effect_conc','_wasm_get_effect_temp','_wasm_get_effect_sat_temp','_wasm_get_effect_pressure','_wasm_get_effect_bpe','_wasm_get_effect_delta_t','_wasm_get_effect_area','_wasm_get_effect_heat_duty','_wasm_get_last_error','_wasm_simulate_json','_malloc','_free'] -s ALLOW_MEMORY_GROWTH=1 -O3"
    )
endif()

# Testes Automatizados
enable_testing()

set(TEST_SOURCES
    tests/test_main.c
    tests/test_thermodynamics.c
    tests/test_solver_regression.c
    tests/test_validation.c
)

add_executable(evap_tests ${TEST_SOURCES})
target_link_libraries(evap_tests PRIVATE evap_core)

add_test(NAME AllTests COMMAND evap_tests)

