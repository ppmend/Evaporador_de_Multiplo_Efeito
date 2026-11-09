# Protocolo de Validação, Testes Automatizados e Reprodutibilidade

**Projeto:** Dimensionamento de Evaporadores de Múltiplo Efeito  
**Framework de Testes:** Suite nativa C com runner de integração CTest  

---

## 1. Níveis de Testes Implementados

A integridade do software foi testada em 3 níveis complementares:

### Nível 1: Testes Termodinâmicos Unitários (`tests/test_thermodynamics.c`)
- **Pressão de Saturação (Antoine):** $P_{sat}(100 \text{ °C}) \approx 101,325 \text{ kPa}$ (erro verificado $< 0,1\%$).
- **Temperatura de Saturação Inversa:** $T_{sat}(101,325 \text{ kPa}) \approx 100,0 \text{ °C}$ (erro verificado $< 0,05 \text{ °C}$).
- **Inversibilidade Mútua:** $T_{sat}(P_{sat}(T)) \equiv T$ em $60 \text{ °C}$ e $140 \text{ °C}$.
- **Calor Latente de Watson:** $\Delta H_{vap}(100 \text{ °C}) = 2256,47 \text{ kJ/kg}$ e monotonia decrescente com $T$.
- **Entalpia de Vapor:** $H_{vsat}(100 \text{ °C}) \approx 2676 \text{ kJ/kg}$.
- **Entalpia de Solução de NaOH:** Monotonia estritamente crescente com a temperatura em concentração fixa.
- **Elevação do Ponto de Ebulição (EPE):** Estritamente positiva para soluções de NaOH e crescente com a concentração (em $x = 0,50$, $EPE \approx 42 \text{ °C}$).

### Nível 2: Testes de Validação e Robustez de Limites (`tests/test_validation.c`)
- Rejeição de número de efeitos fora de $[2, 20]$.
- Rejeição de vazões negativas ou nulas.
- Rejeição de concentrações inconsistentes ($x_F \ge x_N$ ou $x_N \ge 0,70$).
- Rejeição de gradiente térmico global inviável ($T_S - T_{sat,N} < 15 \text{ °C}$).
- Rejeição de fator de relaxação $\omega$ fora do intervalo $(0, 1)$.

### Nível 3: Testes de Regressão da Literatura (`tests/test_solver_regression.c`)
Confronto direto com todos os 6 casos de estudo da Tabela 2 do artigo do XVI COBEQ-IC (2026):
- Para cada $N \in \{2, 3, 4, 5, 6, 7\}$, são verificadas:
  1. Área de troca térmica calculada vs publicada (erro relativo $< 0,15\%$);
  2. Consumo de vapor vivo $S$ calculado vs publicado (erro relativo $< 0,15\%$);
  3. Economia de vapor $E$ calculada vs publicada (erro relativo $< 0,15\%$);
  4. Igualdade rigorosa de áreas entre todos os efeitos: $\max |A_i - A_j| < 10^{-4} \text{ m}^2$;
  5. Fechamento do balanço de massa global: $|F - (\sum V_i + L_N)| < 10^{-4} \text{ kg/h}$;
  6. Fechamento do balanço de soluto: $|F x_F - L_N x_N| < 10^{-4} \text{ kg/h}$.

---

## 2. Como Executar os Testes

```powershell
# Configuração do build com CMake e Ninja
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Execução da suite de testes
./build/evap_tests.exe
# ou via CTest
ctest --test-dir build --output-on-failure
```

