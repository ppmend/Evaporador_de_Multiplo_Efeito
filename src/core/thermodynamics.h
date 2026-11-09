/*
 * thermodynamics.h — Thermodynamic property correlations for the evaporator solver.
 *
 * Universidade Federal de Uberlândia — Faculdade de Engenharia Química
 * COBEQ-IC 2026 (UNICAMP)
 * Authors: Pedro Paulo Nogueira Mendonça, Cláudio Roberto Duarte
 *
 * All correlations are ported verbatim from the original monolithic code.
 * Units, validity ranges, and data sources are documented per function.
 */

#ifndef EVAP_THERMODYNAMICS_H
#define EVAP_THERMODYNAMICS_H

/**
 * Enthalpy of NaOH aqueous solution.
 *
 * Uses a multivariate polynomial fitted to Perry's Chemical Engineers'
 * Handbook data.  The correlation coefficients were fitted in BTU/lbm vs °F,
 * so temperature is first converted °C → °F and the result is converted
 * BTU/lbm → kJ/kg (factor 2.326).
 *
 * @param temp_c     Solution temperature [°C].
 * @param mass_frac  NaOH mass fraction [0–1].
 * @return           Specific enthalpy [kJ/kg].
 *
 * Validity: 0 ≤ x ≤ 0.50, 30 °C ≤ T ≤ 200 °C (approx.).
 * Source:   Perry's Chemical Engineers' Handbook — NaOH–H₂O enthalpy data.
 *           Fit R² = 0.99919.
 */
double thermo_enthalpy_solution_kj_kg(double temp_c, double mass_frac);

/**
 * Latent heat of vaporization of pure water — Watson correlation.
 *
 * Uses the Watson equation referenced to the normal boiling point (100 °C):
 *   ΔHvap = 2256.47 × ((1 − (T+273.15)/647.096) / (1 − 373.15/647.096))^0.38
 *
 * @param temp_c  Saturation temperature [°C].
 * @return        Heat of vaporization [kJ/kg].
 *
 * Validity: 0 °C ≤ T < 374 °C (below critical point).
 * Source:   Watson (1943); reference value at 100 °C = 2256.47 kJ/kg.
 */
double thermo_heat_vaporization_kj_kg(double temp_c);

/**
 * Specific enthalpy of saturated water vapor — polynomial fit.
 *
 * Third-order polynomial fitted to NIST steam tables:
 *   Hvsat = 2500.9 + 1.805·T − 2.1e-4·T² + 3.5e-7·T³
 *
 * @param temp_c  Saturation temperature [°C].
 * @return        Saturated-vapor enthalpy [kJ/kg].
 *
 * Validity: 0 °C ≤ T ≤ 300 °C.
 * Source:   Polynomial fit to steam tables.
 */
double thermo_enthalpy_sat_vapor_kj_kg(double temp_c);

/**
 * Specific enthalpy of superheated steam.
 *
 * Computes the enthalpy increase above the saturated-vapor state by
 * integrating Cp(T) = a + b·T + c·T² from T_sat(P) to T:
 *   h = Hvsat(T_sat) + ∫[T_sat→T] Cp dT
 *
 * Coefficients: a = 1.872, b = 0.00055, c = −1.6e-7  [kJ/(kg·°C)].
 *
 * @param temp_c       Vapor temperature [°C].
 * @param pressure_kpa Pressure at which the vapor exists [kPa].
 * @return             Superheated-steam enthalpy [kJ/kg].
 *
 * Validity: T ≥ T_sat(P), P > 0.
 * Source:   Cp correlation from steam property fit.
 */
double thermo_enthalpy_superheated_kj_kg(double temp_c, double pressure_kpa);

/**
 * Saturation pressure of pure water — Antoine equation.
 *
 * Uses the Antoine equation with two coefficient sets (T ≤ 100 °C and
 * T > 100 °C).  The equation gives P in mmHg; the result is converted
 * to kPa with 1 mmHg = 0.133322 kPa.
 *
 *   log10(P_mmHg) = A − B / (T + C)
 *
 * @param temp_c  Temperature [°C].
 * @return        Saturation pressure [kPa].
 *
 * Validity: 1 °C ≤ T ≤ 374 °C (two-range Antoine).
 * Source:   NIST Antoine data for water.
 */
double thermo_psat_kpa(double temp_c);

/**
 * Saturation temperature of pure water — inverse Antoine equation.
 *
 * Inverts the Antoine equation:
 *   T = B / (A − log10(P_mmHg)) − C
 *
 * Two coefficient sets are selected based on pressure (≤ or > 101.325 kPa).
 *
 * @param pressure_kpa  Pressure [kPa].
 * @return              Saturation temperature [°C].
 *
 * Validity: pressure range corresponding to 1–374 °C.
 * Source:   NIST Antoine data for water.
 */
double thermo_tsat_c(double pressure_kpa);

/**
 * Boiling temperature of NaOH aqueous solution at a given pressure.
 *
 * Uses a modified Antoine-type correlation with composition-dependent
 * coefficients A(x), B(x) and constant C:
 *   T = −C + B(x) / (log10(P_kPa) − A(x))
 *
 * where:
 *   A(x) = 3.62321·x³ + 1.13082·x² − 0.71947·x + 6.77113
 *   B(x) = −2711.78458·x³ − 11.18678·x² + 47.44337·x − 1395.64615
 *   C    = 190.80153
 *
 * When x = 0 this reduces to a pure-water Antoine variant and returns
 * values close to thermo_tsat_c(P).
 *
 * @param pressure_kpa  Pressure [kPa].
 * @param mass_frac     NaOH mass fraction [0–1].
 * @return              Boiling temperature [°C].
 *
 * Validity: 0 ≤ x ≤ 0.50, P > 0.
 * Source:   Modified Antoine fit for NaOH–H₂O solutions.
 */
double thermo_boiling_temp_c(double pressure_kpa, double mass_frac);

#endif /* EVAP_THERMODYNAMICS_H */
