// Browser-worker compatibility shim for a real OS worker thread integration test.
const {parentPort,workerData}=require('worker_threads'),fs=require('fs'),vm=require('vm'),path=require('path');
global.postMessage=m=>parentPort.postMessage(m);
global.importScripts=(...files)=>files.forEach(f=>vm.runInThisContext(fs.readFileSync(path.join(__dirname,'../dist',f.split('?')[0]),'utf8'),{filename:f}));
global.fetch=async()=>{if(workerData?.delay)await new Promise(r=>setTimeout(r,workerData.delay));return({ok:true,arrayBuffer:async()=>{let b=fs.readFileSync(path.join(__dirname,'../dist/compact-engine.wasm'));return b.buffer.slice(b.byteOffset,b.byteOffset+b.byteLength)}})};
vm.runInThisContext(fs.readFileSync(path.join(__dirname,'../dist/worker.js'),'utf8'),{filename:'worker.js'});
parentPort.on('message',data=>global.onmessage({data}));
