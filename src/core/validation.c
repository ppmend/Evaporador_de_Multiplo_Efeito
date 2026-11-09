/**
 * @file validation.c
 * @brief Implementação dos módulos de validação de consistência e estabilidade física.
 */

#include "evaporator/validation.h"
#include <stdio.h>
#include <math.h>

EvapStatus validation_check_input(const SimulationInput *input, SolverError *error) {
    if (!input) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message), "Ponteiro de entrada nulo.");
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->n_effects < EVAP_MIN_EFFECTS || input->n_effects > EVAP_MAX_EFFECTS) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Número de efeitos (%d) fora dos limites permitidos [%d, %d].",
                     input->n_effects, EVAP_MIN_EFFECTS, EVAP_MAX_EFFECTS);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->feed_rate_kg_h <= 0.0) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Vazão de alimentação (%.2f kg/h) deve ser estritamente positiva.",
                     input->feed_rate_kg_h);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->feed_conc <= 0.0 || input->feed_conc >= 1.0) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Fração mássica de alimentação xF (%.4f) deve pertencer ao intervalo (0, 1).",
                     input->feed_conc);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->product_conc <= input->feed_conc || input->product_conc >= 0.70) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Fração mássica de produto xN (%.4f) deve ser maior que xF (%.4f) e inferior a 0.70.",
                     input->product_conc, input->feed_conc);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->feed_temp_c <= 0.0 || input->feed_temp_c > 200.0) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Temperatura de alimentação (%.1f °C) fora da faixa operacional comum (0 a 200 °C).",
                     input->feed_temp_c);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->last_sat_temp_c <= 20.0 || input->last_sat_temp_c >= input->steam_temp_c) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Temperatura de saturação do último efeito (%.1f °C) deve ser > 20 °C e < TS (%.1f °C).",
                     input->last_sat_temp_c, input->steam_temp_c);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->steam_temp_c - input->last_sat_temp_c < 15.0) {
        if (error) {
            error->status = EVAP_ERR_INFEASIBLE;
            snprintf(error->message, sizeof(error->message),
                     "Diferença global bruta (TS - T_sat,N = %.1f °C) insuficiente para múltiplos efeitos.",
                     input->steam_temp_c - input->last_sat_temp_c);
            error->effect_index = 0;
        }
        return EVAP_ERR_INFEASIBLE;
    }

    if (input->htc_w_m2k <= 0.0) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Coeficiente global U (%.1f W/(m²·K)) deve ser estritamente positivo.",
                     input->htc_w_m2k);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    if (input->omega <= 0.0 || input->omega >= 1.0) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message),
                     "Fator de sub-relaxação ômega (%.4f) deve satisfazer 0 < omega < 1.",
                     input->omega);
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    return EVAP_OK;
}

EvapStatus validation_check_stability(int n_effects,
                                      double total_dt,
                                      const double *t_sat,
                                      const double *epe,
                                      SolverError *error) {
    if (total_dt <= 0.0) {
        if (error) {
            error->status = EVAP_ERR_INFEASIBLE;
            snprintf(error->message, sizeof(error->message),
                     "Sistema fisicamente inviável: Delta T global disponível (%.4f °C) <= 0.",
                     total_dt);
            error->effect_index = 0;
        }
        return EVAP_ERR_INFEASIBLE;
    }

    for (int i = 1; i < n_effects; i++) {
        double delta_tsat = t_sat[i - 1] - t_sat[i];
        if (delta_tsat <= epe[i]) {
            if (error) {
                error->status = EVAP_ERR_INFEASIBLE;
                snprintf(error->message, sizeof(error->message),
                         "Efeito %d termicamente inviável: gradiente de saturação (T_sat[%d] - T_sat[%d] = %.2f °C) "
                         "inferior à elevação do ponto de ebulição (EPE[%d] = %.2f °C).",
                         i + 1, i, i + 1, delta_tsat, i + 1, epe[i]);
                error->effect_index = i + 1;
            }
            return EVAP_ERR_INFEASIBLE;
        }
    }

    return EVAP_OK;
}

EvapStatus validation_check_results(const SimulationInput *input,
                                    const SimulationResult *result,
                                    SolverError *error) {
    if (!result) {
        if (error) {
            error->status = EVAP_ERR_NUMERICAL;
            snprintf(error->message, sizeof(error->message), "Resultado nulo.");
            error->effect_index = 0;
        }
        return EVAP_ERR_NUMERICAL;
    }

    if (result->steam_rate_kg_h <= 0.0 || !isfinite(result->steam_rate_kg_h)) {
        if (error) {
            error->status = EVAP_ERR_NUMERICAL;
            snprintf(error->message, sizeof(error->message),
                     "Consumo de vapor vivo não físico (S = %.2f kg/h).",
                     result->steam_rate_kg_h);
            error->effect_index = 0;
        }
        return EVAP_ERR_NUMERICAL;
    }

    for (int i = 0; i < input->n_effects; i++) {
        if (!isfinite(result->area_m2[i]) || result->area_m2[i] <= 0.0) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Área calculada não física no efeito %d (A = %.3f m²).",
                         i + 1, result->area_m2[i]);
                error->effect_index = i + 1;
            }
            return EVAP_ERR_NUMERICAL;
        }

        if (result->liquid_rate_kg_h[i] <= 0.0 || !isfinite(result->liquid_rate_kg_h[i])) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Vazão de líquido não física no efeito %d (L = %.2f kg/h).",
                         i + 1, result->liquid_rate_kg_h[i]);
                error->effect_index = i + 1;
            }
            return EVAP_ERR_NUMERICAL;
        }

        if (result->vapor_rate_kg_h[i] <= 0.0 || !isfinite(result->vapor_rate_kg_h[i])) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Vazão de vapor não física no efeito %d (V = %.2f kg/h).",
                         i + 1, result->vapor_rate_kg_h[i]);
                error->effect_index = i + 1;
            }
            return EVAP_ERR_NUMERICAL;
        }
    }

    return EVAP_OK;
}
