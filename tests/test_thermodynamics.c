/**
 * @file test_thermodynamics.c
 * @brief Testes unitários para as correlações e modelos termodinâmicos.
 */

#include "evaporator/thermodynamics.h"
#include <stdio.h>
#include <math.h>

#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { \
        printf("    [FALHA] Linha %d: %s\n", __LINE__, msg); \
        fail_count++; \
    } \
} while(0)

int run_thermodynamic_tests(void) {
    int fail_count = 0;

    /* 1. Antoine: Psat a 100°C deve ser próximo de 101.325 kPa */
    double p100 = thermo_psat_kpa(100.0);
    TEST_ASSERT(fabs(p100 - 101.325) < 0.5, "Psat a 100°C deve ser ~101.3 kPa");

    /* 2. Antoine inverso: Tsat a 101.325 kPa deve ser próximo de 100°C */
    double t101 = thermo_tsat_c(101.325);
    TEST_ASSERT(fabs(t101 - 100.0) < 0.2, "Tsat a 101.325 kPa deve ser ~100.0 °C");

    /* 3. Inversibilidade mútua de Antoine: Tsat(Psat(T)) == T */
    double t_test = 60.0;
    double p_calc = thermo_psat_kpa(t_test);
    double t_recalc = thermo_tsat_c(p_calc);
    TEST_ASSERT(fabs(t_recalc - t_test) < 0.05, "Inversibilidade Antoine Tsat(Psat(60))");

    t_test = 140.0;
    p_calc = thermo_psat_kpa(t_test);
    t_recalc = thermo_tsat_c(p_calc);
    TEST_ASSERT(fabs(t_recalc - t_test) < 0.05, "Inversibilidade Antoine Tsat(Psat(140))");

    /* 4. Calor de vaporização de Watson: dHvap a 100°C deve ser 2256.47 kJ/kg */
    double dh100 = thermo_heat_vaporization_kj_kg(100.0);
    TEST_ASSERT(fabs(dh100 - 2256.47) < 1.0, "dHvap a 100°C deve ser 2256.47 kJ/kg");
    TEST_ASSERT(thermo_heat_vaporization_kj_kg(60.0) > dh100, "dHvap deve ser decrescente com T");

    /* 5. Entalpia de vapor saturado: Hvsat a 100°C ~ 2676 kJ/kg */
    double hvsat100 = thermo_enthalpy_sat_vapor_kj_kg(100.0);
    TEST_ASSERT(hvsat100 > 2670.0 && hvsat100 < 2685.0, "Hvsat a 100°C deve ser ~2676 kJ/kg");

    /* 6. Entalpia da solução de NaOH: HL deve ser monotônica com a temperatura */
    double hl_30 = thermo_enthalpy_solution_kj_kg(30.0, 0.30);
    double hl_60 = thermo_enthalpy_solution_kj_kg(60.0, 0.30);
    double hl_90 = thermo_enthalpy_solution_kj_kg(90.0, 0.30);
    TEST_ASSERT(hl_60 > hl_30, "HL deve ser estritamente crescente com T (30 -> 60)");
    TEST_ASSERT(hl_90 > hl_60, "HL deve ser estritamente crescente com T (60 -> 90)");

    /* 7. Elevação do ponto de ebulição (EPE): deve ser positiva para soluções de NaOH */
    double epe_10 = thermo_bpe_c(101.325, 0.10);
    double epe_30 = thermo_bpe_c(101.325, 0.30);
    double epe_50 = thermo_bpe_c(101.325, 0.50);
    TEST_ASSERT(epe_10 > 0.0, "EPE a 10% deve ser positiva");
    TEST_ASSERT(epe_30 > epe_10, "EPE deve crescer com a concentracao (10% -> 30%)");
    TEST_ASSERT(epe_50 > epe_30, "EPE deve crescer com a concentracao (30% -> 50%)");
    TEST_ASSERT(epe_50 > 35.0 && epe_50 < 55.0, "EPE a 50% deve estar na faixa de 35 a 55 °C");

    if (fail_count == 0) {
        printf("    [SUCESSO] 7/7 baterias de testes termodinamicos passaram.\n");
    }
    return fail_count;
}

