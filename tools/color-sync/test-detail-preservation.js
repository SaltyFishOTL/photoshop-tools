// Synthetic data only: no user artwork is required or distributed.
const fs = require('fs');
const path = require('path');
const assert = require('assert/strict');
const source = fs.readFileSync(process.argv[2] || path.join(__dirname, 'ColorSync.psjs'), 'utf8');
const engine = new Function(source.slice(source.indexOf('const engine='), source.indexOf('const createHost=')) + 'return engine;')();

function scene(target) {
 const width = 768, height = 512, data = new Uint8Array(width * height * 4);
 const expected = new Uint8Array(data.length), labels = new Int8Array(width * height).fill(-1);
 for (let y = 0; y < height; y++) for (let x = 0; x < width; x++) {
  const p = y * width + x, i = p * 4;
  let c = [255,255,255], bias = [0,0,0], label = -1;
  // Same suit material, with spatially different errors in left/right sleeves.
  if (x > 35 && x < 730 && y > 250 && y < 475) {
   c = [94,66,43].map(v => v + Math.round(y / 80));
   const d = target ? (x < 320 ? 8 : x > 445 ? -6 : 0) : 0;
   bias = [d,d,d]; label = x < 270 ? 0 : x > 490 ? 1 : -1;
  }
  // Lens reflections shift with the drawing; colour correction must not copy
  // their shape or regress their local contrast towards a spatial mean.
  if (x > 170 && x < 590 && y > 50 && y < 200) {
   const phase = target ? 3 : 0;
   const wave = Math.sin((x + phase) / 3.7) * Math.cos((y - phase) / 6.3);
   c = [120 + Math.round(17*wave), 49 + Math.round(8*wave), 24 + Math.round(3*wave)];
   bias = target ? [6,4,2] : [0,0,0]; label = 2;
  }
  if ((x===380 && y>60 && y<190) || (y===400 && x>50 && x<720)) { c=[8,8,8]; bias=[0,0,0]; label=-1; }
  for (let ch=0;ch<3;ch++) { data[i+ch]=c[ch]+bias[ch]; expected[i+ch]=c[ch]; }
  data[i+3]=expected[i+3]=255;
  if (x>60 && x<700 && y>70 && y<450 && y!==400) labels[p]=label;
 }
 return {width,height,data,expected,labels};
}

(async () => {
 const ref = scene(false), target = scene(true), prepared = engine.prepare(ref);
 const output = await engine.synchronize(prepared, target);
 const sums = Array.from({length:3}, () => ({n:0,before:0,after:0,bias:[0,0,0]}));
 let inputContrast=0, outputContrast=0;
 for(let p=0;p<target.labels.length;p++) {
  const k=target.labels[p],i=p*4;if(k<0)continue;
  const s=sums[k];s.n++;
  for(let ch=0;ch<3;ch++) {s.before+=Math.abs(target.data[i+ch]-target.expected[i+ch]);s.after+=Math.abs(output.data[i+ch]-target.expected[i+ch]);s.bias[ch]+=output.data[i+ch]-target.expected[i+ch];}
  if(k===2 && target.labels[p+1]===2) for(let ch=0;ch<3;ch++) {inputContrast+=Math.abs(target.data[i+ch]-target.data[i+ch+4]);outputContrast+=Math.abs(output.data[i+ch]-output.data[i+ch+4]);}
 }
 const regions=sums.map(s=>({before:s.before/s.n/3,after:s.after/s.n/3,bias:s.bias.map(v=>v/s.n)}));
 const contrastRatio=outputContrast/inputContrast;
 console.log(JSON.stringify({regions,contrastRatio},null,2));
 for(const k of [0,1])assert(regions[k].after<regions[k].before*.3,'Local sleeve offset was not corrected');
 assert(regions[2].bias.every(x=>Math.abs(x)<1.5),'Lens mean colour did not match');
 assert(contrastRatio>.94 && contrastRatio<1.06,'Lens reflection contrast was changed');
 for (const phase of [.3,.6,.75,.9]) {
  let caught=false;
  try { await engine.synchronize(prepared,target,async p=>{if(p>phase)throw Error('cancel');}); }
  catch(e) { caught=e.message==='cancel'; }
  assert(caught,'Cancellation missed at phase '+phase);
 }
 assert.deepEqual(target.data,scene(true).data,'Original changed');
 console.log('Detail, local material correction and cancellation checks passed.');
})().catch(e=>{console.error(e);process.exitCode=1;});
