# Analysis

CMSC 124 Problem Set 1. Renz Frederick P. Bañas and Ralph Ryan T. Escabarte.

## 1. Types other languages give for free

> Pick 3 of the 10 categories. For each, pick a language that gives it to you
> for free and say what that language pays for it. "Python has dictionaries"
> isn't an answer. What does Python's dictionary cost in memory or in speed
> compared to what you built, and where would you notice?

### Strings in Python

Python strings store their length like our dt_str, so getting the length is a field read instead of a walk to a zero byte. The cost is that Python strings are immutable, so every append builds a new string and copies everything so far into it. You would notice this when building a large string piece by piece in a loop, since each step copies the whole string again. Our dt_str_append instead grows the buffer by doubling the capacity when it's full. This means that a copy only happens when the buffer is full, which becomes rare as the string grows.

### Arrays in Java

Java checks array bounds on every access, like our index_to_offset helper does for dt_array_get and dt_array_set. The cost is one comparison per access, which can slow things down in loops that access the array a lot, although Java's compiler can often remove the check when it knows the index is safe. Java arrays also always start at index 0, so for an array over -1..1 you'd have to write the index - lower_bound formula yourself every time. Our array already stores the lower bound in its descriptor, so get and set do it automatically.

### Records in Python

In our dt_record, the fields are set in rec new, so rec set person salary 1 sends an error since salary is not defined. In Python, when you write person.salary = 1, it doesn't error, it just adds a new field called salary if salary was not defined earlier. A typo like agee would also be added as a new field instead of erroring. The cost is that each Python object carries an extra dictionary, so it uses more memory than our record. You would notice this when a program creates millions of objects.

## 2. The hand-written tag check

> You wrote the tag check in dt_value_as_int by hand. Some languages don't let
> you. They make the tagged union a language construct, so the compiler writes
> the check for you, refuses to compile a read that skips it, and refuses to
> compile a set of cases that misses one. Rust's enum and match work this way,
> and so do ML's datatypes and Swift's enumerations with associated values.
> What does the C version let you do that a compiler enforcing the check
> wouldn't, and is any of it worth wanting?

C will still compile even if we forget the tag check, read the wrong member, or add a new kind later and forget to handle it. C allows this because it wants to give the programmer freedom to handle tags manually. Rust would refuse to compile all three. In our code, the tag check in dt_value_as_int only exists because we wrote it by hand, and nothing forces us to. We don't think this freedom is worth it. The mistakes C allows are silent and hard to find. For example, without our check, as str 42 would compile fine and hand the printer the number 42 as if it were a memory address, which could crash or print garbage much later. Rust would catch the same mistake before the program even runs.

## 3. Dropping insertion order from the map

> Your dt_map keeps insertion order separately from the hash buckets, which is
> memory spent on something no lookup uses. Argue the other side: describe a
> design that drops it, say what breaks, and say whether you'd ship it.

The purpose of the insertion order for the dt_map is for it to behave similar to an array/linked list. This is useful in this specific context since the file needs to print the contents of the map in the order they were added. However, values in a map are called by key though, not insertion order. Removing the list would save memory and make the map a little simpler, since normal hashmap lookups do not use insertion order anyway. What would break is anything that expects consistent insertion-order iteration, like printing values in the same order they were added or accessing the map as if it had a stable sequence.  But you can just store keys in a separate array if you need that behavior, but that would bring back some of the memory cost.  That is what I would probably ship, a simpler version of the map without the order and whenever I need it, I’ll use a separate array.

## 4. Access after release vs. an unreleased allocation

> Compare access after release with an allocation that remains unreleased at
> the driver's final check. What damage can each cause in a long-running
> server? How does that answer change for a command-line tool that exits in a
> second?

Accessing an allocation after it has been released is more dangerous than leaving an allocation unreleased. If memory has already been freed, using it can cause crashes, corrupted data, or unpredictable behavior because that memory could already be reused for something else. This is called a use-after-free bug. An allocation that remains unreleased is a memory leak. In a long-running server, leaks can keep building up over time until the server uses too much memory, slows down, or eventually crashes. For a command-line tool that exits after a second, a small leak is usually less serious because the operating system reclaims the program’s memory when it exits. However, a use-after-free is still serious even in a short program because it can break the program before it has a chance to exit.
