/**
 * @file main.c
 * @brief Interface de Linha de Comando (CLI) para simulação de evaporadores de múltiplo efeito.
 *
 * Pedro Paulo Nogueira Mendonça e Cláudio Roberto Duarte (UFU/COBEQ-IC 2026).
 */

#include "evaporator/solver.h"
#include "evaporator/thermodynamics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void print_banner(void) {
    printf("================================================================================\n");
    printf("  SIMULADOR DE EVAPORADORES DE MULTIPLO EFEITO COM PRESSAO VARIAVEL E EPE       \n");
    printf("  Pedro Paulo Nogueira Mendonca & Claudio Roberto Duarte (UFU - COBEQ-IC 2026) \n");
    printf("================================================================================\n\n");
}

static void print_help(const char *prog_name) {
    print_banner();
    printf("Uso: %s [opcoes]\n\n", prog_name);
    printf("Opcoes de Simulacao:\n");
    printf("  -n, --effects <int>      Numero de efeitos (2 a 7 recomendados, padrao: 3)\n");
    printf("  -F, --feed <double>      Vazao de alimentacao [kg/h] (padrao: 10000.0)\n");
    printf("      --xf <double>        Concentracao de NaOH na alimentacao [0-1] (padrao: 0.10)\n");
    printf("      --xn <double>        Concentracao de NaOH no produto [0-1] (padrao: 0.50)\n");
    printf("      --tf <double>        Temperatura da alimentacao [°C] (padrao: 50.0)\n");
    printf("      --ts <double>        Temperatura do vapor vivo [°C] (padrao: 160.0)\n");
    printf("      --tsat-last <double> Temperatura de saturacao do ultimo efeito [°C] (padrao: 60.0)\n");
    printf("  -U, --htc <double>       Coeficiente global de troca termica [W/(m²·K)] (padrao: 2000.0)\n");
    printf("  -w, --omega <double>     Fator de sub-relaxacao SOR (0 < omega < 1) (padrao: 0.496)\n");
    printf("      --tol <double>       Tolerancia de convergencia (padrao: 1e-10)\n");
    printf("\nOpcoes de Saida e Execucao:\n");
    printf("      --json               Formatar saida em JSON estruturado\n");
    printf("      --csv                Formatar tabela de efeitos em CSV\n");
    printf("      --benchmark          Executar reproducao completa da Tabela 2 do artigo (N=2 a 7)\n");
    printf("      --sweep-omega        Realizar varredura do parametro omega para o N selecionado\n");
    printf("  -h, --help               Exibir esta mensagem de ajuda e sair\n\n");
}

static void print_text_results(const SimulationInput *in, const SimulationResult *res) {
    printf("--------------------------------------------------------------------------------\n");
    printf("RESUMO GLOBAL DA SIMULACAO (N = %d efeitos)\n", res->n_effects);
    printf("--------------------------------------------------------------------------------\n");
    printf("  Consumo de vapor vivo (S)      : %10.2f kg/h\n", res->steam_rate_kg_h);
    printf("  Economia de vapor (E)          : %10.4f (kg evaporado / kg vapor vivo)\n", res->vapor_economy);
    printf("  Vazao de concentrado final (LN): %10.2f kg/h\n", res->product_rate_kg_h);
    printf("  Area de troca termica por efeito: %10.3f m²\n", res->area_per_effect_m2);
    printf("  Area total instalada           : %10.3f m² (N x %.3f m²)\n", res->total_area_m2, res->area_per_effect_m2);
    printf("  Delta T util total             : %10.2f °C\n", res->total_delta_t_c);
    printf("  Soma das EPEs                  : %10.2f °C\n", res->total_bpe_c);
    printf("  Iteracoes do laco de area      : %10d\n", res->iter_outer);
    printf("  Total de iteracoes SOR internas: %10d\n", res->iter_inner_total);
    printf("  Discrepancia residual de areas : %10.3e m²\n", res->max_area_discrepancy_m2);
    printf("  Erro no balanco de massa global: %10.3e kg/h\n", res->mass_balance_error_kg_h);
    printf("\n");

    printf("------------------------------------------------------------------------------------------------------\n");
    printf(" PERFIS DETALHADOS POR EFEITO                                                                        \n");
    printf("------------------------------------------------------------------------------------------------------\n");
    printf(" Ef |  V (kg/h)  |  L (kg/h)  | x (%% m/m) | T (°C) | Tsat (°C)|  P (kPa)  | EPE (°C)| dT (°C) | Area (m²)\n");
    printf("----+------------+------------+-----------+--------+----------+-----------+---------+---------+----------\n");

    for (int i = 0; i < res->n_effects; i++) {
        printf(" %2d | %10.2f | %10.2f |   %6.2f  | %6.2f |  %6.2f  | %9.3f |  %6.2f |  %6.2f | %9.3f\n",
               i + 1,
               res->vapor_rate_kg_h[i],
               res->liquid_rate_kg_h[i],
               res->concentration[i] * 100.0,
               res->temperature_c[i],
               res->sat_temperature_c[i],
               res->pressure_kpa[i],
               res->bpe_c[i],
               res->delta_t_c[i],
               res->area_m2[i]);
    }
    printf("------------------------------------------------------------------------------------------------------\n\n");
}

static void print_json_results(const SimulationInput *in, const SimulationResult *res) {
    printf("{\n");
    printf("  \"input\": {\n");
    printf("    \"n_effects\": %d,\n", in->n_effects);
    printf("    \"feed_rate_kg_h\": %.2f,\n", in->feed_rate_kg_h);
    printf("    \"feed_conc\": %.4f,\n", in->feed_conc);
    printf("    \"product_conc\": %.4f,\n", in->product_conc);
    printf("    \"feed_temp_c\": %.2f,\n", in->feed_temp_c);
    printf("    \"steam_temp_c\": %.2f,\n", in->steam_temp_c);
    printf("    \"last_sat_temp_c\": %.2f,\n", in->last_sat_temp_c);
    printf("    \"htc_w_m2k\": %.2f,\n", in->htc_w_m2k);
    printf("    \"omega\": %.4f\n", in->omega);
    printf("  },\n");
    printf("  \"summary\": {\n");
    printf("    \"steam_rate_kg_h\": %.4f,\n", res->steam_rate_kg_h);
    printf("    \"vapor_economy\": %.4f,\n", res->vapor_economy);
    printf("    \"product_rate_kg_h\": %.4f,\n", res->product_rate_kg_h);
    printf("    \"area_per_effect_m2\": %.4f,\n", res->area_per_effect_m2);
    printf("    \"total_area_m2\": %.4f,\n", res->total_area_m2);
    printf("    \"total_delta_t_c\": %.4f,\n", res->total_delta_t_c);
    printf("    \"total_bpe_c\": %.4f,\n", res->total_bpe_c);
    printf("    \"iter_outer\": %d,\n", res->iter_outer);
    printf("    \"iter_inner_total\": %d,\n", res->iter_inner_total);
    printf("    \"residual_area\": %.6e\n", res->residual_area);
    printf("  },\n");
    printf("  \"effects\": [\n");
    for (int i = 0; i < res->n_effects; i++) {
        printf("    {\n");
        printf("      \"effect\": %d,\n", i + 1);
        printf("      \"vapor_rate_kg_h\": %.4f,\n", res->vapor_rate_kg_h[i]);
        printf("      \"liquid_rate_kg_h\": %.4f,\n", res->liquid_rate_kg_h[i]);
        printf("      \"concentration\": %.4f,\n", res->concentration[i]);
        printf("      \"temperature_c\": %.4f,\n", res->temperature_c[i]);
        printf("      \"sat_temperature_c\": %.4f,\n", res->sat_temperature_c[i]);
        printf("      \"pressure_kpa\": %.4f,\n", res->pressure_kpa[i]);
        printf("      \"bpe_c\": %.4f,\n", res->bpe_c[i]);
        printf("      \"delta_t_c\": %.4f,\n", res->delta_t_c[i]);
        printf("      \"area_m2\": %.4f,\n", res->area_m2[i]);
        printf("      \"heat_duty_kw\": %.4f\n", res->heat_duty_kw[i]);
        printf("    }%s\n", (i == res->n_effects - 1) ? "" : ",");
    }
    printf("  ]\n");
    printf("}\n");
}

static void print_csv_results(const SimulationResult *res) {
    printf("effect,vapor_rate_kg_h,liquid_rate_kg_h,concentration,temperature_c,sat_temperature_c,pressure_kpa,bpe_c,delta_t_c,area_m2,heat_duty_kw\n");
    for (int i = 0; i < res->n_effects; i++) {
        printf("%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f\n",
               i + 1,
               res->vapor_rate_kg_h[i],
               res->liquid_rate_kg_h[i],
               res->concentration[i],
               res->temperature_c[i],
               res->sat_temperature_c[i],
               res->pressure_kpa[i],
               res->bpe_c[i],
               res->delta_t_c[i],
               res->area_m2[i],
               res->heat_duty_kw[i]);
    }
}

static void run_benchmark(void) {
    print_banner();
    printf("REPRODUCAO EXATA DA TABELA 2 DO ARTIGO (COBEQ-IC 2026)\n");
    printf("Parametros: F = 10000 kg/h, xF = 10%%, xN = 50%%, TF = 50°C, TS = 160°C, Tsat,N = 60°C, U = 2000 W/(m²K)\n\n");

    printf("| N | Area Calc (m²) | Area Artigo | S Calc (kg/h) | S Artigo | Economia | Econ Artigo | omega | ItArea | ItSOR |\n");
    printf("|---|----------------|-------------|---------------|----------|----------|-------------|-------|--------|-------|\n");

    const double omega_ref[8] = {0, 0, 0.665, 0.496, 0.445, 0.429, 0.377, 0.345};
    const double area_artigo[8] = {0, 0, 58.083, 70.128, 89.608, 125.442, 211.225, 684.210};
    const double s_artigo[8] = {0, 0, 5940.4, 4593.6, 3949.4, 3585.8, 3363.0, 3221.2};
    const double econ_artigo[8] = {0, 0, 1.3467, 1.7415, 2.0256, 2.2310, 2.3788, 2.4836};

    for (int n = 2; n <= 7; n++) {
        SimulationInput in = evap_default_input();
        in.n_effects = n;
        in.omega = omega_ref[n];

        SimulationResult res;
        SolverError err;
        EvapStatus st = solver_run(&in, &res, &err);

        if (st == EVAP_OK) {
            printf("| %d | %14.3f | %11.3f | %13.1f | %8.1f | %8.4f | %11.4f | %5.3f | %6d | %5d |\n",
                   n,
                   res.area_per_effect_m2,
                   area_artigo[n],
                   res.steam_rate_kg_h,
                   s_artigo[n],
                   res.vapor_economy,
                   econ_artigo[n],
                   in.omega,
                   res.iter_outer,
                   res.iter_inner_total);
        } else {
            printf("| %d | FALHA: %s\n", n, err.message);
        }
    }
    printf("\nNota: Area Calc e por efeito individual, igual em todos os N efeitos.\n\n");
}

static void run_sweep_omega(int n_effects) {
    print_banner();
    printf("VARREDURA DO PARAMETRO DE RELAXACAO SOR (omega) PARA N = %d EFEITOS\n\n", n_effects);
    printf("omega    | Status | ItArea | ItSOR  | S (kg/h) | Area (m²)\n");
    printf("---------+--------+--------+--------+----------+----------\n");

    for (int i = 5; i <= 95; i += 5) {
        double w = (double)i / 100.0;
        SimulationInput in = evap_default_input();
        in.n_effects = n_effects;
        in.omega = w;

        SimulationResult res;
        SolverError err;
        EvapStatus st = solver_run(&in, &res, &err);

        if (st == EVAP_OK) {
            printf("  %5.3f  | CONV   | %6d | %6d | %8.1f | %8.3f\n",
                   w, res.iter_outer, res.iter_inner_total, res.steam_rate_kg_h, res.area_per_effect_m2);
        } else {
            printf("  %5.3f  | DIVERG |      - |      - |        - |        -\n", w);
        }
    }
    printf("\n");
}

int main(int argc, char **argv) {
    SimulationInput input = evap_default_input();
    bool output_json = false;
    bool output_csv = false;
    bool mode_benchmark = false;
    bool mode_sweep = false;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--benchmark") == 0) {
            mode_benchmark = true;
        } else if (strcmp(argv[i], "--sweep-omega") == 0) {
            mode_sweep = true;
        } else if (strcmp(argv[i], "--json") == 0) {
            output_json = true;
        } else if (strcmp(argv[i], "--csv") == 0) {
            output_csv = true;
        } else if ((strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--effects") == 0) && i + 1 < argc) {
            input.n_effects = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-F") == 0 || strcmp(argv[i], "--feed") == 0) && i + 1 < argc) {
            input.feed_rate_kg_h = atof(argv[++i]);
        } else if (strcmp(argv[i], "--xf") == 0 && i + 1 < argc) {
            input.feed_conc = atof(argv[++i]);
        } else if (strcmp(argv[i], "--xn") == 0 && i + 1 < argc) {
            input.product_conc = atof(argv[++i]);
        } else if (strcmp(argv[i], "--tf") == 0 && i + 1 < argc) {
            input.feed_temp_c = atof(argv[++i]);
        } else if (strcmp(argv[i], "--ts") == 0 && i + 1 < argc) {
            input.steam_temp_c = atof(argv[++i]);
        } else if (strcmp(argv[i], "--tsat-last") == 0 && i + 1 < argc) {
            input.last_sat_temp_c = atof(argv[++i]);
        } else if ((strcmp(argv[i], "-U") == 0 || strcmp(argv[i], "--htc") == 0) && i + 1 < argc) {
            input.htc_w_m2k = atof(argv[++i]);
        } else if ((strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--omega") == 0) && i + 1 < argc) {
            input.omega = atof(argv[++i]);
        } else if (strcmp(argv[i], "--tol") == 0 && i + 1 < argc) {
            double tol = atof(argv[++i]);
            input.tol_area = tol;
            input.tol_system = tol;
        } else {
            fprintf(stderr, "Opcao desconhecida: %s\n", argv[i]);
            fprintf(stderr, "Execute '%s --help' para ver a lista de opcoes.\n", argv[0]);
            return (int)EVAP_ERR_INVALID_INPUT;
        }
    }

    if (mode_benchmark) {
        run_benchmark();
        return 0;
    }

    if (mode_sweep) {
        run_sweep_omega(input.n_effects);
        return 0;
    }

    SimulationResult result;
    SolverError error;
    EvapStatus status = solver_run(&input, &result, &error);

    if (status != EVAP_OK) {
        if (output_json) {
            printf("{\n  \"status\": %d,\n  \"error\": \"%s\",\n  \"effect_index\": %d\n}\n",
                   status, error.message, error.effect_index);
        } else {
            fprintf(stderr, "\n[ERRO DE SIMULACAO]: %s\n\n", error.message);
        }
        return (int)status;
    }

    if (output_json) {
        print_json_results(&input, &result);
    } else if (output_csv) {
        print_csv_results(&result);
    } else {
        print_banner();
        print_text_results(&input, &result);
    }

    return (int)EVAP_OK;
}

