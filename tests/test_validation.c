/**
 * @file test_validation.c
 * @brief Testes para o módulo de validação de entradas e robustez.
 */

#include "evaporator/validation.h"
#include <stdio.h>
#include <math.h>

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    [FALHA] Linha %d: %s\n", __LINE__, msg); \
        fail_count++; \
    } \
} while(0)

int run_validation_tests(void) {
    int fail_count = 0;
    SolverError err;

    /* 1. Entrada válida padrão deve passar */
    SimulationInput valid = evap_default_input();
    TEST_ASSERT(validation_check_input(&valid, &err) == EVAP_OK, "Entrada padrao deve ser valida");

    /* 2. Rejeição de N < 2 e N > 20 */
    SimulationInput invalid_n = valid;
    invalid_n.n_effects = 1;
    TEST_ASSERT(validation_check_input(&invalid_n, &err) == EVAP_ERR_INVALID_INPUT, "N=1 deve ser rejeitado");
    invalid_n.n_effects = 25;
    TEST_ASSERT(validation_check_input(&invalid_n, &err) == EVAP_ERR_INVALID_INPUT, "N=25 deve ser rejeitado");

    /* 3. Rejeição de vazão negativa ou nula */
    SimulationInput invalid_f = valid;
    invalid_f.feed_rate_kg_h = -500.0;
    TEST_ASSERT(validation_check_input(&invalid_f, &err) == EVAP_ERR_INVALID_INPUT, "Vazao negativa deve ser rejeitada");

    /* 4. Rejeição de concentracao invertida (xF >= xN) */
    SimulationInput invalid_x = valid;
    invalid_x.feed_conc = 0.50;
    invalid_x.product_conc = 0.10;
    TEST_ASSERT(validation_check_input(&invalid_x, &err) == EVAP_ERR_INVALID_INPUT, "xF >= xN deve ser rejeitado");

    /* 5. Rejeição de gradiente termico global insuficiente */
    SimulationInput invalid_dt = valid;
    invalid_dt.steam_temp_c = 65.0;
    invalid_dt.last_sat_temp_c = 60.0;
    TEST_ASSERT(validation_check_input(&invalid_dt, &err) == EVAP_ERR_INFEASIBLE, "DT global < 15 deve ser inviavel");

    /* 6. Rejeição de omega invalido */
    SimulationInput invalid_w = valid;
    invalid_w.omega = 1.2;
    TEST_ASSERT(validation_check_input(&invalid_w, &err) == EVAP_ERR_INVALID_INPUT, "omega > 1 deve ser rejeitado");
    invalid_w.omega = -0.1;
    TEST_ASSERT(validation_check_input(&invalid_w, &err) == EVAP_ERR_INVALID_INPUT, "omega < 0 deve ser rejeitado");

    if (fail_count == 0) {
        printf("    [SUCESSO] 6/6 baterias de testes de validacao passaram.\n");
    }
    return fail_count;
}

