---
name: portfolio-frontend-design
description: Diretrizes de design e desenvolvimento para interfaces web de Engenharia Química e Computação Científica
---

# Diretrizes de Design de Front-End — Portfólio Científico

Esta skill orienta o desenvolvimento de interfaces de produtos digitais, portfolios e ferramentas de simulação para Engenharia Química e Ciência da Computação.

## 1. Princípios de Direção de Arte
- **Identidade:** Caderno de campo + artigo científico contemporâneo + painel de engenharia de processos.
- **Paleta de Cores:**
  - Fundo principal: Bege técnico / papel marfim (#F8F7F4).
  - Superfícies escuras (Hero e cabeçalhos): Azul petróleo profundo (#132230 ou #1B2838).
  - Texto primário: Cinza grafite de alto contraste (#1A1A1A em superfícies claras, #F3EFEA em escuras).
  - Destaque primário: Azul técnico (#1E5F8C).
  - Destaque secundário: Cobre de processo / terracota (#C4592A) para CTAs e pontos quentes.
  - Dados numéricos / Monospace: Verde petróleo (#235338).
  - Bordas e divisores: #D5D0C8 (linhas limpas, 1px).
- **Tipografia:**
  - Títulos e Destaques: `DM Serif Display`, `Georgia`, serif (tradição acadêmica e elegância).
  - Texto de Leitura: `Inter`, system-ui, sans-serif (legibilidade neutra e moderna).
  - Variáveis, tabelas e dados: `JetBrains Mono`, `Fira Code`, monospace (precisão dimensional).

## 2. Acessibilidade e Semântica (WCAG 2.2 AA)
- Rótulos explícitos (`<label for="...">`) para todos os campos do simulador.
- Estados visuais evidentes de foco (`:focus-visible`).
- Região viva com `aria-live="polite"` para atualização do estado de cálculo do solver.
- Suporte a `prefers-reduced-motion`.
- Todas as tabelas com cabeçalhos semânticos (`<th>`).
- Diagramas e gráficos com alternativas textuais.

## 3. Integridade dos Dados no Simulador
- O simulador deve apenas reportar resultados da simulação real executada pelo solver C via WebAssembly ou indicar claramente o estado de indisponibilidade.
- Resultados da literatura (COBEQ-IC 2026) devem ser mantidos em seção histórica separada, nunca mascarados como novas execuções.

