// Builds a standalone PSJS regression test. Run it in Photoshop; uses disposable documents only.
const fs=require('fs'),path=require('path');
fs.mkdirSync(path.resolve(__dirname,'../../release'),{recursive:true});
const source=fs.readFileSync(path.join(__dirname,'ColorSync.psjs'),'utf8');
const head=source.slice(0,source.indexOf("const ps=require('photoshop'),context="));
const test=String.raw`
const ps=require('photoshop'),context=require('uxp').script.executionContext;
const {app,imaging,action}=ps,host=createHost(ps,engine,context);
const original=app.documents.length?app.activeDocument:null;
const report={ok:false,cases:[]};let doc;
const get=async layer=>(await action.batchPlay([{_obj:'get',_target:[{_ref:'layer',_id:layer.id}]}],{}))[0];
const set=async(layer,values)=>{const [r]=await action.batchPlay([{_obj:'set',_target:[{_ref:'layer',_id:layer.id}],to:{_obj:'layer',...values}}],{});if(r._obj==='error')throw Error(r.message);};
async function maskBytes(layer){const r=await imaging.getLayerMask({documentID:doc.id,layerID:layer.id,sourceBounds:{left:0,top:0,right:128,bottom:128}});try{return new Uint8Array(await r.imageData.getData({chunky:true}));}finally{r.imageData.dispose();}}
function equal(a,b,label){if(a.length!==b.length)throw Error(label+' length');for(let i=0;i<a.length;i++)if(a[i]!==b[i])throw Error(label+' at '+i+': '+a[i]+' != '+b[i]);}
async function raw(layer){const d=await get(layer);if(d.hasUserMask)await set(layer,{userMaskEnabled:false});try{return await host.read(doc,layer);}finally{if(d.hasUserMask)await set(layer,{userMaskEnabled:d.userMaskEnabled!==false});}}
async function baked(layer){const copy=await layer.duplicate();try{const d=await get(copy);if(d.hasUserMask){const [r]=await action.batchPlay([{_obj:'delete',_target:[{_ref:'channel',_enum:'channel',_value:'mask'},{_ref:'layer',_id:copy.id}],apply:d.userMaskEnabled!==false}],{});if(r._obj==='error')throw Error(r.message);}return await host.read(doc,copy);}finally{await copy.delete();}}
async function make(name,delta,masked,disabled=false){
 const layer=await doc.createLayer({name});const data=new Uint8Array(96*96*4);
 for(let y=0;y<96;y++)for(let x=0;x<96;x++){const i=(y*96+x)*4;data[i]=170+delta-Math.floor(y/10);data[i+1]=135+delta-Math.floor(y/12);data[i+2]=94+delta;data[i+3]=x>90?128:255;if(x===50&&y>20&&y<70)data[i]=data[i+1]=data[i+2]=12;}
 const image=await imaging.createImageDataFromBuffer(data,{width:96,height:96,components:4,colorSpace:'RGB'});try{await imaging.putPixels({documentID:doc.id,layerID:layer.id,imageData:image,replace:true,targetBounds:{left:16,top:16}});}finally{image.dispose();}
 if(masked){const m=new Uint8Array(128*128);for(let y=0;y<128;y++)for(let x=0;x<128;x++)m[y*128+x]=x<28?0:x<60?Math.round((x-28)/32*255):255;
 const im=await imaging.createImageDataFromBuffer(m,{width:128,height:128,components:1,colorSpace:'Grayscale'});try{await imaging.putLayerMask({documentID:doc.id,layerID:layer.id,imageData:im,replace:true});}finally{im.dispose();}
 await set(layer,{userMaskEnabled:!disabled,userMaskLinked:false,userMaskDensity:{_unit:'percentUnit',_value:85},userMaskFeather:{_unit:'pixelsUnit',_value:1.5}});
 }return layer;
}
try{
 doc=await app.documents.add({width:128,height:128,resolution:72,name:'ColorSync mask regression',mode:'RGBColorMode',fill:'transparent'});
 const ref=await make('reference',0,true);
 const hard=await make('hard mask',9,true);await set(hard,{userMaskLinked:true,userMaskDensity:{_unit:'percentUnit',_value:100},userMaskFeather:{_unit:'pixelsUnit',_value:0}});
 const targets=[await make('soft mask',10,true),hard,await make('disabled mask',7,true,true),await make('unmasked',8,false)];
 const before=[],expected=[],refBefore=await raw(ref),refMask=await maskBytes(ref);
 const refBake=await baked(ref),prepared=engine.prepare(refBake);
 for(const layer of targets){const d=await get(layer),unmasked=await raw(layer),visible=await baked(layer),result=await engine.synchronize(prepared,visible);
 const wanted=new Uint8Array(unmasked.data);let changed=0;
 for(let y=0;y<visible.height;y++)for(let x=0;x<visible.width;x++){const s=(y*visible.width+x)*4,rx=x+visible.bounds.left-unmasked.bounds.left,ry=y+visible.bounds.top-unmasked.bounds.top;if(!visible.data[s+3]||rx<0||ry<0||rx>=unmasked.width||ry>=unmasked.height)continue;const p=(ry*unmasked.width+rx)*4;for(let c=0;c<3;c++){if(wanted[p+c]!==result.data[s+c])changed++;wanted[p+c]=result.data[s+c];}}
 before.push({d,raw:unmasked.data,mask:d.hasUserMask?await maskBytes(layer):null});expected.push({wanted,changed});
 }
 let state=host.capture();state.reference=ref;const count=doc.layers.length;
 const outputs=await host.apply(state,targets,async()=>{});
 if(doc.layers.length!==count+targets.length)throw Error('Temporary layer leaked');
 equal((await raw(ref)).data,refBefore.data,'reference pixels');equal(await maskBytes(ref),refMask,'reference mask');
 for(let n=0;n<targets.length;n++){const output=outputs[n],out=await raw(output),d=await get(output);
 equal(out.data,expected[n].wanted,'output '+n);equal((await raw(targets[n])).data,before[n].raw,'source '+n);
 if(before[n].mask){equal(await maskBytes(output),before[n].mask,'output mask');equal(await maskBytes(targets[n]),before[n].mask,'source mask');for(const k of ['userMaskEnabled','userMaskLinked','userMaskDensity','userMaskFeather'])if(JSON.stringify(d[k])!==JSON.stringify(before[n].d[k]))throw Error('Mask property changed: '+k);}
 report.cases.push({name:targets[n].name,changedChannels:expected[n].changed,alphaAndMaskExact:true});if(!expected[n].changed)throw Error('No actual colour change');
 }
 // Failure during processing must roll back temporary copies and appearance changes.
 state=host.capture();state.reference=ref;const countBefore=doc.layers.length,historyBefore=doc.activeHistoryState.id;
 let cancelled=false;try{await host.apply(state,[targets[0]],async text=>{if(text.startsWith('同步 '))throw Error('test cancellation');});}catch(e){cancelled=e.message==='test cancellation';}
 if(!cancelled||doc.layers.length!==countBefore||doc.activeHistoryState.id!==historyBefore)throw Error('Cancellation rollback failed');
 equal((await raw(targets[0])).data,before[0].raw,'cancel original');report.rollback=true;
 const empty=await make('fully hidden mask',5,true);const black=await imaging.createImageDataFromBuffer(new Uint8Array(128*128),{width:128,height:128,components:1,colorSpace:'Grayscale'});
 try{await imaging.putLayerMask({documentID:doc.id,layerID:empty.id,imageData:black,replace:true});}finally{black.dispose();}
 await set(empty,{userMaskDensity:{_unit:'percentUnit',_value:100},userMaskFeather:{_unit:'pixelsUnit',_value:0}});
 state=host.capture();state.reference=ref;const emptyCount=doc.layers.length,emptyHistory=doc.activeHistoryState.id;let rejected=false;
 try{await host.apply(state,[empty],async()=>{});}catch(e){rejected=true;report.emptyMaskMessage=e.message;}
 if(!rejected||doc.layers.length!==emptyCount||doc.activeHistoryState.id!==emptyHistory)throw Error('Empty mask rollback failed');report.emptyMaskRollback=true;report.ok=true;
}catch(e){report.error=String(e.stack||e);}finally{if(doc)await doc.closeWithoutSaving();if(original)await action.batchPlay([{_obj:'select',_target:[{_ref:'document',_id:original.id}]}],{});
const folder=await require('uxp').storage.localFileSystem.getFolder();
}
`;
// Explicit result path avoids a file picker in the Photoshop test.
const output=path.resolve(__dirname,'../../release/mask-validation.json').replace(/\\/g,'/');
const body=test.replace("const folder=await require('uxp').storage.localFileSystem.getFolder();",`await require('fs').writeFile(${JSON.stringify(output)},JSON.stringify(report,null,2),'utf8');`);
const dest=path.resolve(__dirname,'../../release/mask-validation.psjs');
fs.writeFileSync(dest,head+body+"})();}catch(e){console.error(e);}");
console.log(dest);
