/**
 * @file solver.h
 * @brief Interface pública do solver numérico de evaporadores de múltiplo efeito.
 */

#ifndef EVAPORATOR_SOLVER_H
#define EVAPORATOR_SOLVER_H

#include "evaporator/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Executa a simulação completa de dimensionamento do sistema de evaporadores.
 *
 * O algoritmo implementa um procedimento iterativo em dois níveis:
 * 1. Laço interno: Resolve os balanços acoplados de massa e energia via Gauss-Seidel
 *    com sub-relaxação SOR (Successive Over-Relaxation).
 * 2. Laço externo: Equaliza as áreas de troca térmica através da redistribuição
 *    proporcional dos gradientes de temperatura (dT_i).
 *
 * @param input Parâmetros de entrada da simulação (vazão, concentrações, temperaturas, etc).
 * @param result Ponteiro para a estrutura onde serão gravados os resultados.
 * @param error Ponteiro para estrutura que registrará mensagem de erro em caso de falha.
 * @return EVAP_OK em caso de sucesso e convergência; código de erro correspondente caso contrário.
 */
EvapStatus solver_run(const SimulationInput *input,
                      SimulationResult *result,
                      SolverError *error);

#ifdef __cplusplus
}
#endif

#endif /* EVAPORATOR_SOLVER_H */

