/**
 * @file app.js
 * @brief Aplicação web para controle interativo e visualização de evaporadores de múltiplo efeito.
 * @author Pedro Paulo Nogueira Mendonça & Cláudio Roberto Duarte (UFU)
 */

document.addEventListener('DOMContentLoaded', async () => {
    // Inicializa renderização do KaTeX para as equações matemáticas
    if (typeof renderMathInElement === 'function') {
        renderMathInElement(document.body, {
            delimiters: [
                { left: '$$', right: '$$', display: true },
                { left: '\\[', right: '\\]', display: true },
                { left: '\\(', right: '\\)', display: false },
                { left: '$', right: '$', display: false }
            ],
            macros: { '\\cdotp': '\\cdot' },
            strict: 'ignore',
            throwOnError: false
        });
    }

    // Configuração do Lightbox para a imagem MultiplosEfeitos.png
    const lightboxModal = document.getElementById('lightbox-modal');
    const lightboxImg = document.getElementById('lightbox-img');
    const figureImgWrap = document.getElementById('figure-img-wrap');
    const lightboxClose = document.getElementById('lightbox-close');

    if (figureImgWrap && lightboxModal && lightboxImg) {
        figureImgWrap.addEventListener('click', () => {
            const src = figureImgWrap.querySelector('img').src;
            lightboxImg.src = src;
            lightboxModal.classList.add('active');
        });

        lightboxModal.addEventListener('click', (e) => {
            if (e.target !== lightboxImg) {
                lightboxModal.classList.remove('active');
            }
        });

        if (lightboxClose) {
            lightboxClose.addEventListener('click', () => {
                lightboxModal.classList.remove('active');
            });
        }
    }

    // Elementos do DOM do Simulador
    const form = document.getElementById('sim-form');
    const btnCalculate = document.getElementById('btn-calculate');
    const btnReset = document.getElementById('btn-reset');
    const statusBox = document.getElementById('sim-status-box');
    const statusText = document.getElementById('status-text');

    const inputEffects = document.getElementById('param-effects');
    const inputFeed = document.getElementById('param-feed');
    const inputXf = document.getElementById('param-xf');
    const inputXn = document.getElementById('param-xn');
    const inputTf = document.getElementById('param-tf');
    const inputTs = document.getElementById('param-ts');
    const inputTsatLast = document.getElementById('param-tsat-last');
    const inputHtc = document.getElementById('param-htc');
    const inputOmega = document.getElementById('param-omega');

    // Elementos de KPI
    const kpiSteam = document.getElementById('kpi-steam');
    const kpiEconomy = document.getElementById('kpi-economy');
    const kpiArea = document.getElementById('kpi-area');
    const kpiTotalArea = document.getElementById('kpi-total-area');
    const kpiDt = document.getElementById('kpi-dt');
    const kpiBpe = document.getElementById('kpi-bpe');

    // Tabela e exportação
    const tableBody = document.querySelector('#effects-table tbody');
    const btnExportJson = document.getElementById('btn-export-json');
    const btnExportCsv = document.getElementById('btn-export-csv');

    // Canvas
    const canvasTemp = document.getElementById('chart-temperature');
    const canvasFlow = document.getElementById('chart-flows');

    const CHART_FONT = '10px "JetBrains Mono", monospace';

    let currentSimulationResult = null;

    // Inicialização da ponte WebAssembly C
    setStatus('running', 'Carregando núcleo numérico WebAssembly (C11)...');
    const wasmLoaded = await window.EvaporatorBridge.init();

    if (wasmLoaded) {
        setStatus('ready', 'Núcleo numérico compilado em C carregado e pronto para simulação.');
        if (btnCalculate) btnCalculate.disabled = false;
        // Executa simulação inicial automática para preencher os gráficos e tabela
        executeSimulation();
    } else {
        setStatus('error', 'Módulo WebAssembly indisponível. Para executar simulações, compile o núcleo ou consulte a Tabela 2 abaixo.');
        if (btnCalculate) btnCalculate.disabled = true;
    }

    // Eventos de formulário
    if (form) {
        form.addEventListener('submit', (e) => {
            e.preventDefault();
            executeSimulation();
        });
    }

    if (btnReset) {
        btnReset.addEventListener('click', () => {
            resetToDefaults();
            if (window.EvaporatorBridge.isReady) {
                executeSimulation();
            }
        });
    }

    if (btnExportJson) {
        btnExportJson.addEventListener('click', exportJson);
    }

    if (btnExportCsv) {
        btnExportCsv.addEventListener('click', exportCsv);
    }

    // ResizeObserver para redesenhar gráficos responsivamente sempre que a tela mudar de largura
    if (window.ResizeObserver && canvasTemp && canvasFlow) {
        const resizeObserver = new ResizeObserver(() => {
            if (currentSimulationResult) {
                drawTemperatureChart(currentSimulationResult);
                drawFlowChart(currentSimulationResult);
            }
        });
        if (canvasTemp.parentElement) resizeObserver.observe(canvasTemp.parentElement);
        if (canvasFlow.parentElement) resizeObserver.observe(canvasFlow.parentElement);
    }

    function setStatus(type, message) {
        if (!statusBox || !statusText) return;
        statusBox.className = 'sim-status-box status-' + type;
        statusText.textContent = message;
    }

    function resetToDefaults() {
        if (inputEffects) inputEffects.value = 3;
        if (inputFeed) inputFeed.value = 10000;
        if (inputXf) inputXf.value = 10;
        if (inputXn) inputXn.value = 50;
        if (inputTf) inputTf.value = 50;
        if (inputTs) inputTs.value = 160;
        if (inputTsatLast) inputTsatLast.value = 60;
        if (inputHtc) inputHtc.value = 2000;
        if (inputOmega) inputOmega.value = 0.496;
    }

    function getFormParameters() {
        return {
            n_effects: parseInt(inputEffects.value, 10),
            feed_rate_kg_h: parseFloat(inputFeed.value),
            feed_conc: parseFloat(inputXf.value) / 100.0,
            product_conc: parseFloat(inputXn.value) / 100.0,
            feed_temp_c: parseFloat(inputTf.value),
            steam_temp_c: parseFloat(inputTs.value),
            last_sat_temp_c: parseFloat(inputTsatLast.value),
            htc_w_m2k: parseFloat(inputHtc.value),
            omega: parseFloat(inputOmega.value)
        };
    }

    function executeSimulation() {
        if (!window.EvaporatorBridge.isReady) {
            setStatus('error', 'O módulo WebAssembly não está carregado.');
            return;
        }

        const params = getFormParameters();

        // Validação no front-end
        if (params.feed_conc >= params.product_conc) {
            setStatus('error', 'Concentração final (xN) deve ser estritamente maior que inicial (xF).');
            return;
        }

        if (params.steam_temp_c <= params.last_sat_temp_c + 15) {
            setStatus('error', 'Temperatura do vapor vivo (TS) insuficiente para gradiente útil.');
            return;
        }

        setStatus('running', 'Calculando balanços acoplados e equalização de áreas...');

        try {
            const startTime = performance.now();
            const res = window.EvaporatorBridge.simulate(params);
            const elapsed = (performance.now() - startTime).toFixed(1);

            currentSimulationResult = res;
            renderResults(res);
            setStatus('ready', `Convergido em ${elapsed} ms (${res.iter_outer} ciclos de área, ${res.iter_inner_total} iterações SOR).`);
        } catch (err) {
            setStatus('error', err.message);
        }
    }

    function formatNumber(val, decimals = 2) {
        if (val === null || val === undefined || isNaN(val)) return '-';
        return val.toLocaleString('pt-BR', {
            minimumFractionDigits: decimals,
            maximumFractionDigits: decimals
        });
    }

    function renderResults(res) {
        if (kpiSteam) kpiSteam.textContent = formatNumber(res.steam_rate_kg_h, 1);
        if (kpiEconomy) kpiEconomy.textContent = formatNumber(res.vapor_economy, 4);
        if (kpiArea) kpiArea.textContent = formatNumber(res.area_per_effect_m2, 3);
        if (kpiTotalArea) kpiTotalArea.textContent = formatNumber(res.total_area_m2, 2);
        if (kpiDt) kpiDt.textContent = formatNumber(res.total_delta_t_c, 1);
        if (kpiBpe) kpiBpe.textContent = formatNumber(res.total_bpe_c, 1);

        // Preenche Tabela
        if (tableBody) {
            tableBody.innerHTML = '';
            res.effects.forEach((ef) => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><strong>${ef.effect}</strong></td>
                    <td>${formatNumber(ef.vapor_rate_kg_h, 1)}</td>
                    <td>${formatNumber(ef.liquid_rate_kg_h, 1)}</td>
                    <td>${formatNumber(ef.concentration * 100, 2)}%</td>
                    <td>${formatNumber(ef.temperature_c, 2)}</td>
                    <td>${formatNumber(ef.sat_temperature_c, 2)}</td>
                    <td>${formatNumber(ef.pressure_kpa, 2)}</td>
                    <td>${formatNumber(ef.bpe_c, 2)}</td>
                    <td>${formatNumber(ef.delta_t_c, 2)}</td>
                    <td><strong>${formatNumber(ef.area_m2, 3)}</strong></td>
                `;
                tableBody.appendChild(tr);
            });
        }

        // Desenha Gráficos Canvas
        drawTemperatureChart(res);
        drawFlowChart(res);
    }

    /* Legenda com quebra de linha: mede o texto para que os itens nunca se sobreponham */
    function layoutLegend(ctx, items, x0, x1) {
        ctx.font = CHART_FONT;
        let x = x0, row = 0;
        const pos = items.map((it) => {
            const w = 12 + ctx.measureText(it.text).width;
            if (x > x0 && x + w > x1) { row++; x = x0; }
            const p = { x, row, color: it.color, text: it.text };
            x += w + 16;
            return p;
        });
        return { pos, rows: row + 1 };
    }

    function drawLegend(ctx, lay) {
        ctx.font = CHART_FONT;
        lay.pos.forEach((p) => {
            const y = 8 + p.row * 16;
            ctx.fillStyle = p.color;
            ctx.fillRect(p.x, y, 8, 8);
            ctx.fillStyle = '#48525E';
            ctx.fillText(p.text, p.x + 12, y + 8);
        });
    }

    function drawEffectLabel(ctx, i, n, x, y, spacing) {
        if (spacing < 42 && n > 4 && i % 2 === 1) return;
        ctx.fillStyle = '#1C2024';
        ctx.textAlign = 'center';
        ctx.fillText(`Ef. ${i + 1}`, x, y);
        ctx.textAlign = 'left';
    }

    /* Gráfico 1: Perfis Térmicos Responsivo */
    function drawTemperatureChart(res) {
        if (!canvasTemp || !canvasTemp.parentElement) return;
        const parent = canvasTemp.parentElement;
        const dw = parent.clientWidth || 400;
        const dh = parent.clientHeight || 240;

        const dpr = window.devicePixelRatio || 1;
        canvasTemp.width = dw * dpr;
        canvasTemp.height = dh * dpr;

        const ctx = canvasTemp.getContext('2d');
        ctx.resetTransform ? ctx.resetTransform() : ctx.setTransform(1, 0, 0, 1, 0, 0);
        ctx.scale(dpr, dpr);
        ctx.clearRect(0, 0, dw, dh);

        const padLeft = 45, padRight = 20, padBottom = 35;
        const legendT = layoutLegend(ctx, [
            { color: '#A3361E', text: 'T da solução (com EPE)' },
            { color: '#143D66', text: 'T sat. (água pura)' }
        ], padLeft, dw - padRight);
        const padTop = 18 + legendT.rows * 16;
        const plotW = Math.max(dw - padLeft - padRight, 50);
        const plotH = Math.max(dh - padTop - padBottom, 50);

        const n = res.effects.length;
        const tSol = res.effects.map(e => e.temperature_c);
        const tSat = res.effects.map(e => e.sat_temperature_c);
        const maxT = Math.max(...tSol, 160) + 10;
        const minT = Math.min(...tSat) - 10;

        const getY = (val) => padTop + plotH - ((val - minT) / (maxT - minT)) * plotH;
        const getX = (idx) => padLeft + (idx / Math.max(n - 1, 1)) * plotW;

        // Grade e Eixos
        ctx.strokeStyle = '#E5E2DC';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#727D8C';
        ctx.font = '10px "JetBrains Mono", monospace';

        for (let t = Math.ceil(minT / 20) * 20; t <= maxT; t += 20) {
            const y = getY(t);
            ctx.beginPath();
            ctx.moveTo(padLeft, y);
            ctx.lineTo(dw - padRight, y);
            ctx.stroke();
            ctx.fillText(t + '°C', 8, y + 3);
        }

        // Curva Tsat (Água Pura)
        ctx.beginPath();
        ctx.strokeStyle = '#143D66';
        ctx.lineWidth = 2;
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            const y = getY(tSat[i]);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        ctx.stroke();

        // Curva Tsol (Solução com EPE)
        ctx.beginPath();
        ctx.strokeStyle = '#A3361E';
        ctx.lineWidth = 2;
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            const y = getY(tSol[i]);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        ctx.stroke();

        // Pontos e rótulos
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            ctx.fillStyle = '#143D66';
            ctx.beginPath();
            ctx.arc(x, getY(tSat[i]), 3.5, 0, Math.PI * 2);
            ctx.fill();

            ctx.fillStyle = '#A3361E';
            ctx.beginPath();
            ctx.arc(x, getY(tSol[i]), 3.5, 0, Math.PI * 2);
            ctx.fill();

            drawEffectLabel(ctx, i, n, x, dh - 10, plotW / Math.max(n - 1, 1));
        }

        drawLegend(ctx, legendT);
    }

    /* Gráfico 2: Vazões Mássicas Responsivo */
    function drawFlowChart(res) {
        if (!canvasFlow || !canvasFlow.parentElement) return;
        const parent = canvasFlow.parentElement;
        const dw = parent.clientWidth || 400;
        const dh = parent.clientHeight || 240;

        const dpr = window.devicePixelRatio || 1;
        canvasFlow.width = dw * dpr;
        canvasFlow.height = dh * dpr;

        const ctx = canvasFlow.getContext('2d');
        ctx.resetTransform ? ctx.resetTransform() : ctx.setTransform(1, 0, 0, 1, 0, 0);
        ctx.scale(dpr, dpr);
        ctx.clearRect(0, 0, dw, dh);

        const padLeft = 55, padRight = 20, padBottom = 35;
        const legendF = layoutLegend(ctx, [
            { color: '#143D66', text: 'Vapor V (kg/h)' },
            { color: '#1C5438', text: 'Líquido L (kg/h)' }
        ], padLeft, dw - padRight);
        const padTop = 18 + legendF.rows * 16;
        const plotW = Math.max(dw - padLeft - padRight, 50);
        const plotH = Math.max(dh - padTop - padBottom, 50);

        const n = res.effects.length;
        const maxFlow = Math.max(...res.effects.map(e => Math.max(e.liquid_rate_kg_h, e.vapor_rate_kg_h))) * 1.15;

        const getY = (val) => padTop + plotH - (val / maxFlow) * plotH;

        // Grade
        ctx.strokeStyle = '#E5E2DC';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#727D8C';
        ctx.font = '10px "JetBrains Mono", monospace';

        const step = Math.ceil((maxFlow / 4) / 1000) * 1000;
        for (let f = 0; f <= maxFlow; f += step) {
            const y = getY(f);
            ctx.beginPath();
            ctx.moveTo(padLeft, y);
            ctx.lineTo(dw - padRight, y);
            ctx.stroke();
            ctx.fillText(f.toLocaleString('pt-BR'), 8, y + 3);
        }

        const barGroupW = plotW / n;
        const barW = Math.min(barGroupW * 0.35, 22);

        res.effects.forEach((ef, i) => {
            const groupX = padLeft + i * barGroupW + (barGroupW - barW * 2 - 4) / 2;

            // Barra V (Vapor)
            const yV = getY(ef.vapor_rate_kg_h);
            ctx.fillStyle = '#143D66';
            ctx.fillRect(groupX, yV, barW, padTop + plotH - yV);

            // Barra L (Líquido)
            const yL = getY(ef.liquid_rate_kg_h);
            ctx.fillStyle = '#1C5438';
            ctx.fillRect(groupX + barW + 4, yL, barW, padTop + plotH - yL);

            drawEffectLabel(ctx, i, n, padLeft + (i + 0.5) * barGroupW, dh - 10, barGroupW);
        });

        drawLegend(ctx, legendF);
    }

    function exportJson() {
        if (!currentSimulationResult) return;
        const blob = new Blob([JSON.stringify(currentSimulationResult, null, 2)], { type: 'application/json' });
        downloadBlob(blob, `simulacao_evaporadores_N${currentSimulationResult.n_effects}.json`);
    }

    function exportCsv() {
        if (!currentSimulationResult) return;
        let csv = 'efeito,vapor_kg_h,liquido_kg_h,conc_massa,temp_c,temp_sat_c,pressao_kpa,epe_c,delta_t_c,area_m2\n';
        currentSimulationResult.effects.forEach(e => {
            csv += `${e.effect},${e.vapor_rate_kg_h.toFixed(2)},${e.liquid_rate_kg_h.toFixed(2)},${e.concentration.toFixed(4)},${e.temperature_c.toFixed(2)},${e.sat_temperature_c.toFixed(2)},${e.pressure_kpa.toFixed(2)},${e.bpe_c.toFixed(2)},${e.delta_t_c.toFixed(2)},${e.area_m2.toFixed(3)}\n`;
        });
        const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
        downloadBlob(blob, `simulacao_evaporadores_N${currentSimulationResult.n_effects}.csv`);
    }

    function downloadBlob(blob, filename) {
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        link.click();
        URL.revokeObjectURL(link.href);
    }
});