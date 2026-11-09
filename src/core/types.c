/**
 * @file types.c
 * @brief Implementação de funções auxiliares de inicialização e conversão de status.
 */

#include "evaporator/types.h"

SimulationInput evap_default_input(void) {
    SimulationInput input;
    input.n_effects = 3;
    input.feed_rate_kg_h = 10000.0;
    input.feed_conc = 0.10;
    input.product_conc = 0.50;
    input.feed_temp_c = 50.0;
    input.steam_temp_c = 160.0;
    input.last_sat_temp_c = 60.0;
    input.htc_w_m2k = 2000.0;
    input.omega = 0.496; /* Valor ótimo do artigo para N=3 */
    input.tol_area = EVAP_DEFAULT_TOL;
    input.tol_system = EVAP_DEFAULT_TOL;
    input.max_iter_outer = EVAP_DEFAULT_MAX_ITER_OUTER;
    input.max_iter_inner = EVAP_DEFAULT_MAX_ITER_INNER;
    return input;
}

const char* evap_status_string(EvapStatus status) {
    switch (status) {
        case EVAP_OK:
            return "Sucesso: convergência atingida.";
        case EVAP_ERR_INVALID_INPUT:
            return "Erro: parâmetros de entrada inválidos.";
        case EVAP_ERR_INFEASIBLE:
            return "Erro: sistema fisicamente inviável (gradientes térmicos negativos ou insuficientes).";
        case EVAP_ERR_NOT_CONVERGED:
            return "Erro: limite máximo de iterações atingido sem convergência.";
        case EVAP_ERR_NUMERICAL:
            return "Erro: instabilidade numérica detectada (NaN, Infinito ou divisão inválida).";
        case EVAP_ERR_ALLOC:
            return "Erro: falha de alocação de memória.";
        default:
            return "Erro: status desconhecido.";
    }
}

