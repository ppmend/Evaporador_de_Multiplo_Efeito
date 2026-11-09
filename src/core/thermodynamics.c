/**
 * @file thermodynamics.c
 * @brief Implementação dos modelos e correlações termodinâmicas.
 */

#include "evaporator/thermodynamics.h"
#include <math.h>

/* Fator de conversão: 1 mmHg = 0.133322 kPa */
#define MMHG_TO_KPA 0.133322
#define KPA_TO_MMHG (1.0 / 0.133322)

/* Fator de conversão: 1 BTU/lbm = 2.326 kJ/kg */
#define BTU_PER_LBM_TO_KJ_PER_KG 2.326

/* Temperatura crítica da água [K] e temperatura de ebulição normal [K] */
#define WATER_T_CRIT_K 647.096
#define WATER_T_BOIL_K 373.15

double thermo_psat_kpa(double temp_c) {
    double A, B, C;

    /* Antoine para água pura: seleção da faixa pela temperatura normal de ebulição */
    if (temp_c <= 100.0) {
        A = 8.07131;
        B = 1730.63;
        C = 233.426;
    } else {
        A = 8.14019;
        B = 1810.94;
        C = 244.485;
    }

    double P_mmHg = pow(10.0, A - B / (temp_c + C));
    return MMHG_TO_KPA * P_mmHg;
}

double thermo_tsat_c(double pressure_kpa) {
    double A, B, C;
    double P_mmHg = pressure_kpa * KPA_TO_MMHG;

    if (P_mmHg <= 0.0) {
        return -273.15; /* Pressão não física */
    }

    /* Seleção de faixa baseada na pressão atmosférica (~101.325 kPa) */
    if (pressure_kpa <= 101.325) {
        A = 8.07131;
        B = 1730.63;
        C = 233.426;
    } else {
        A = 8.14019;
        B = 1810.94;
        C = 244.485;
    }

    double logP = log10(P_mmHg);
    if (fabs(A - logP) < 1e-12) {
        return 100.0;
    }

    return (B / (A - logP)) - C;
}

double thermo_boiling_temp_c(double pressure_kpa, double mass_fraction) {
    if (pressure_kpa <= 0.0) {
        return -273.15;
    }

    double x = mass_fraction;
    double x2 = x * x;
    double x3 = x2 * x;

    /* Polinômios de Antoine modificado para solução aquosa de NaOH */
    double A = 3.62321 * x3 + 1.13082 * x2 - 0.71947 * x + 6.77113;
    double B = -2711.78458 * x3 - 11.18678 * x2 + 47.44337 * x - 1395.64615;
    double C = 190.80153;

    double logP = log10(pressure_kpa);
    double denom = logP - A;

    if (fabs(denom) < 1e-12) {
        return thermo_tsat_c(pressure_kpa);
    }

    return -C + (B / denom);
}

double thermo_bpe_c(double pressure_kpa, double mass_fraction) {
    double t_sol = thermo_boiling_temp_c(pressure_kpa, mass_fraction);
    double t_sat = thermo_tsat_c(pressure_kpa);
    double bpe = t_sol - t_sat;
    return (bpe > 0.0) ? bpe : 0.0;
}

double thermo_heat_vaporization_kj_kg(double temp_c) {
    double T_k = temp_c + 273.15;
    
    /* Proteção contra aproximação do ponto crítico */
    if (T_k >= WATER_T_CRIT_K) {
        return 0.0;
    }

    double num = 1.0 - (T_k / WATER_T_CRIT_K);
    double den = 1.0 - (WATER_T_BOIL_K / WATER_T_CRIT_K);
    double ratio = num / den;

    if (ratio <= 0.0) {
        return 0.0;
    }

    return 2256.47 * pow(ratio, 0.38);
}

double thermo_enthalpy_sat_vapor_kj_kg(double temp_c) {
    double T = temp_c;
    double T2 = T * T;
    double T3 = T2 * T;

    return 2500.9 + 1.805 * T - (2.1e-4 * T2) + (3.5e-7 * T3);
}

double thermo_enthalpy_superheated_kj_kg(double temp_c, double pressure_kpa) {
    double T_sat = thermo_tsat_c(pressure_kpa);
    double h_vsat = thermo_enthalpy_sat_vapor_kj_kg(T_sat);

    /* Coeficientes da correlação polinomial de calor específico Cp(T) para vapor */
    double a = 1.872;
    double b = 0.00055;
    double c = -1.6e-7;

    double T = temp_c;
    double integral = a * (T - T_sat)
                    + (b / 2.0) * (T * T - T_sat * T_sat)
                    + (c / 3.0) * (T * T * T - T_sat * T_sat * T_sat);

    return h_vsat + integral;
}

double thermo_enthalpy_solution_kj_kg(double temp_c, double mass_fraction) {
    /* Conversão da temperatura de Celsius para Fahrenheit */
    double Tf = 32.0 + (9.0 * temp_c / 5.0);

    /* Coeficientes empíricos da correlação ajustada em BTU/lbm */
    double z0 = -26.01021;
    double a  = -207.89085;
    double b  = 0.99923;
    double c  = 795.64522;
    double d  = -1.27127e-5;
    double f  = -0.47635;

    double x = mass_fraction;
    double H_btu_lbm = z0 
                     + a * x 
                     + b * Tf 
                     + c * (x * x) 
                     + d * (Tf * Tf) 
                     + f * x * Tf;

    return BTU_PER_LBM_TO_KJ_PER_KG * H_btu_lbm;
}
