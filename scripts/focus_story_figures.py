#!/usr/bin/env python3
"""Static figures of the FOCUS-traced people, rendered directly from the trace.

Reads <sim>-SinglePerson.xls written with <numberFocusToTrace> set (issue #98)
and writes four PNGs (and PDFs) next to it:

    focus_before.png   years living with HIV, and years out of care, when FOCUS found them
    focus_after.png    the group's care state month by month after selection
    focus_rows.png     every person as one thin row, aligned on the selection month
    focus_reselect.png how many times FOCUS selected each person

Usage: python3 focus_story_figures.py <run-SinglePerson.xls> [outdir] [--month0 600 --year0 2007]
"""
import argparse, collections, os, sys
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from focus_timeline import parse, summarize, CARE

COL = {'care': '#1baf7a', 'lost': '#eb6834', 'dead': '#b4bcc8', 'undx': '#8d96a6', 'unlinked': '#eda100',
       'neg': '#e6e9ef', 'prep': '#2a78d6', 'ink': '#25292f', 'ink2': '#4b5365', 'ink3': '#7a8397', 'rule': '#dde2ea'}
plt.rcParams.update({'font.family': 'Liberation Sans', 'font.size': 9, 'axes.edgecolor': COL['rule'],
                     'axes.labelcolor': COL['ink2'], 'xtick.color': COL['ink3'], 'ytick.color': COL['ink3'],
                     'axes.titlesize': 11, 'axes.titleweight': 'bold', 'axes.titlelocation': 'left',
                     'pdf.fonttype': 42, 'figure.dpi': 110})
role = lambda s: CARE.get(s, (s, 'neg'))[1]
grp = lambda r: 'care' if r in ('care', 'rtc') else 'lost' if r == 'ltfu' else r
PRE = 120  # months shown before selection in the row figure


def monthly_states(p):
    """care-state role per month relative to first selection, from creation to end"""
    ev = sorted(p['events'], key=lambda e: (e['month'], e['order']))
    start = p['summary']['born'] if p['summary']['born'] is not None else ev[0]['month']
    out, care, prep, i = {}, 'neg', False, 0
    for m in range(start, p['summary']['end'] + 1):
        while i < len(ev) and ev[i]['month'] <= m:
            e = ev[i]; i += 1
            if e['kind'] == 'infected': care = 'undx'
            elif e['kind'] == 'care': care = role(e['care'])
            elif e['kind'] == 'hiv_status' and care == 'undx' and '(observed)' in e['status'] and 'unobserved' not in e['status']: care = 'unlinked'
            elif e['kind'] == 'selected' and e.get('care_after'): care = role(e['care_after'])
            elif e['kind'] == 'prep_on': prep = True
            elif e['kind'] == 'prep_off': prep = False
        out[m - p['joined']] = (care, prep)
    return out


def dot_strip(ax, values, xmax, title, xlabel, color):
    """one dot per person, stacked where they coincide"""
    bins = collections.Counter()
    for v in sorted(values):
        b = round(v * 8); ax.plot(v, bins[b], 'o', ms=4.2, color=color, mec='white', mew=0.5); bins[b] += 1
    med = float(np.median(values))
    ax.axvline(med, color=COL['ink'], ls=(0, (3, 3)), lw=0.9)
    ax.text(med + 0.08, max(bins.values()) + 0.6, f'median {med:.1f}', color=COL['ink2'], fontsize=8.5, va='bottom')
    ax.set_xlim(-0.2, xmax); ax.set_ylim(-0.8, max(bins.values()) + 2.5); ax.set_yticks([])
    ax.set_xlabel(xlabel); ax.set_title(title, fontsize=9.5, color=COL['ink2'], fontweight='normal')
    for s in ('left', 'right', 'top'): ax.spines[s].set_visible(False)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('trace'); ap.add_argument('outdir', nargs='?')
    ap.add_argument('--month0', type=int, default=600); ap.add_argument('--year0', type=int, default=2007)
    a = ap.parse_args()
    people, last_month = parse(a.trace)
    P = sorted((p for p in people.values() if p['joined'] is not None), key=lambda p: (p['joined'], p['id']))
    for p in P: p['summary'] = summarize(p, last_month)
    if not P: sys.exit('no traced people found; was <numberFocusToTrace> set and --focus on?')
    N = len(P); outdir = a.outdir or os.path.dirname(os.path.abspath(a.trace)); os.makedirs(outdir, exist_ok=True)
    S = [monthly_states(p) for p in P]
    date = lambda m: f"{['Jan','Feb','Mar','Apr','May','Jun','Jul','Aug','Sep','Oct','Nov','Dec'][(m - a.month0) % 12]} {a.year0 + (m - a.month0) // 12}"
    save = lambda fig, name: [fig.savefig(os.path.join(outdir, f'{name}.{ext}'), bbox_inches='tight', pad_inches=0.15, dpi=200) for ext in ('png', 'pdf')] and plt.close(fig)
    sel_range = f'{date(min(p["joined"] for p in P))} to {date(max(p["joined"] for p in P))}'

    # ---- 1. before FOCUS
    yrs = [(p['joined'] - p['summary']['infected']) / 12 for p in P if p['summary']['infected'] is not None]
    lost = []
    for p in P:
        if not (p['summary']['care_at_selection'] and 'LTFU' in p['summary']['care_at_selection']): continue
        t = None
        for e in p['events']:
            if e['month'] >= p['joined']: break
            if e['kind'] == 'care' and role(e['care']) == 'ltfu': t = e['month']
        if t is not None: lost.append((p['joined'] - t) / 12)
    fig, (a1, a2) = plt.subplots(1, 2, figsize=(10, 3.2))
    dot_strip(a1, yrs, np.ceil(max(yrs)) + 0.5, f'{len(yrs)} people: years living with HIV when FOCUS selected them', 'years since infection', COL['lost'])
    dot_strip(a2, lost, np.ceil(max(lost)) + 0.5, f'{len(lost)} people: years out of care when FOCUS selected them', 'years since last in care', COL['ink2'])
    fig.suptitle(f'Who FOCUS found: the first {N} people it selected, {sel_range}', x=0.01, y=1.04, ha='left', fontsize=11, fontweight='bold')
    fig.subplots_adjust(wspace=0.12)
    save(fig, 'focus_before')

    # ---- 2. after FOCUS, stacked area over the fully observed window
    fullT = last_month - max(p['joined'] for p in P)
    T = np.arange(fullT + 1); C = {k: np.zeros(fullT + 1) for k in ('care', 'unlinked', 'lost', 'dead')}
    for s in S:
        for t in T:
            st = s.get(t)
            if st is None: C['dead'][t] += 1
            else: C[grp(st[0]) if grp(st[0]) in C else 'unlinked'][t] += 1
    fig, ax = plt.subplots(figsize=(10, 3.6))
    ax.stackplot(T / 12, C['care'], C['unlinked'], C['lost'], C['dead'],
                 colors=[COL['care'], COL['unlinked'], COL['lost'], COL['dead']], lw=0)
    for y in (2, 5, 10): ax.axvline(y, color=COL['ink3'], ls=(0, (3, 3)), lw=0.8)
    ax.set_xlim(0, fullT / 12); ax.set_ylim(0, N); ax.set_ylabel('people'); ax.set_xlabel('years after FOCUS first selected them')
    ax.set_xticks(range(0, int(fullT / 12) + 1, 2)); ax.set_xticklabels(['FOCUS'] + [f'{y} yr' for y in range(2, int(fullT / 12) + 1, 2)])
    ax.legend(handles=[Patch(color=COL['care'], label='in care (incl. returned to care)'), Patch(color=COL['unlinked'], label='diagnosed, not linked'),
                       Patch(color=COL['lost'], label='lost to follow-up again'), Patch(color=COL['dead'], label='died')],
              loc='lower left', frameon=False, ncol=4, fontsize=8.5, bbox_to_anchor=(0, -0.3))
    for s in ('right', 'top'): ax.spines[s].set_visible(False)
    c = lambda t: ' / '.join(str(int(C[k][min(t, fullT)])) for k in ('care', 'lost', 'dead'))
    ax.set_title(f'What happened next: in care / lost again / died at 2 years {c(24)}, 5 years {c(60)}, {fullT // 12} years {c(fullT)}')
    save(fig, 'focus_after')

    # ---- 3. every person, one row, aligned on selection
    maxT = max(p['summary']['end'] - p['joined'] for p in P)
    order = sorted(range(N), key=lambda k: ((2 if P[k]['summary']['died'] else 1 if 'LTFU' in P[k]['summary']['care_end'] else 0),
                                             P[k]['summary']['end'] - P[k]['joined'], k))
    fig, ax = plt.subplots(figsize=(10, 0.075 * N + 1.4))
    rc = {'neg': COL['neg'], 'undx': COL['undx'], 'unlinked': COL['unlinked'], 'care': COL['care'], 'rtc': COL['care'], 'ltfu': COL['lost']}
    for row, k in enumerate(order):
        p, s = P[k], S[k]
        for t in range(-PRE, maxT + 1):
            st = s.get(t)
            if st is None:
                if p['summary']['died'] and t > p['summary']['end'] - p['joined']:
                    ax.barh(row, (maxT - t + 1) / 12, left=t / 12, height=0.85, color=COL['dead'], lw=0); break
                continue
            ax.barh(row, 1 / 12, left=t / 12, height=0.85, color=rc[st[0]], lw=0)
            if st[1]: ax.barh(row - 0.3, 1 / 12, left=t / 12, height=0.25, color=COL['prep'], lw=0)
        for e in p['events']:
            if e['kind'] == 'selected' and e['month'] > p['joined']:
                ax.plot((e['month'] - p['joined']) / 12, row + 0.1, marker='v', ms=2.6, color=COL['ink'], mec='none')
    ax.axvline(0, color=COL['ink'], lw=1.1)
    g = [sum(1 for p in P if not p['summary']['died'] and 'LTFU' not in p['summary']['care_end']),
         sum(1 for p in P if not p['summary']['died'] and 'LTFU' in p['summary']['care_end']), sum(p['summary']['died'] for p in P)]
    for y, lab in ((g[0] / 2, f'{g[0]} alive,\nin care\nat the end'), (g[0] + g[1] / 2, f'{g[1]} alive,\nlost'), (g[0] + g[1] + g[2] / 2, f'{g[2]} died')):
        ax.text(-PRE / 12 - 0.4, y, lab, ha='right', va='center', fontsize=8.5, color=COL['ink2'])
    ax.set_xlim(-PRE / 12, maxT / 12); ax.set_ylim(N - 0.5, -0.5); ax.set_yticks([])
    ax.set_xticks(range(-10, int(maxT / 12) + 1, 5)); ax.set_xticklabels([('FOCUS' if v == 0 else f'{v:+d} yr') for v in range(-10, int(maxT / 12) + 1, 5)])
    for s_ in ('left', 'right', 'top'): ax.spines[s_].set_visible(False)
    ax.legend(handles=[Patch(color=COL['neg'], label='HIV-negative'), Patch(color=COL['prep'], label='on PrEP'), Patch(color=COL['undx'], label='undiagnosed'),
                       Patch(color=COL['unlinked'], label='diagnosed, not linked'), Patch(color=COL['care'], label='in care'), Patch(color=COL['lost'], label='lost to follow-up'),
                       Patch(color=COL['dead'], label='died'), plt.Line2D([], [], marker='v', ls='', color=COL['ink'], label='selected by FOCUS again')],
              loc='lower left', bbox_to_anchor=(0, 1.0), ncol=4, frameon=False, fontsize=8.5)
    ax.set_title(f'Every person, one row: ten years before FOCUS and everything after, sorted by outcome', pad=34)
    save(fig, 'focus_rows')

    # ---- 4. re-selections
    counts = collections.Counter(p['summary']['selections'] for p in P); mx = max(counts)
    fig, ax = plt.subplots(figsize=(7, 2.8))
    ax.bar(range(1, mx + 1), [counts[i] for i in range(1, mx + 1)], color=COL['lost'], width=0.7)
    for i in range(1, mx + 1): ax.text(i, counts[i] + 0.6, str(counts[i]), ha='center', fontsize=8.5, color=COL['ink2'])
    ax.set_xticks(range(1, mx + 1)); ax.set_xlabel('times selected by FOCUS')
    ax.set_ylabel('people'); ax.set_ylim(0, max(counts.values()) * 1.25)
    for s_ in ('right', 'top'): ax.spines[s_].set_visible(False)
    atl = lambda k: sum(1 for p in P if p['summary']['selections'] >= k)
    ax.set_title(f'How many times FOCUS selected each person: {atl(2)} of {N} were found again after dropping out, {atl(3)} three times or more')
    save(fig, 'focus_reselect')
    print(f'{N} people; wrote focus_before/after/rows/reselect .png and .pdf in {outdir}')


if __name__ == '__main__':
    main()
