/**
 * @file wasm_api.h
 * @brief Interface WebAssembly para execução do solver numérico em navegadores web.
 */

#ifndef EVAPORATOR_WASM_API_H
#define EVAPORATOR_WASM_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Executa a simulação via JSON e retorna uma string JSON contendo o resultado completo.
 *
 * @param json_input String JSON contendo os parâmetros de entrada.
 * @return Ponteiro para string JSON alocada internamente com os resultados (ou mensagem de erro).
 */
const char* wasm_simulate_json(const char *json_input);

/**
 * @brief Executa a simulação através de parâmetros escalares diretos.
 */
int wasm_solve_direct(int n_effects,
                      double feed_rate_kg_h,
                      double feed_conc,
                      double product_conc,
                      double feed_temp_c,
                      double steam_temp_c,
                      double last_sat_temp_c,
                      double htc_w_m2k,
                      double omega);

/* Getters para extração direta via JavaScript */
double wasm_get_steam_rate(void);
double wasm_get_vapor_economy(void);
double wasm_get_area_per_effect(void);
double wasm_get_total_area(void);
double wasm_get_total_dt(void);
double wasm_get_total_bpe(void);
int wasm_get_outer_iter(void);
int wasm_get_inner_iter(void);

double wasm_get_effect_vapor(int effect_idx);
double wasm_get_effect_liquid(int effect_idx);
double wasm_get_effect_conc(int effect_idx);
double wasm_get_effect_temp(int effect_idx);
double wasm_get_effect_sat_temp(int effect_idx);
double wasm_get_effect_pressure(int effect_idx);
double wasm_get_effect_bpe(int effect_idx);
double wasm_get_effect_delta_t(int effect_idx);
double wasm_get_effect_area(int effect_idx);
double wasm_get_effect_heat_duty(int effect_idx);

const char* wasm_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* EVAPORATOR_WASM_API_H */
