/**
 * @file solver.c
 * @brief Implementação do solver numérico acoplado para evaporadores de múltiplo efeito.
 */

#include "evaporator/solver.h"
#include "evaporator/thermodynamics.h"
#include "evaporator/validation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/**
 * @brief Estrutura interna para encapsular o estado de cálculo sem estado global.
 */
typedef struct {
    int n;
    double V[EVAP_MAX_EFFECTS];
    double L[EVAP_MAX_EFFECTS];
    double T[EVAP_MAX_EFFECTS];
    double T_sat[EVAP_MAX_EFFECTS];
    double dT[EVAP_MAX_EFFECTS];
    double x[EVAP_MAX_EFFECTS];
    double U[EVAP_MAX_EFFECTS];
    double A[EVAP_MAX_EFFECTS];
    double EPE[EVAP_MAX_EFFECTS];
    double P[EVAP_MAX_EFFECTS];
    double S;
    double DT;
} InternalState;

/**
 * @brief Inicializa o domínio e as estimativas iniciais de vazões, pressões e temperaturas.
 */
static void initialize_domain(const SimulationInput *input, InternalState *st) {
    st->n = input->n_effects;

    for (int i = 0; i < st->n; i++) {
        st->U[i] = input->htc_w_m2k;
    }

    /* Balanço de soluto global: L_N = (xF / xN) * F */
    st->x[st->n - 1] = input->product_conc;
    st->L[st->n - 1] = (input->feed_conc / input->product_conc) * input->feed_rate_kg_h;
    st->T_sat[st->n - 1] = input->last_sat_temp_c;
    st->P[st->n - 1] = thermo_psat_kpa(st->T_sat[st->n - 1]);

    /* Inicialização equitativa da taxa de evaporação */
    double total_evaporation = input->feed_rate_kg_h - st->L[st->n - 1];
    double v_guess = total_evaporation / (double)st->n;

    for (int i = 0; i < st->n; i++) {
        st->V[i] = v_guess;
    }

    st->L[0] = input->feed_rate_kg_h - st->V[0];
    for (int i = 1; i < st->n - 1; i++) {
        st->L[i] = st->L[i - 1] - st->V[i];
    }

    for (int i = 0; i < st->n - 1; i++) {
        st->x[i] = (input->feed_conc * input->feed_rate_kg_h) / st->L[i];
    }

    /* Distribuição inicial de pressões entre vapor vivo e condensador final */
    double P1 = thermo_psat_kpa(input->steam_temp_c);
    double Pn = thermo_psat_kpa(st->T_sat[st->n - 1]);

    for (int i = 0; i < st->n; i++) {
        st->P[i] = P1 - ((double)i * (P1 - Pn)) / (double)(st->n - 1);
    }

    for (int i = 0; i < st->n; i++) {
        st->T[i] = thermo_boiling_temp_c(st->P[i], st->x[i]);
    }

    for (int i = 0; i < st->n - 1; i++) {
        st->T_sat[i] = thermo_tsat_c(st->P[i]);
    }

    for (int i = 0; i < st->n; i++) {
        st->EPE[i] = st->T[i] - st->T_sat[i];
    }

    double sumEPE = 0.0;
    for (int i = 0; i < st->n - 1; i++) {
        sumEPE += st->EPE[i];
    }

    st->DT = input->steam_temp_c - st->T_sat[st->n - 1] - sumEPE;
    st->S = 0.0;
}

/**
 * @brief Resolve o sistema acoplado de balanço de massa e energia via Gauss-Seidel SOR.
 */
static EvapStatus solve_system_sor(const SimulationInput *input,
                                   InternalState *st,
                                   int *inner_iterations,
                                   double *final_residual,
                                   SolverError *error) {
    double errorIt = 1.0;
    int it = 0;
    double tol = (input->tol_system > 0.0) ? input->tol_system : EVAP_DEFAULT_TOL;
    int max_it = (input->max_iter_inner > 0) ? input->max_iter_inner : EVAP_DEFAULT_MAX_ITER_INNER;
    double omega = input->omega;

    /* Estimativa inicial de vapor vivo caso nulo */
    if (st->S <= 0.0) {
        st->S = (input->feed_rate_kg_h - st->L[st->n - 1]) / (double)st->n;
    }

    while (errorIt > tol && it < max_it) {
        it++;
        double S_old = st->S;
        double V_old[EVAP_MAX_EFFECTS];
        double L_old[EVAP_MAX_EFFECTS];

        for (int i = 0; i < st->n; i++) {
            V_old[i] = st->V[i];
            L_old[i] = st->L[i];
        }

        /* Balanço de energia no Efeito 1:
           V_1*h_v1 + L_1*H_L1 - L_0*H_L0 = 0 => V_1 isolado através dos efeitos seguintes */
        double h_super_1 = thermo_enthalpy_superheated_kj_kg(st->T[1], st->P[1]);
        double hl_1 = thermo_enthalpy_solution_kj_kg(st->T[1], st->x[1]);
        double hl_0 = thermo_enthalpy_solution_kj_kg(st->T[0], st->x[0]);
        double dhvap_0 = thermo_heat_vaporization_kj_kg(st->T[0]);

        if (fabs(dhvap_0) < 1e-9) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Calor de vaporização nulo no efeito 1 (T = %.2f °C).", st->T[0]);
                error->effect_index = 1;
            }
            return EVAP_ERR_NUMERICAL;
        }

        st->V[0] = (st->V[1] * h_super_1 + st->L[1] * hl_1 - st->L[0] * hl_0) / dhvap_0;
        st->L[0] = input->feed_rate_kg_h - st->V[0];

        /* Efeitos intermediários: i = 1 ... n-2 */
        for (int i = 1; i < st->n - 1; i++) {
            double h_super_next = thermo_enthalpy_superheated_kj_kg(st->T[i + 1], st->P[i + 1]);
            double hl_next = thermo_enthalpy_solution_kj_kg(st->T[i + 1], st->x[i + 1]);
            double hl_curr = thermo_enthalpy_solution_kj_kg(st->T[i], st->x[i]);
            double dhvap_curr = thermo_heat_vaporization_kj_kg(st->T[i]);

            if (fabs(dhvap_curr) < 1e-9) {
                if (error) {
                    error->status = EVAP_ERR_NUMERICAL;
                    snprintf(error->message, sizeof(error->message),
                             "Calor de vaporização nulo no efeito %d (T = %.2f °C).", i + 1, st->T[i]);
                    error->effect_index = i + 1;
                }
                return EVAP_ERR_NUMERICAL;
            }

            st->V[i] = (st->V[i + 1] * h_super_next + st->L[i + 1] * hl_next - st->L[i] * hl_curr) / dhvap_curr;
            st->L[i] = st->L[i - 1] - st->V[i];
        }

        /* Efeito final n: balanço global de massa para fechar com L_N */
        st->V[st->n - 1] = st->L[st->n - 2] - st->L[st->n - 1];

        /* Consumo de vapor vivo S no primeiro efeito */
        double h_super_0 = thermo_enthalpy_superheated_kj_kg(st->T[0], st->P[0]);
        double hl_feed = thermo_enthalpy_solution_kj_kg(input->feed_temp_c, input->feed_conc);
        double dhvap_s = thermo_heat_vaporization_kj_kg(input->steam_temp_c);

        if (fabs(dhvap_s) < 1e-9) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Calor de vaporização nulo para vapor vivo (TS = %.2f °C).", input->steam_temp_c);
                error->effect_index = 0;
            }
            return EVAP_ERR_NUMERICAL;
        }

        double S_calculated = (st->V[0] * h_super_0 + st->L[0] * hl_0 - input->feed_rate_kg_h * hl_feed) / dhvap_s;

        /* Aplicação da sub-relaxação SOR */
        st->S = S_old + omega * (S_calculated - S_old);

        for (int i = 0; i < st->n; i++) {
            st->V[i] = V_old[i] + omega * (st->V[i] - V_old[i]);
            st->L[i] = L_old[i] + omega * (st->L[i] - L_old[i]);
        }

        /* Checagem de finitude */
        if (!isfinite(st->S)) {
            if (error) {
                error->status = EVAP_ERR_NUMERICAL;
                snprintf(error->message, sizeof(error->message),
                         "Divergência numérica no cálculo de vapor vivo S (iteração interna %d).", it);
                error->effect_index = 0;
            }
            return EVAP_ERR_NUMERICAL;
        }

        /* Avaliação do resíduo relativo */
        double sum_sq = 0.0;
        for (int i = 0; i < st->n; i++) {
            double v_denom = (fabs(V_old[i]) > 1e-6) ? V_old[i] : 1.0;
            double l_denom = (fabs(L_old[i]) > 1e-6) ? L_old[i] : 1.0;
            sum_sq += pow((st->V[i] - V_old[i]) / v_denom, 2.0);
            sum_sq += pow((st->L[i] - L_old[i]) / l_denom, 2.0);
        }

        double s_denom = (fabs(S_old) > 1e-6) ? S_old : 1.0;
        errorIt = pow((st->S - S_old) / s_denom, 2.0) + sum_sq;

        /* Atualização das frações mássicas líquidas */
        for (int i = 0; i < st->n - 1; i++) {
            if (st->L[i] > 1e-6) {
                st->x[i] = (input->feed_conc * input->feed_rate_kg_h) / st->L[i];
            }
        }
    }

    *inner_iterations = it;
    *final_residual = errorIt;

    if (it >= max_it && errorIt > tol) {
        if (error) {
            error->status = EVAP_ERR_NOT_CONVERGED;
            snprintf(error->message, sizeof(error->message),
                     "Laço interno SOR não convergiu após %d iterações (resíduo = %.3e > tolerância = %.3e).",
                     max_it, errorIt, tol);
            error->effect_index = 0;
        }
        return EVAP_ERR_NOT_CONVERGED;
    }

    return EVAP_OK;
}

EvapStatus solver_run(const SimulationInput *input,
                      SimulationResult *result,
                      SolverError *error) {
    if (!result) {
        if (error) {
            error->status = EVAP_ERR_INVALID_INPUT;
            snprintf(error->message, sizeof(error->message), "Ponteiro de resultado nulo.");
            error->effect_index = 0;
        }
        return EVAP_ERR_INVALID_INPUT;
    }

    memset(result, 0, sizeof(SimulationResult));

    /* 1. Validação prévia das entradas */
    EvapStatus status = validation_check_input(input, error);
    if (status != EVAP_OK) {
        return status;
    }

    /* 2. Inicialização do estado de cálculo */
    InternalState st;
    initialize_domain(input, &st);

    /* 3. Avaliação da viabilidade na configuração inicial */
    status = validation_check_stability(st.n, st.DT, st.T_sat, st.EPE, error);
    if (status != EVAP_OK) {
        return status;
    }

    double tol_area = (input->tol_area > 0.0) ? input->tol_area : EVAP_DEFAULT_TOL;
    int max_iter_outer = (input->max_iter_outer > 0) ? input->max_iter_outer : EVAP_DEFAULT_MAX_ITER_OUTER;

    double errorIt = 1.0;
    int iter_outer = 0;
    int total_inner_iter = 0;
    double last_system_residual = 0.0;

    /* 4. Laço externo: equalização das áreas */
    while (errorIt > tol_area && iter_outer < max_iter_outer) {
        iter_outer++;

        /* Gradientes motrizes locais de temperatura */
        st.dT[0] = input->steam_temp_c - st.T[0];
        for (int i = 1; i < st.n; i++) {
            st.dT[i] = st.T_sat[i - 1] - st.T[i];
        }

        /* Checagem de viabilidade térmica */
        status = validation_check_stability(st.n, st.DT, st.T_sat, st.EPE, error);
        if (status != EVAP_OK) {
            return status;
        }

        /* Resolução dos balanços de massa e energia acoplados via SOR */
        int inner_it = 0;
        status = solve_system_sor(input, &st, &inner_it, &last_system_residual, error);
        total_inner_iter += inner_it;
        if (status != EVAP_OK) {
            return status;
        }

        /* Dimensionamento térmico: A_i = Q_i / (U_i * dT_i)
           Fator 3.6: converte W/(m²·K) e kJ/h de modo que a área resulte diretamente em m².
           demonstração: (kJ/h) / [ (W/(m²·K)) * K * 3.6 ]
                       = (kJ/h) / [ (10^-3 kJ/s) * 3600 s/h ] = m² */
        double dhvap_s = thermo_heat_vaporization_kj_kg(input->steam_temp_c);
        st.A[0] = (st.S * dhvap_s) / (st.U[0] * st.dT[0] * 3.6);

        for (int i = 1; i < st.n; i++) {
            double dhvap_prev = thermo_heat_vaporization_kj_kg(st.T_sat[i - 1]);
            st.A[i] = (st.V[i - 1] * dhvap_prev) / (st.U[i] * st.dT[i] * 3.6);
        }

        /* Avaliação do critério de parada: máxima discrepância entre áreas */
        errorIt = fabs(st.A[0] - st.A[st.n - 1]);
        for (int i = 1; i < st.n; i++) {
            double diff = fabs(st.A[i] - st.A[i - 1]);
            if (diff > errorIt) {
                errorIt = diff;
            }
        }

        /* Média ponderada da área para redistribuição proporcional dos gradientes térmicos */
        double sum1 = 0.0;
        double sum2 = 0.0;
        for (int i = 0; i < st.n; i++) {
            sum1 += st.A[i] * st.dT[i];
            sum2 += st.dT[i];
        }

        double Am = (fabs(sum2) > 1e-9) ? (sum1 / sum2) : st.A[0];

        /* Redistribuição proporcional dos gradientes de temperatura */
        for (int i = 0; i < st.n; i++) {
            st.dT[i] = (st.dT[i] * st.A[i]) / Am;
        }

        /* Atualização das propriedades físicas para a próxima iteração */
        for (int i = 0; i < st.n - 1; i++) {
            if (st.L[i] > 1e-6) {
                st.x[i] = (input->feed_conc * input->feed_rate_kg_h) / st.L[i];
            }
            st.T_sat[i] = st.T[i + 1] + st.dT[i + 1];
            st.P[i] = thermo_psat_kpa(st.T_sat[i]);
            st.T[i] = thermo_boiling_temp_c(st.P[i], st.x[i]);
            st.EPE[i] = st.T[i] - st.T_sat[i];
        }

        double sumEPE = 0.0;
        for (int i = 0; i < st.n - 1; i++) {
            sumEPE += st.EPE[i];
        }

        /* Delta T total disponível */
        st.DT = input->steam_temp_c - st.T[st.n - 1] - sumEPE;
    }

    if (iter_outer >= max_iter_outer && errorIt > tol_area) {
        if (error) {
            error->status = EVAP_ERR_NOT_CONVERGED;
            snprintf(error->message, sizeof(error->message),
                     "Laço externo de equalização de áreas não convergiu após %d iterações (discrepância = %.3e m² > tolerância = %.3e m²).",
                     max_iter_outer, errorIt, tol_area);
            error->effect_index = 0;
        }
        return EVAP_ERR_NOT_CONVERGED;
    }

    /* 5. Preenchimento da estrutura de resultados pública */
    result->n_effects = st.n;
    result->steam_rate_kg_h = st.S;
    result->steam_temp_c = input->steam_temp_c;
    result->product_rate_kg_h = st.L[st.n - 1];
    result->vapor_economy = (input->feed_rate_kg_h - st.L[st.n - 1]) / st.S;

    double total_area = 0.0;
    double total_bpe = 0.0;
    double total_evap = 0.0;

    for (int i = 0; i < st.n; i++) {
        result->vapor_rate_kg_h[i] = st.V[i];
        result->liquid_rate_kg_h[i] = st.L[i];
        result->concentration[i] = st.x[i];
        result->temperature_c[i] = st.T[i];
        result->sat_temperature_c[i] = st.T_sat[i];
        result->pressure_kpa[i] = st.P[i];
        result->bpe_c[i] = st.EPE[i];
        result->delta_t_c[i] = st.dT[i];
        result->area_m2[i] = st.A[i];

        /* Carga térmica Q em kW: Q (kJ/h) / 3600 = kW */
        if (i == 0) {
            double q_kj_h = st.S * thermo_heat_vaporization_kj_kg(input->steam_temp_c);
            result->heat_duty_kw[i] = q_kj_h / 3600.0;
        } else {
            double q_kj_h = st.V[i - 1] * thermo_heat_vaporization_kj_kg(st.T_sat[i - 1]);
            result->heat_duty_kw[i] = q_kj_h / 3600.0;
        }

        total_area += st.A[i];
        total_bpe += st.EPE[i];
        total_evap += st.V[i];
    }

    result->total_area_m2 = total_area;
    result->area_per_effect_m2 = total_area / (double)st.n;
    result->total_delta_t_c = st.DT;
    result->total_bpe_c = total_bpe;

    result->iter_outer = iter_outer;
    result->iter_inner_total = total_inner_iter;
    result->residual_area = errorIt;
    result->residual_system = last_system_residual;
    result->max_area_discrepancy_m2 = errorIt;
    result->mass_balance_error_kg_h = fabs(input->feed_rate_kg_h - (total_evap + st.L[st.n - 1]));

    /* 6. Verificação final de sanidade física dos resultados */
    status = validation_check_results(input, result, error);
    if (status != EVAP_OK) {
        return status;
    }

    if (error) {
        error->status = EVAP_OK;
        snprintf(error->message, sizeof(error->message),
                 "Simulação convergiu com sucesso em %d iterações de área e %d iterações internas.",
                 iter_outer, total_inner_iter);
        error->effect_index = 0;
    }

    return EVAP_OK;
}
