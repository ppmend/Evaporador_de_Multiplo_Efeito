# Prompt mestre — evolução do solver de evaporadores de múltiplo efeito

> **Ambiente-alvo:** Antigravity IDE  
> **Modelo:** Claude Opus 4.6  
> **Entradas de referência obrigatórias:** `EvaporadoresMultiplos.c` e `artigo-cobeqic-evaporador-multiplo-efeito.pdf`  
> **Idioma do projeto e da documentação:** português do Brasil (`pt-BR`)  
> **Objetivo final:** transformar o código acadêmico em um projeto computacional organizado, verificável, documentado e apresentável publicamente, com um front-end web funcional conectado ao solver numérico real.

---

# 1. Instrução principal

Atue como uma equipe técnica multidisciplinar experiente, reunindo competências de:

- Engenharia Química, operações unitárias e projeto de evaporadores;
- Termodinâmica aplicada e propriedades de soluções aquosas de NaOH;
- Fenômenos de transporte e transferência de calor;
- Métodos numéricos para sistemas não lineares, Gauss–Seidel e SOR;
- Programação científica em C, análise estática, depuração e otimização;
- Arquitetura de software, testes automatizados e integração contínua;
- Desenvolvimento web com HTML5, CSS3 e JavaScript moderno;
- WebAssembly/Emscripten, para executar o núcleo numérico em navegadores;
- Design de produto digital, design editorial, acessibilidade e experiência do usuário.

Sua tarefa é **inspecionar, auditar, corrigir, refatorar, testar, documentar e apresentar este projeto de ponta a ponta**. Não se limite a sugerir mudanças, escrever um plano ou produzir exemplos isolados: trabalhe diretamente no workspace do Antigravity, implemente as melhorias, execute os testes disponíveis e deixe os arquivos finais prontos para revisão e publicação pelo autor.

O resultado precisa preservar a identidade científica do trabalho. Não transforme o projeto em uma demonstração genérica de programação, não substitua o modelo matemático por aproximações arbitrárias e não construa um site que apenas simule resultados fictícios. O site deverá ser uma interface real para o solver desenvolvido em C.

**Regra de integridade científica:** antes de alterar uma equação, correlação, unidade, hipótese ou critério de convergência, identifique sua origem no artigo ou no código, explique a inconsistência encontrada e verifique a formulação correta. Não “corrija” silenciosamente algo que possa representar uma decisão de modelagem. Quando não for possível determinar a intenção original com segurança, preserve o comportamento de referência em uma versão claramente identificada, registre a dúvida e implemente a alternativa somente se puder justificá-la tecnicamente e testá-la.

**Regra de execução:** avance até onde o ambiente permitir. Não encerre o trabalho após a auditoria. Se alguma ferramenta ou dependência não estiver instalada, documente a limitação, prepare os scripts e a configuração necessários e continue as demais etapas. Não invente resultados de testes, não declare uma compilação como bem-sucedida sem executá-la e não publique ou envie alterações para o GitHub sem autorização explícita do usuário.

---

# 2. Contexto científico e objetivo do projeto

O projeto implementa um algoritmo para o **dimensionamento de sistemas de evaporadores de múltiplo efeito em série, com alimentação direta (forward feed), considerando a variação de pressão entre os efeitos e a elevação do ponto de ebulição (EPE) da solução**.

O trabalho foi desenvolvido no contexto da Faculdade de Engenharia Química da Universidade Federal de Uberlândia (UFU) e deu origem ao artigo:

**MENDONÇA, Pedro Paulo Nogueira; DUARTE, Cláudio Roberto. _Projeto de evaporadores de múltiplo efeito com influência da pressão e EPE_. XVI Congresso Brasileiro de Engenharia Química em Iniciação Científica (COBEQ-IC), UNICAMP, 2026.**

O artigo e o código-fonte anexados a esta tarefa são as referências primárias do trabalho. Leia os dois integralmente antes de modificar a implementação. Preserve a autoria e a participação dos dois autores na apresentação pública do estudo. O cabeçalho do arquivo C identifica Pedro Paulo Nogueira Mendonça como autor do código; o artigo possui dois autores. Não atribua contribuições individuais que não estejam documentadas.

## 2.1. Problema de projeto de referência

O artigo apresenta o seguinte caso-base:

| Parâmetro | Valor de referência |
|---|---:|
| Número de efeitos estudados | 2 a 7 |
| Vazão mássica de alimentação, `F` | 10.000 kg/h |
| Concentração mássica de NaOH na alimentação, `xF` | 0,10 (10% em massa) |
| Concentração mássica de NaOH no produto, `xN` | 0,50 (50% em massa) |
| Temperatura da alimentação, `TF` | 50 °C |
| Temperatura do vapor vivo, `TS` | 160 °C |
| Temperatura de saturação da água no último efeito | 60 °C |
| Coeficiente global de transferência de calor, `U` | 2.000 W/(m²·K) em todos os efeitos |
| Área de troca térmica | Igual em todos os efeitos |
| Tolerância numérica declarada no artigo | 10⁻¹⁰ |

O estudo considera pressão variável entre os efeitos, propriedades termodinâmicas dependentes da temperatura e da concentração, vapor gerado sem arraste de soluto, ausência de perdas térmicas e alimentação em série no sentido do primeiro para o último efeito.

O objetivo do algoritmo é calcular, de forma acoplada, vazões de vapor e de líquido, concentrações, temperaturas, pressões, EPE, distribuição de diferenças de temperatura, consumo de vapor vivo e área de troca térmica necessária para que os efeitos tenham áreas iguais.

## 2.2. Formulação que deve ser preservada e auditada

Considere `i = 1, ..., N`, com `L0 = F` e `x0 = xF`.

**Balanços de massa por efeito:**

\[
L_i + V_i = L_{i-1},
\qquad
L_i x_i = L_{i-1}x_{i-1}.
\]

Como o soluto é considerado não volátil e permanece na fase líquida:

\[
L_N = \frac{x_F F}{x_N}.
\]

**Balanço de energia por efeito**, conforme a convenção de correntes e índices do artigo:

\[
V_i h_{v,i} + L_i H_L(T_i,x_i)
=
V_{i-1}\,\Delta H_{vap}(T_{sat,i-1})
+L_{i-1}H_L(T_{i-1},x_{i-1}).
\]

No primeiro efeito, o aquecimento é fornecido pelo vapor vivo. A taxa de calor é calculada a partir de `S` e da entalpia de vaporização na temperatura `TS`. Nos efeitos seguintes, o vapor gerado no efeito anterior fornece a carga térmica do efeito subsequente.

**Equação de dimensionamento térmico:**

\[
A_i=\frac{Q_i}{U_i\,\Delta T_i},
\qquad A_1=A_2=\cdots=A_N.
\]

Na implementação, verifique cuidadosamente a conversão de unidades entre kg/h, kJ/kg e W. O fator `3,6` aparece no código para compatibilizar a taxa de energia em kJ/h com `U` em W/(m²·K). Não o remova nem o mantenha sem demonstrar dimensionalmente sua necessidade.

As diferenças locais de temperatura utilizadas pelo código são, em essência:

\[
\Delta T_1=T_S-T_1,
\qquad
\Delta T_i=T_{sat,i-1}-T_i \quad (i\geq 2).
\]

A elevação do ponto de ebulição é definida por:

\[
EPE_i=T_i-T_{sat,i}.
\]

**Atenção à notação:** o artigo utiliza símbolos cuja interpretação precisa ser confrontada com o código, sobretudo na condição de viabilidade térmica e na expressão da diferença global de temperatura. Antes de implementar essas condições, escreva uma tabela de nomenclatura indicando, sem ambiguidade, o que representa `T_i`, `T_sat,i`, `EPE_i` e `ΔT_i`. Derive a condição de viabilidade a partir das diferenças locais efetivamente usadas em `A_i=Q_i/(U_i ΔT_i)`. Não aplique uma desigualdade literalmente se ela estiver incompatível com as definições adotadas.

## 2.3. Modelos de propriedades termodinâmicas

O artigo informa que foram utilizados:

1. Equação de Antoine para pressão de saturação da água pura e sua inversa para temperatura de saturação;
2. Equação de Watson para entalpia de vaporização da água;
3. Ajuste polinomial para entalpia de vapor saturado;
4. Integração de uma correlação de `Cp(T)` para obter a entalpia do vapor superaquecido;
5. Correlação do tipo Antoine modificado para a temperatura de ebulição da solução de NaOH em função da pressão e da concentração, incorporando a EPE;
6. Correlação polinomial para a entalpia da solução aquosa de NaOH em função da temperatura e da concentração.

As funções atualmente presentes no código são `HL`, `dHvap`, `Hvsat`, `h_super`, `Psat`, `Tsat` e `calc_T`. Mantenha a rastreabilidade entre cada função, sua equação, suas unidades, sua faixa de validade e a fonte dos dados.

O artigo informa, entre outros, os seguintes coeficientes para os ajustes:

- Entalpia da solução: `h00 = -26,01021`, `h10 = -207,89085`, `h01 = 0,99923`, `h20 = 795,64522`, `h02 = -1,27127e-5` e `h11 = -0,47635`;
- Correlação modificada de pressão/temperatura: `A(x) = a0 + a1 x + a2 x² + a3 x³`, com `a0 = 6,77113`, `a1 = -0,71947`, `a2 = 1,13082` e `a3 = 3,62321`;
- `B(x) = b0 + b1 x + b2 x² + b3 x³`, com `b0 = -1395,64615`, `b1 = 47,44337`, `b2 = -11,18678` e `b3 = -2711,78458`;
- Constante `C = 190,80153` na correlação modificada.

A documentação deve indicar os coeficientes com precisão suficiente para reproduzir o código e o artigo. Não arredonde os coeficientes internos sem avaliar o impacto nos resultados.

---

# 3. Auditoria inicial obrigatória: inconsistências a verificar

Os pontos abaixo foram identificados em uma leitura preliminar dos arquivos. Trate-os como **itens de auditoria**, não como autorização para modificar a formulação sem análise.

## 3.1. Divergência entre comentário inicial e parâmetros executados

O comentário de abertura de `EvaporadoresMultiplos.c` descreve um sistema de triplo efeito, alimentação de 15.000 kg/h, vapor vivo a 140 °C e último efeito com saturação a 50 °C. Entretanto, a função `SetDomain()` configura `F = 10000 kg/h`, `TS = 160 °C` e `T_sat` do último efeito igual a 60 °C, valores compatíveis com o caso-base publicado no artigo.

**Ação:** corrigir os comentários e centralizar a definição do caso-base, evitando divergências entre documentação, parâmetros e execução.

## 3.2. Número de efeitos fixado no `main`

O `main()` define `par->n = 8`, enquanto os resultados publicados no artigo foram obtidos para `N = 2, 3, 4, 5, 6 e 7`.

**Ação:** remover essa configuração rígida. Permitir que o número de efeitos seja fornecido por configuração/CLI e pela interface web. Para a demonstração pública, usar inicialmente o intervalo efetivamente estudado no artigo (2 a 7 efeitos). Qualquer extensão para outros valores deve ser identificada como extrapolação e submetida a testes de validade física e numérica.

## 3.3. Temperatura usada na correlação de entalpia da solução

Na função `HL(T,x)`, o código converte a temperatura de Celsius para Fahrenheit antes de aplicar os coeficientes:

\[
T_F=32+\frac{9}{5}T_C.
\]

Entretanto, a Equação 11 do artigo descreve a temperatura como estando em °C. Essa diferença pode alterar materialmente os resultados.

**Ação:** rastrear a origem do ajuste e verificar em qual escala térmica os coeficientes foram efetivamente ajustados. Compare a implementação com os dados digitalizados, com a tabela do artigo e, quando possível, com pontos de referência do Perry. Não altere a escala apenas para fazer o código coincidir com a descrição textual. Registre a conclusão em `docs/auditoria-modelo.md`, com justificativa e testes numéricos.

## 3.4. Unidade de pressão

O campo `P` em `StateVec` está comentado como `[Pa]`, mas `Psat()` retorna kPa, `Tsat()` recebe kPa e os resultados impressos também indicam kPa.

**Ação:** padronizar a unidade internamente (preferencialmente kPa, como já usado pelas correlações), corrigir nomes, comentários, validações, documentação e interface. Não misturar Pa e kPa.

## 3.5. Critério de convergência

O artigo declara uma tolerância de `10⁻¹⁰` para a variação relativa das variáveis. No código, o laço externo compara diferenças absolutas entre áreas, enquanto o laço interno soma quadrados de variações relativas de `V`, `L` e `S`. Essas medidas não são equivalentes.

**Ação:** reconstruir o critério numérico realmente usado no estudo, definir resíduos e escalas de normalização, documentar a escolha e implementar critérios independentes e explícitos para:

- convergência do balanço de massa;
- convergência do balanço de energia;
- igualdade das áreas;
- estabilidade das variáveis iteradas;
- viabilidade física.

Não apresente uma tolerância como “erro relativo de todas as variáveis” se o código estiver calculando outra grandeza. Escolha critérios que evitem dependência inadequada da escala das variáveis.

## 3.6. Inicialização e diferença global de temperatura

Em `SetDomain()`, a soma inicial de EPE considera o intervalo `i < n-1`, enquanto em `Solver()` a expressão de `par->DT` utiliza `T[n-1]` e soma as EPEs até `n-2`. A aparente diferença pode decorrer da representação da temperatura final, mas precisa ser demonstrada.

**Ação:** derive a relação global a partir das diferenças locais. Verifique se a EPE do último efeito está contabilizada exatamente uma vez e se a inicialização e as iterações utilizam a mesma convenção. Adicione teste para a identidade entre a soma das diferenças locais e a diferença global disponível.

## 3.7. Proteção dos laços iterativos

Os laços `do/while` de `Solver()` e `SolveSistema()` não possuem limite máximo de iterações. Também há divisões por valores anteriores, como `S_old`, `V_old[i]` e `L_old[i]`, que podem ser nulos ou numericamente muito pequenos. Valores `NaN` ou infinitos podem encerrar um laço indevidamente, sem representar convergência.

**Ação:** implementar limites configuráveis de iteração, verificação de `isfinite`, proteção de denominadores, detecção de estagnação/divergência e estados de retorno explícitos. Um resultado só pode ser classificado como convergido quando todos os critérios definidos forem satisfeitos.

## 3.8. Estado global e tratamento de erros

O código usa `params *par` como variável global. `AvaliarEstabilidade()` chama `exit(1)`, o que impede que o núcleo seja reutilizado com segurança por uma interface web ou por testes automatizados. As alocações com `calloc()` não são verificadas integralmente.

**Ação:** remover o estado global do núcleo, passar estruturas de entrada/saída explicitamente, verificar todas as alocações e propagar erros por códigos de estado e mensagens. Nenhuma função da biblioteca numérica deve encerrar o processo com `exit()`.

## 3.9. Relatórios e rótulos

Verifique os rótulos de unidades e nomes das variáveis em `printResults()`. Por exemplo, `TS` é temperatura, não vazão mássica. Verifique também o comentário de `x`, que representa a concentração mássica da solução líquida, não “concentração do condensado”.

**Ação:** corrigir nomenclatura, unidades e exportação dos resultados. A saída precisa ser consistente entre terminal, JSON, documentação e site.

## 3.10. Outras verificações obrigatórias

Audite também:

- tratamento do caso `N < 2` e limites máximos de efeitos;
- indexação de vetores nos efeitos inicial e final;
- estado inicial de `S` antes da primeira iteração;
- segurança e consistência do uso de vetores de tamanho variável;
- coerência entre `T`, `T_sat`, `P`, `EPE` e `dT` após cada atualização;
- validade das faixas de temperatura e pressão das correlações;
- comportamento da equação de Watson próximo à temperatura crítica da água;
- continuidade entre as faixas de Antoine selecionadas por temperatura/pressão;
- validade de `h_super()` quando `T < Tsat(P)`;
- sinais, magnitudes e positividade das vazões, concentrações, áreas e diferenças térmicas;
- consistência dimensional de cada equação e conversão de unidades;
- vazamentos de memória, comportamento indefinido e avisos do compilador.

---

# 4. Fase I — diagnóstico e preservação da referência

Antes de refatorar:

1. Inspecione toda a estrutura do workspace e verifique se já existe um repositório Git, histórico, README, licença, scripts ou outras versões do solver.
2. Leia integralmente os dois arquivos fornecidos e identifique a versão efetivamente compilada.
3. Preserve o arquivo original. Não o sobrescreva antes de criar uma cópia rastreável ou uma referência Git.
4. Compile o código original com o compilador disponível e registre a saída de compilação.
5. Execute uma reprodução controlada do comportamento original, quando possível. Registre os parâmetros utilizados, o compilador, as opções e os resultados. Não confunda essa execução com a reprodução do artigo se `N`, `ω` ou outros parâmetros forem diferentes.
6. Use avisos rigorosos de compilação e ferramentas de análise disponíveis. Investigue todos os avisos relevantes.
7. Crie `docs/auditoria-modelo.md` com:
   - descrição do modelo atual;
   - mapa entre equações do artigo e funções do código;
   - tabela de variáveis, símbolos e unidades;
   - problemas encontrados, severidade e evidência;
   - decisões adotadas e justificativas;
   - divergências que permanecerem sem solução conclusiva.

Classifique os achados, por exemplo, como críticos, altos, médios ou baixos. Diferencie claramente bug de implementação, inconsistência documental, limitação do modelo e possível escolha legítima de modelagem.

Não interrompa o trabalho aguardando aprovação depois de produzir o diagnóstico. Tome decisões conservadoras, justificáveis e reversíveis, e prossiga.

---

# 5. Fase II — refatoração da arquitetura em C

Transforme o arquivo monolítico em um núcleo numérico organizado, com responsabilidades separadas. Uma estrutura inicial possível é:

```text
.
├── src/
│   ├── core/
│   │   ├── solver.c
│   │   ├── solver.h
│   │   ├── thermodynamics.c
│   │   ├── thermodynamics.h
│   │   ├── balances.c
│   │   ├── balances.h
│   │   ├── validation.c
│   │   └── validation.h
│   ├── cli/
│   │   └── main.c
│   └── wasm/
│       ├── wasm_api.c
│       └── wasm_api.h
├── include/
├── tests/
├── docs/
├── web/
├── scripts/
├── .github/
│   └── workflows/
├── .agents/
│   └── skills/
├── CMakeLists.txt
├── CMakePresets.json
├── .gitignore
├── README.md
└── LICENSE (somente após definição explícita da licença)
```

A estrutura é uma referência, não uma exigência cega. Ajuste-a se houver uma arquitetura tecnicamente melhor, mantendo separação clara entre núcleo matemático, interface de linha de comando, camada WebAssembly, testes e front-end.

## 5.1. API do núcleo

Projete uma API explícita e reutilizável, sem variáveis globais. Como referência:

```c
int solver_run(const SimulationInput *input,
               SimulationResult *result,
               SolverError *error);

void solver_result_free(SimulationResult *result);
```

Ajuste a assinatura se necessário. O essencial é:

- entradas independentes e validadas;
- resultados em estrutura previsível;
- estado de convergência explícito;
- mensagens de erro úteis;
- ausência de `printf()` e `exit()` no núcleo;
- gerenciamento de memória bem definido;
- possibilidade de executar múltiplas simulações em sequência sem contaminar o estado.

Inclua na entrada, no mínimo, número de efeitos, vazão de alimentação, concentrações inicial/final, temperatura da alimentação, temperatura do vapor vivo, temperatura de saturação de referência do último efeito, coeficiente global de transferência de calor e fator de relaxação. Inclua tolerâncias e limites de iteração configuráveis, com padrões coerentes com o estudo.

A concentração deve ter uma convenção interna única. Recomenda-se armazenar a fração mássica entre 0 e 1 e converter para porcentagem apenas na apresentação. Faça o mesmo com todas as grandezas dimensionais.

## 5.2. Separação das propriedades

Mantenha as funções termodinâmicas isoladas do algoritmo iterativo. Cada função deve possuir:

- protótipo e documentação;
- unidade de cada argumento e do retorno;
- faixa de validade;
- tratamento de entrada fora da faixa;
- testes de unidade;
- referência bibliográfica ou identificação de ajuste próprio.

Utilize nomes que tornem a unidade evidente quando isso reduzir ambiguidades, por exemplo `pressure_kpa`, `temperature_c`, `mass_flow_kg_h` e `enthalpy_kj_kg`.

## 5.3. Qualidade e manutenção

- Prefira C11 ou C17, conforme a compatibilidade real do ambiente, com padrão declarado no sistema de build.
- Elimine dependências desnecessárias e macros que dificultem a leitura.
- Use `const` onde apropriado e tipos adequados para tamanhos de vetores e contadores.
- Evite VLA quando uma estrutura de alocação explícita for mais segura e portátil.
- Verifique os retornos de alocação.
- Documente as estruturas públicas e as funções não triviais.
- Não faça micro-otimizações antes de garantir equivalência matemática e testes.
- Não altere as correlações apenas para deixar o código mais elegante.

## 5.4. Interface de linha de comando

Disponibilize uma CLI para executar uma simulação individual e, quando pertinente, uma varredura de `ω`. Deve ser possível informar os parâmetros sem editar o código-fonte. Inclua:

- execução com parâmetros padrão do artigo;
- execução com parâmetros personalizados;
- seleção de `N` e `ω`;
- opção de saída legível no terminal;
- opção de saída estruturada em JSON ou CSV;
- código de saída diferente para sucesso, entrada inválida, inviabilidade física, não convergência e erro interno.

A CLI e a interface web devem chamar o mesmo núcleo. Não mantenha duas implementações independentes das equações.

---

# 6. Fase III — robustez numérica e validação física

O método publicado é baseado em Gauss–Seidel com sub-relaxação SOR. Preserve essa estratégia como método de referência. Melhorias no controle de convergência e na estabilidade são desejáveis, mas não substitua o método por Newton, Broyden ou outra estratégia sem manter a versão SOR reproduzível e explicar a mudança.

## 6.1. Critérios numéricos

Implemente e documente:

- tolerância absoluta e relativa de forma separada;
- norma ou conjunto de resíduos para balanços de massa e energia;
- critério de igualdade das áreas;
- número máximo de iterações internas e externas;
- detecção de estagnação, oscilação e divergência;
- detecção de `NaN`, infinito e overflow;
- proteção contra denominadores nulos ou próximos de zero;
- relatório do motivo de encerramento.

Não aceite convergência por simples saída do laço causada por `NaN`. Não esconda falhas elevando a tolerância automaticamente.

## 6.2. Viabilidade física

Antes e depois da solução, verifique condições como:

- vazões de líquido, vapor e vapor vivo não negativas;
- concentrações dentro do domínio físico e da faixa de validade das correlações;
- pressões positivas e dentro do domínio dos modelos;
- temperaturas dentro das faixas em que as correlações são válidas;
- EPE calculada de forma consistente com a definição;
- diferenças locais de temperatura estritamente positivas, quando necessárias para transferência de calor;
- áreas de troca térmica positivas e finitas;
- igualdade de áreas dentro da tolerância especificada;
- fechamento dos balanços de massa e energia.

Retorne mensagens específicas, por exemplo: “diferença de temperatura não positiva no efeito 3”, “correlação de entalpia fora da faixa”, “SOR não convergiu após o limite de iterações”. Não encerre o processo inteiro por causa de uma entrada inviável.

## 6.3. Testes automatizados

Crie testes em níveis diferentes:

1. **Testes unitários de propriedades:** pontos de referência, unidades, comportamento monotônico quando fisicamente esperado, limites de validade e entradas inválidas.
2. **Testes de balanços:** fechamento de massa total, soluto e energia em cada efeito.
3. **Testes de consistência térmica:** cálculo de `ΔT`, EPE e igualdade de áreas.
4. **Testes de robustez:** `N` mínimo/máximo suportado, fatores de relaxação próximos aos limites permitidos, entradas extremas e condições fisicamente inviáveis.
5. **Testes de memória e comportamento indefinido:** use AddressSanitizer e UndefinedBehaviorSanitizer quando disponíveis.
6. **Testes de regressão do artigo:** tente reproduzir os resultados publicados para `N = 2` a `7`.

## 6.4. Resultados de referência publicados

A Tabela 2 do artigo informa os seguintes valores:

| N | Área reportada (m²) | Vapor vivo, S (kg/h) | Economia de vapor | ω ótimo | Limite superior do intervalo de convergência |
|---:|---:|---:|---:|---:|---:|
| 2 | 58,083 | 5.940,4 | 1,3467 | 0,665 | 0,988 |
| 3 | 70,128 | 4.593,6 | 1,7415 | 0,496 | 0,937 |
| 4 | 89,608 | 3.949,4 | 2,0256 | 0,445 | 0,884 |
| 5 | 125,442 | 3.585,8 | 2,2310 | 0,429 | 0,823 |
| 6 | 211,225 | 3.363,0 | 2,3788 | 0,377 | 0,743 |
| 7 | 684,210 | 3.221,2 | 2,4836 | 0,345 | 0,613 |

A economia de vapor é consistente com a razão entre a evaporação total e o vapor vivo, isto é, `E = (F - L_N)/S`, mas confirme essa definição no código e na interpretação da tabela antes de fixá-la na API.

**Atenção à área:** a tabela reporta “Área (m²)” e o modelo impõe áreas iguais nos efeitos. Verifique se o valor é a área de cada efeito ou a área total instalada. Na ausência de evidência de que seja área total, apresente o valor como **área de troca por efeito**, explicando que existem `N` efeitos com áreas iguais. Não multiplique ou divida o resultado por `N` sem justificativa.

Use esses dados como referência de regressão, não como valores a serem forçados no solver. Defina tolerâncias de comparação justificáveis considerando arredondamento, versão das correlações e precisão publicada. Se os resultados não coincidirem, investigue a causa e registre o desvio; jamais ajuste artificialmente as saídas para reproduzir a tabela.

O artigo conclui que, para as condições estudadas, sistemas entre 3 e 5 efeitos apresentam melhor compromisso entre desempenho térmico e robustez computacional. Apresente isso como conclusão do estudo de referência, não como regra universal para qualquer planta ou condição operacional.

---

# 7. Fase IV — integração do solver com a Web

## 7.1. Arquitetura preferencial: C compilado para WebAssembly

O requisito central é que o site execute o **mesmo núcleo numérico em C**, e não uma reimplementação aproximada em JavaScript. Avalie o ambiente e, se o Emscripten estiver disponível ou puder ser configurado de forma reproduzível, compile o núcleo C para WebAssembly.

Arquitetura esperada:

```text
HTML + CSS + JavaScript
          │
          ▼
  camada de interface JS
          │
          ▼
  API de integração WebAssembly
          │
          ▼
  núcleo numérico em C
          │
          ▼
  SimulationResult + status
```

O JavaScript será responsável pela interface, validação de formulário, apresentação dos resultados, gráficos e mensagens. O C será responsável pelos cálculos do modelo.

Implemente uma camada de integração clara, com contrato documentado. Pode utilizar uma estrutura de dados plana ou funções exportadas, desde que a passagem de parâmetros e resultados seja segura e testável. Evite depender de offsets de memória sem documentação. Faça o tratamento assíncrono de inicialização do módulo e mostre um estado de carregamento enquanto o WebAssembly não estiver pronto.

**Não faça isto:**

- não simule o resultado com números estáticos quando o usuário clicar em “Calcular”;
- não replique as equações em JavaScript para contornar falhas de integração;
- não apresente a tabela do artigo como se fosse a saída de uma nova execução;
- não oculte erros de carregamento do módulo;
- não afirme que o solver executou quando a chamada real falhou.

Se não for possível compilar WebAssembly no ambiente atual, conclua a API nativa, os testes, o front-end e os scripts de build. Prepare a integração e uma rotina de CI para gerar o artefato WebAssembly. Documente exatamente o que foi testado e o que depende de execução posterior. O site ainda deve distinguir claramente resultados publicados de resultados de uma simulação executada.

## 7.2. Interface de simulação

A página deverá oferecer um formulário para executar o dimensionamento. Inclua, no mínimo:

- número de efeitos (`N`), inicialmente limitado a 2–7;
- vazão de alimentação (`F`, kg/h);
- concentração inicial de NaOH (`xF`, % m/m);
- concentração final de NaOH (`xN`, % m/m);
- temperatura da alimentação (`TF`, °C);
- temperatura do vapor vivo (`TS`, °C);
- temperatura de saturação da água no último efeito (°C);
- coeficiente global de transferência de calor (`U`, W/(m²·K));
- fator de sub-relaxação (`ω`), com `0 < ω < 1`.

Pré-preencha os parâmetros com o caso-base do artigo. Exiba unidades ao lado dos campos e explique os parâmetros menos óbvios em textos auxiliares discretos. Não use campos sem rótulo acessível.

A interface deve:

- validar entradas antes de chamar o solver;
- impedir concentrações incompatíveis, temperaturas fora das faixas e `ω` inválido;
- oferecer restauração dos parâmetros do artigo;
- impedir envios repetidos enquanto a simulação está em andamento;
- apresentar carregamento, sucesso, inviabilidade e falha de convergência de maneira distinta;
- preservar os parâmetros usados junto ao resultado;
- permitir copiar ou exportar os resultados, preferencialmente em CSV e JSON;
- informar que o resultado depende das hipóteses e das faixas de validade das correlações.

## 7.3. Resultados que devem ser apresentados

Exiba, quando calculados e disponíveis no resultado do C:

- status da simulação e número de iterações;
- consumo de vapor vivo `S` (kg/h);
- economia de vapor;
- vazões de vapor `V_i` e líquido `L_i` por efeito;
- concentração de NaOH por efeito;
- temperatura da solução `T_i` e temperatura de saturação `T_sat,i`;
- pressão de operação por efeito;
- EPE por efeito;
- diferença de temperatura `ΔT_i` por efeito;
- área de troca térmica por efeito;
- resíduos ou erros normalizados de massa, energia e igualdade de áreas.

Use tabelas legíveis e gráficos pertinentes, por exemplo:

1. perfil de temperatura de saturação e temperatura de ebulição da solução ao longo dos efeitos;
2. distribuição de `ΔT`;
3. vazões de líquido e vapor por efeito;
4. áreas calculadas por efeito, permitindo observar a igualdade imposta;
5. quando houver uma varredura real de `ω`, número de iterações em função de `ω`.

Gráficos devem ser construídos com dados realmente retornados pelo solver ou com dados históricos explicitamente identificados como publicados no artigo. Não invente séries para preencher visualizações. Gráficos SVG próprios ou Canvas são aceitáveis; mantenha dependências pequenas e justificadas.

## 7.4. Separação entre dados do artigo e simulação atual

A interface deverá distinguir visualmente:

- **Resultados publicados em 2026:** valores da tabela do artigo;
- **Simulação executada agora:** valores retornados pelo WebAssembly;
- **Parâmetros da execução:** entradas específicas usadas naquela chamada.

A tabela histórica pode ser exibida mesmo quando o WebAssembly não estiver disponível, mas deve conter identificação explícita de que são resultados do artigo. Nunca misture esses dados com os resultados calculados na sessão.

---

# 8. Fase V — criação do site com identidade visual profissional

Desenvolva o front-end em **HTML semântico, CSS moderno e JavaScript**, preferencialmente sem framework pesado. A intenção é apresentar um projeto de Engenharia Química e Computação Científica de maneira convincente para professores, pesquisadores e recrutadores técnicos.

O site não deve parecer um template genérico de SaaS nem uma página criada automaticamente por IA. Precisa ter direção de arte, hierarquia editorial, ritmo visual, consistência de espaçamento e componentes desenhados para o conteúdo real do projeto.

## 8.1. Skills de design: uso obrigatório

Antes de desenhar a interface:

1. Inspecione as skills disponíveis no Antigravity IDE, pela área de Customizations e pelos diretórios de skills do workspace e do usuário.
2. Procure skills aplicáveis a `frontend-design`, `web-design-guidelines`, design systems, acessibilidade, responsividade e testes de interface. Os nomes podem variar; use as skills realmente encontradas, lendo seus arquivos `SKILL.md` integralmente.
3. Priorize instruções de design editorial, interface de produto, design visual, acessibilidade e browser testing. Se existirem skills complementares, combine-as sem criar conflito entre suas regras.
4. Se nenhuma skill de design adequada estiver disponível, crie uma skill local e versionável em `.agents/skills/portfolio-frontend-design/SKILL.md`, com frontmatter YAML válido (`name` e `description`) e instruções específicas para a identidade visual, componentes, acessibilidade e critérios de qualidade deste projeto. Use essa skill durante a construção e revisão do site.
5. Se houver skill de teste visual ou acessibilidade, utilize-a depois da implementação. Não declare que utilizou uma skill que não foi localizada e lida.

A documentação oficial de skills do Antigravity descreve o formato `SKILL.md` e a localização padrão `.agents/skills/`. Siga a convenção vigente no ambiente.

## 8.2. Direção de arte

Adote uma linguagem visual de **engenharia de processos + publicação científica contemporânea**. A interface pode remeter a um caderno técnico, diagrama de processo ou relatório de projeto bem editado, sem imitar literalmente um artigo acadêmico.

Diretrizes:

- paleta sóbria, com fundo claro ou grafite, texto de alto contraste e uma cor principal profunda (azul petróleo, azul técnico ou equivalente);
- uma cor de acento secundária, usada com parcimônia para destacar dados e ações;
- tipografia com personalidade e boa legibilidade, combinando uma família para títulos, outra para texto corrido e uma monoespaçada para números, variáveis e dados técnicos;
- hierarquia tipográfica nítida e espaçamento consistente;
- grid bem definido e margens generosas;
- linhas, eixos, diagramas e pequenos detalhes gráficos que tenham relação com o processo de evaporação;
- ilustrações vetoriais próprias, caso sejam necessárias, sem imagens genéricas de laboratório;
- animações sutis, funcionais e respeitosas a `prefers-reduced-motion`.

Evite deliberadamente:

- gradientes chamativos sem função;
- glassmorphism;
- sombras excessivas;
- excesso de cartões arredondados;
- ícones e emojis decorativos;
- imagens de banco de imagens sem relação com o sistema real;
- títulos enormes sem conteúdo;
- excesso de etiquetas em caixa alta;
- cores fluorescentes;
- efeitos de entrada que atrasem a leitura;
- aparência de dashboard administrativo genérico.

O resultado deve parecer uma página de portfólio técnico feita com intenção por alguém da área de Engenharia Química e Computação Científica.

## 8.3. Estrutura editorial sugerida

Construa uma página principal organizada em seções coesas. Adapte a ordem se houver justificativa de experiência do usuário.

### A. Abertura / hero

Apresente o título:

**Dimensionamento de evaporadores de múltiplo efeito**

Subtítulo sugerido:

“Modelagem de balanços de massa e energia com pressão variável e elevação do ponto de ebulição — implementação numérica em C e simulação interativa no navegador.”

Inclua chamadas claras para “Executar simulação” e “Consultar artigo”. Utilize um esquema vetorial original da bateria de evaporadores em série, com alimentação, vapor vivo, correntes de líquido, vapor gerado e efeitos sucessivos. O diagrama deve refletir o processo real e não ser apenas um desenho decorativo.

### B. Visão geral e relevância

Explique, em linguagem técnica acessível, por que evaporadores de múltiplo efeito são empregados, por que a pressão variável e a EPE importam no dimensionamento e qual problema computacional foi enfrentado.

### C. Resultados publicados

Apresente os resultados de `N = 2` a `7` da tabela do artigo. Destaque com parcimônia a evolução da economia de vapor, da área e do fator ótimo de relaxação. Informe que os dados são do artigo de 2026.

### D. Simulador interativo

A simulação deve ser uma seção central do site, não um elemento secundário. Inclua formulário, validação, execução real do solver C/WebAssembly, resultados, tabelas e gráficos.

### E. Modelo matemático e hipóteses

Resuma os balanços de massa e energia, a igualdade de áreas, as propriedades termodinâmicas e as hipóteses físicas. Utilize equações legíveis em MathML, LaTeX renderizado ou notação textual acessível. Evite despejar todo o artigo na página.

### F. Método numérico

Explique o procedimento iterativo de dois níveis e o uso de Gauss–Seidel com sub-relaxação SOR. Destaque o papel de `ω` na estabilidade e na convergência, sem simplificar em excesso.

### G. Publicação científica

Inclua a referência bibliográfica completa do artigo, autores, evento, ano e um link para abrir/baixar o PDF anexado, caso os direitos e o repositório permitam sua distribuição. Não crie DOI, ISBN ou URL inexistente.

### H. Reprodutibilidade e repositório

Apresente linguagem, arquitetura, como compilar, executar, testar e reproduzir os resultados. Inclua link para o GitHub somente após confirmar a URL correta. O artigo registra o endereço `https://github.com/ppmend/Evaporador_de_Multiplo_Efeito`; verifique se esse repositório ainda existe e se é o destino apropriado antes de utilizá-lo.

### I. Autoria e contato

Apresente a autoria de acordo com os arquivos fornecidos. Reserve espaço para GitHub e LinkedIn, mas não invente perfis ou contatos. Não exponha automaticamente o e-mail pessoal presente no artigo; deixe-o de fora ou utilize uma configuração explícita que o autor possa revisar.

## 8.4. Responsividade e acessibilidade

O site deve funcionar em dispositivos móveis, tablets e desktops. Teste pelo menos larguras próximas de 375 px, 768 px e 1440 px.

Requisitos mínimos:

- HTML semântico;
- navegação por teclado;
- foco visível;
- contraste compatível com WCAG 2.2 AA, sempre que aplicável;
- labels associados a todos os campos;
- mensagens de erro compreensíveis e associadas aos respectivos campos;
- tabelas com cabeçalhos semânticos;
- gráficos com alternativa textual ou tabela acessível;
- `aria-live` para informar o estado da simulação;
- suporte a `prefers-reduced-motion`;
- layout sem rolagem horizontal indevida;
- metadados básicos de SEO e compartilhamento.

---

# 9. Fase VI — README e documentação do repositório

Produza um `README.md` com padrão de qualidade de projeto open source. O README deve explicar o que o projeto faz, como foi modelado, como compilar, como executar, como testar e como utilizar o simulador web.

Estrutura recomendada:

1. título e descrição objetiva;
2. contexto e motivação de Engenharia Química;
3. funcionalidades implementadas;
4. imagem ou captura do site, quando disponível;
5. diagrama de arquitetura do software;
6. formulação matemática resumida;
7. hipóteses físicas e limitações;
8. correlações termodinâmicas e referências;
9. método numérico e critérios de convergência;
10. parâmetros padrão do artigo;
11. requisitos e dependências;
12. instruções de compilação nativa;
13. instruções para executar a CLI;
14. instruções para executar testes;
15. instruções para compilar o WebAssembly;
16. instruções para executar o front-end localmente em servidor HTTP;
17. instruções de implantação no GitHub Pages, se a arquitetura escolhida permitir;
18. reprodução dos resultados publicados;
19. estrutura de diretórios;
20. publicação científica e citação do trabalho;
21. limitações e trabalho futuro;
22. créditos e autoria;
23. licença, somente quando definida pelo autor.

O README deve conter comandos reais, testados no ambiente quando possível. Não deixe instruções genéricas ou placeholders apresentados como se fossem comandos funcionais.

Crie também, quando pertinente:

- `docs/auditoria-modelo.md`;
- `docs/modelo-matematico.md`;
- `docs/arquitetura.md`;
- `docs/validacao-e-testes.md`;
- `CITATION.cff`, se os metadados necessários puderem ser preenchidos sem inventar dados;
- `.gitignore` apropriado para C, CMake, ferramentas locais, builds e artefatos temporários;
- workflow de CI para compilar, executar testes e, se viável, gerar e publicar o front-end.

**Licença:** não escolha automaticamente MIT, GPL ou outra licença. O projeto possui coautoria e artigo publicado. Registre no README que a licença precisa ser definida pelos titulares/autores e deixe o arquivo de licença para decisão explícita.

**PDF do artigo:** avalie a inclusão do PDF no repositório. Se houver qualquer dúvida de direito de distribuição, mantenha o PDF fora do repositório e forneça apenas a referência ou o link oficial, se disponível. Não modifique o conteúdo científico do artigo.

---

# 10. Fase VII — qualidade, testes de interface e implantação

## 10.1. Build e automação

Configure um sistema de build reprodutível, preferencialmente CMake, com alvos separados para:

- biblioteca/núcleo C;
- CLI;
- testes;
- build WebAssembly;
- front-end.

Inclua scripts simples para automatizar as tarefas recorrentes. Se o ambiente permitir, configure GitHub Actions para executar compilação e testes em cada push ou pull request. A etapa de implantação no GitHub Pages deve ser configurada de modo que funcione em repositório de projeto, respeitando caminhos relativos e o nome real do repositório.

Não execute `git push`, não publique o site e não altere permissões ou configurações remotas sem autorização explícita.

## 10.2. Verificação visual

Abra o site em navegador por meio do servidor local. Verifique:

- carregamento de CSS, JavaScript e WebAssembly;
- funcionamento real do botão de simulação;
- coerência de unidades e formatação numérica em português;
- ausência de erros no console;
- comportamento responsivo;
- navegação por teclado;
- tabelas e gráficos;
- estado de falha do solver;
- estado de WebAssembly ainda não carregado;
- ausência de dados fictícios ou resultados confundidos com dados publicados.

Faça uma segunda rodada de revisão visual após corrigir problemas. Não considere suficiente apenas observar o HTML no editor.

## 10.3. Desempenho e segurança

- evite dependências externas desnecessárias;
- minimize o tamanho dos arquivos distribuídos;
- carregue o WebAssembly de forma assíncrona;
- valide dados tanto no front-end quanto no núcleo C;
- não use `eval()`;
- não injete conteúdo do usuário com `innerHTML` sem sanitização;
- não inclua tokens, senhas ou credenciais;
- não exponha informações pessoais sem autorização;
- garanta que o front-end continue legível quando scripts ou WebAssembly falharem.

---

# 11. Critérios de aceitação — definição de concluído

O trabalho só deve ser considerado concluído quando os itens aplicáveis abaixo estiverem implementados e verificados:

## Núcleo científico e computacional

- [ ] O modelo matemático foi confrontado com o artigo e documentado.
- [ ] As inconsistências relevantes entre artigo e código foram analisadas.
- [ ] O código não depende de estado global para executar uma simulação.
- [ ] O número de efeitos e os parâmetros são configuráveis.
- [ ] Os laços iterativos possuem limites, verificações de finitude e mensagens de falha.
- [ ] As unidades estão consistentes e documentadas.
- [ ] O núcleo C compila sem avisos relevantes.
- [ ] Os testes de propriedades, balanços e robustez foram implementados.
- [ ] Os resultados de referência do artigo foram comparados e as diferenças documentadas.

## Interface web

- [ ] O front-end é implementado em HTML, CSS e JavaScript.
- [ ] O site usa o núcleo C real por meio de WebAssembly, quando o ambiente permitir.
- [ ] O formulário transmite os parâmetros efetivamente ao solver.
- [ ] Os resultados mostrados vêm da execução real ou estão identificados como dados publicados.
- [ ] O site possui identidade visual própria e adequada a um portfólio técnico.
- [ ] O site é responsivo e acessível.
- [ ] As simulações apresentam erros de entrada, inviabilidade e não convergência de forma explícita.
- [ ] O funcionamento foi verificado em navegador.

## Repositório e documentação

- [ ] O README explica instalação, compilação, execução, testes e uso do site.
- [ ] A auditoria e as decisões científicas estão documentadas.
- [ ] A publicação está referenciada corretamente.
- [ ] O `.gitignore` está configurado.
- [ ] A licença não foi inventada nem escolhida sem autorização.
- [ ] A configuração de CI está funcional ou sua limitação está documentada.
- [ ] Nenhuma alteração foi publicada remotamente sem autorização.

---

# 12. Forma de trabalho e comunicação

Siga esta ordem de execução:

1. **Inspecionar:** arquivos, workspace, ferramentas, skills e histórico Git.
2. **Auditar:** artigo, código, equações, unidades, correlações e algoritmo.
3. **Registrar:** criar a auditoria técnica e a linha de base.
4. **Refatorar:** separar núcleo, CLI, propriedades, validação e integração.
5. **Corrigir:** resolver bugs com justificativas e testes.
6. **Validar:** testes unitários, balanços, regressão e sanitizers.
7. **Integrar:** compilar o núcleo para WebAssembly e conectar o front-end.
8. **Projetar:** construir o site aplicando as skills de design identificadas.
9. **Documentar:** README, modelo matemático, arquitetura e reprodutibilidade.
10. **Verificar:** build limpo, testes, navegador e revisão visual.

Mantenha um registro conciso das decisões técnicas importantes. Sempre que encontrar uma divergência científica, indique: (a) o que consta no artigo; (b) o que consta no código; (c) o impacto provável; (d) a evidência utilizada; (e) a decisão implementada; (f) o teste que sustenta a decisão.

Não esconda limitações. Não invente benchmark, validação experimental, dados industriais ou precisão que não tenham sido demonstrados. O projeto é um trabalho de modelagem e simulação computacional; apresente suas contribuições reais com rigor.

Ao final, apresente um relatório com:

- resumo das mudanças realizadas;
- arquitetura final e arquivos principais;
- bugs corrigidos e justificativas;
- decisões de modelagem que exigiram atenção;
- testes executados e seus resultados reais;
- como compilar e executar o solver;
- como executar o site localmente;
- como gerar o WebAssembly;
- como publicar no GitHub Pages, caso esteja configurado;
- limitações remanescentes e ações que dependem do autor.

**Comece agora pela inspeção integral do workspace e dos dois arquivos de referência. Em seguida, execute todas as fases, sem parar na elaboração do plano.**

---

# 13. Referências técnicas para consulta

Utilize a documentação oficial atualizada quando precisar confirmar detalhes de ferramentas ou padrões:

- Antigravity — Agent Skills: <https://antigravity.google/docs/skills?app=antigravity-ide>
- Emscripten — documentação geral: <https://emscripten.org/docs/>
- Emscripten — compilação para WebAssembly: <https://emscripten.org/docs/compiling/WebAssembly.html>
- Emscripten — integração de projetos e build: <https://emscripten.org/docs/compiling/Building-Projects.html>
- MDN — HTML: <https://developer.mozilla.org/docs/Web/HTML>
- MDN — CSS: <https://developer.mozilla.org/docs/Web/CSS>
- MDN — JavaScript: <https://developer.mozilla.org/docs/Web/JavaScript>
- W3C — WCAG 2.2: <https://www.w3.org/TR/WCAG22/>
- GitHub Docs — GitHub Pages: <https://docs.github.com/pages>

Consulte essas referências para resolver dúvidas concretas. Não adicione dependências ou ferramentas apenas por tendência; escolha soluções compatíveis com a arquitetura e com a manutenção de longo prazo.
