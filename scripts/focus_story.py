#!/usr/bin/env python3
"""A story view of the FOCUS-traced people: everyone aligned on the month FOCUS
found them, the group picture above the individual one.

Reads the same SinglePerson trace as focus_timeline.py and writes a
self-contained HTML page with: what state people were in before FOCUS, what
happened to the group month by month after it, every person as one thin
aligned row, and how often FOCUS had to find the same people again.

Usage: python3 focus_story.py <run-SinglePerson.xls> [out.html] [--month0 600 --year0 2007]
"""
import argparse, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from focus_timeline import parse, summarize, CARE


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('trace'); ap.add_argument('out', nargs='?')
    ap.add_argument('--month0', type=int, default=600); ap.add_argument('--year0', type=int, default=2007)
    a = ap.parse_args()
    people, last_month = parse(a.trace)
    traced = sorted((p for p in people.values() if p['joined'] is not None), key=lambda p: (p['joined'], p['id']))
    if not traced:
        sys.exit('no "joins the trace" lines found; was <numberFocusToTrace> set and --focus on?')
    rows = [{'id': p['id'], 'sex': p['sex'], 'profile': p['profile'], 'joined': p['joined'],
             'events': [{k: v for k, v in e.items() if k != 'text'} for e in p['events']],
             'summary': summarize(p, last_month)} for p in traced]
    out = a.out or os.path.join(os.path.dirname(os.path.abspath(a.trace)), 'focus_story.html')
    data = json.dumps({'people': rows, 'last_month': last_month, 'month0': a.month0, 'year0': a.year0,
                       'care': CARE, 'source': os.path.basename(a.trace)}, separators=(',', ':'))
    open(out, 'w').write(TEMPLATE.replace('/*DATA*/', data))
    print(f'{len(rows)} people; wrote {out}')


TEMPLATE = r'''<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>After FOCUS</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=IBM+Plex+Sans:wght@400;500;600&family=Spectral:wght@600;700&display=swap">
<style>
:root{--ground:#f6f7f9;--surface:#fff;--surface-2:#eef1f5;--ink:#10141c;--ink-2:#4b5365;--ink-3:#7a8397;--rule:#dde2ea;
 --care:#1baf7a;--lost:#eb6834;--dead:#b4bcc8;--undx:#8d96a6;--unlinked:#eda100;--neg:#e6e9ef;--prep:#2a78d6;--art:#4a3aa7;
 --serif:"Spectral",Georgia,serif;--sans:"IBM Plex Sans",system-ui,sans-serif;--mono:"IBM Plex Mono",ui-monospace,monospace}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--ground:#0f1216;--surface:#161a20;--surface-2:#1f242b;--ink:#f2f4f8;--ink-2:#b6bdcb;--ink-3:#8891a2;--rule:#272d36;
 --care:#199e70;--lost:#d95926;--dead:#4a5361;--undx:#737d8e;--unlinked:#c98500;--neg:#262c35;--prep:#3987e5;--art:#9085e9}}
*{box-sizing:border-box} body{background:var(--ground);color:var(--ink);font-family:var(--sans);font-size:15px;line-height:1.55;margin:0;padding:44px 20px}
.wrap{max-width:1080px;margin:0 auto;display:flex;flex-direction:column;gap:40px}
.eyebrow{font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--ink-3)}
h1{font-family:var(--serif);font-weight:700;font-size:clamp(30px,5vw,48px);line-height:1.08;margin:8px 0 12px;text-wrap:balance}
h2{font-family:var(--serif);font-weight:600;font-size:24px;margin:0 0 4px}
.standfirst,.sub{color:var(--ink-2);max-width:68ch;margin:0} .sub{font-size:14.5px;margin-bottom:12px}
.panel{background:var(--surface);border:1px solid var(--rule);border-radius:4px;padding:18px}
.big{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:12px}
.stat{background:var(--surface);border:1px solid var(--rule);border-radius:4px;padding:16px 18px}
.stat .n{font-family:var(--serif);font-weight:700;font-size:40px;line-height:1.05} .stat .l{color:var(--ink-2);font-size:13.5px;margin-top:4px}
.key{display:flex;flex-wrap:wrap;gap:6px 20px;font-size:13px;color:var(--ink-2);margin-bottom:10px}
.key span{display:inline-flex;align-items:center;gap:7px} .key i{display:inline-block;width:16px;height:9px;border-radius:2px}
svg{display:block;width:100%;height:auto;overflow:visible} svg text{font-family:var(--sans);font-size:11.5px;fill:var(--ink-3)}
.lab{font-size:12.5px;fill:var(--ink-2)} .stage{position:relative}
.tip{position:absolute;pointer-events:none;background:var(--ink);color:var(--surface);font-size:12.5px;line-height:1.45;padding:7px 10px;border-radius:4px;max-width:280px;opacity:0;transition:opacity .08s;z-index:3}
.cap{font-family:var(--mono);font-size:11px;color:var(--ink-3);margin-top:10px;line-height:1.6}
.two{display:grid;grid-template-columns:1fr 1fr;gap:18px} @media (max-width:760px){.two{grid-template-columns:1fr}}
.notes{color:var(--ink-2);font-size:14px;max-width:78ch} .notes li{margin-bottom:6px}
</style></head><body><div class="wrap">
<header><div class="eyebrow">TRANSM · FOCUS · the first 100 people it reached</div>
<h1 id="h1"></h1><p class="standfirst" id="stand"></p></header>

<section><div class="big" id="stats"></div></section>

<section><h2>Before FOCUS found them</h2>
<p class="sub">Each dot is one person. Left: how long they had been living with HIV when FOCUS selected them. Right: how long they had been out of care at that moment. Hover a dot for the person.</p>
<div class="panel"><div class="two">
 <div class="stage"><svg id="yrs" viewBox="0 0 500 220"></svg><div class="tip"></div></div>
 <div class="stage"><svg id="lost" viewBox="0 0 500 220"></svg><div class="tip"></div></div>
</div></div></section>

<section><h2>What happened next, month by month</h2>
<p class="sub">All 100 people lined up on the month FOCUS first selected them. The band shows how many were in care, had been lost to follow-up again, or had died, each month after that.</p>
<div class="panel">
 <div class="key"><span><i style="background:var(--care)"></i>in care (including returned to care)</span><span><i style="background:var(--lost)"></i>lost to follow-up again</span><span><i style="background:var(--dead)"></i>died</span></div>
 <div class="stage"><svg id="area" viewBox="0 0 1000 300"></svg><div class="tip"></div></div>
 <div class="cap" id="areacap"></div>
</div></section>

<section><h2>Every person, one row</h2>
<p class="sub">The same alignment, one thin row per person: ten years before FOCUS on the left, everything after on the right. Rows are sorted by outcome. Hover a row for the person.</p>
<div class="panel">
 <div class="key"><span><i style="background:var(--neg)"></i>HIV-negative</span><span><i style="background:var(--prep)"></i>on PrEP</span><span><i style="background:var(--undx)"></i>undiagnosed</span><span><i style="background:var(--unlinked)"></i>diagnosed, not linked</span><span><i style="background:var(--care)"></i>in care</span><span><i style="background:var(--lost)"></i>lost to follow-up</span><span><i style="background:var(--dead)"></i>died</span><span>▾ FOCUS selection</span></div>
 <div class="stage"><svg id="rows"></svg><div class="tip"></div></div>
 <div class="cap" id="rowscap"></div>
</div></section>

<section><h2>How many times FOCUS had to find the same person</h2>
<p class="sub" id="resub"></p>
<div class="panel"><svg id="resel" viewBox="0 0 1000 200"></svg></div></section>

<section><h2>How to read this</h2><ul class="notes">
<li><b>One run, 100 people.</b> The first 100 people FOCUS selected in a 100,000-agent run of the Miami FOCUS scenario (seed 0). Counts move by chance; a different seed gives different people.</li>
<li><b>Before selection</b> only milestones are known (infection, diagnosis, care changes, ART, PrEP), so the pre-FOCUS bands are filled from the last known state. After selection the full trace applies.</li>
<li><b>"In care"</b> merges CEPAC's <i>in care</i> and <i>returned to care</i> states. "Lost to follow-up again" means CEPAC moved them back to LTFU after FOCUS re-engaged them; FOCUS can then select them again.</li>
<li><b>Calendar.</b> Simulation month <span id="m0"></span> is January <span id="y0"></span> (the FOCUS convention). The run ends at month <span id="mend"></span>.</li>
</ul></section>
</div>
<script>
const D=/*DATA*/; const P=D.people; const N=P.length; const $=id=>document.getElementById(id);
const MON=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
const dateOf=m=>{const k=m-D.month0;return MON[((k%12)+12)%12]+' '+(D.year0+Math.floor(k/12));};
const role=s=>(D.care[s]||[s,'neg'])[1];
$('m0').textContent=D.month0;$('y0').textContent=D.year0;$('mend').textContent=D.last_month;

/* per-person monthly state from -120 to +end, relative to first selection */
const PRE=120;
function states(p){
  const j=p.joined,end=p.summary.end,out=new Map(); let care='neg',prep=false,art=false,cur=p.summary.born??p.events[0].month;
  const ev=[...p.events].sort((a,b)=>a.month-b.month||a.order-b.order); let i=0;
  for(let m=cur;m<=end;m++){
    while(i<ev.length&&ev[i].month<=m){const e=ev[i++];
      if(e.kind==='infected')care='undx'; else if(e.kind==='care')care=role(e.care);
      else if(e.kind==='hiv_status'&&care==='undx'&&/\(observed\)/.test(e.status)&&!/unobserved/.test(e.status))care='unlinked';
      else if(e.kind==='selected'&&e.care_after)care=role(e.care_after);
      else if(e.kind==='prep_on')prep=true; else if(e.kind==='prep_off')prep=false;
      else if(e.kind==='art_on')art=true; else if(e.kind==='art_off')art=false;}
    out.set(m-j,{care,prep,art});}
  return out;
}
const S=P.map(states);
const grp=c=>c==='care'||c==='rtc'?'care':c==='ltfu'?'lost':c;
const maxT=Math.max(...P.map(p=>p.summary.end-p.joined));
const fullT=D.last_month-Math.max(...P.map(p=>p.joined));   /* last month after selection at which everyone is still observed */

/* stats */
const med=a=>{a=[...a].sort((x,y)=>x-y);return a.length?a[a.length>>1]:0;};
const yrsInf=P.map(p=>p.summary.infected!==null?(p.joined-p.summary.infected)/12:null).filter(v=>v!==null);
const lostFor=P.map(p=>{const s=p.summary;if(!s.care_at_selection||!/LTFU/.test(s.care_at_selection))return null;
  let t=null;for(const e of p.events){if(e.month>=p.joined)break;if(e.kind==='care'&&role(e.care)==='ltfu')t=e.month;}return t===null?null:p.joined-t;}).filter(v=>v!==null);
const lostAgain=P.filter((p,k)=>{for(const [t,s] of S[k])if(t>0&&t<=24&&s.care==='ltfu')return true;return false;}).length;
const died=P.filter(p=>p.summary.died).length, inCareEnd=P.filter(p=>!p.summary.died&&/in_care|RTC/.test(p.summary.care_end)).length;
const reselected=P.filter(p=>p.summary.selections>1).length;
$('h1').textContent=`FOCUS found ${N} people. Within two years, ${lostAgain} had dropped out of care again.`;
$('stand').textContent=`The first ${N} people FOCUS selected, ${dateOf(Math.min(...P.map(p=>p.joined)))} to ${dateOf(Math.max(...P.map(p=>p.joined)))}: who they were when it found them, and what happened over the following ${Math.round(maxT/12)} years.`;
const stat=(n,l)=>`<div class="stat"><div class="n">${n}</div><div class="l">${l}</div></div>`;
$('stats').innerHTML=stat(med(yrsInf).toFixed(1)+' yr','median time living with HIV when FOCUS found them')+
 stat(Math.round(med(lostFor)/12*10)/10+' yr','median time out of care, for the '+lostFor.length+' who had been lost')+
 stat(lostAgain,'lost to follow-up again at least once within two years')+stat(inCareEnd,'alive and in care at the end of the run')+stat(died,'died during the run (median age '+Math.round(med(P.filter(p=>p.summary.died).map(p=>(p.events.find(e=>e.kind==='death')||{}).age||0))/12)+')');

/* tooltip helper */
function tipOn(stage,el,html){const t=stage.querySelector('.tip');el.addEventListener('mousemove',ev=>{const r=stage.getBoundingClientRect();t.innerHTML=html;t.style.left=Math.min(ev.clientX-r.left+12,r.width-290)+'px';t.style.top=(ev.clientY-r.top+12)+'px';t.style.opacity=1;});el.addEventListener('mouseleave',()=>t.style.opacity=0);}
const who=p=>`<b>Person ${p.id}</b> · ${p.sex==='Male'?'man':'woman'}, ${p.profile.split(':').slice(-2).join(' ').toLowerCase().replace('non_hispanic','non-Hispanic')}<br>`+
 (p.summary.infected!==null?`HIV ${dateOf(p.summary.infected)} · `:'')+`FOCUS ${dateOf(p.joined)}${p.summary.selections>1?' ×'+p.summary.selections:''}`+(p.summary.died?` · died ${dateOf(p.summary.end)}`:'');

/* dot strips */
function strip(id,vals,xmax,unit,color,title){
  const svg=$(id),W=500,H=220,L=14,R=14,T=36,B=40; const X=v=>L+v/xmax*(W-L-R);
  const bins=new Map(); let s=`<text x="${L}" y="16" class="lab">${title}</text>`;
  for(let v=0;v<=xmax;v+=xmax>20?5:xmax>8?2:1)s+=`<line x1="${X(v)}" x2="${X(v)}" y1="${T}" y2="${H-B}" stroke="var(--rule)"/><text x="${X(v)}" y="${H-B+16}" text-anchor="middle">${v}</text>`;
  s+=`<text x="${W-R}" y="${H-6}" text-anchor="end">${unit}</text>`;
  const dots=[];for(const [p,v] of vals){const b=Math.round(X(v)/7);const k=(bins.get(b)||0);bins.set(b,k+1);dots.push({p,v,x:X(v),y:H-B-8-k*7});}
  for(const d of dots)s+=`<circle class="dot" data-id="${d.p.id}" cx="${d.x}" cy="${Math.max(T+4,d.y)}" r="3.2" fill="${color}" stroke="var(--surface)" stroke-width="1"/>`;
  const m=med(vals.map(x=>x[1]));s+=`<line x1="${X(m)}" x2="${X(m)}" y1="${T-6}" y2="${H-B}" stroke="var(--ink)" stroke-dasharray="3 3"/><text x="${X(m)+4}" y="${T+4}" class="lab">median ${m.toFixed(1)}</text>`;
  svg.innerHTML=s; const stage=svg.parentElement;
  svg.querySelectorAll('.dot').forEach(c=>{const p=P.find(q=>q.id==c.dataset.id);tipOn(stage,c,who(p)+`<br>${vals.find(x=>x[0]===p)[1].toFixed(1)} ${unit}`);});
}
strip('yrs',P.filter(p=>p.summary.infected!==null).map(p=>[p,(p.joined-p.summary.infected)/12]),Math.ceil(Math.max(...yrsInf)),'years living with HIV','var(--lost)',`${yrsInf.length} people: years since infection when FOCUS selected them`);
const lostPairs=P.map(p=>{const s=p.summary;if(!s.care_at_selection||!/LTFU/.test(s.care_at_selection))return null;let t=null;for(const e of p.events){if(e.month>=p.joined)break;if(e.kind==='care'&&role(e.care)==='ltfu')t=e.month;}return t===null?null:[p,(p.joined-t)/12];}).filter(Boolean);
strip('lost',lostPairs,Math.ceil(Math.max(...lostPairs.map(x=>x[1]))),'years out of care','var(--ink-2)',`${lostPairs.length} people: years since they were last in care`);

/* stacked area after selection */
{
  const svg=$('area'),W=1000,H=300,L=44,R=16,T=14,B=36; const X=t=>L+t/fullT*(W-L-R),Y=v=>H-B-v/N*(H-T-B);
  const counts=[];for(let t=0;t<=fullT;t++){const c={care:0,lost:0,dead:0,other:0,n:0};for(let k=0;k<N;k++){const p=P[k];c.n++;const s=S[k].get(t);if(!s){c.dead++;continue;}const g=grp(s.care);if(g==='care')c.care++;else if(g==='lost')c.lost++;else c.other++;}counts.push(c);}
  const layers=[['care','var(--care)'],['other','var(--unlinked)'],['lost','var(--lost)'],['dead','var(--dead)']];
  let s='',base=counts.map(()=>0);
  for(const [k,col] of layers){const top=counts.map((c,i)=>base[i]+c[k]);
    const d='M'+counts.map((c,i)=>X(i).toFixed(1)+' '+Y(top[i]).toFixed(1)).join('L')+'L'+counts.map((c,i)=>X(counts.length-1-i).toFixed(1)+' '+Y(base[counts.length-1-i]).toFixed(1)).join('L')+'Z';
    s+=`<path d="${d}" fill="${col}" stroke="var(--surface)" stroke-width="1"/>`;base=top;}
  for(let v=0;v<=N;v+=25)s+=`<text x="${L-8}" y="${Y(v)+4}" text-anchor="end">${v}</text><line x1="${L}" x2="${W-R}" y1="${Y(v)}" y2="${Y(v)}" stroke="var(--rule)" opacity=".6"/>`;
  for(let y=0;y*12<=fullT;y+=2)s+=`<text x="${X(y*12)}" y="${H-B+18}" text-anchor="middle">${y===0?'FOCUS':y+' yr'}</text>`;
  for(const [t,lab] of [[24,'2 years'],[60,'5 years'],[120,'10 years']])if(t<fullT)s+=`<line x1="${X(t)}" x2="${X(t)}" y1="${T}" y2="${H-B}" stroke="var(--ink-3)" stroke-dasharray="3 3"/>`;
  s+=`<rect id="hov" x="0" y="${T}" width="1" height="${H-T-B}" fill="var(--ink)" opacity="0"/>`;
  svg.innerHTML=s; const stage=svg.parentElement,tip=stage.querySelector('.tip'),hov=svg.querySelector('#hov');
  svg.addEventListener('mousemove',ev=>{const r=svg.getBoundingClientRect();const t=Math.max(0,Math.min(fullT,Math.round(((ev.clientX-r.left)/r.width*W-L)/(W-L-R)*fullT)));const c=counts[t];
    hov.setAttribute('x',X(t));hov.setAttribute('opacity',.5);tip.innerHTML=`<b>${t} months after FOCUS</b> (${(t/12).toFixed(1)} yr)<br>in care ${c.care} · lost again ${c.lost}${c.other?' · diagnosed, not linked '+c.other:''} · died ${c.dead}`;
    tip.style.left=Math.min(ev.clientX-r.left+12,r.width-230)+'px';tip.style.top='10px';tip.style.opacity=1;});
  svg.addEventListener('mouseleave',()=>{tip.style.opacity=0;hov.setAttribute('opacity',0);});
  const c24=counts[Math.min(24,fullT)],c60=counts[Math.min(60,fullT)],cl=counts[counts.length-1];
  $('areacap').textContent=`in care / lost again / died — 2 years after FOCUS: ${c24.care} / ${c24.lost} / ${c24.dead} · 5 years: ${c60.care} / ${c60.lost} / ${c60.dead} · ${(fullT/12).toFixed(0)} years (the last month all ${N} are observed, since selections span ${Math.max(...P.map(p=>p.joined))-Math.min(...P.map(p=>p.joined))} months): ${cl.care} / ${cl.lost} / ${cl.dead}`;
}

/* aligned rows */
{
  const svg=$('rows'),W=1000,RH=5.5,L=0,R=0; const X=t=>L+(t+PRE)/(PRE+maxT)*(W-L-R);
  const order=[...P.keys()].sort((a,b)=>{const pa=P[a].summary,pb=P[b].summary;const ra=pa.died?2:/LTFU/.test(pa.care_end)?1:0,rb=pb.died?2:/LTFU/.test(pb.care_end)?1:0;return ra-rb||(pa.end-P[a].joined)-(pb.end-P[b].joined)||a-b;});
  const H=order.length*RH+30; svg.setAttribute('viewBox',`0 0 ${W} ${H}`); svg.style.height=H+'px';
  let s='';const col={neg:'var(--neg)',undx:'var(--undx)',unlinked:'var(--unlinked)',care:'var(--care)',rtc:'var(--care)',ltfu:'var(--lost)'};
  order.forEach((k,i)=>{const p=P[k],st=S[k],y=20+i*RH;let seg=null;
    const flush=()=>{if(seg)s+=`<rect x="${X(seg.a).toFixed(1)}" y="${y}" width="${(X(seg.b+1)-X(seg.a)).toFixed(1)}" height="${RH-1}" fill="${col[seg.c]}"/>`;seg=null;};
    for(let t=-PRE;t<=maxT;t++){const v=st.get(t);const c=v?v.care:(t>p.summary.end-p.joined&&p.summary.died?'dead':null);
      if(!c){flush();continue;} if(c==='dead'){flush();s+=`<rect x="${X(t).toFixed(1)}" y="${y}" width="${(X(maxT+1)-X(t)).toFixed(1)}" height="${RH-1}" fill="var(--dead)"/>`;break;}
      if(seg&&seg.c===c)seg.b=t;else{flush();seg={a:t,b:t,c};}}
    flush();
    for(let t=-PRE;t<=maxT;t++){const v=st.get(t);if(v&&v.prep)s+=`<rect x="${X(t).toFixed(1)}" y="${y}" width="${(X(t+1)-X(t)).toFixed(1)}" height="1.5" fill="var(--prep)"/>`;}
    for(const e of p.events)if(e.kind==='selected'&&e.month-p.joined>0)s+=`<path d="M${X(e.month-p.joined)-2.2} ${y-0.5}L${X(e.month-p.joined)+2.2} ${y-0.5}L${X(e.month-p.joined)} ${y+2.5}Z" fill="var(--ink)"/>`;
    s+=`<rect class="hit" data-k="${k}" x="0" y="${y-0.5}" width="${W}" height="${RH}" fill="transparent"/>`;});
  s+=`<line x1="${X(0)}" x2="${X(0)}" y1="12" y2="${H-8}" stroke="var(--ink)" stroke-width="1.2"/><text x="${X(0)+5}" y="10" class="lab">FOCUS selection</text>`;
  for(const y of [-10,-5,5,10,15,20])if(y*12>-PRE&&y*12<maxT)s+=`<text x="${X(y*12)}" y="10" text-anchor="middle">${y>0?'+':''}${y} yr</text>`;
  svg.innerHTML=s; const stage=svg.parentElement;
  svg.querySelectorAll('.hit').forEach(r=>{const p=P[+r.dataset.k];tipOn(stage,r,who(p));});
  const groups=[P.filter(p=>!p.summary.died&&!/LTFU/.test(p.summary.care_end)).length,P.filter(p=>!p.summary.died&&/LTFU/.test(p.summary.care_end)).length,died];
  $('rowscap').textContent=`top to bottom: ${groups[0]} alive and in care at the end, then ${groups[1]} alive but lost to follow-up, then ${groups[2]} who died (earliest death first)`;
}

/* reselection bars */
{
  const svg=$('resel'),W=1000,H=200,L=40,R=16,T=20,B=40; const max=Math.max(...P.map(p=>p.summary.selections));
  const c=Array.from({length:max},(_,i)=>P.filter(p=>p.summary.selections===i+1).length); const top=Math.max(...c);
  const bw=(W-L-R)/max; let s='';
  c.forEach((n,i)=>{const h=n/top*(H-T-B),x=L+i*bw+bw*0.15;s+=`<rect x="${x}" y="${H-B-h}" width="${bw*0.7}" height="${h}" rx="3" fill="var(--lost)"/><text x="${x+bw*0.35}" y="${H-B-h-6}" text-anchor="middle" class="lab">${n}</text><text x="${x+bw*0.35}" y="${H-B+18}" text-anchor="middle">${i+1}${i===0?' time':' times'}</text>`;});
  svg.innerHTML=s;
  const atLeast=k=>P.filter(p=>p.summary.selections>=k).length;
  $('resub').textContent=`Number of times each of the ${N} people was selected over the run. ${reselected} were selected again after being lost once more; ${atLeast(3)} three times or more, ${atLeast(5)} five times or more.`;
}
</script></body></html>
'''

if __name__ == '__main__':
    main()
