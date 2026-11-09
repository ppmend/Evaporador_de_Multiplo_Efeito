/**
 * @file test_solver_regression.c
 * @brief Testes de regressão confrontando os resultados com a Tabela 2 do artigo (COBEQ-IC 2026).
 */

#include "evaporator/solver.h"
#include <stdio.h>
#include <math.h>

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    [FALHA] Linha %d: %s\n", __LINE__, msg); \
        fail_count++; \
    } \
} while(0)

int run_solver_regression_tests(void) {
    int fail_count = 0;

    /* Valores de referência publicados na Tabela 2 do artigo */
    const double omega_ref[8]   = {0, 0, 0.665, 0.496, 0.445, 0.429, 0.377, 0.345};
    const double area_artigo[8]  = {0, 0, 58.083, 70.128, 89.608, 125.442, 211.225, 684.210};
    const double s_artigo[8]     = {0, 0, 5940.4, 4593.6, 3949.4, 3585.8, 3363.0, 3221.2};
    const double econ_artigo[8]  = {0, 0, 1.3467, 1.7415, 2.0256, 2.2310, 2.3788, 2.4836};

    for (int n = 2; n <= 7; n++) {
        SimulationInput in = evap_default_input();
        in.n_effects = n;
        in.omega = omega_ref[n];

        SimulationResult res;
        SolverError err;
        EvapStatus st = solver_run(&in, &res, &err);

        TEST_ASSERT(st == EVAP_OK, "Solver deve convergir para o caso de referencia");

        /* Verificação de proximidade com os dados publicados (tolerância de 0.15%) */
        double err_rel_area = fabs(res.area_per_effect_m2 - area_artigo[n]) / area_artigo[n];
        double err_rel_s = fabs(res.steam_rate_kg_h - s_artigo[n]) / s_artigo[n];
        double err_rel_econ = fabs(res.vapor_economy - econ_artigo[n]) / econ_artigo[n];

        TEST_ASSERT(err_rel_area < 0.0015, "Area deve coincidir com o artigo (erro < 0.15%)");
        TEST_ASSERT(err_rel_s < 0.0015, "Consumo S deve coincidir com o artigo (erro < 0.15%)");
        TEST_ASSERT(err_rel_econ < 0.0015, "Economia deve coincidir com o artigo (erro < 0.15%)");

        /* Verificação da igualdade estrita de áreas calculadas entre os efeitos */
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                double diff_a = fabs(res.area_m2[i] - res.area_m2[j]);
                TEST_ASSERT(diff_a < 1e-4, "Areas de todos os efeitos devem ser rigorosamente iguais");
            }
        }

        /* Fechamento do balanço de massa total: F = sum(V_i) + L_N */
        double total_v = 0.0;
        for (int i = 0; i < n; i++) {
            total_v += res.vapor_rate_kg_h[i];
        }
        double mass_closure = fabs(in.feed_rate_kg_h - (total_v + res.product_rate_kg_h));
        TEST_ASSERT(mass_closure < 1e-4, "Balanco de massa global deve fechar (< 1e-4 kg/h)");

        /* Fechamento do balanço de soluto: F * xF = L_N * xN */
        double solute_feed = in.feed_rate_kg_h * in.feed_conc;
        double solute_prod = res.product_rate_kg_h * res.concentration[n - 1];
        TEST_ASSERT(fabs(solute_feed - solute_prod) < 1e-4, "Balanco de soluto global deve fechar");

        printf("      N=%d: Area = %.3f m² (Ref: %.3f), S = %.1f kg/h (Ref: %.1f), Econ = %.4f (Ref: %.4f) [OK]\n",
               n, res.area_per_effect_m2, area_artigo[n], res.steam_rate_kg_h, s_artigo[n], res.vapor_economy, econ_artigo[n]);
    }

    if (fail_count == 0) {
        printf("    [SUCESSO] Todos os 6 casos de referencia (N=2 a 7) validados contra a Tabela 2!\n");
    }
    return fail_count;
}

