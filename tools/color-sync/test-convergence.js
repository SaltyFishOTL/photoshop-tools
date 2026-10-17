const fs=require('fs'),path=require('path'),assert=require('assert/strict');
const source=fs.readFileSync(path.join(__dirname,'ColorSync.psjs'),'utf8');
const engine=new Function(source.slice(source.indexOf('const engine='),source.indexOf('const createHost='))+'return engine;')();
function scene(variant=0){
 const width=384,height=288,data=new Uint8Array(width*height*4),expected=new Uint8Array(data.length),labels=new Array(width*height);
 const shifts=variant===1?[[9,9,9],[-12,-12,-12],[-8,3,5],[7,-4,2]]:variant===2?[[-7,-7,-7],[10,10,10],[6,-4,-7],[-9,4,3]]:[[0,0,0],[0,0,0],[0,0,0],[0,0,0]];
 const dx=variant===1?3:variant===2?-2:0,dy=variant===1?-2:variant===2?3:0;
 for(let y=0;y<height;y++)for(let x=0;x<width;x++){
  const i=(y*width+x)*4,X=x-dx,Y=y-dy;let c=[255,255,255],k=-1;
  if(X>=194&&X<360&&Y>=24&&Y<258){c=[58,62,67].map(v=>v+Math.floor(Y/24));k=0;}
  if(X>=250&&X<332&&Y>=56&&Y<98){c=[156,160,165].map(v=>v+Math.floor(Y/24));k=1;}
  if(X>=20&&X<172&&Y>=24&&Y<134){c=[180,130,86].map(v=>v-Math.floor(Y/16));k=2;}
  if(X>=20&&X<172&&Y>=154&&Y<258){c=[145,30,26].map(v=>v+Math.floor(Y/24));k=3;}
  const line=(X===90&&Y>35&&Y<125)||(X===210&&Y>45&&Y<245)||(Y===180&&X>40&&X<150);
  if(line){c=[12,12,12];k=-1;}
  for(let ch=0;ch<3;ch++){expected[i+ch]=c[ch];data[i+ch]=c[ch]+(k<0?0:shifts[k][ch])+(variant===2?(k===0?Math.floor((X-194)/40):k===2?Math.floor((Y-24)/28):0):0);}
  data[i+3]=expected[i+3]=x<4?0:x<8?128:255;
  // Only evaluate eroded region interiors. Edges and changed geometry are tested separately.
  if(k>=0&&X%12>2&&Y%12>2)labels[y*width+x]=k;
 }
 return {data,width,height,expected,labels};
}
function metrics(im,out){const v=Array.from({length:4},()=>({before:0,after:0,n:0}));for(let p=0;p<im.labels.length;p++){const k=im.labels[p];if(k===undefined)continue;const i=p*4;for(let c=0;c<3;c++){v[k].before+=Math.abs(im.data[i+c]-im.expected[i+c]);v[k].after+=Math.abs(out[i+c]-im.expected[i+c]);v[k].n++;}}return v.map((r,k)=>({region:['neutral jacket','neutral highlight','skin gradient','shirt'][k],before:r.before/r.n,after:r.after/r.n}));}
(async()=>{
 const ref=scene(),snapshot=new Uint8Array(ref.data),prepared=engine.prepare(ref),results=[];
 for(const variant of [1,2]){const im=scene(variant),original=new Uint8Array(im.data);let progress=-1;const out=await engine.synchronize(prepared,im,async p=>{assert(p>=progress&&p>=0&&p<=1);progress=p;});assert.equal(progress,1);assert.deepEqual(im.data,original,'target mutated');assert.deepEqual(ref.data,snapshot,'reference mutated');
  for(let i=0;i<im.data.length;i+=4){assert.equal(out.data[i+3],im.data[i+3]);if(im.data[i+3]===0||Math.max(...im.data.slice(i,i+3))<32||Math.min(...im.data.slice(i,i+3))>238)assert.deepEqual(out.data.slice(i,i+4),im.data.slice(i,i+4),'ink/background/transparent pixel changed');}
  const error=metrics(im,out.data);for(const r of error)assert(r.after<r.before*.3,JSON.stringify(r));
  const twice=await engine.synchronize(prepared,{...im,data:out.data});const repeated=metrics(im,twice.data);for(let k=0;k<error.length;k++)assert(repeated[k].after<=error[k].after+.3,'repeat drift');
  results.push({variant,error,repeated});
 }
 const unchanged=await engine.synchronize(prepared,ref);assert.deepEqual(unchanged.data,ref.data,'identity changed');assert.equal(unchanged.changed,0);
 let cancelled=false;try{await engine.synchronize(prepared,scene(1),async p=>{if(p>.3)throw Error('cancel');});}catch(e){cancelled=e.message==='cancel';}assert(cancelled);
 const report={ok:true,identity:true,alphaAndInkPreserved:true,cancelled,results};console.log(JSON.stringify(report,null,2));fs.mkdirSync(path.resolve(__dirname,'../../release'),{recursive:true});fs.writeFileSync(path.resolve(__dirname,'../../release/regional-color-validation.json'),JSON.stringify(report,null,2));
})().catch(e=>{console.error(e);process.exitCode=1});
