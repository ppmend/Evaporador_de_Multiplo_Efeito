# Evaporadores de Múltiplo Efeito — Simulador Numérico e Site

Núcleo numérico em C11, compilado para WebAssembly, que dimensiona evaporadores de múltiplo efeito em série com alimentação direta (*forward feed*). O modelo considera pressão variável entre os efeitos, elevação do ponto de ebulição (EPE) de soluções de NaOH e áreas de troca térmica iguais em todos os efeitos. O site apresenta o artigo, o modelo matemático e um simulador que roda no navegador.

Trabalho da Faculdade de Engenharia Química da UFU, publicado no XVI COBEQ-IC (UNICAMP, 2026). O artigo completo está em [`artigo-cobeqic-evaporador-multiplo-efeito.pdf`](artigo-cobeqic-evaporador-multiplo-efeito.pdf).

## Conteúdo do repositório

```text
.
├── artigo-cobeqic-evaporador-multiplo-efeito.pdf   # artigo publicado
├── web/                    # site e simulador
│   ├── index.html
│   ├── css/style.css
│   ├── assets/MultiplosEfeitos.png
│   └── js/
│       ├── app.js          # interface, gráficos e exportação
│       ├── wasm-bridge.js  # ponte JavaScript <-> WebAssembly
│       ├── evap_solver.js  # módulo gerado pelo Emscripten
│       └── evap_solver.wasm
├── src/
│   ├── core/               # solver, termodinâmica, validação e tipos (C11)
│   └── wasm/wasm_api.c     # funções exportadas para o navegador
├── include/evaporator/     # cabeçalhos do núcleo
└── scripts/build_wasm.sh   # recompila o núcleo para WebAssembly
```

## Executar o site

O WebAssembly não carrega via `file://`. Na raiz do repositório, inicie um servidor HTTP:

```bash
python -m http.server 8080
```

Abra `http://localhost:8080/web/`. O servidor deve partir da raiz porque o botão de download aponta para o PDF que fica ao lado da pasta `web/`.

## Recompilar o WebAssembly

Só é necessário ao alterar o código C. Requer o [Emscripten](https://emscripten.org/):

```bash
bash scripts/build_wasm.sh
```

O script regenera `web/js/evap_solver.js` e `web/js/evap_solver.wasm`.

## Caso-base e resultados do artigo

Condições: $F = 10.000\ \text{kg/h}$, $x_F = 10\%$, $x_N = 50\%$, $T_F = 50\ °\text{C}$, $T_S = 160\ °\text{C}$, $T_{sat,N} = 60\ °\text{C}$ e $U = 2.000\ \text{W/(m}^2\text{·K)}$. O simulador reproduz a Tabela 2 do artigo:

| Efeitos ($N$) | Área por efeito (m²) | Vapor vivo $S$ (kg/h) | Economia $E$ | $\omega$ ótimo | Área total (m²) |
|:---:|:---:|:---:|:---:|:---:|:---:|
| 2 | 58,083 | 5.940,4 | 1,3467 | 0,665 | 116,166 |
| 3 | 70,128 | 4.593,6 | 1,7415 | 0,496 | 210,384 |
| 4 | 89,608 | 3.949,4 | 2,0256 | 0,445 | 358,432 |
| 5 | 125,442 | 3.585,8 | 2,2310 | 0,429 | 627,210 |
| 6 | 211,225 | 3.363,0 | 2,3788 | 0,377 | 1.267,350 |
| 7 | 684,210 | 3.221,2 | 2,4836 | 0,345 | 4.789,470 |

## Citação

```text
MENDONÇA, Pedro Paulo Nogueira; DUARTE, Cláudio Roberto. Projeto de evaporadores de múltiplo efeito com influência da pressão e EPE. In: XVI Congresso Brasileiro de Engenharia Química em Iniciação Científica (COBEQ-IC). Campinas: UNICAMP, 2026.
```

## Autores

- [Pedro Paulo Nogueira Mendonça](https://www.linkedin.com/in/pedro-paulo-nogueira-mendon%C3%A7a-866821219/) — UFU. Algoritmo, implementação em C e interface web.
- Prof. Dr. Cláudio Roberto Duarte — UFU. Orientador.

## Licença

Todos os direitos do código e do artigo pertencem aos autores e à Universidade Federal de Uberlândia. Nenhuma licença de código aberto foi definida.
