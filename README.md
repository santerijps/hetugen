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

## Multithreading (Windows version)

Every SSN line is exactly 12 bytes, so the byte offset of every
(century, separator, year) block can be computed before anything is generated.
The Windows version splits the work into these 1300 blocks (~4 MB each) and
worker threads claim them from a shared atomic counter, writing straight into
their own region of the memory mapped file. No locks or copying are needed, and
the output is byte-for-byte identical to the single threaded version.

```
hetugen.win.exe            # use all logical processors
hetugen.win.exe 1          # single threaded, for comparison
hetugen.win.exe 4          # 4 worker threads
```

The program prints how long generation and unmapping took. Note that the
timings measure how long it takes to fill the mapping. Windows writes the
dirty pages to disk in the background afterwards, so for the whole file to
reach the disk, disk write speed is still the limit.

## Optimization ideas

- IO improvement: the program writes 200 million lines in 1 second, but the other ~ 200 million takes 6-7?
- Getting rid of modulo operation, e.g. using a counter
- Not checking the buffer size on every iteration of `n`
- Avoiding the nesting of loops (currently 6 nested loops)
