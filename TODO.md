Short-term
- Update XML<->excel macro and inputs for 3.4.0
- Check to see if we're not selecting randomly when applying incident prevalence

Problems
- Disabling events keeps incidence from being recorded, this sort of dependency needs to be removed

Testing
- Get rid of global state as much as possible
- Make methods smaller (should only do one thing)
- Clearer boundary between public and private interface
- Unit tests on all important functionality
- Integration tests - build a suite of runs with expected outputs to be compared to new outputs

Future features
- Single XLSX output instead of multiple CSVs
- Native GUI integrating inputs, simulation, and results viewing
- Evaluate possibility of GPGPU port

Formatting
- DONE Fix indentation - 4 space tab width, all spaces: helps readability
- DONE Opening bracket on its own line: helps readability
- DONE No space after if,while,for,function call
- DONE No space before first param or after last param
- DONE One space after comma separating parameters
- DONE One space between functions
- DONE Indented // comments inside functions
- Prefer declaring variable as late as possible (except if it affects speed)
- Avoid extraneous parens except to resolve ambiguity or when using result of assignment as value
- Doxygen style comments everywhere

Cleaning/updating
- Find and remove commented out code
- Find and remove unreachable code
- Find and remove code that is never called
- Prefer modern C++11 idioms (some can't be implemented until VS2013)

Optimization
- With ART rollout is significantly slower, can it be improved?
- Profile

Refactoring
- Keep it DRY
- Unify random number generation
- Make code more testable
- Avoid friend classes
- Keep functions shorter - they should only do one thing
- Keep classes smaller - extract functionality into neat testable units
- Better variable names, avoid abbreviations
- Underscore suffix on all member variables
- Decide when to use assertions vs exceptions

Other:
clean up Enum and EnumCls and BaseEnumCls
organize headers
fix FullVector
QueryField
fix nullptr_
change constant sized vectors to arrays
move sparsetable inside tabularoutput
make distributions structs
order classes public protected private

-Help ensure that all of Nadia's new scenarios finish running on Odyssey successfully before the weekend.
-Combine the functionality of some of the other post-calibration scripts into a single coherent application for ease of use and to prevent code duplication (e.g. the code for reading the month of 1990 from post calib.out).
-Make version 3.34 official (finish adding time-dependent parameters for behavioral parameters, compile Windows executable+GUI, update Excel inputs spreadsheet, update change log and future changes, ensure cross-platform consistency)
-Fix all remaining compiler warnings (compilers warn about different things that could cause problems--we're down to a few hundred warnings from a few thousand when I started)
-Look for optimizations. I've already found quite a few places where I could speed things up. I just need to make sure that it won't have an effect on reproducibility.
-Set up a continuous integration build system (see http://en.wikipedia.org/wiki/Continuous_integration)
-Set up a suite of test runs for integration testing (see http://en.wikipedia.org/wiki/Integration_testing)
-Start on version 3.4. This release will include: switch to CEPAC 4.5b, ART rollout proportion feedback loop (as we discussed with Marc a few weeks ago), new optimizations, template-based time-dependent parameters (as you suggested), and some behind-the-scenes for the MSM model (which will be complete in version 4.0).
-Abena noticed that the tracer doesn't work in some cases for tracing females which can now show up in the trace due to tracing of prevalent cases. This should be resolved before we start using the tracer again.
-Potentially re-run calibration by fitting to the middle of each year instead of the beginning and interpolating prevalence month-by-month. I will then compare the resulting parameters to our previous parameters.
