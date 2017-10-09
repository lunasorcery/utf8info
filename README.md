# utf8info

[![Build Status](https://travis-ci.org/willkirkby/utf8info.svg?branch=master)](https://travis-ci.org/willkirkby/utf8info)

**utf8info** is a small utility that reads UTF-8 and prints out the raw codepoint information. It's useful for seeing exactly what a string contains, and for catching things like U+202E "RIGHT-TO-LEFT OVERRIDE" and all of the different types of space (because there are [more than you might assume](https://www.cs.tut.fi/~jkorpela/chars/spaces.html)).

Options:

```
-v, --verbose   Enable verbose output. This prints out the raw bytes next to the codepoint info)
```

![Thanks to @tha_rami for teaching me a small amount of Arabic](http://kirk.by/s/qxz5xvC)
