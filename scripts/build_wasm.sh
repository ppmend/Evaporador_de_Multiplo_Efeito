#!/usr/bin/env bash
set -e

# Em MSYS2/UCRT64 o Emscripten é carregado por este profile; nos demais ambientes, use o emcc do PATH.
if [ -f /ucrt64/etc/profile.d/emscripten.sh ]; then
    export PATH=/ucrt64/bin:/usr/bin:$PATH
    source /ucrt64/etc/profile.d/emscripten.sh
fi

cd "$(dirname "$0")/.."

echo "Compilando núcleo C para WebAssembly com Emscripten (-O1)..."

emcc -O1 -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']" \
    -s EXPORTED_FUNCTIONS="['_wasm_solve_direct','_wasm_get_steam_rate','_wasm_get_vapor_economy','_wasm_get_area_per_effect','_wasm_get_total_area','_wasm_get_total_dt','_wasm_get_total_bpe','_wasm_get_outer_iter','_wasm_get_inner_iter','_wasm_get_effect_vapor','_wasm_get_effect_liquid','_wasm_get_effect_conc','_wasm_get_effect_temp','_wasm_get_effect_sat_temp','_wasm_get_effect_pressure','_wasm_get_effect_bpe','_wasm_get_effect_delta_t','_wasm_get_effect_area','_wasm_get_effect_heat_duty','_wasm_get_last_error','_wasm_simulate_json','_malloc','_free']" \
    -s MODULARIZE=1 \
    -s EXPORT_NAME="createEvapModule" \
    -s ALLOW_MEMORY_GROWTH=1 \
    -Iinclude -Isrc \
    src/wasm/wasm_api.c \
    src/core/solver.c \
    src/core/thermodynamics.c \
    src/core/validation.c \
    src/core/types.c \
    -o web/js/evap_solver.js

echo "Build WebAssembly concluído com sucesso em web/js/evap_solver.js e web/js/evap_solver.wasm!"
