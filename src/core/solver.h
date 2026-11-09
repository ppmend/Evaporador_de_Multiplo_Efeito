/*
 * solver.h — Public API for the multi-effect evaporator solver.
 *
 * Universidade Federal de Uberlândia — Faculdade de Engenharia Química
 * COBEQ-IC 2026 (UNICAMP)
 * Authors: Pedro Paulo Nogueira Mendonça, Cláudio Roberto Duarte
 */

#ifndef EVAP_SOLVER_H
#define EVAP_SOLVER_H

#include "types.h"

/**
 * Run the multi-effect evaporator simulation.
 *
 * This is the main entry point. It:
 *   1. Validates the input parameters.
 *   2. Initialises the internal state (temperatures, pressures, flows).
 *   3. Runs the outer loop (area equalisation with ΔT correction).
 *      Inside each outer iteration it runs the inner Gauss-Seidel SOR
 *      loop (mass and energy balances).
 *   4. Fills the result structure with per-effect data.
 *   5. Performs physical feasibility checks on the converged solution.
 *
 * The solver uses NO global state; everything is passed via the structs.
 *
 * @param input   Operating conditions and solver parameters.
 * @param result  Filled on success with per-effect results.
 * @param error   Filled on failure with error code and descriptive message.
 * @return        EVAP_OK on success; an EvapStatus error code otherwise.
 */
EvapStatus solver_run(const SimulationInput *input,
                      SimulationResult *result,
                      SolverError *error);

/**
 * Return a human-readable string for an EvapStatus code.
 */
const char *evap_status_string(EvapStatus status);

#endif /* EVAP_SOLVER_H */
