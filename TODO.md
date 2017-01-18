# TODO HIV-CDM (01-2017)

*Urgency:* \*|\*\*|\*\*\*

*Difficulty:* +|++|+++

## Usability
- [ ] Implement "graceful" handling of malformed input files (instead of just crashing) ***+
- [ ] Redesign the input files: they are too long and unwieldy (split them according to function?) *++
- [ ] Re-implement a user tool for input files generation (used to be an Excel file) **+

## Performance
- [ ] Improve handling of partnerships (it is too slow) **++
- [ ] Look for more inefficiencies in CEPAC code (CEPAC 50a is slower than 44a) *++

## Fixes
- [x] ~~Fix "Events" output~~ **++ (GA 01-17)
- [ ] Redesign partnership selection (currently done by males only) to allow for MSM **+++

## Design
- [ ] Do a proper redesign of the data structures used in CDM *+++
- [ ] Automate the aggregation of the results of a single runset (instead of running ad-hoc scripts afterwards) **+++
