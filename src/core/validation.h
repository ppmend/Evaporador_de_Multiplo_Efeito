/*
 * validation.h — Input validation and physical feasibility checks.
 *
 * Universidade Federal de Uberlândia — Faculdade de Engenharia Química
 * COBEQ-IC 2026 (UNICAMP)
 * Authors: Pedro Paulo Nogueira Mendonça, Cláudio Roberto Duarte
 */

#ifndef EVAP_VALIDATION_H
#define EVAP_VALIDATION_H

#include "types.h"

/**
 * Validate all fields of a SimulationInput structure.
 *
 * Checks:
 *  - n_effects in [2, EVAP_MAX_EFFECTS]
 *  - Positive feed rate
 *  - 0 < feed_conc < product_conc ≤ 1
 *  - feed_temp_c > 0
 *  - steam_temp_c > last_sat_temp_c > 0
 *  - htc_w_m2k > 0
 *  - 0 < omega < 1  (under-relaxation for Gauss-Seidel SOR)
 *  - Positive tolerances
 *  - Positive iteration limits
 *
 * @param input  Pointer to the input to validate.
 * @param error  On failure, filled with status code and message.
 * @return       EVAP_OK on success, EVAP_ERR_INVALID_INPUT on failure.
 */
EvapStatus validate_input(const SimulationInput *input, SolverError *error);

/**
 * Check physical feasibility of a converged simulation result.
 *
 * Checks:
 *  - All vapor and liquid flow rates are positive
 *  - All concentrations are in (0, 1]
 *  - All pressures are positive
 *  - All ΔT values are positive (driving force exists)
 *  - All areas are positive and finite
 *  - Temperatures are within correlation validity ranges
 *
 * @param result  Pointer to the simulation result.
 * @param n       Number of effects.
 * @param error   On failure, filled with status code and message.
 * @return        EVAP_OK on success, EVAP_ERR_INFEASIBLE on failure.
 */
EvapStatus validate_feasibility(const SimulationResult *result, int n, SolverError *error);

#endif /* EVAP_VALIDATION_H */
