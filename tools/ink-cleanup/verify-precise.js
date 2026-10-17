const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict'),os=require('os');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'ink-precise-')),exe=path.join(__dirname,'build/Release/engine_cli.exe');
const report=[];
function run(a,w,h,precise,area=60){
 const input=path.join(dir,'input.rgba'),output=path.join(dir,'output.rgba');fs.writeFileSync(input,a);
 cp.execFileSync(exe,[input,''+w,''+h,'60',''+area,output,''+precise]);return fs.readFileSync(output);
}
try {
 for(const kind of ['light','dark','same-luminance','antialiased','connected','diagonal','large','transparent','long-highlight']){
  const w=96,h=96,a=Buffer.alloc(w*h*4),dot=[],big=[];
  const bg=kind==='dark'?[180,180,180]:[70,70,70];
  const color=kind==='dark'?[50,50,50]:kind==='same-luminance'?[160,24,70]:[180,180,180];
  function paint(x,y,c,alpha=255){const i=(y*w+x)*4;for(let k=0;k<3;k++)a[i+k]=c[k];a[i+3]=alpha;return i;}
  for(let y=0;y<h;y++)for(let x=0;x<w;x++)paint(x,y,bg);
  // A large highlight lies only ONE background pixel away from the 3x3 speck.
  for(let y=20;y<75;y++)for(let x=kind==='antialiased'?45:44;x<75;x++)big.push(paint(x,y,color));
  if(kind==='antialiased')for(let y=39;y<=43;y++)for(let x=39;x<=43;x++)dot.push(paint(x,y,[110,110,110]));
  if(kind==='large')for(let y=30;y<40;y++)for(let x=20;x<30;x++)dot.push(paint(x,y,color));
  else if(kind==='long-highlight')for(let x=10;x<35;x++)dot.push(paint(x,80,color));
  else for(let y=40;y<=42;y++)for(let x=40;x<=42;x++){const i=paint(x,y,color,kind==='transparent'?200:255);if(!dot.includes(i))dot.push(i);}
  if(kind==='connected')paint(43,41,color);
  if(kind==='diagonal')paint(43,43,color);
  const ordinary=run(a,w,h,0),precise=run(a,w,h,1);
  const changed=dot.filter(i=>precise[i+3]).length;
  const isSpeck=['light','dark','same-luminance','antialiased'].includes(kind);
  assert.equal(changed,isSpeck?dot.length:0,kind+' incorrect dot classification');
  assert.equal(big.filter(i=>precise[i+3]).length,0,kind+' large highlight was cut');
  if(isSpeck)for(const i of dot)for(let c=0;c<3;c++)assert.equal(precise[i+c],bg[c],kind+' surround fill contaminated');
  if(kind==='light'){const limited=run(a,w,h,1,8);assert.equal(dot.filter(i=>limited[i+3]).length,0,'maximum area must apply');}
  report.push({kind,ordinary:dot.filter(i=>ordinary[i+3]).length,precise:changed,highlightPreserved:true});
 }
 console.log(JSON.stringify({ok:true,cases:report},null,2));
}finally{fs.rmSync(dir,{recursive:true,force:true});}
