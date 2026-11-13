#!/usr/bin/env python3
"""Turn a SinglePerson trace into one timeline per FOCUS-traced person.

Reads <sim>-SinglePerson.xls written with <numberFocusToTrace> set (issue #98)
and writes a self-contained HTML page: one row per person, from their creation
to the end of the run, with bands for PrEP, HIV, care state and ART, and marks
for FOCUS screenings and selections, diagnosis and death. The back-dated
history lines give the part before FOCUS reached them; the normal trace gives
the rest.

Usage: python3 focus_timeline.py <run-SinglePerson.xls> [out.html] [--month0 600 --year0 2007]

The calendar mapping defaults to the FOCUS convention (simulation month 600 =
January 2007). Pass --month0/--year0 to change it.
"""
import argparse, collections, html, json, os, re, sys

CARE = {  # CEPAC care-state strings as they appear in the trace -> short label, colour role
    'HIV-': ('negative', 'neg'), 'HIV+undetected': ('undiagnosed', 'undx'),
    'HIV+unlinked(detected not in care': ('diagnosed, unlinked', 'unlinked'),
    'HIV+in_care': ('in care', 'care'), 'HIV+LTFU': ('lost to follow-up', 'ltfu'),
    'HIV+RTC': ('returned to care', 'rtc'), 'none': ('no CEPAC patient', 'neg'),
}
MON = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']


def parse(path):
    people = {}          # id -> dict
    month = 0
    last_month = 0
    join_re = re.compile(r'^Demographic info for person:@ (Male|Female) (\d+) \((\S+) age (\d+), (HIGH|LOW) risk\) joins the trace')
    hist_re = re.compile(r'^ !!# (\d+) history, month (\d+): (.*)$')
    ev_re = re.compile(r'^ !!# ?(\d+) ?(.*)$')
    acts_re = re.compile(r'^# \S+ (\d+) engages in (\d+) acts with (?:his|her) (\w+) (\d+)')

    def person(pid):
        return people.setdefault(pid, {'id': pid, 'events': [], 'cd4': [], 'partners': collections.defaultdict(set), 'joined': None})

    for line in open(path, errors='ignore'):
        line = line.rstrip('\n')
        m = re.match(r'^=== Month (\d+) ===', line)
        if m:
            month = int(m.group(1)); last_month = max(last_month, month); continue
        m = join_re.match(line)
        if m:
            p = person(int(m.group(2)))
            p.update(sex=m.group(1), profile=m.group(3), age_at_join=int(m.group(4)), risk=m.group(5), joined=month)
            continue
        m = hist_re.match(line)
        if m:
            pid, mo, text = int(m.group(1)), int(m.group(2)), m.group(3)
            if pid in people:
                ev = classify(mo, text, history=True)
                # the selection that put them on the trace is also written by the
                # forward trace in the same month; keep that copy, it has more detail
                if ev and not (ev['kind'] == 'selected' and mo == people[pid]['joined']):
                    people[pid]['events'].append(ev)
            continue
        m = acts_re.match(line)
        if m:
            a, b = int(m.group(1)), int(m.group(4))
            for x, y in ((a, b), (b, a)):
                if x in people and people[x]['joined'] is not None:
                    people[x]['partners'][month].add(y)
            continue
        m = ev_re.match(line)
        if m and int(m.group(1)) in people:
            pid, text = int(m.group(1)), m.group(2).strip()
            p = people[pid]
            if p['joined'] is None:
                continue
            c = re.match(r'CD4 changed from ([\d.]+) to ([\d.]+)', text)
            if c:
                p['cd4'].append((month, float(c.group(2)))); continue
            if text.startswith('HVL changed'):
                continue
            ev = classify(month, text, history=False)
            if ev:
                p['events'].append(ev)
    for p in people.values():
        p['events'].sort(key=lambda e: (e['month'], e['order']))
        p['partners'] = {m: len(s) for m, s in p['partners'].items()}
    return people, last_month


def classify(month, text, history):
    """Map a trace or history line to an event dict, or None for lines the timeline ignores."""
    E = lambda kind, order=5, **kw: dict(kind=kind, month=month, order=order, history=history, text=text, **kw)
    if history:
        if text.startswith('created at age'):
            return E('created', 0, age=int(re.search(r'age (\d+)', text).group(1)))
        if text == 'becomes sexually active':
            return E('debut', 1)
        if text == 'starts PrEP': return E('prep_on', 2)
        if text == 'stops PrEP': return E('prep_off', 2)
        if text.startswith('seeded with prevalent HIV'): return E('infected', 3, generation=0)
        if text.startswith('infected with HIV'):
            return E('infected', 3, generation=int(re.search(r'generation (\d+)', text).group(1)))
        if text.startswith('HIV status becomes'):
            return E('hiv_status', 4, status=text.split('becomes ', 1)[1])
        if text.startswith('care state becomes'):
            return E('care', 4, care=text.split('becomes ', 1)[1])
        if text == 'starts ART': return E('art_on', 6)
        if text == 'stops ART': return E('art_off', 6)
        if text.startswith('screened by FOCUS'):
            return E('screened', 7, cohort=re.search(r'cohort (\S+)\)', text).group(1))
        if text.startswith('selected by FOCUS'):
            return E('selected', 8, cohort=re.search(r'cohort (\S+)\)', text).group(1))
        return None
    if text == 'starts PrEP': return E('prep_on', 2)
    if text == 'stops PrEP': return E('prep_off', 2)
    if text.startswith('HIV status changed'):
        return E('hiv_status', 4, status=text.split(' to ', 1)[1])
    if text.startswith('care state changed'):
        return E('care', 4, care=text.split(' to ', 1)[1])
    if text == 'started treatment.': return E('art_on', 6)
    if text == 'stopped treatment.': return E('art_off', 6)
    if text.startswith('selected by FOCUS'):
        m = re.search(r'cohort (\S+) \(selection (\d+), month \d+, care state (.*) -> (.*)\)', text)
        return E('selected', 8, cohort=m.group(1), selection=int(m.group(2)), care_before=m.group(3), care_after=m.group(4))
    if text.startswith('dies at age'):
        m = re.search(r'dies at age (\d+) months, (.*), care state (.*)$', text)
        return E('death', 9, age=int(m.group(1)), status=m.group(2), care=m.group(3))
    return None


def care_label(s):
    return CARE.get(s, (s, 'neg'))


def summarize(p, last_month):
    ev = p['events']
    first = lambda k: next((e for e in ev if e['kind'] == k), None)
    created = first('created'); infected = first('infected'); death = first('death')
    sel = [e for e in ev if e['kind'] == 'selected' and not e['history']]
    dx = next((e for e in ev if e['kind'] == 'hiv_status' and 'observed)' in e['status'] and 'unobserved' not in e['status']), None)
    care_now = 'none'
    for e in ev:
        if e['kind'] == 'care': care_now = e['care']
        if e['kind'] == 'death': care_now = e['care']
    end = death['month'] if death else last_month
    return {
        'born': created['month'] - created['age'] if created else None,
        'infected': infected['month'] if infected else None,
        'generation': infected['generation'] if infected else None,
        'diagnosed': dx['month'] if dx else None,
        'first_selected': p['joined'],
        'selections': len([e for e in ev if e['kind'] == 'selected']),
        'screenings': len([e for e in ev if e['kind'] == 'screened']),
        'care_at_selection': sel[0]['care_before'] if sel else None,
        'care_end': care_now, 'end': end, 'died': death is not None,
        'art_months': sum(1 for e in ev if e['kind'] == 'art_on'),
        'prep_ever': any(e['kind'] == 'prep_on' for e in ev),
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('trace'); ap.add_argument('out', nargs='?')
    ap.add_argument('--month0', type=int, default=600); ap.add_argument('--year0', type=int, default=2007)
    a = ap.parse_args()
    people, last_month = parse(a.trace)
    traced = sorted((p for p in people.values() if p['joined'] is not None), key=lambda p: (p['joined'], p['id']))
    if not traced:
        sys.exit('no "joins the trace" lines found; was <numberFocusToTrace> set and --focus on?')
    rows = [{'id': p['id'], 'sex': p['sex'], 'profile': p['profile'], 'risk': p['risk'], 'age_at_join': p['age_at_join'],
             'joined': p['joined'], 'events': p['events'], 'cd4': p['cd4'], 'partners': p['partners'],
             'summary': summarize(p, last_month)} for p in traced]
    out = a.out or os.path.join(os.path.dirname(os.path.abspath(a.trace)), 'focus_timelines.html')
    data = json.dumps({'people': rows, 'last_month': last_month, 'month0': a.month0, 'year0': a.year0,
                       'care': {k: v for k, v in CARE.items()}, 'source': os.path.basename(a.trace)}, separators=(',', ':'))
    open(out, 'w').write(TEMPLATE.replace('/*DATA*/', data))
    s = [r['summary'] for r in rows]
    print(f'{len(rows)} traced people, months {min(r["joined"] for r in rows)}-{last_month}; '
          f'{sum(x["died"] for x in s)} died, {sum(x["selections"] > 1 for x in s)} selected more than once, '
          f'{sum(x["prep_ever"] for x in s)} ever on PrEP, {sum(x["generation"] == 0 for x in s if x["generation"] is not None)} prevalent cases')
    print('wrote', out)


TEMPLATE = r'''<!DOCTYPE html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1">
<title>FOCUS Person Timelines</title>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500&family=IBM+Plex+Sans:wght@400;500;600&family=Spectral:wght@600;700&display=swap">
<style>
:root{--ground:#f6f7f9;--surface:#fff;--surface-2:#eef1f5;--ink:#10141c;--ink-2:#4b5365;--ink-3:#7a8397;--rule:#dde2ea;
 --prep:#2a78d6;--hiv:#eb6834;--care:#1baf7a;--rtc:#8fd9bd;--ltfu:#25292f;--undx:#b4bcc8;--unlinked:#eda100;--art:#4a3aa7;--neg:#e6e9ef;
 --serif:"Spectral",Georgia,serif;--sans:"IBM Plex Sans",system-ui,sans-serif;--mono:"IBM Plex Mono",ui-monospace,monospace}
@media (prefers-color-scheme:dark){:root:not([data-theme="light"]){--ground:#0f1216;--surface:#161a20;--surface-2:#1f242b;--ink:#f2f4f8;--ink-2:#b6bdcb;--ink-3:#8891a2;--rule:#272d36;
 --prep:#3987e5;--hiv:#d95926;--care:#199e70;--rtc:#4f9f80;--ltfu:#e6e9ef;--undx:#566070;--unlinked:#c98500;--art:#9085e9;--neg:#262c35}}
*{box-sizing:border-box} body{background:var(--ground);color:var(--ink);font-family:var(--sans);font-size:14.5px;line-height:1.55;margin:0;padding:40px 20px}
.wrap{max-width:1180px;margin:0 auto;display:flex;flex-direction:column;gap:28px}
.eyebrow{font-family:var(--mono);font-size:11px;letter-spacing:.14em;text-transform:uppercase;color:var(--ink-3)}
h1{font-family:var(--serif);font-weight:700;font-size:clamp(28px,4.5vw,44px);line-height:1.1;margin:6px 0 10px}
h2{font-family:var(--serif);font-weight:600;font-size:22px;margin:0 0 8px}
.standfirst{color:var(--ink-2);max-width:70ch;margin:0}
.panel{background:var(--surface);border:1px solid var(--rule);border-radius:4px;padding:16px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px}
.tile{background:var(--surface);border:1px solid var(--rule);border-radius:4px;padding:12px 14px}
.tile .lab{font-size:12px;color:var(--ink-2)} .tile .num{font-family:var(--serif);font-weight:600;font-size:26px}
.key{display:flex;flex-wrap:wrap;gap:6px 18px;font-size:12.5px;color:var(--ink-2);margin-bottom:10px}
.key span{display:inline-flex;align-items:center;gap:6px} .key i{display:inline-block;width:18px;height:8px;border-radius:2px}
.key b{font-weight:500;color:var(--ink)}
.controls{display:flex;flex-wrap:wrap;gap:10px 18px;align-items:center;font-size:13px;color:var(--ink-2)}
select,input{font:inherit;font-size:13px;padding:4px 8px;border:1px solid var(--rule);border-radius:4px;background:var(--surface);color:var(--ink)}
.row{display:grid;grid-template-columns:150px 1fr;gap:10px;align-items:center;border-top:1px solid var(--rule);padding:6px 0}
.row:first-of-type{border-top:0}
.who{font-size:12px;color:var(--ink-2);line-height:1.35} .who b{color:var(--ink);font-family:var(--mono);font-weight:500;font-size:12.5px}
svg{display:block;width:100%;height:auto;overflow:visible} svg text{font-family:var(--sans);font-size:10px;fill:var(--ink-3)}
.axis text{font-size:11px} .mark{cursor:default}
.cap{font-family:var(--mono);font-size:11px;color:var(--ink-3);margin-top:8px;line-height:1.6}
table{border-collapse:collapse;width:100%;font-size:12.5px;font-variant-numeric:tabular-nums} th,td{text-align:right;padding:4px 8px;border-bottom:1px solid var(--rule);white-space:nowrap}
th{color:var(--ink-2);font-weight:500} th:first-child,td:first-child{text-align:left}
.scroll{overflow-x:auto} details summary{cursor:pointer;color:var(--ink-2)}
.notes{color:var(--ink-2);font-size:13.5px;max-width:80ch} .notes li{margin-bottom:5px}
</style></head><body><div class="wrap">
<header><div class="eyebrow">TRANSM single-person trace · FOCUS-selected people</div>
<h1>What happened to the people FOCUS reached</h1>
<p class="standfirst">One row per person, from the month they were created to the end of the run or their death. The part before FOCUS reached them comes from each person's milestone history; the part after comes from the full trace. Hover any band or mark for the detail.</p></header>
<section><div class="tiles" id="tiles"></div></section>
<section><div class="controls">
 <label>Sort by <select id="sort"><option value="joined">month selected</option><option value="born">birth</option><option value="infected">infection</option><option value="cohort">cohort</option></select></label>
 <label>Show <select id="filter"><option value="all">everyone</option><option value="died">died</option><option value="multi">selected more than once</option><option value="prep">ever on PrEP</option><option value="ltfu_end">lost to follow-up at the end</option></select></label>
 <label>From <input id="from" type="number" style="width:7em"></label>
 <span id="count"></span></div></section>
<section><div class="panel">
 <div class="key">
  <span><i style="background:var(--neg)"></i>HIV-negative</span><span><i style="background:var(--prep)"></i>on PrEP</span>
  <span><i style="background:var(--undx)"></i>HIV+ undiagnosed</span><span><i style="background:var(--unlinked)"></i>diagnosed, not linked</span>
  <span><i style="background:var(--care)"></i>in care</span><span><i style="background:var(--rtc)"></i>returned to care</span><span><i style="background:var(--ltfu)"></i>lost to follow-up</span>
  <span><i style="background:var(--art);height:4px"></i>on ART (thin bar below)</span>
  <span><b>▽</b> screened by FOCUS</span><span><b style="color:var(--hiv)">▼</b> selected by FOCUS</span><span><b>|</b> diagnosed</span><span><b>✕</b> death</span>
 </div>
 <div id="axis"></div><div id="rows"></div>
 <div class="cap" id="cap"></div></div></section>
<section><h2>Every person</h2><details open><summary>Table</summary><div class="scroll"><table id="tbl"></table></div></details></section>
<section><h2>How to read this</h2><ul class="notes">
 <li><b>Time.</b> Simulation months; the calendar labels use the FOCUS convention, month <span id="m0"></span> = January <span id="y0"></span>.</li>
 <li><b>Before selection</b> only milestones are known: creation, sexual debut, PrEP start/stop, infection, diagnosis, care-state changes, ART start/stop and earlier FOCUS screenings. The band between milestones is filled in from the last known state.</li>
 <li><b>After selection</b> the full trace applies: the same milestones plus partnerships and monthly CD4, which the table summarises.</li>
 <li><b>"Selected" vs "screened".</b> FOCUS samples people from a cohort (screened), then a probability roll decides who is selected. A person can be screened and selected several times.</li>
 <li><b>Care states</b> are CEPAC's: undiagnosed, diagnosed-but-unlinked, in care, lost to follow-up, returned to care. "Returned to care" is CEPAC's own state after re-engagement.</li>
</ul></section>
</div>
<script>
const D=/*DATA*/;
const $=id=>document.getElementById(id);
const MON=['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'];
const dateOf=m=>{const k=m-D.month0;const y=D.year0+Math.floor(k/12);return MON[((k%12)+12)%12]+' '+y;};
const careRole=s=>(D.care[s]||[s,'neg'])[1], careLab=s=>(D.care[s]||[s,'neg'])[0];
$('m0').textContent=D.month0;$('y0').textContent=D.year0;
const people=D.people;
const minMonth=Math.min(...people.map(p=>p.summary.born??p.joined)), maxMonth=D.last_month;
const W=1000,L=4,R=4;const X=m=>L+(m-minMonth)/(maxMonth-minMonth)*(W-L-R);

/* state bands: walk the events and emit segments for care, PrEP and ART */
function bands(p){
  const segs=[],ev=p.events,end=p.summary.end;
  let care='neg',prep=false,art=false,start=p.summary.born??ev[0].month,ps=null,as=null;
  const push=(m)=>{if(m>start)segs.push({a:start,b:m,role:care});start=m;};
  for(const e of ev){
    if(e.kind==='infected'){push(e.month);care='undx';}
    else if(e.kind==='care'){push(e.month);care=careRole(e.care);}
    else if(e.kind==='selected'&&e.care_after){push(e.month);care=careRole(e.care_after);}
    else if(e.kind==='hiv_status'&&care==='undx'&&/\(observed\)/.test(e.status)&&!/unobserved/.test(e.status)){push(e.month);care='unlinked';}
    else if(e.kind==='prep_on'){ps=e.month;} else if(e.kind==='prep_off'&&ps!==null){segs.push({a:ps,b:e.month,role:'prep'});ps=null;}
    else if(e.kind==='art_on'){as=e.month;} else if(e.kind==='art_off'&&as!==null){segs.push({a:as,b:e.month,role:'art'});as=null;}
  }
  push(end); if(ps!==null)segs.push({a:ps,b:end,role:'prep'}); if(as!==null)segs.push({a:as,b:end,role:'art'});
  return segs;
}
function rowSvg(p){
  const H=34, y0=6, h=14; let s='';
  for(const g of bands(p)){
    const x=X(g.a),w=Math.max(1.2,X(g.b)-X(g.a));
    if(g.role==='prep') s+=`<rect x="${x}" y="${y0-4}" width="${w}" height="4" fill="var(--prep)"><title>on PrEP, ${dateOf(g.a)} to ${dateOf(g.b)}</title></rect>`;
    else if(g.role==='art') s+=`<rect x="${x}" y="${y0+h+1}" width="${w}" height="4" fill="var(--art)"><title>on ART, ${dateOf(g.a)} to ${dateOf(g.b)}</title></rect>`;
    else s+=`<rect x="${x}" y="${y0}" width="${w}" height="${h}" fill="var(--${g.role})"><title>${careLab(Object.keys(D.care).find(k=>D.care[k][1]===g.role)||g.role)}, ${dateOf(g.a)} to ${dateOf(g.b)} (${g.b-g.a} months)</title></rect>`;
  }
  s+=`<line x1="${X(p.joined)}" x2="${X(p.joined)}" y1="0" y2="${H}" stroke="var(--hiv)" stroke-width="1" stroke-dasharray="2 2"/>`;
  for(const e of p.events){
    const x=X(e.month), t=`${dateOf(e.month)} (month ${e.month}): ${e.text}`;
    if(e.kind==='screened') s+=`<path class="mark" d="M${x-4} ${y0-9}L${x+4} ${y0-9}L${x} ${y0-3}Z" fill="none" stroke="var(--ink-2)" stroke-width="1"><title>${t}</title></path>`;
    else if(e.kind==='selected') s+=`<path class="mark" d="M${x-4.5} ${y0-10}L${x+4.5} ${y0-10}L${x} ${y0-3}Z" fill="var(--hiv)"><title>${t}</title></path>`;
    else if(e.kind==='hiv_status'&&/\(observed\)/.test(e.status)&&!/unobserved/.test(e.status)) s+=`<line class="mark" x1="${x}" x2="${x}" y1="${y0-2}" y2="${y0+h+2}" stroke="var(--ink)" stroke-width="1.5"><title>${t}</title></line>`;
    else if(e.kind==='death') s+=`<path class="mark" d="M${x-4} ${y0+2}L${x+4} ${y0+h-2}M${x+4} ${y0+2}L${x-4} ${y0+h-2}" stroke="var(--ink)" stroke-width="1.8"><title>${t}</title></path>`;
    else if(e.kind==='debut') s+=`<circle class="mark" cx="${x}" cy="${y0+h/2}" r="2.2" fill="var(--ink-3)"><title>${t}</title></circle>`;
  }
  return `<svg viewBox="0 -6 ${W} ${H+4}" preserveAspectRatio="none" style="height:${H+10}px">${s}</svg>`;
}
function axis(){
  let s='';const y1=D.year0+Math.ceil((minMonth-D.month0)/12), y2=D.year0+Math.floor((maxMonth-D.month0)/12);
  const step=(y2-y1)>60?10:(y2-y1)>25?5:2;   /* label spacing follows the span */
  for(let y=y1;y<=y2;y++){const m=D.month0+(y-D.year0)*12;if(m<minMonth||m>maxMonth)continue;const x=X(m);
    if(y%step!==0)continue;
    s+=`<line x1="${x}" x2="${x}" y1="0" y2="18" stroke="var(--rule)"/><text x="${x+3}" y="12">${y}</text>`;}
  $('axis').innerHTML=`<svg class="axis" viewBox="0 0 ${W} 20" preserveAspectRatio="none" style="height:20px;margin-left:160px;width:calc(100% - 160px)">${s}</svg>`;
}
function render(){
  const sort=$('sort').value, filt=$('filter').value, from=+$('from').value||0;
  let list=people.filter(p=>p.joined>=from);
  if(filt==='died')list=list.filter(p=>p.summary.died); if(filt==='multi')list=list.filter(p=>p.summary.selections>1);
  if(filt==='prep')list=list.filter(p=>p.summary.prep_ever); if(filt==='ltfu_end')list=list.filter(p=>p.summary.care_end==='HIV+LTFU');
  const key={joined:p=>p.joined,born:p=>p.summary.born??0,infected:p=>p.summary.infected??1e9,cohort:p=>(p.events.find(e=>e.kind==='selected')||{}).cohort||''}[sort];
  list.sort((a,b)=>key(a)<key(b)?-1:key(a)>key(b)?1:a.id-b.id);
  $('count').textContent=`${list.length} of ${people.length} people`;
  $('rows').innerHTML=list.map(p=>{const s=p.summary,sel=p.events.find(e=>e.kind==='selected');
    const race=p.profile.split(':').slice(-2).join(' ').toLowerCase().replace('non_hispanic','non-Hispanic').replace(/^hispanic/,'Hispanic').replace('black','Black').replace('white','White').replace('other','Other race');
    return `<div class="row"><div class="who"><b>${p.id}</b> · ${p.sex==='Male'?'man':'woman'}, ${race}<br>${s.born!==null?'born '+dateOf(s.born):''}${s.infected!==null?' · HIV '+dateOf(s.infected):''}<br>FOCUS ${dateOf(p.joined)}${sel?' · '+sel.cohort.replace(/:/g,' ').toLowerCase():''}${s.selections>1?' ×'+s.selections:''}${s.died?' · died '+dateOf(s.end):''}</div>${rowSvg(p)}</div>`;}).join('');
  $('cap').textContent=`${D.source} · ${people.length} people put on the trace by FOCUS · run ends month ${maxMonth} (${dateOf(maxMonth)})`;
}
function tiles(){
  const s=people.map(p=>p.summary), n=people.length, pct=k=>Math.round(100*k/n)+'%';
  const dxDelay=s.filter(x=>x.infected!==null&&x.diagnosed!==null).map(x=>x.diagnosed-x.infected);
  const med=a=>{if(!a.length)return '–';a=[...a].sort((p,q)=>p-q);return a[a.length>>1];};
  const t=(l,v,n2)=>`<div class="tile"><div class="lab">${l}</div><div class="num">${v}</div><div class="lab">${n2||''}</div></div>`;
  $('tiles').innerHTML=t('People traced',n,'first '+n+' FOCUS selected')+
    t('Out of care at selection',s.filter(x=>x.care_at_selection&&/LTFU|undetected|unlinked/.test(x.care_at_selection)).length,pct(s.filter(x=>x.care_at_selection&&/LTFU|undetected|unlinked/.test(x.care_at_selection)).length))+
    t('Selected more than once',s.filter(x=>x.selections>1).length,pct(s.filter(x=>x.selections>1).length))+
    t('In care at the end',s.filter(x=>!x.died&&/in_care|RTC/.test(x.care_end)).length,pct(s.filter(x=>!x.died&&/in_care|RTC/.test(x.care_end)).length))+
    t('Lost again by the end',s.filter(x=>!x.died&&x.care_end==='HIV+LTFU').length,pct(s.filter(x=>!x.died&&x.care_end==='HIV+LTFU').length))+
    t('Died',s.filter(x=>x.died).length,pct(s.filter(x=>x.died).length))+
    t('Ever on PrEP before',s.filter(x=>x.prep_ever).length,pct(s.filter(x=>x.prep_ever).length))+
    t('Months infected before diagnosis',med(dxDelay),'median, '+dxDelay.length+' people');
}
function table(){
  const H=['Person','Sex','Profile','Born','Infected','Gen.','Diagnosed','First FOCUS','Cohort','Care before FOCUS','Selections','Screenings','Care at end','End','Died'];
  $('tbl').innerHTML='<tr>'+H.map(h=>`<th>${h}</th>`).join('')+'</tr>'+people.map(p=>{const s=p.summary,sel=p.events.find(e=>e.kind==='selected');
    const c=[p.id,p.sex,p.profile,s.born!==null?dateOf(s.born):'',s.infected!==null?dateOf(s.infected):'',s.generation??'',s.diagnosed!==null?dateOf(s.diagnosed):'',dateOf(p.joined),sel?sel.cohort:'',s.care_at_selection?careLab(s.care_at_selection):'',s.selections,s.screenings,careLab(s.care_end),dateOf(s.end),s.died?'yes':''];
    return '<tr>'+c.map(v=>`<td>${v}</td>`).join('')+'</tr>';}).join('');
}
for(const id of ['sort','filter','from'])$(id).addEventListener('input',render);
axis();tiles();table();render();
</script></body></html>
'''

if __name__ == '__main__':
    main()
