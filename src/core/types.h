/*
 * types.h — Common types and structures for the multi-effect evaporator solver.
 *
 * Universidade Federal de Uberlândia — Faculdade de Engenharia Química
 * COBEQ-IC 2026 (UNICAMP)
 * Authors: Pedro Paulo Nogueira Mendonça, Cláudio Roberto Duarte
 *
 * This header defines the data structures used throughout the solver:
 *   - SimulationInput:  user-specified operating conditions and solver parameters
 *   - SimulationResult: per-effect results and convergence diagnostics
 *   - SolverError:      error status and human-readable message
 *   - EvapStatus:       enumerated error/success codes
 */

#ifndef EVAP_TYPES_H
#define EVAP_TYPES_H

#include <stdbool.h>

/* Maximum number of evaporator effects supported. */
#define EVAP_MAX_EFFECTS 20

/* Default solver tolerances and iteration limits. */
#define EVAP_DEFAULT_TOL 1e-10
#define EVAP_DEFAULT_MAX_ITER_OUTER 5000
#define EVAP_DEFAULT_MAX_ITER_INNER 100000

/* ---------------------------------------------------------------------------
 * EvapStatus — Return codes for all public API functions.
 * ---------------------------------------------------------------------------*/
typedef enum {
    EVAP_OK = 0,              /* Success                                     */
    EVAP_ERR_INVALID_INPUT,   /* One or more input parameters out of range   */
    EVAP_ERR_INFEASIBLE,      /* Physically infeasible configuration         */
    EVAP_ERR_NOT_CONVERGED,   /* Solver did not converge within iteration limit */
    EVAP_ERR_NUMERICAL,       /* NaN, Inf, or division-by-zero detected      */
    EVAP_ERR_ALLOC            /* Memory allocation failure                   */
} EvapStatus;

/* ---------------------------------------------------------------------------
 * SimulationInput — All inputs needed to define and solve the evaporator system.
 * ---------------------------------------------------------------------------*/
typedef struct {
    int n_effects;           /* Number of effects (2–20)                      */
    double feed_rate_kg_h;   /* F  — Feed mass flow rate [kg/h]               */
    double feed_conc;        /* xF — Feed NaOH mass fraction [0–1]            */
    double product_conc;     /* xN — Product NaOH mass fraction [0–1]         */
    double feed_temp_c;      /* TF — Feed temperature [°C]                    */
    double steam_temp_c;     /* TS — Live steam (saturated) temperature [°C]  */
    double last_sat_temp_c;  /* T_sat of last effect (condenser side) [°C]    */
    double htc_w_m2k;        /* U  — Overall heat-transfer coefficient
                                     [W/(m²·K)], assumed equal for all effects */
    double omega;            /* SOR relaxation factor (0 < ω < 1)             */
    double tol_area;         /* Tolerance for area-equality (outer loop)       */
    double tol_system;       /* Tolerance for mass/energy balance (inner loop) */
    int max_iter_outer;      /* Maximum outer iterations (area equalization)   */
    int max_iter_inner;      /* Maximum inner iterations (Gauss-Seidel SOR)   */
} SimulationInput;

/* ---------------------------------------------------------------------------
 * SimulationResult — Complete per-effect results and convergence diagnostics.
 * ---------------------------------------------------------------------------*/
typedef struct {
    int n_effects;
    double steam_rate_kg_h;                       /* S   — Live steam consumption [kg/h]       */
    double vapor_economy;                         /* E   = (F − L_N) / S                      */
    double product_rate_kg_h;                     /* L_N — Product flow rate [kg/h]            */
    double vapor_rate_kg_h[EVAP_MAX_EFFECTS];     /* V_i   [kg/h]                             */
    double liquid_rate_kg_h[EVAP_MAX_EFFECTS];    /* L_i   [kg/h]                             */
    double concentration[EVAP_MAX_EFFECTS];       /* x_i   (mass fraction)                    */
    double temperature_c[EVAP_MAX_EFFECTS];       /* T_i   [°C]                               */
    double sat_temperature_c[EVAP_MAX_EFFECTS];   /* T_sat_i [°C]                             */
    double pressure_kpa[EVAP_MAX_EFFECTS];        /* P_i   [kPa]                              */
    double bpe_c[EVAP_MAX_EFFECTS];               /* EPE_i [°C]                               */
    double delta_t_c[EVAP_MAX_EFFECTS];           /* ΔT_i  [°C]                               */
    double area_m2[EVAP_MAX_EFFECTS];             /* A_i   [m²]                               */
    double heat_duty_kw[EVAP_MAX_EFFECTS];        /* Q_i   [kW]                               */
    /* Convergence diagnostics */
    int iter_outer;                               /* Outer loop iterations used                */
    int iter_inner_total;                         /* Total inner loop iterations               */
    double residual_area;                         /* Final area residual                       */
    double residual_system;                       /* Final system residual                     */
    double mass_balance_error;                    /* Relative mass-balance closure error       */
    double energy_balance_error;                  /* Relative energy-balance closure error     */
} SimulationResult;

/* ---------------------------------------------------------------------------
 * SolverError — Structured error information returned by the solver.
 * ---------------------------------------------------------------------------*/
typedef struct {
    EvapStatus status;
    char message[512];
} SolverError;

/* ---------------------------------------------------------------------------
 * evap_default_input() — Returns a SimulationInput pre-filled with the
 *   reference case from the COBEQ-IC 2026 article.
 * ---------------------------------------------------------------------------*/
static inline SimulationInput evap_default_input(void) {
    SimulationInput input = {0};
    input.n_effects       = 3;
    input.feed_rate_kg_h  = 10000.0;
    input.feed_conc       = 0.10;
    input.product_conc    = 0.50;
    input.feed_temp_c     = 50.0;
    input.steam_temp_c    = 160.0;
    input.last_sat_temp_c = 60.0;
    input.htc_w_m2k       = 2000.0;
    input.omega           = 0.5;
    input.tol_area        = EVAP_DEFAULT_TOL;
    input.tol_system      = EVAP_DEFAULT_TOL;
    input.max_iter_outer  = EVAP_DEFAULT_MAX_ITER_OUTER;
    input.max_iter_inner  = EVAP_DEFAULT_MAX_ITER_INNER;
    return input;
}

#endif /* EVAP_TYPES_H */
