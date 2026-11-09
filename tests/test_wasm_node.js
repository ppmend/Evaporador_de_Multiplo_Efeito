const createEvapModule = require('../web/js/evap_solver.js');

async function testWasm() {
    console.log("Iniciando teste do modulo WebAssembly...");
    const m = await createEvapModule();
    console.log("Modulo carregado!");

    // N=3, F=10000, xF=0.1, xN=0.5, TF=50, TS=160, TsatN=60, U=2000, omega=0.496
    const status = m._wasm_solve_direct(3, 10000, 0.1, 0.5, 50, 160, 60, 2000, 0.496);
    console.log("Status de retorno:", status);

    const S = m._wasm_get_steam_rate();
    const Econ = m._wasm_get_vapor_economy();
    const Area = m._wasm_get_area_per_effect();
    const OuterIter = m._wasm_get_outer_iter();
    const InnerIter = m._wasm_get_inner_iter();

    console.log(`Resultados WebAssembly N=3:`);
    console.log(`  S (vapor vivo)   : ${S.toFixed(2)} kg/h (Esperado: 4593.6 kg/h)`);
    console.log(`  Economia de vapor: ${Econ.toFixed(4)} (Esperado: 1.7415)`);
    console.log(`  Area por efeito  : ${Area.toFixed(3)} m2 (Esperado: 70.128 m2)`);
    console.log(`  Iteracoes de Area: ${OuterIter}`);
    console.log(`  Iteracoes SOR    : ${InnerIter}`);

    const ok = Math.abs(Area - 70.128) < 0.05 && Math.abs(S - 4593.6) < 0.5;
    if (ok) {
        console.log("TESTE DO NUCLEO WEBASSEMBLY PASSOU COM SUCESSO!");
        process.exit(0);
    } else {
        console.error("Divergência nos resultados do WebAssembly!");
        process.exit(1);
    }
}

testWasm().catch(err => {
    console.error("Erro no teste WASM:", err);
    process.exit(1);
});

