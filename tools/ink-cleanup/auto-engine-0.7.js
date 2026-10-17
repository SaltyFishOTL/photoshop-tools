// No target color: identify departures from a locally consistent background.
function detect(rgba,w,h,p,selection=null,edges={left:true,right:true,top:true,bottom:true}){
  const n=w*h,dark=new Uint8Array(n),seed=new Uint8Array(n),uncertain=new Uint8Array(n),seen=new Uint8Array(n),mask=new Uint8Array(n),queue=new Int32Array(n),groups=[];
  if(rgba.length!==n*4)throw new Error('RGBA size mismatch');
  const radii=[3,5,8],threshold=Math.max(2,16-.2*p.strength);
  for(let y=0;y<h;y++)for(let x=0;x<w;x++){
    const i=y*w+x,k=i*4;if(rgba[k+3]<250)continue;
    const candidates=[];
    for(let scale=0;scale<radii.length;scale++)for(const [ux,uy]of [[1,0],[0,1],[1,1],[1,-1]]){
      const dx=ux*radii[scale],dy=uy*radii[scale];
      const ax=x+dx,ay=y+dy,bx=x-dx,by=y-dy;
      if(ax<0||ay<0||bx<0||by<0||ax>=w||bx>=w||ay>=h||by>=h)continue;
      const a=(ay*w+ax)*4,b=(by*w+bx)*4;if(rgba[a+3]<250||rgba[b+3]<250)continue;
      const delta=Math.max(Math.abs(rgba[a]-rgba[b]),Math.abs(rgba[a+1]-rgba[b+1]),Math.abs(rgba[a+2]-rgba[b+2]));
      if(delta<=20)candidates.push([(rgba[a]+rgba[b])/2,(rgba[a+1]+rgba[b+1])/2,(rgba[a+2]+rgba[b+2])/2,scale]);
    }
    let best=null,bestVotes=0,bestDiff=Infinity,bestScales=0;
    for(const color of candidates){
      let votes=0,r=0,g=0,b=0,scales=0;
      for(const other of candidates)if(Math.max(Math.abs(color[0]-other[0]),Math.abs(color[1]-other[1]),Math.abs(color[2]-other[2]))<=12){votes++;r+=other[0];g+=other[1];b+=other[2];scales|=1<<other[3];}
      const mean=[r/votes,g/votes,b/votes],difference=Math.max(Math.abs(rgba[k]-mean[0]),Math.abs(rgba[k+1]-mean[1]),Math.abs(rgba[k+2]-mean[2]));
      // On tied evidence, prefer the explanation closest to the existing paint.
      if(votes>bestVotes||(votes===bestVotes&&difference<bestDiff)){best=mean;bestVotes=votes;bestDiff=difference;bestScales=scales;}
    }
    if(best&&bestDiff>=2)dark[i]=1;
    if(bestVotes>=3&&(bestScales&(bestScales-1))!==0&&bestDiff>=threshold)seed[i]=1;
    if(bestVotes<2){
      // Unknown boundaries are protection barriers, not arbitrary fill targets.
      let range=0;
      for(let dy=-1;dy<=1;dy++)for(let dx=-1;dx<=1;dx++){
        const nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;
        const j=(ny*w+nx)*4;
        for(let c=0;c<3;c++)range=Math.max(range,Math.abs(rgba[k+c]-rgba[j+c]));
      }
      if(range>=4)uncertain[i]=1;
    }
  }
  for(let start=0;start<n;start++){
    if(!dark[start]||seen[start])continue;
    let head=0,tail=1,eligible=true,hasSeed=false;queue[0]=start;seen[start]=1;
    while(head<tail){
      const i=queue[head++],x=i%w,y=Math.floor(i/w);hasSeed=hasSeed||!!seed[i];
      if((x<2&&edges.left)||(y<2&&edges.top)||(x>=w-2&&edges.right)||(y>=h-2&&edges.bottom)||(selection&&selection[i]<128))eligible=false;
      for(let dy=-2;dy<=2;dy++)for(let dx=-2;dx<=2;dx++){
        const nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;
        const j=ny*w+nx;
        if(!dark[j]||seen[j])continue;
        if(Math.abs(dx)>1||Math.abs(dy)>1){
          // Bridge a real continuation, not every nearby isolated speck.
          // Both ends need backward/forward support along the gap direction.
          const ux=Math.sign(dx),uy=Math.sign(dy);
          const bx=x-ux,by=y-uy,fx=nx+ux,fy=ny+uy;
          if(bx<0||by<0||fx<0||fy<0||bx>=w||fx>=w||by>=h||fy>=h)continue;
          if(!dark[by*w+bx]||!dark[fy*w+fx])continue;
        }
        seen[j]=1;queue[tail++]=j;
      }
    }
    if(!eligible||!hasSeed||tail>p.area)continue;
    const points=Array.from(queue.subarray(0,tail));points.forEach(i=>mask[i]=255);groups.push(points);
  }
  // Exclude uncertain structural pixels from all repair sampling.
  for(let i=0;i<n;i++)if(uncertain[i])dark[i]=1;
  // Rescue only islands fully enclosed by one measured background color.
  // Unlike the wide-scale residual mask, this cannot connect clean skin to ink.
  const visited=new Int32Array(n);let stamp=0;
  const delta=(i,c)=>Math.max(Math.abs(rgba[i*4]-c[0]),Math.abs(rgba[i*4+1]-c[1]),Math.abs(rgba[i*4+2]-c[2]));
  for(const tolerance of [3,8]){
  const islandThreshold=Math.max(threshold,tolerance===3?0:12);
  for(let y=8;y<h-8;y++)for(let x=8;x<w-8;x++){
    const start=y*w+x;if(!dark[start]||mask[start])continue;
    for(const radius of [2,3,5,8]){
      const ring=[];
      for(const [dx,dy] of [[1,0],[-1,0],[0,1],[0,-1],[1,1],[-1,1],[1,-1],[-1,-1]]){
        const j=(y+dy*radius)*w+x+dx*radius;ring.push(j);
      }
      let support=[];
      for(const j of ring){const color=Array.from(rgba.subarray(j*4,j*4+3)),ids=ring.filter(k=>rgba[k*4+3]>=250&&delta(k,color)<=tolerance);if(ids.length>support.length)support=ids;}
      if(support.length<6)continue;
      const bg=[0,1,2].map(c=>support.map(j=>rgba[j*4+c]).sort((a,b)=>a-b)[Math.floor(support.length/2)]);
      if(delta(start,bg)<islandThreshold)continue;
      stamp++;const points=[start];visited[start]=stamp;let valid=true;
      for(let head=0;head<points.length&&valid;head++){
        const i=points[head],px=i%w,py=Math.floor(i/w);
        if(px<2||py<2||px>=w-2||py>=h-2||rgba[i*4+3]<250||(selection&&selection[i]<128)||mask[i]){valid=false;break;}
        for(let dy=-1;dy<=1&&valid;dy++)for(let dx=-1;dx<=1;dx++){
          const j=(py+dy)*w+px+dx;if(visited[j]===stamp)continue;visited[j]=stamp;
          if(rgba[j*4+3]<250){valid=false;break;}
          if(delta(j,bg)>tolerance){points.push(j);if(points.length>p.area){valid=false;break;}}
        }
      }
      if(!valid)continue;
      // Protect aligned line continuations, not every unrelated nearby tattoo.
      const set=new Set(points);
      for(const i of points){const px=i%w,py=Math.floor(i/w);for(let dy=-2;dy<=2&&valid;dy++)for(let dx=-2;dx<=2;dx++){
        const j=(py+dy)*w+px+dx;if(set.has(j))continue;
        if(rgba[j*4+3]<250){valid=false;break;}
        if(delta(j,bg)>tolerance){
          const ux=Math.sign(dx),uy=Math.sign(dy),back=(py-uy)*w+px-ux,fx=px+dx+ux,fy=py+dy+uy;
          if(set.has(back)&&fx>=0&&fy>=0&&fx<w&&fy<h&&delta(fy*w+fx,bg)>tolerance){valid=false;break;}
        }
      }if(!valid)break;}
      if(!valid)continue;
      points.background=bg;groups.push(points);for(const i of points)mask[i]=255;
      break;
    }
  }
  }
  // Immediate-boundary rescue for bright holes fully enclosed by dark ink.
  seen.fill(0);const bright=i=>Math.max(rgba[i*4],rgba[i*4+1],rgba[i*4+2])>32;
  for(let start=0;start<n;start++){
    if(seen[start]||!bright(start))continue;
    const points=[start],border=[];seen[start]=1;let valid=true;
    for(let head=0;head<points.length;head++){
      const i=points[head],x=i%w,y=Math.floor(i/w);
      if(rgba[i*4+3]<250||mask[i]||x===0||y===0||x===w-1||y===h-1||(selection&&selection[i]<128))valid=false;
      for(let dy=-1;dy<=1;dy++)for(let dx=-1;dx<=1;dx++){
        const nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;const j=ny*w+nx;
        if(rgba[j*4+3]<250)valid=false;
        if(bright(j)){if(!seen[j]){seen[j]=1;points.push(j);}}
        else if(points.length<=p.area)border.push(j);
      }
    }
    if(!valid||points.length>p.area||border.length<8)continue;
    const bg=[0,1,2].map(c=>border.map(j=>rgba[j*4+c]).sort((a,b)=>a-b)[Math.floor(border.length/2)]);
    if(!points.some(i=>delta(i,bg)>=Math.max(12,threshold)))continue;
    points.background=bg;groups.push(points);for(const i of points)mask[i]=255;
  }
  return {groups,mask,dark};
}

function repair(rgba,w,h,d,selection=null){
  const output=new Uint8Array(rgba.length);let repaired=0,skipped=0;
  for(const points of d.groups){
    if(points.background){for(const i of points){output.set(points.background,i*4);output[i*4+3]=selection?selection[i]:255;}repaired++;continue;}
    const samples=new Set();
    for(const i of points){const x=i%w,y=Math.floor(i/w);
      for(let dy=-5;dy<=5;dy++)for(let dx=-5;dx<=5;dx++){
        if(dx*dx+dy*dy>25)continue;
        const nx=x+dx,ny=y+dy;if(nx<0||ny<0||nx>=w||ny>=h)continue;
        const j=ny*w+nx;if(!d.dark[j]&&rgba[j*4+3]>=250)samples.add(j);
      }
    }
    if(samples.size<4){skipped++;continue;}
    const clusters=[];
    for(const j of samples){
      const rgb=[rgba[j*4],rgba[j*4+1],rgba[j*4+2]];let best=null,limit=20*20;
      for(const c of clusters){const distance=c.mean.reduce((sum,v,k)=>sum+(v-rgb[k])**2,0);if(distance<=limit){limit=distance;best=c;}}
      if(!best){clusters.push({mean:rgb,ids:[j]});continue;}
      const n=best.ids.length;best.mean=best.mean.map((v,k)=>(v*n+rgb[k])/(n+1));best.ids.push(j);
    }
    clusters.sort((a,b)=>b.ids.length-a.ids.length);
    // Don't replace a structural color boundary with an arbitrary dominant side.
    if(clusters[0].ids.length/samples.size<.85){skipped++;continue;}
    const clean=clusters[0].ids;
    for(const i of points){
      const x=i%w,y=Math.floor(i/w);let sum=0,r=0,g=0,b=0;
      for(const j of clean){const weight=1/(1+(x-j%w)**2+(y-Math.floor(j/w))**2);sum+=weight;r+=rgba[j*4]*weight;g+=rgba[j*4+1]*weight;b+=rgba[j*4+2]*weight;}
      output[i*4]=Math.round(r/sum);output[i*4+1]=Math.round(g/sum);output[i*4+2]=Math.round(b/sum);output[i*4+3]=selection?selection[i]:255;
    }
    repaired++;
  }
  return {output,repaired,skipped};
}
module.exports={detect,repair};
