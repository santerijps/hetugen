# hetugen

`hetugen` is an attempt at a fast Finnish social security number generator.

Currently the implementation generates a text file that contains every single valid Finnish SSN.
This includes the new separators for the 20th and 21st centuries, but excludes the temporary individual numbers.

All together there are **426,386,564** SSNs generated.
On my PC it takes **~2 seconds** to generate them all (Windows version that uses memory mapping).
The output file size is **5,116,638,768** bytes (~5GB).

The most significant optimizations done so far include:

- The use of lookup tables whenever possible instead of heavier computations
- Not using fprintf to write SSNs to file
- Memory mapping

The biggest bottle neck is IO, i.e. writing to file. Over 90% of CPU time is used on IO, so the next steps would be to focus on optimizing that.

## Optimization ideas

- IO improvement: the program writes 200 million lines in 1 second, but the other ~ 200 million takes 6-7?
- Getting rid of modulo operation, e.g. using a counter
- Not checking the buffer size on every iteration of `n`
- Avoiding the nesting of loops (currently 6 nested loops)
