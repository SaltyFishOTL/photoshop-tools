const fs=require('fs'),assert=require('assert/strict'),cp=require('child_process'),e=require('./auto-engine-0.7.js');
let seed=91327;const random=n=>{seed=(Math.imul(seed,1664525)+1013904223)>>>0;return seed%n;};
const root=__dirname,exe=root+'/build/Release/engine_cli.exe',report=[];
for(let test=0;test<24;test++){
 const w=test<2?13:87,h=test<2?11:79,a=new Uint8Array(w*h*4),base=[60+random(150),60+random(150),60+random(150)];
 for(let y=0;y<h;y++)for(let x=0;x<w;x++){let k=(y*w+x)*4;for(let c=0;c<3;c++)a[k+c]=Math.max(0,Math.min(255,base[c]+(test%3===0?Math.floor(x/8):0)+(test%3===1?random(3)-1:0)));a[k+3]=255;}
 for(let j=0;j<100;j++){let x=random(w),y=random(h),color=[random(256),random(256),random(256)],length=1+random(test%2?10:3);for(let q=0;q<length&&x+q<w;q++){let k=(y*w+x+q)*4;for(let c=0;c<3;c++)a[k+c]=color[c];if(j%15===0)a[k+3]=[0,249,250,254][j%4];}}
 for(let x=3;x<w-3;x++){let k=(Math.floor(h/2)*w+x)*4;for(let c=0;c<3;c++)a[k+c]=Math.round(base[c]*(x%4?.94:.1));}
 const p={strength:[0,35,60,79,80,100][test%6],area:[1,4,24,60,1000][test%5]};
 const expected=e.repair(a,w,h,e.detect(a,w,h,p,null,{left:false,right:false,top:false,bottom:false})).output;
 fs.writeFileSync(root+'/case-input.rgba',a);cp.execFileSync(exe,[root+'/case-input.rgba',''+w,''+h,''+p.strength,''+p.area,root+'/case-native.rgba']);const actual=fs.readFileSync(root+'/case-native.rgba');let diff=0;for(let i=0;i<actual.length;i++)diff+=actual[i]!==expected[i];assert.equal(diff,0,'case '+test);report.push({test,...p,difference:diff});
}
fs.writeFileSync(root+'/cpp-cases-validation.json',JSON.stringify(report,null,2));console.log('PASS 24 deterministic cases: colors, gradients, faint lines, alpha 0/249/250/254, image edges, strength 0..100, area 1..1000; zero byte differences');
