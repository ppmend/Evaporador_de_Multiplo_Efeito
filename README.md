# Dimensionamento de Evaporadores de Múltiplo Efeito

[![CI Status](https://github.com/ppmend/Evaporador_de_Multiplo_Efeito/actions/workflows/ci.yml/badge.svg)](https://github.com/ppmend/Evaporador_de_Multiplo_Efeito/actions)
[![Standard C11](https://img.shields.io/badge/C-C11-00599C?logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![WebAssembly](https://img.shields.io/badge/WebAssembly-WASM-654FF0?logo=webassembly&logoColor=white)](https://webassembly.org/)
[![License: Restrita](https://img.shields.io/badge/License-Sob_Consulta-lightgrey.svg)](#licença)

Solução computacional acoplada para simulação e dimensionamento de evaporadores de múltiplo efeito em série com alimentação direta (*forward feed*), considerando **pressão variável entre os efeitos**, **elevação do ponto de ebulição (EPE)** e **restrição construtiva de áreas de troca térmica idênticas**.

O projeto reúne um núcleo de cálculo numérico de alto desempenho em C (C11), interface de linha de comando (CLI) completa com suporte a JSON/CSV e modos de varredura (*sweep/benchmark*), e uma aplicação web interativa executada via **WebAssembly** no navegador.

Trabalho desenvolvido no âmbito da Faculdade de Engenharia Química da Universidade Federal de Uberlândia (UFU), publicado no **XVI Congresso Brasileiro de Engenharia Química em Iniciação Científica (COBEQ-IC 2026, UNICAMP)**.

---

## 1. Contexto e Motivação de Engenharia Química

A concentração de soluções industriais (como hidróxido de sódio - NaOH, caldo de cana, licor negro) é uma das operações de maior consumo de energia na indústria química. Os sistemas de evaporadores de múltiplo efeito reduzem esse consumo reutilizando o vapor gerado em um efeito como fluido de aquecimento no efeito subsequente.

O dimensionamento rigoroso dessa bateria de trocadores requer o equacionamento acoplado de:
1. **Balanços materiais globais e de soluto** em cada estágio;
2. **Balanços de energia não lineares**, considerando calor latente variável e entalpias de vapor superaquecido;
3. **Elevação do Ponto de Ebulição (EPE)**, que reduz a força motriz térmica útil ($\Delta T$);
4. **Variação contínua da pressão**, exigindo a reavaliação das propriedades de saturação;
5. **Critério construtivo de áreas iguais** ($A_1 = A_2 = \cdots = A_N$).

---

## 2. Resultados de Referência Publicados (COBEQ-IC 2026)

O solver reproduz com precisão rigorosa os resultados da **Tabela 2** do artigo para o caso-base ($F = 10.000\text{ kg/h}$, $x_F = 10\%$, $x_N = 50\%$, $T_F = 50\text{ °C}$, $T_S = 160\text{ °C}$, $T_{sat,N} = 60\text{ °C}$, $U = 2.000\text{ W/(m²·K)}$):

| Efeitos ($N$) | Área por Efeito ($m^2$) | Vapor Vivo $S$ ($kg/h$) | Economia ($E$) | $\omega$ Ótimo | Área Total Instalada ($m^2$) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| **2** | 58,083 | 5.940,4 | 1,3467 | 0,665 | 116,166 |
| **3** | 70,128 | 4.593,6 | 1,7415 | 0,496 | 210,384 |
| **4** | 89,608 | 3.949,4 | 2,0256 | 0,445 | 358,432 |
| **5** | 125,442 | 3.585,8 | 2,2310 | 0,429 | 627,210 |
| **6** | 211,225 | 3.363,0 | 2,3788 | 0,377 | 1.267,350 |
| **7** | 684,210 | 3.221,2 | 2,4836 | 0,345 | 4.789,470 |

> **Conclusão Técnica:** Para as condições operacionais estudadas, sistemas entre 3 e 5 efeitos oferecem o melhor compromisso técnico-econômico entre consumo de vapor e área de troca térmica instalada.

---

## 3. Arquitetura de Software

```text
.
├── src/
│   ├── core/                  # Núcleo numérico (sem estado global, C11)
│   │   ├── solver.c           # Algoritmo acoplado SOR e equalização de áreas
│   │   ├── thermodynamics.c   # Modelos termodinâmicos (Antoine, Watson, Perry)
│   │   ├── validation.c       # Checagens físicas e consistência de domínio
│   │   └── types.c            # Inicializadores e utilitários de estado
│   ├── cli/
│   │   └── main.c             # Interface de linha de comando (CLI)
│   └── wasm/
│       └── wasm_api.c         # Camada de integração WebAssembly (Emscripten)
├── include/evaporator/        # Headers públicos e contratos de dados
│   ├── solver.h
│   ├── thermodynamics.h
│   ├── validation.h
│   └── types.h
├── tests/                     # Suite de testes automatizados
│   ├── test_main.c
│   ├── test_thermodynamics.c
│   ├── test_validation.c
│   └── test_solver_regression.c
├── docs/                      # Documentação técnica e científica
│   ├── auditoria-modelo.md
│   ├── modelo-matematico.md
│   ├── arquitetura.md
│   └── validacao-e-testes.md
├── web/                       # Aplicação Web (HTML5/CSS3/Vanilla JS)
│   ├── index.html
│   ├── css/style.css
│   └── js/
│       ├── app.js
│       └── wasm-bridge.js
├── scripts/
│   ├── build_wasm.sh
│   └── run_server.py
├── CMakeLists.txt
└── README.md
```

---

## 4. Compilação e Execução Nativa

### Requisitos
- Compilador C com suporte a **C11** (GCC, Clang ou MSVC);
- **CMake** versão $\ge 3.15$;
- Gerador de build (Ninja ou Make).

### Compilação com CMake
```bash
# Configuração do projeto
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release

# Compilação dos alvos (lib, cli e testes)
cmake --build build
```

### Execução da Suite de Testes
```bash
# Executar suite completa
./build/evap_tests.exe
# ou via CTest
ctest --test-dir build --output-on-failure
```

---

## 5. Uso da Linha de Comando (CLI)

O executável `evap_cli` permite simulações pontuais, varredura de relaxação e reprodução direta do artigo:

```bash
# Simulação padrão de triplo efeito (N=3)
./build/evap_cli.exe -n 3

# Reprodução completa da Tabela 2 do artigo (N=2 a 7)
./build/evap_cli.exe --benchmark

# Simulação com parâmetros customizados
./build/evap_cli.exe -n 4 -F 15000 --xf 0.12 --xn 0.48 --ts 155.0 -w 0.445

# Saída estruturada em JSON (ideal para automações e pipelines)
./build/evap_cli.exe -n 3 --json

# Saída tabular em CSV
./build/evap_cli.exe -n 3 --csv

# Varredura do parâmetro de sub-relaxação SOR (omega)
./build/evap_cli.exe -n 3 --sweep-omega
```

---

## 6. Compilação e Uso do Front-End Web (WebAssembly)

### Compilando o Núcleo C para WebAssembly
Utilize o script automatizado ou invoque o Emscripten (`emcc`):

```bash
bash scripts/build_wasm.sh
```

Isso gerará os artefatos `web/js/evap_solver.js` e `web/js/evap_solver.wasm`.

### Executando o Servidor Localmente
Para carregar o WebAssembly no navegador (evitando restrições de CORS de `file://`), inicie um servidor HTTP local:

```bash
python scripts/run_server.py
# ou usando o módulo nativo do Python:
python -m http.server 8080 --directory web
```
Acesse no navegador: `http://localhost:8080`.

---

## 7. Publicação Científica e Citação

Caso utilize este modelo, código ou resultados em trabalhos acadêmicos ou projetos de engenharia, cite o artigo de referência:

### ABNT
```text
MENDONÇA, Pedro Paulo Nogueira; DUARTE, Cláudio Roberto. Projeto de evaporadores de múltiplo efeito com influência da pressão e EPE. In: XVI Congresso Brasileiro de Engenharia Química em Iniciação Científica (COBEQ-IC). Campinas: UNICAMP, 2026.
```

### BibTeX
```bibtex
@inproceedings{mendonca2026evaporadores,
  author    = {Mendon{\c{c}}a, Pedro Paulo Nogueira and Duarte, Cl{\'a}udio Roberto},
  title     = {Projeto de evaporadores de m{\'u}ltiplo efeito com influ{\^e}ncia da press{\~a}o e EPE},
  booktitle = {Anais do XVI Congresso Brasileiro de Engenharia Qu{\'i}mica em Inicia{\c{c}}{\~a}o Cient{\'i}fica (COBEQ-IC)},
  year      = {2026},
  address   = {Campinas, SP},
  publisher = {UNICAMP}
}
```

---

## 8. Autores e Créditos

- **Pedro Paulo Nogueira Mendonça** — Engenharia Química, UFU (Autor do código original e coautor da publicação).
- **Prof. Dr. Cláudio Roberto Duarte** — Faculdade de Engenharia Química, UFU (Coautor e orientador).

---

## 9. Licença

Todos os direitos do código original e do artigo pertencem aos autores e à Universidade Federal de Uberlândia (UFU). A definição de uma licença de código aberto (ex.: MIT, Apache, GPL) depende de decisão explícita dos titulares dos direitos autorais.

