const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict'),os=require('os');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'ink-specks-')),exe=path.join(__dirname,'build/Release/engine_cli.exe');
const report=[];
try{for(const polarity of [-1,1])for(const kind of ['dot','long-line','large-island','transparent']){
 const w=96,h=96,a=Buffer.alloc(w*h*4),points=[];
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){const i=(y*w+x)*4,v=Math.round(4*Math.sin(x/8)+3*Math.cos(y/7));a[i]=90+v;a[i+1]=65+v;a[i+2]=85+v;a[i+3]=255;}
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  const target=kind==='long-line'?(y===48&&x>=10&&x<86):kind==='large-island'?(x>=40&&x<50&&y>=40&&y<50):(x>=46&&x<49&&y>=46&&y<49);
  if(target){const i=(y*w+x)*4;for(let c=0;c<3;c++)a[i+c]+=polarity*40;if(kind==='transparent')a[i+3]=200;points.push(i);}
 }
 fs.writeFileSync(path.join(dir,'input.rgba'),a);cp.execFileSync(exe,[path.join(dir,'input.rgba'),''+w,''+h,'60','60',path.join(dir,'out.rgba')]);const out=fs.readFileSync(path.join(dir,'out.rgba'));const repaired=points.filter(i=>out[i+3]!==0).length;
 if(kind==='dot')assert.equal(repaired,9);else assert.equal(repaired,0,kind+' must be protected');
 report.push({polarity:polarity>0?'light':'dark',kind,repaired});
}console.log(JSON.stringify({ok:true,cases:report},null,2));}finally{fs.rmSync(dir,{recursive:true,force:true});}
