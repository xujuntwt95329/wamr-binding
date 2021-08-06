const {wamr} = require('..')
const fs = require('fs')

const wasm_content = fs.readFileSync('./test.wasm')

const runtime = new wamr();

let mod = runtime.load(wasm_content);

let inst = runtime.instantiate(mod);

let func = runtime.lookupFunction(inst, 'main');

runtime.executeFunction(func, [0, 0]);

runtime.deinstantiate(inst);

runtime.unload(mod);
