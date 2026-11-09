/**
 * @file types.h
 * @brief Tipos e estruturas centrais do dimensionamento de evaporadores de múltiplo efeito.
 *
 * Projeto baseado no trabalho de Pedro Paulo Nogueira Mendonça e Cláudio Roberto Duarte (UFU/COBEQ-IC 2026).
 */

#ifndef EVAPORATOR_TYPES_H
#define EVAPORATOR_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Número máximo de efeitos suportados */
#define EVAP_MAX_EFFECTS 20

/** Número mínimo de efeitos */
#define EVAP_MIN_EFFECTS 2

/** Tolerância numérica padrão para convergência */
#define EVAP_DEFAULT_TOL 1e-10

/** Limite máximo de iterações do laço externo (igualação de áreas) */
#define EVAP_DEFAULT_MAX_ITER_OUTER 5000

/** Limite máximo de iterações do laço interno (Gauss-Seidel SOR) */
#define EVAP_DEFAULT_MAX_ITER_INNER 100000

/**
 * @brief Códigos de status retornados pelo solver.
 */
typedef enum {
    EVAP_OK = 0,                   /**< Sucesso, convergência atingida */
    EVAP_ERR_INVALID_INPUT = 1,    /**< Parâmetros de entrada inválidos */
    EVAP_ERR_INFEASIBLE = 2,       /**< Sistema termicamente inviável (ex.: ΔT <= 0) */
    EVAP_ERR_NOT_CONVERGED = 3,    /**< Não convergiu no número máximo de iterações */
    EVAP_ERR_NUMERICAL = 4,        /**< Instabilidade numérica (NaN, Inf, divisão por zero) */
    EVAP_ERR_ALLOC = 5             /**< Falha de alocação de memória */
} EvapStatus;

/**
 * @brief Parâmetros de entrada para a simulação do evaporador.
 */
typedef struct {
    int n_effects;                 /**< Número de efeitos (2 a 20; artigo estudou 2 a 7) */
    double feed_rate_kg_h;         /**< F: Vazão mássica de alimentação [kg/h] */
    double feed_conc;              /**< xF: Concentração mássica de NaOH na alimentação [0 a 1] */
    double product_conc;           /**< xN: Concentração mássica de NaOH no produto [0 a 1] */
    double feed_temp_c;            /**< TF: Temperatura da alimentação [°C] */
    double steam_temp_c;           /**< TS: Temperatura do vapor vivo [°C] */
    double last_sat_temp_c;        /**< T_sat do último efeito (condensador) [°C] */
    double htc_w_m2k;              /**< U: Coeficiente global de transferência de calor [W/(m²·K)] */
    double omega;                  /**< ω: Fator de sub-relaxação SOR (0 < ω < 1) */
    double tol_area;               /**< Tolerância para igualdade de áreas (laço externo) */
    double tol_system;             /**< Tolerância para balanços (laço interno) */
    int max_iter_outer;            /**< Limite de iterações externas */
    int max_iter_inner;            /**< Limite de iterações internas por ciclo externo */
} SimulationInput;

/**
 * @brief Resultados do dimensionamento calculados pelo solver.
 */
typedef struct {
    int n_effects;                                 /**< Número de efeitos resolvidos */
    double steam_rate_kg_h;                         /**< S: Consumo de vapor vivo [kg/h] */
    double steam_temp_c;                            /**< TS: Temperatura do vapor vivo [°C] */
    double vapor_economy;                           /**< Economia de vapor: (F - L_N) / S */
    double product_rate_kg_h;                       /**< L_N: Vazão de concentrado final [kg/h] */
    double total_area_m2;                           /**< Área total instalada: sum(A_i) [m²] */
    double area_per_effect_m2;                      /**< Área de troca térmica por efeito [m²] */
    double total_delta_t_c;                         /**< ΔT total útil do sistema [°C] */
    double total_bpe_c;                             /**< Soma das EPEs [°C] */
    
    /* Perfis por efeito (índice 0 = efeito 1, ..., n-1 = efeito n) */
    double vapor_rate_kg_h[EVAP_MAX_EFFECTS];       /**< V_i: Vazão de vapor gerado [kg/h] */
    double liquid_rate_kg_h[EVAP_MAX_EFFECTS];      /**< L_i: Vazão de líquido saindo [kg/h] */
    double concentration[EVAP_MAX_EFFECTS];          /**< x_i: Concentração mássica de NaOH [0 a 1] */
    double temperature_c[EVAP_MAX_EFFECTS];          /**< T_i: Temperatura de ebulição da solução [°C] */
    double sat_temperature_c[EVAP_MAX_EFFECTS];      /**< T_sat,i: Temperatura de saturação da água [°C] */
    double pressure_kpa[EVAP_MAX_EFFECTS];           /**< P_i: Pressão de operação [kPa] */
    double bpe_c[EVAP_MAX_EFFECTS];                  /**< EPE_i: Elevação do ponto de ebulição [°C] */
    double delta_t_c[EVAP_MAX_EFFECTS];              /**< ΔT_i: Diferença de temperatura motriz [°C] */
    double area_m2[EVAP_MAX_EFFECTS];                /**< A_i: Área calculada por efeito [m²] */
    double heat_duty_kw[EVAP_MAX_EFFECTS];           /**< Q_i: Carga térmica por efeito [kW] */

    /* Diagnóstico de convergência */
    int iter_outer;                                 /**< Número de iterações do laço externo */
    int iter_inner_total;                           /**< Soma de iterações internas executadas */
    double residual_area;                           /**< Resíduo final da igualdade de áreas [m²] */
    double residual_system;                         /**< Resíduo final do sistema interno */
    double max_area_discrepancy_m2;                 /**< max(|A_i - A_j|) final */
    double mass_balance_error_kg_h;                 /**< Erro de fechamento do balanço de massa global */
} SimulationResult;

/**
 * @brief Estrutura para relatório de erros detalhados.
 */
typedef struct {
    EvapStatus status;
    char message[512];
    int effect_index;                               /**< Índice do efeito problemático (se aplicável, 1-based, 0 se geral) */
} SolverError;

/**
 * @brief Retorna uma configuração padrão baseada no caso do artigo COBEQ-IC 2026.
 */
SimulationInput evap_default_input(void);

/**
 * @brief Converte código de status em string legível.
 */
const char* evap_status_string(EvapStatus status);

#ifdef __cplusplus
}
#endif

#endif /* EVAPORATOR_TYPES_H */
