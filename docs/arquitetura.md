# Arquitetura de Software e Decisões de Engenharia

**Projeto:** Dimensionamento de Evaporadores de Múltiplo Efeito  
**Linguagem do Núcleo:** C (padrão C11)  
**Alvos:** Biblioteca estática nativa (`libevap_core.a`), Linha de comando (`evap_cli`), Testes (`evap_tests`), WebAssembly (`evap_solver.wasm`)  
**Front-end:** HTML5 semântico, CSS3 moderno, Vanilla JavaScript  

---

## 1. Visão Geral da Arquitetura

O projeto foi transformado de um arquivo monolítico acadêmico de 509 linhas (`EvaporadoresMultiplos.c`) em uma arquitetura modularizada, testável e sem vazamento de estado global.

```text
                               +-----------------------------+
                               |     Interfaces de Usuário   |
                               |  (CLI Terminal / Web HTML5) |
                               +--------------+--------------+
                                              |
                   +--------------------------+--------------------------+
                   |                                                     |
                   v (Chamada direta em C)                               v (Chamada JavaScript)
        +---------------------+                               +----------------------+
        |      src/cli/       |                               |      web/js/         |
        |       main.c        |                               |   wasm-bridge.js     |
        +----------+----------+                               +----------+-----------+
                   |                                                     |
                   |                                                     v (Funções C exportadas)
                   |                                          +----------------------+
                   |                                          |      src/wasm/       |
                   |                                          |      wasm_api.c      |
                   |                                          +----------+-----------+
                   |                                                     |
                   +--------------------------+--------------------------+
                                              |
                                              v
                              +-------------------------------+
                              |    NÚCLEO NUMÉRICO (evap_core) |
                              +-------------------------------+
                              |  src/core/solver.c            |
                              |  src/core/thermodynamics.c    |
                              |  src/core/validation.c        |
                              |  src/core/types.c             |
                              +-------------------------------+
```

---

## 2. Princípios de Projeto

### 2.1. Isolamento Estrito de Estado (Sem Variáveis Globais)
O código original utilizava `params *par` como variável global compartilhada entre todas as funções. Na nova arquitetura:
- Todas as entradas são encapsuladas na estrutura imutável `SimulationInput`.
- Os resultados são gravados na estrutura previsível `SimulationResult`.
- Erros são retornados por valor através da enumeração `EvapStatus` e preenchidos em `SolverError`.
- É possível executar múltiplas simulações concorrentes ou sucessivas no mesmo processo sem poluição de memória.

### 2.2. Separação de Responsabilidades
- `include/evaporator/types.h`: Definição de contratos de dados, constantes e enumerações de erro.
- `src/core/thermodynamics.c`: Modelos físicos de propriedades puras e de misturas (água, vapor e solução de NaOH).
- `src/core/validation.c`: Guardas de domínio físico, faixas operacionais e estabilidade termodinâmica.
- `src/core/solver.c`: Algoritmo iterativo acoplado em dois níveis (Gauss-Seidel SOR e equalização de áreas).
- `src/cli/main.c`: Análise de argumentos da linha de comando, modos benchmark/sweep e formatação textual/JSON/CSV.
- `src/wasm/wasm_api.c`: Camada C com anotações `EMSCRIPTEN_KEEPALIVE` para exportação segura de símbolos.

---

## 3. Contratos de API Pública

### Execução da Simulação
```c
EvapStatus solver_run(const SimulationInput *input,
                      SimulationResult *result,
                      SolverError *error);
```

### Inicialização e Utilidades
```c
SimulationInput evap_default_input(void);
const char* evap_status_string(EvapStatus status);
```

---

## 4. Estratégia de Build e Integração Contínua

O sistema utiliza CMake (versão mínima 3.15) com alvos separados:
- `evap_core`: biblioteca estática;
- `evap_cli`: executável com análise de argumentos;
- `evap_tests`: suite automatizada com testes termodinâmicos, de validação e de regressão do artigo;
- `evap_wasm`: módulo WebAssembly gerado pelo Emscripten (`emcc`).

