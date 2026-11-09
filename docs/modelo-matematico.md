# Modelo Matemático e Termodinâmico

**Projeto:** Dimensionamento de Evaporadores de Múltiplo Efeito em Série com Pressão Variável e EPE  
**Referência:** MENDONÇA, Pedro Paulo Nogueira; DUARTE, Cláudio Roberto. *Projeto de evaporadores de múltiplo efeito com influência da pressão e EPE*. XVI COBEQ-IC, UNICAMP, 2026.  

---

## 1. Introdução e Nomenclatura

O sistema modelado consiste em uma bateria de $N$ evaporadores em série com alimentação direta (*forward feed*). A solução entra no primeiro efeito junto com o vapor vivo de caldeira ($S$) e escoa sequencialmente até o último efeito, de onde sai a solução concentrada ($L_N$). O vapor gerado em cada estágio ($V_i$) atua como vapor de aquecimento para o estágio subsequente ($i+1$).

### Tabela de Nomenclatura e Notação

| Símbolo | Unidade | Descrição |
|---|:---:|---|
| $N$ | - | Número total de efeitos na associação ($2 \le N \le 7$) |
| $F$ | kg/h | Vazão mássica da corrente de alimentação |
| $x_F$ | kg soluto/kg sol. | Fração mássica de NaOH na alimentação |
| $x_N$ | kg soluto/kg sol. | Fração mássica de NaOH no produto concentrado final |
| $T_F$ | °C | Temperatura da alimentação |
| $T_S$ | °C | Temperatura de saturação do vapor vivo da caldeira |
| $S$ | kg/h | Consumo mássico de vapor vivo no primeiro efeito |
| $V_i$ | kg/h | Vazão mássica de vapor gerado no efeito $i$ ($i=1,\ldots,N$) |
| $L_i$ | kg/h | Vazão mássica de solução líquida saindo do efeito $i$ |
| $x_i$ | kg soluto/kg sol. | Fração mássica de soluto (NaOH) na solução do efeito $i$ |
| $P_i$ | kPa | Pressão absoluta de operação na câmara de vapor do efeito $i$ |
| $T_{sat,i}$ | °C | Temperatura de saturação da água pura na pressão $P_i$ |
| $T_i$ | °C | Temperatura de ebulição da solução no efeito $i$ |
| $EPE_i$ | °C | Elevação do Ponto de Ebulição no efeito $i$ ($EPE_i = T_i - T_{sat,i}$) |
| $\Delta T_i$ | °C | Diferença útil de temperatura (força motriz térmica) no efeito $i$ |
| $U_i$ | W/(m²·K) | Coeficiente global de transferência de calor no efeito $i$ |
| $A_i$ | m² | Área de troca térmica da superfície de aquecimento do efeito $i$ |
| $Q_i$ | kJ/h (ou kW) | Carga térmica transferida no efeito $i$ |
| $\omega$ | - | Fator de sub-relaxação do método SOR ($0 < \omega < 1$) |

---

## 2. Balanços Materiais

### 2.1. Balanço Global de Massa por Efeito
Para o efeito $i$ genérico ($i = 1, \ldots, N$), considerando regime permanente e ausência de arraste:
$$L_i + V_i = L_{i-1}$$
com a condição de contorno para o primeiro efeito:
$$L_0 = F, \quad x_0 = x_F$$

### 2.2. Balanço de Soluto (NaOH)
Considerando soluto estritamente não volátil (o vapor gerado é água pura):
$$L_i x_i = L_{i-1} x_{i-1} = F x_F$$
Logo, para qualquer estágio $i$:
$$x_i = \frac{F x_F}{L_i}$$
E para o último efeito ($i=N$), a vazão final de concentrado é dada diretamente por:
$$L_N = \frac{F x_F}{x_N}$$

A taxa total de água evaporada em toda a bateria é:
$$V_{total} = \sum_{i=1}^N V_i = F - L_N = F \left(1 - \frac{x_F}{x_N}\right)$$

---

## 3. Balanços de Energia e Transferência de Calor

### 3.1. Primeiro Efeito ($i = 1$)
O aquecimento é realizado pelo vapor vivo saturado da caldeira que se condensa a $T_S$:
$$S \, \Delta H_{vap}(T_S) + F \, H_L(T_F, x_F) = V_1 \, h_{v,1} + L_1 \, H_L(T_1, x_1)$$
onde:
- $\Delta H_{vap}(T_S)$ é o calor latente de vaporização da água na temperatura $T_S$ [kJ/kg];
- $H_L(T, x)$ é a entalpia específica da solução aquosa de NaOH [kJ/kg];
- $h_{v,1}$ é a entalpia específica do vapor superaquecido a $T_1$ e $P_1$ [kJ/kg].

Isolando o consumo de vapor vivo $S$:
$$S = \frac{V_1 h_{v,1} + L_1 H_L(T_1, x_1) - F H_L(T_F, x_F)}{\Delta H_{vap}(T_S)}$$

### 3.2. Efeitos Intermediários ($i = 2, \ldots, N-1$)
O calor latente liberado pela condensação do vapor $V_{i-1}$ na temperatura de saturação $T_{sat,i-1}$ supre a carga térmica:
$$V_{i-1} \, \Delta H_{vap}(T_{sat,i-1}) + L_{i-1} \, H_L(T_{i-1}, x_{i-1}) = V_i \, h_{v,i} + L_i \, H_L(T_i, x_i)$$

### 3.3. Forças Motrizes Térmicas ($\Delta T_i$)
As diferenças locais de temperatura que promovem a transferência de calor são:
$$\Delta T_1 = T_S - T_1$$
$$\Delta T_i = T_{sat,i-1} - T_i \quad (i = 2, \ldots, N)$$

### 3.4. Condição Construtiva de Igualdade de Áreas
$$A_i = \frac{Q_i}{U_i \Delta T_i \cdot 3,6}$$
$$A_1 = A_2 = \cdots = A_N$$
O fator $3,6$ decorre da compatibilização entre $Q$ em kJ/h, $U$ em W/(m²·K) e $A$ em m².

---

## 4. Modelos e Correlações Termodinâmicas

### 4.1. Pressão e Temperatura de Saturação da Água (Antoine)
$$\log_{10} P_{mmHg} = A - \frac{B}{T + C}, \quad P_{kPa} = 0,133322 \cdot P_{mmHg}$$
- Para $T \le 100$ °C: $A = 8,07131$, $B = 1730,63$, $C = 233,426$.
- Para $T > 100$ °C: $A = 8,14019$, $B = 1810,94$, $C = 244,485$.

### 4.2. Calor Latente de Vaporização (Watson)
$$\Delta H_{vap}(T) = 2256,47 \left[ \frac{1 - (T + 273,15)/647,096}{1 - 373,15/647,096} \right]^{0,38} \quad [\text{kJ/kg}]$$

### 4.3. Entalpia do Vapor Saturado e Superaquecido
$$H_{vsat}(T) = 2500,9 + 1,805 T - 2,1 \times 10^{-4} T^2 + 3,5 \times 10^{-7} T^3 \quad [\text{kJ/kg}]$$
$$h_{v}(T, P) = H_{vsat}(T_{sat}) + \int_{T_{sat}}^T C_p(T') dT'$$

### 4.4. Temperatura de Ebulição da Solução de NaOH (Antoine Modificado)
$$T = -C + \frac{B(x)}{\log_{10}(P) - A(x)}$$
com $C = 190,80153$ e polinômios cúbicos na concentração $x$:
- $A(x) = 3,62321 x^3 + 1,13082 x^2 - 0,71947 x + 6,77113$
- $B(x) = -2711,78458 x^3 - 11,18678 x^2 + 47,44337 x - 1395,64615$

### 4.5. Entalpia da Solução Aquosa de NaOH
A partir dos dados do Perry's Handbook ajustados em unidades imperiais ($T_F$ em °F, $H$ em BTU/lbm):
$$T_F = 32 + \frac{9}{5} T_C$$
$$H_{BTU/lbm} = z_0 + a x + b T_F + c x^2 + d T_F^2 + f x T_F$$
$$H_L [\text{kJ/kg}] = 2,326 \cdot H_{BTU/lbm}$$
com $z_0 = -26,01021$, $a = -207,89085$, $b = 0,99923$, $c = 795,64522$, $d = -1,27127 \times 10^{-5}$, $f = -0,47635$.

