/**
 * @file wasm-bridge.js
 * @brief Ponte de integração entre o JavaScript da interface e o núcleo WebAssembly compilado em C.
 */

class EvaporatorWasmBridge {
    constructor() {
        this.module = null;
        this.isReady = false;
        this.solveDirect = null;
        this.getSteamRate = null;
        this.getVaporEconomy = null;
        this.getAreaPerEffect = null;
        this.getTotalArea = null;
        this.getTotalDt = null;
        this.getTotalBpe = null;
        this.getOuterIter = null;
        this.getInnerIter = null;
        this.getEffectVapor = null;
        this.getEffectLiquid = null;
        this.getEffectConc = null;
        this.getEffectTemp = null;
        this.getEffectSatTemp = null;
        this.getEffectPressure = null;
        this.getEffectBpe = null;
        this.getEffectDeltaT = null;
        this.getEffectArea = null;
        this.getEffectHeatDuty = null;
        this.getLastError = null;
    }

    async init() {
        try {
            // Tenta carregar o módulo WebAssembly gerado pelo Emscripten
            if (typeof createEvapModule === 'function') {
                this.module = await createEvapModule();
            } else if (typeof EvapModule === 'function') {
                this.module = await EvapModule();
            } else if (window.Module && window.Module._wasm_solve_direct) {
                this.module = window.Module;
            } else {
                // Tenta carregar dinamicamente se o script existir
                await this.loadScript('js/evap_solver.js');
                if (typeof createEvapModule === 'function') {
                    this.module = await createEvapModule();
                } else if (typeof Module !== 'undefined') {
                    this.module = Module;
                }
            }

            if (!this.module || !this.module._wasm_solve_direct) {
                throw new Error("Símbolos do módulo C WebAssembly não encontrados.");
            }

            // Mapeamento das funções exportadas
            this.solveDirect = this.module._wasm_solve_direct;
            this.getSteamRate = this.module._wasm_get_steam_rate;
            this.getVaporEconomy = this.module._wasm_get_vapor_economy;
            this.getAreaPerEffect = this.module._wasm_get_area_per_effect;
            this.getTotalArea = this.module._wasm_get_total_area;
            this.getTotalDt = this.module._wasm_get_total_dt;
            this.getTotalBpe = this.module._wasm_get_total_bpe;
            this.getOuterIter = this.module._wasm_get_outer_iter;
            this.getInnerIter = this.module._wasm_get_inner_iter;

            this.getEffectVapor = this.module._wasm_get_effect_vapor;
            this.getEffectLiquid = this.module._wasm_get_effect_liquid;
            this.getEffectConc = this.module._wasm_get_effect_conc;
            this.getEffectTemp = this.module._wasm_get_effect_temp;
            this.getEffectSatTemp = this.module._wasm_get_effect_sat_temp;
            this.getEffectPressure = this.module._wasm_get_effect_pressure;
            this.getEffectBpe = this.module._wasm_get_effect_bpe;
            this.getEffectDeltaT = this.module._wasm_get_effect_delta_t;
            this.getEffectArea = this.module._wasm_get_effect_area;
            this.getEffectHeatDuty = this.module._wasm_get_effect_heat_duty;
            this.getLastError = this.module.UTF8ToString ? 
                () => this.module.UTF8ToString(this.module._wasm_get_last_error()) :
                () => "Erro na simulação C.";

            this.isReady = true;
            return true;
        } catch (e) {
            console.warn("WebAssembly C Core não pôde ser inicializado:", e.message);
            this.isReady = false;
            return false;
        }
    }

    loadScript(src) {
        return new Promise((resolve, reject) => {
            const script = document.createElement('script');
            script.src = src;
            script.onload = () => resolve();
            script.onerror = () => reject(new Error(`Falha ao carregar ${src}`));
            document.head.appendChild(script);
        });
    }

    simulate(params) {
        if (!this.isReady) {
            throw new Error("Módulo WebAssembly C não carregado. Verifique a compilação do núcleo.");
        }

        const status = this.solveDirect(
            params.n_effects,
            params.feed_rate_kg_h,
            params.feed_conc,
            params.product_conc,
            params.feed_temp_c,
            params.steam_temp_c,
            params.last_sat_temp_c,
            params.htc_w_m2k,
            params.omega
        );

        if (status !== 0) {
            const errMsg = this.getLastError ? this.getLastError() : `Erro de código ${status}`;
            throw new Error(errMsg);
        }

        const n = params.n_effects;
        const result = {
            n_effects: n,
            steam_rate_kg_h: this.getSteamRate(),
            vapor_economy: this.getVaporEconomy(),
            area_per_effect_m2: this.getAreaPerEffect(),
            total_area_m2: this.getTotalArea(),
            total_delta_t_c: this.getTotalDt(),
            total_bpe_c: this.getTotalBpe(),
            iter_outer: this.getOuterIter(),
            iter_inner_total: this.getInnerIter(),
            effects: []
        };

        for (let i = 0; i < n; i++) {
            result.effects.push({
                effect: i + 1,
                vapor_rate_kg_h: this.getEffectVapor(i),
                liquid_rate_kg_h: this.getEffectLiquid(i),
                concentration: this.getEffectConc(i),
                temperature_c: this.getEffectTemp(i),
                sat_temperature_c: this.getEffectSatTemp(i),
                pressure_kpa: this.getEffectPressure(i),
                bpe_c: this.getEffectBpe(i),
                delta_t_c: this.getEffectDeltaT(i),
                area_m2: this.getEffectArea(i),
                heat_duty_kw: this.getEffectHeatDuty(i)
            });
        }

        return result;
    }
}

window.EvaporatorBridge = new EvaporatorWasmBridge();

