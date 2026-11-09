/**
 * @file app.js
 * @brief Aplicação web para controle interativo e visualização de evaporadores de múltiplo efeito.
 */

document.addEventListener('DOMContentLoaded', async () => {
    // Referências do DOM
    const form = document.getElementById('sim-form');
    const btnCalculate = document.getElementById('btn-calculate');
    const btnReset = document.getElementById('btn-reset');
    const statusBox = document.getElementById('sim-status');
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
    const omegaVal = document.getElementById('omega-val');
    
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

    let currentSimulationResult = null;

    // Atualização do slider de ômega
    if (inputOmega && omegaVal) {
        inputOmega.addEventListener('input', (e) => {
            omegaVal.textContent = parseFloat(e.target.value).toFixed(3);
        });
    }

    // Inicialização da ponte WebAssembly
    setStatus('running', 'Inicializando núcleo de cálculo WebAssembly...');
    const wasmLoaded = await window.EvaporatorBridge.init();

    if (wasmLoaded) {
        setStatus('ready', 'Núcleo numérico C (WebAssembly) carregado e pronto para simulação.');
        btnCalculate.disabled = false;
        // Executa simulação inicial automática para preencher os gráficos e a tabela
        executeSimulation();
    } else {
        setStatus('error', 'WebAssembly não disponível localmente. Para executar simulações personalizadas no navegador, compile com Emscripten ou consulte a tabela histórica de 2026 abaixo.');
        btnCalculate.disabled = true;
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

    function setStatus(type, message) {
        if (!statusBox || !statusText) return;
        statusBox.className = 'sim-status status-' + type;
        statusText.textContent = message;
    }

    function resetToDefaults() {
        inputEffects.value = 3;
        inputFeed.value = 10000;
        inputXf.value = 10;
        inputXn.value = 50;
        inputTf.value = 50;
        inputTs.value = 160;
        inputTsatLast.value = 60;
        inputHtc.value = 2000;
        inputOmega.value = 0.496;
        omegaVal.textContent = "0.496";
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
            setStatus('error', 'A concentração final (xN) deve ser estritamente maior que a inicial (xF).');
            return;
        }

        if (params.steam_temp_c <= params.last_sat_temp_c + 10) {
            setStatus('error', 'A temperatura do vapor vivo (TS) deve ser significativamente superior à temperatura do último efeito.');
            return;
        }

        setStatus('running', 'Calculando balanços e equalização de áreas...');

        try {
            const startTime = performance.now();
            const res = window.EvaporatorBridge.simulate(params);
            const elapsed = (performance.now() - startTime).toFixed(1);

            currentSimulationResult = res;
            renderResults(res);
            setStatus('ready', `Convergido em ${elapsed} ms (${res.iter_outer} iterações de área, ${res.iter_inner_total} SOR).`);
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
        // Atualiza KPIs
        kpiSteam.textContent = formatNumber(res.steam_rate_kg_h, 1);
        kpiEconomy.textContent = formatNumber(res.vapor_economy, 4);
        kpiArea.textContent = formatNumber(res.area_per_effect_m2, 3);
        kpiTotalArea.textContent = formatNumber(res.total_area_m2, 2);
        kpiDt.textContent = formatNumber(res.total_delta_t_c, 1);
        kpiBpe.textContent = formatNumber(res.total_bpe_c, 1);

        // Preenche Tabela
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

        // Desenha Gráficos Canvas
        drawTemperatureChart(res);
        drawFlowChart(res);
    }

    /* Gráfico 1: Perfil Térmico (T de ebulição vs Tsat pura) */
    function drawTemperatureChart(res) {
        if (!canvasTemp) return;
        const ctx = canvasTemp.getContext('2d');
        const w = canvasTemp.width = canvasTemp.offsetWidth * window.devicePixelRatio;
        const h = canvasTemp.height = canvasTemp.offsetHeight * window.devicePixelRatio;
        ctx.scale(window.devicePixelRatio, window.devicePixelRatio);

        const dw = canvasTemp.offsetWidth;
        const dh = canvasTemp.offsetHeight;
        ctx.clearRect(0, 0, dw, dh);

        const padLeft = 45, padRight = 20, padTop = 25, padBottom = 35;
        const plotW = dw - padLeft - padRight;
        const plotH = dh - padTop - padBottom;

        const n = res.effects.length;
        const tSol = res.effects.map(e => e.temperature_c);
        const tSat = res.effects.map(e => e.sat_temperature_c);
        const maxT = Math.max(...tSol, res.steam_rate_kg_h ? 170 : 150) + 10;
        const minT = Math.min(...tSat) - 10;

        const getY = (val) => padTop + plotH - ((val - minT) / (maxT - minT)) * plotH;
        const getX = (idx) => padLeft + (idx / (n - 1 || 1)) * plotW;

        // Eixos e grade
        ctx.strokeStyle = '#E2DDD5';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#7E8695';
        ctx.font = '10px JetBrains Mono';

        for (let t = Math.ceil(minT / 20) * 20; t <= maxT; t += 20) {
            const y = getY(t);
            ctx.beginPath();
            ctx.moveTo(padLeft, y);
            ctx.lineTo(dw - padRight, y);
            ctx.stroke();
            ctx.fillText(t + '°C', 5, y + 3);
        }

        // Curva Tsat (Água Pura)
        ctx.beginPath();
        ctx.strokeStyle = '#1A5F8E';
        ctx.lineWidth = 2.5;
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            const y = getY(tSat[i]);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        ctx.stroke();

        // Curva Tsol (Solução com EPE)
        ctx.beginPath();
        ctx.strokeStyle = '#C25425';
        ctx.lineWidth = 2.5;
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            const y = getY(tSol[i]);
            if (i === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        }
        ctx.stroke();

        // Pontos e rótulos do eixo X
        for (let i = 0; i < n; i++) {
            const x = getX(i);
            // Ponto Tsat
            ctx.fillStyle = '#1A5F8E';
            ctx.beginPath();
            ctx.arc(x, getY(tSat[i]), 4, 0, Math.PI * 2);
            ctx.fill();

            // Ponto Tsol
            ctx.fillStyle = '#C25425';
            ctx.beginPath();
            ctx.arc(x, getY(tSol[i]), 4, 0, Math.PI * 2);
            ctx.fill();

            ctx.fillStyle = '#1C1D1F';
            ctx.fillText(`Ef. ${i + 1}`, x - 12, dh - 10);
        }

        // Legenda
        ctx.fillStyle = '#C25425';
        ctx.fillRect(dw - 180, 8, 10, 10);
        ctx.fillText('T solução (com EPE)', dw - 165, 17);

        ctx.fillStyle = '#1A5F8E';
        ctx.fillRect(dw - 180, 22, 10, 10);
        ctx.fillText('Tsat pura', dw - 165, 31);
    }

    /* Gráfico 2: Vazões de Vapor e Líquido */
    function drawFlowChart(res) {
        if (!canvasFlow) return;
        const ctx = canvasFlow.getContext('2d');
        const w = canvasFlow.width = canvasFlow.offsetWidth * window.devicePixelRatio;
        const h = canvasFlow.height = canvasFlow.offsetHeight * window.devicePixelRatio;
        ctx.scale(window.devicePixelRatio, window.devicePixelRatio);

        const dw = canvasFlow.offsetWidth;
        const dh = canvasFlow.offsetHeight;
        ctx.clearRect(0, 0, dw, dh);

        const padLeft = 55, padRight = 20, padTop = 25, padBottom = 35;
        const plotW = dw - padLeft - padRight;
        const plotH = dh - padTop - padBottom;

        const n = res.effects.length;
        const maxFlow = Math.max(...res.effects.map(e => Math.max(e.liquid_rate_kg_h, e.vapor_rate_kg_h))) * 1.15;

        const getY = (val) => padTop + plotH - (val / maxFlow) * plotH;

        // Grade
        ctx.strokeStyle = '#E2DDD5';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#7E8695';
        ctx.font = '10px JetBrains Mono';

        for (let f = 0; f <= maxFlow; f += 2500) {
            const y = getY(f);
            ctx.beginPath();
            ctx.moveTo(padLeft, y);
            ctx.lineTo(dw - padRight, y);
            ctx.stroke();
            ctx.fillText(f.toLocaleString('pt-BR'), 5, y + 3);
        }

        const barGroupW = plotW / n;
        const barW = Math.min(barGroupW * 0.35, 26);

        res.effects.forEach((ef, i) => {
            const groupX = padLeft + i * barGroupW + (barGroupW - barW * 2 - 6) / 2;

            // Barra V (Vapor)
            const yV = getY(ef.vapor_rate_kg_h);
            ctx.fillStyle = '#1A5F8E';
            ctx.fillRect(groupX, yV, barW, padTop + plotH - yV);

            // Barra L (Líquido)
            const yL = getY(ef.liquid_rate_kg_h);
            ctx.fillStyle = '#226743';
            ctx.fillRect(groupX + barW + 6, yL, barW, padTop + plotH - yL);

            // Rótulo Efeito
            ctx.fillStyle = '#1C1D1F';
            ctx.fillText(`Ef. ${i + 1}`, groupX + barW - 6, dh - 10);
        });

        // Legenda
        ctx.fillStyle = '#1A5F8E';
        ctx.fillRect(dw - 170, 8, 10, 10);
        ctx.fillText('Vapor V (kg/h)', dw - 155, 17);

        ctx.fillStyle = '#226743';
        ctx.fillRect(dw - 170, 22, 10, 10);
        ctx.fillText('Líquido L (kg/h)', dw - 155, 31);
    }

    function exportJson() {
        if (!currentSimulationResult) return;
        const blob = new Blob([JSON.stringify(currentSimulationResult, null, 2)], { type: 'application/json' });
        downloadBlob(blob, `simulacao_evaporador_N${currentSimulationResult.n_effects}.json`);
    }

    function exportCsv() {
        if (!currentSimulationResult) return;
        let csv = 'efeito,vapor_kg_h,liquido_kg_h,conc_massa,temp_c,temp_sat_c,pressao_kpa,epe_c,delta_t_c,area_m2\n';
        currentSimulationResult.effects.forEach(e => {
            csv += `${e.effect},${e.vapor_rate_kg_h.toFixed(2)},${e.liquid_rate_kg_h.toFixed(2)},${e.concentration.toFixed(4)},${e.temperature_c.toFixed(2)},${e.sat_temperature_c.toFixed(2)},${e.pressure_kpa.toFixed(2)},${e.bpe_c.toFixed(2)},${e.delta_t_c.toFixed(2)},${e.area_m2.toFixed(3)}\n`;
        });
        const blob = new Blob([csv], { type: 'text/csv;charset=utf-8;' });
        downloadBlob(blob, `simulacao_evaporador_N${currentSimulationResult.n_effects}.csv`);
    }

    function downloadBlob(blob, filename) {
        const link = document.createElement('a');
        link.href = URL.createObjectURL(blob);
        link.download = filename;
        link.click();
        URL.revokeObjectURL(link.href);
    }
});

