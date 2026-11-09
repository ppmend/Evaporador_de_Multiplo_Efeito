/**
 * @file thermodynamics.h
 * @brief Modelos e correlações termodinâmicas para água, vapor d'água e soluções aquosas de NaOH.
 *
 * Mapeamento e rastreabilidade:
 * - thermo_psat_kpa: Equação de Antoine (água pura) em duas faixas (T <= 100°C e T > 100°C).
 * - thermo_tsat_c: Inversa da Equação de Antoine para água pura.
 * - thermo_boiling_temp_c: Antoine modificado com coeficientes A(x), B(x), C para solução aquosa de NaOH (inclui EPE).
 * - thermo_heat_vaporization_kj_kg: Correlação de Watson para calor latente de vaporização da água.
 * - thermo_enthalpy_sat_vapor_kj_kg: Ajuste polinomial de tabelas de vapor saturado (0 a 300°C).
 * - thermo_enthalpy_superheated_kj_kg: Integral de Cp(T) para vapor superaquecido a partir do estado saturado.
 * - thermo_enthalpy_solution_kj_kg: Ajuste polinomial multivariável para entalpia de solução de NaOH (Perry's Handbook).
 */

#ifndef EVAPORATOR_THERMODYNAMICS_H
#define EVAPORATOR_THERMODYNAMICS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pressão de saturação da água pura via Antoine.
 * @param temp_c Temperatura em graus Celsius [°C]. Faixa típica: 0 a 200 °C.
 * @return Pressão de saturação em kPa.
 */
double thermo_psat_kpa(double temp_c);

/**
 * @brief Temperatura de saturação da água pura via inverso de Antoine.
 * @param pressure_kpa Pressão em kPa. Faixa típica: 1 a 2000 kPa.
 * @return Temperatura de saturação em °C.
 */
double thermo_tsat_c(double pressure_kpa);

/**
 * @brief Temperatura de ebulição da solução aquosa de NaOH (inclui EPE).
 *
 * Utiliza correlação modificada do tipo Antoine:
 *   T = -C + B(x) / (log10(P) - A(x))
 * onde A(x) e B(x) são polinômios cúbicos na fração mássica x, e C = 190.80153.
 *
 * @param pressure_kpa Pressão em kPa.
 * @param mass_fraction Fração mássica de NaOH (0 <= x <= 0.60).
 * @return Temperatura de ebulição em °C.
 */
double thermo_boiling_temp_c(double pressure_kpa, double mass_fraction);

/**
 * @brief Elevação do Ponto de Ebulição (EPE) da solução de NaOH.
 * @param pressure_kpa Pressão em kPa.
 * @param mass_fraction Fração mássica de NaOH.
 * @return EPE em °C (T_ebulicao - T_sat_pura).
 */
double thermo_bpe_c(double pressure_kpa, double mass_fraction);

/**
 * @brief Calor latente de vaporização da água via equação de Watson.
 *
 *   ΔHvap = 2256.47 * [ (1 - T_r) / (1 - T_r,eb) ]^0.38 [kJ/kg]
 * com T_c = 647.096 K e T_eb = 373.15 K.
 *
 * @param temp_c Temperatura em °C.
 * @return Calor latente de vaporização em kJ/kg.
 */
double thermo_heat_vaporization_kj_kg(double temp_c);

/**
 * @brief Entalpia do vapor d'água saturado (ajuste empírico 0-300 °C).
 * @param temp_c Temperatura em °C.
 * @return Entalpia do vapor saturado em kJ/kg.
 */
double thermo_enthalpy_sat_vapor_kj_kg(double temp_c);

/**
 * @brief Entalpia do vapor d'água superaquecido a P e T.
 *
 * Integra a correlação de Cp(T) entre T_sat(P) e T, somando à entalpia de vapor saturado:
 *   h_super = Hvsat(T_sat) + \int_{T_sat}^{T} Cp(T') dT'
 *
 * @param temp_c Temperatura do vapor em °C.
 * @param pressure_kpa Pressão de operação em kPa.
 * @return Entalpia em kJ/kg.
 */
double thermo_enthalpy_superheated_kj_kg(double temp_c, double pressure_kpa);

/**
 * @brief Entalpia específica de solução aquosa de NaOH (HL).
 *
 * A correlação original foi ajustada em unidades imperiais (T em °F, H em BTU/lbm)
 * a partir dos dados experimentais compilados no Perry's Chemical Engineers' Handbook.
 * O código converte T (°C) para °F, avalia o polinômio e converte o resultado
 * de BTU/lbm para kJ/kg através do fator 2.326 kJ/kg por BTU/lbm.
 *
 * @param temp_c Temperatura em °C.
 * @param mass_fraction Fração mássica de NaOH (0 a 0.55).
 * @return Entalpia em kJ/kg.
 */
double thermo_enthalpy_solution_kj_kg(double temp_c, double mass_fraction);

#ifdef __cplusplus
}
#endif

#endif /* EVAPORATOR_THERMODYNAMICS_H */

