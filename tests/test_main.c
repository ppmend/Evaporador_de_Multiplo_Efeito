/**
 * @file test_main.c
 * @brief Executor principal do framework de testes automatizados.
 */

#include <stdio.h>
#include <stdlib.h>

extern int run_thermodynamic_tests(void);
extern int run_validation_tests(void);
extern int run_solver_regression_tests(void);

int main(void) {
    printf("================================================================================\n");
    printf("  SUITE DE TESTES AUTOMATIZADOS - SOLVER DE EVAPORADORES MULTIPLO EFEITO       \n");
    printf("================================================================================\n\n");

    int failures = 0;

    printf("[1/3] Executando testes termodinamicos...\n");
    failures += run_thermodynamic_tests();

    printf("\n[2/3] Executando testes de validacao e robustez...\n");
    failures += run_validation_tests();

    printf("\n[3/3] Executando testes de regressao com a Tabela 2 do artigo (N=2 a 7)...\n");
    failures += run_solver_regression_tests();

    printf("\n================================================================================\n");
    if (failures == 0) {
        printf("  RESULTADO: TODOS OS TESTES PASSARAM COM SUCESSO!\n");
    } else {
        printf("  RESULTADO: %d TESTE(S) FALHARAM.\n", failures);
    }
    printf("================================================================================\n");

    return (failures == 0) ? 0 : 1;
}

