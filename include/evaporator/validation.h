/**
 * @file validation.h
 * @brief Funções de validação de parâmetros de entrada e viabilidade física.
 */

#ifndef EVAPORATOR_VALIDATION_H
#define EVAPORATOR_VALIDATION_H

#include "evaporator/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Valida os parâmetros de entrada antes de iniciar a simulação.
 * @param input Ponteiro para a estrutura de entradas.
 * @param error Ponteiro para estrutura que receberá detalhes em caso de falha.
 * @return EVAP_OK se válidos, ou código de erro apropriado.
 */
EvapStatus validation_check_input(const SimulationInput *input, SolverError *error);

/**
 * @brief Valida a viabilidade termodinâmica do sistema (gradientes térmicos, EPE).
 * @param n_effects Número de efeitos.
 * @param total_dt Força motriz global disponível [°C].
 * @param t_sat Vetor de temperaturas de saturação [°C].
 * @param epe Vetor de EPE [°C].
 * @param error Ponteiro para estrutura de erro.
 * @return EVAP_OK se viável, EVAP_ERR_INFEASIBLE caso contrário.
 */
EvapStatus validation_check_stability(int n_effects,
                                      double total_dt,
                                      const double *t_sat,
                                      const double *epe,
                                      SolverError *error);

/**
 * @brief Valida os resultados finais do solver (fechamento de balanço, áreas positivas, etc).
 * @param input Parâmetros originais.
 * @param result Resultados calculados.
 * @param error Ponteiro para estrutura de erro.
 * @return EVAP_OK se coerente, código de erro caso contrário.
 */
EvapStatus validation_check_results(const SimulationInput *input,
                                    const SimulationResult *result,
                                    SolverError *error);

#ifdef __cplusplus
}
#endif

#endif /* EVAPORATOR_VALIDATION_H */

