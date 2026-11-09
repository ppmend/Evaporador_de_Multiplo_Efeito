/**
 * @file wasm_api.c
 * @brief Implementação da API WebAssembly do solver de evaporadores.
 */

#include "evaporator/wasm_api.h"
#include "evaporator/solver.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define WASM_EXPORT
#endif

static SimulationResult g_last_result;
static SolverError g_last_error;
static char g_json_buffer[8192];
static char g_error_buffer[512];

WASM_EXPORT
int wasm_solve_direct(int n_effects,
                      double feed_rate_kg_h,
                      double feed_conc,
                      double product_conc,
                      double feed_temp_c,
                      double steam_temp_c,
                      double last_sat_temp_c,
                      double htc_w_m2k,
                      double omega) {
    SimulationInput in = evap_default_input();
    in.n_effects = n_effects;
    in.feed_rate_kg_h = feed_rate_kg_h;
    in.feed_conc = feed_conc;
    in.product_conc = product_conc;
    in.feed_temp_c = feed_temp_c;
    in.steam_temp_c = steam_temp_c;
    in.last_sat_temp_c = last_sat_temp_c;
    in.htc_w_m2k = htc_w_m2k;
    in.omega = omega;

    EvapStatus status = solver_run(&in, &g_last_result, &g_last_error);
    if (status != EVAP_OK) {
        snprintf(g_error_buffer, sizeof(g_error_buffer), "%s", g_last_error.message);
        return (int)status;
    }

    g_error_buffer[0] = '\0';
    return (int)EVAP_OK;
}

WASM_EXPORT
double wasm_get_steam_rate(void) {
    return g_last_result.steam_rate_kg_h;
}

WASM_EXPORT
double wasm_get_vapor_economy(void) {
    return g_last_result.vapor_economy;
}

WASM_EXPORT
double wasm_get_area_per_effect(void) {
    return g_last_result.area_per_effect_m2;
}

WASM_EXPORT
double wasm_get_total_area(void) {
    return g_last_result.total_area_m2;
}

WASM_EXPORT
double wasm_get_total_dt(void) {
    return g_last_result.total_delta_t_c;
}

WASM_EXPORT
double wasm_get_total_bpe(void) {
    return g_last_result.total_bpe_c;
}

WASM_EXPORT
int wasm_get_outer_iter(void) {
    return g_last_result.iter_outer;
}

WASM_EXPORT
int wasm_get_inner_iter(void) {
    return g_last_result.iter_inner_total;
}

WASM_EXPORT
double wasm_get_effect_vapor(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.vapor_rate_kg_h[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_liquid(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.liquid_rate_kg_h[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_conc(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.concentration[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_temp(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.temperature_c[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_sat_temp(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.sat_temperature_c[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_pressure(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.pressure_kpa[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_bpe(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.bpe_c[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_delta_t(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.delta_t_c[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_area(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.area_m2[idx];
    return 0.0;
}

WASM_EXPORT
double wasm_get_effect_heat_duty(int idx) {
    if (idx >= 0 && idx < g_last_result.n_effects) return g_last_result.heat_duty_kw[idx];
    return 0.0;
}

WASM_EXPORT
const char* wasm_get_last_error(void) {
    return g_error_buffer;
}

WASM_EXPORT
const char* wasm_simulate_json(const char *json_input) {
    (void)json_input;
    /* Gera JSON a partir do estado atual da última simulação */
    int offset = 0;
    offset += snprintf(g_json_buffer + offset, sizeof(g_json_buffer) - (size_t)offset,
                       "{\"status\":%d,\"steam_rate_kg_h\":%.4f,\"vapor_economy\":%.4f,\"area_per_effect_m2\":%.4f,"
                       "\"total_area_m2\":%.4f,\"total_delta_t_c\":%.4f,\"total_bpe_c\":%.4f,\"effects\":[",
                       g_last_error.status,
                       g_last_result.steam_rate_kg_h,
                       g_last_result.vapor_economy,
                       g_last_result.area_per_effect_m2,
                       g_last_result.total_area_m2,
                       g_last_result.total_delta_t_c,
                       g_last_result.total_bpe_c);

    for (int i = 0; i < g_last_result.n_effects; i++) {
        offset += snprintf(g_json_buffer + offset, sizeof(g_json_buffer) - (size_t)offset,
                           "{\"i\":%d,\"V\":%.2f,\"L\":%.2f,\"x\":%.4f,\"T\":%.2f,\"Tsat\":%.2f,\"P\":%.2f,\"EPE\":%.2f,\"dT\":%.2f,\"A\":%.3f,\"Q\":%.2f}%s",
                           i + 1,
                           g_last_result.vapor_rate_kg_h[i],
                           g_last_result.liquid_rate_kg_h[i],
                           g_last_result.concentration[i],
                           g_last_result.temperature_c[i],
                           g_last_result.sat_temperature_c[i],
                           g_last_result.pressure_kpa[i],
                           g_last_result.bpe_c[i],
                           g_last_result.delta_t_c[i],
                           g_last_result.area_m2[i],
                           g_last_result.heat_duty_kw[i],
                           (i == g_last_result.n_effects - 1) ? "" : ",");
    }

    snprintf(g_json_buffer + offset, sizeof(g_json_buffer) - (size_t)offset, "]}");
    return g_json_buffer;
}

