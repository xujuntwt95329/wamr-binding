# wamr-binding

This module contains the js binding for [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime).

## Example

``` javascript
const { wamr } = require('wamr-binding');
const fs = require('fs');

const wasm_content = fs.readFileSync('./test.wasm');

const runtime = new wamr();

let mod = runtime.load(wasm_content);

let inst = runtime.instantiate(mod);

let func = runtime.lookupFunction(inst, 'func');

let args = [];
runtime.executeFunction(func, args);

runtime.deinstantiate(inst);

runtime.unload(mod);
```