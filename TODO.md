Short-term
- Add life months tally
- Update costs output
- Include windows libraries in repository
- Update Makefile
- Test build on win32, win64, XP, win7, linux, OSX, vs2010, vs2013, etc.
- Update XML->excel macro for 3.34a

Problems
- Disabling events keeps incidence from being recorded, this sort of dependency needs to be removed

Testing
- Get rid of global state as much as possible
- Make methods smaller (should only do one thing)
- Clearer boundary between public and private interface
- Unit tests on all important functionality
- Integration tests - build a suite of runs with expected outputs to be compared to new outputs

Future features
- DONE Separate GUI from simulation
- Single XLSX output instead of multiple CSVs
- Native GUI integrating inputs, simulation, and results viewing
- Compile transm, cepac as shared libraries/DLLs
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
