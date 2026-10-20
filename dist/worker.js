importScripts('engine.js?v=landscape1','compact.js?v=landscape1','topology.js?v=landscape1');
let running=false,status={phase:'Ready; waiting for configuration',percent:0,generation:0,candidate:0},heartbeat=null,lastYield=0;
const report=()=>postMessage({type:'heartbeat',...status});
function phase(name,extra={}){status={...status,phase:name,...extra};report()}
async function breathe(){if(Date.now()-lastYield>=16){await new Promise(resolve=>setTimeout(resolve,0));lastYield=Date.now()}}
postMessage({type:'ready'});
onmessage=async({data:input})=>{
 if(input.type==='ping'){report();return}
 if(running){postMessage({type:'error',message:'An experiment is already running.'});return}
 const c=input.config||input;running=true;lastYield=Date.now();status={phase:'Configuration received',percent:0,generation:0,candidate:0,total:c.generations,population:c.population};
 postMessage({type:'ack'});heartbeat=setInterval(report,5000);report();
 try{
  phase('Loading compiled engine');
  const compact=await createCompactEngine(null,message=>phase(message));
  postMessage({type:'stage',message:'Compiled C / WebAssembly engine ready · '+compact.memoryBytes/1048576+' MiB linear memory.'});
  phase('Creating initial population');
  let r=rng(c.seed+':evolution'),traffic=scenario(c),n=c.cols*c.rows;
  let pop=Array.from({length:c.population},()=>Uint8Array.from({length:n},()=>Math.floor(r()*3))),best=null,trace=[],stagnant=0,cache=new Map(),simulated=0,reused=0;
  for(let g=0;g<c.generations;g++){
   let ranked=[];
   for(let index=0;index<pop.length;index++){
    const genome=pop[index];
    phase('Evaluating particle trial',{generation:g+1,candidate:index+1,trialPercent:0,percent:100*(g+index/c.population)/c.generations});
    postMessage({type:'evaluating',generation:g+1,candidate:index+1,population:c.population});
    let key=Array.from(genome).join(''),result=cache.get(key),cached=!!result;
    if(result)reused++;
    else{
     let sim=await compact.evaluateAsync(genome,c,traffic,(time,collisions)=>{
      status={...status,trialPercent:100*time/c.duration,collisions,percent:100*(g+(index+time/c.duration)/c.population)/c.generations};
     });
     result={score:sim.collisions,exits:sim.exits,bins:sim.bins,topology:traceTopology(genome,c.cols,c.rows)};
     if(cache.size>=20000)cache.clear();cache.set(key,result);simulated++;
    }
    postMessage({type:'candidate',generation:g+1,candidate:index+1,population:c.population,score:result.score,topology:result.topology,cached,reused,simulated});
    ranked.push({genome,...result});await breathe();
   }
   phase('Ranking completed trials');ranked.sort((a,b)=>a.score-b.score);
   let improved=!best||ranked[0].score<best.score,previous=best?best.score:null;
   if(improved){best=ranked[0];stagnant=0}else stagnant++;
   trace.push(best.score);
   phase('Tournament selection and breeding',{trialPercent:100,offspring:0,offspringTarget:c.population-c.elite});
   let tournaments=0,mutationCount=0,next=ranked.slice(0,c.elite).map(a=>a.genome.slice());
   const pick=()=>{tournaments++;let win=ranked[Math.floor(r()*ranked.length)];for(let j=1;j<c.tournament;j++){let a=ranked[Math.floor(r()*ranked.length)];if(a.score<win.score)win=a}return win.genome};
   while(next.length<c.population){
    let a=pick(),b=pick(),cut=Math.floor(r()*n),end=Math.floor(r()*n);if(cut>end)[cut,end]=[end,cut];let child=a.slice();
    for(let i=0;i<n;i++){if(i>=cut&&i<end)child[i]=b[i];if(r()<c.mutation){child[i]=(child[i]+1+Math.floor(r()*2))%3;mutationCount++}}
    next.push(child);status={...status,tournaments,offspring:next.length-c.elite};
    if(next.length%4===0||next.length===c.population)postMessage({type:'breeding',generation:g+1,tournaments,offspring:next.length-c.elite,target:c.population-c.elite,mutations:mutationCount});
    await breathe();
   }
   status.percent=100*(g+1)/c.generations;
   postMessage({type:'progress',generation:g+1,total:c.generations,evaluations:(g+1)*c.population,simulated,reused,simSeconds:simulated*c.duration,best:best.score,generationBest:ranked[0].score,average:ranked.reduce((sum,a)=>sum+a.score,0)/c.population,worst:ranked[ranked.length-1].score,unique:new Set(pop.map(a=>Array.from(a).join(''))).size,population:c.population,improved,previous,stagnant,elite:c.elite,offspring:c.population-c.elite,mutations:mutationCount,hasNext:g+1<c.generations});
   pop=next;
  }
  phase('Complete',{percent:100});postMessage({type:'done',genome:Array.from(best.genome),score:best.score,exits:best.exits,bins:best.bins,trace,topology:best.topology,config:c});
 }catch(e){postMessage({type:'error',message:e.message})}
 finally{clearInterval(heartbeat);heartbeat=null;running=false}
};
