const fs=require('fs'),path=require('path'),assert=require('assert');
function load(file,singlePass=false){const s=fs.readFileSync(file,'utf8');let code=s.slice(s.indexOf('const engine='),s.indexOf('const createHost='));
 // Freeze the 0.5.0 single-round orchestration, without requiring a local backup file.
 if(singlePass){const start=code.indexOf('async function synchronize('),end=code.indexOf('return {prepare,synchronize,analyze,value};');code=code.slice(0,start)+`async function synchronize(reference,target,tick=async()=>{}){const f=field(reference,target),out=new Uint8Array(target.data);let changed=0;for(let y=0;y<target.height;y+=32){await tick(y/target.height);changed+=rows(target,f,out,y,Math.min(target.height,y+32));}await tick(1);return {data:out,changed,analysis:f.target};}\n`+code.slice(end);}
 return new Function(code+'return engine;')();}
const root=path.resolve(__dirname,'../..');
fs.mkdirSync(path.join(root,'release'),{recursive:true});
const old=load(path.join(__dirname,'ColorSync.psjs'),true);
const next=load(path.join(__dirname,'ColorSync.psjs'));
function image(color,shift){const width=192,height=192,data=new Uint8Array(width*height*4);for(let y=0;y<height;y++)for(let x=0;x<width;x++){const i=(y*width+x)*4;for(let c=0;c<3;c++)data[i+c]=color[c]+shift[c]-Math.floor(y/32);data[i+3]=x<4?0:x>185?128:255;if(x===90&&y>20&&y<160)data[i]=data[i+1]=data[i+2]=12;if(y<4)data[i]=data[i+1]=data[i+2]=255;}return {data,width,height};}
function error(a,b){let sum=0,n=0;for(let i=0;i<a.length;i+=4)if(a[i+3]===255&&a[i]>32&&a[i]<238){for(let c=0;c<3;c++){sum+=Math.abs(a[i+c]-b[i+c]);n++;}}return sum/n;}
(async()=>{const results=[];
for(const [name,color,shift] of [['capped skin',[170,130,90],[30,0,0]],['weak dark colour',[60,32,40],[-12,0,0]],['small shift',[170,130,90],[8,8,8]],['already matched',[170,130,90],[0,0,0]]]){
 const reference=image(color,[0,0,0]),target=image(color,shift),prepared=old.prepare(reference),one=await old.synchronize(prepared,target),two=await old.synchronize(prepared,{...target,data:one.data}),three=await old.synchronize(prepared,{...target,data:two.data});
 let progress=-1;const start=performance.now(),out=await next.synchronize(next.prepare(reference),target,async p=>{assert(p>=progress&&p>=0&&p<=1);progress=p;});
 assert.deepStrictEqual(out.data,three.data,name+' differs from guarded manual runs');
 for(let i=0;i<out.data.length;i+=4){assert.equal(out.data[i+3],target.data[i+3]);if(target.data[i+3]===0||Math.max(...target.data.slice(i,i+3))<32||Math.min(...target.data.slice(i,i+3))>238)assert.deepStrictEqual(out.data.slice(i,i+4),target.data.slice(i,i+4));}
 if(name==='already matched')assert.equal(out.passes,1);
 const before=error(reference.data,target.data),after1=error(reference.data,one.data),after=error(reference.data,out.data);assert(after<=after1+1e-6,name+' regressed');
 results.push({name,before,oldSinglePass:after1,newSingleClick:after,passes:out.passes,ms:Math.round(performance.now()-start)});
}
 let cancelled=false;try{const im=image([170,130,90],[0,0,0]);await next.synchronize(next.prepare(im),image([170,130,90],[30,0,0]),async p=>{if(p>.4)throw Error('cancel');});}catch(e){cancelled=e.message==='cancel';}assert(cancelled);
 const report={ok:true,cancellation:true,results};fs.writeFileSync(path.join(root,'release/convergence-validation.json'),JSON.stringify(report,null,2));console.log(JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exitCode=1;});
