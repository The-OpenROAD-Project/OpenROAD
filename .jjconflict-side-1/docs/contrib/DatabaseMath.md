# Database Math 101

## Introduction

DEF defines the units it uses with the `UNITS` command.

```
    UNITS DISTANCE MICRONS 1000 ;
```

Typically the units are 1000 or 2000 database units (DBU) per micron.
DBUs are integers, so the distance resolution is typically 0.001 um or
1nm.

OpenDB uses an `int` to represent a DBU, which on most hardware is 4
bytes. This means a database coordinate can be $\pm 2147483647$, which is
about $2 \cdot 10^9$ units, corresponding to $2 \cdot 10^6$ or $2$ meters.

## Datatype Choice

This section is important as we cover important math considerations for
your datatype choice when dealing with large numbers.

### Why not pure int?

Since chip coordinates cannot be negative, it would make sense to use an
`unsigned int` to represent a distance. This conveys the fact that it
can never be negative and doubles the maximum possible distance that can
be represented. The problem, however, is that doing subtraction with unsigned numbers
is dangerous because the differences can be negative. An unsigned
negative number looks like a very very big number. So this is a very bad
idea and leads to bugs.

Note that calculating an area with `int` values is problematic. An
`int * int` does not fit in an `int`. **Our suggestion is to use `int64_t`
in this situation.** Although `long` "works", its size is implementation-dependent.

### Why not double?

It has been noticed that some programs use `double` to calculate distances. 
This can be problematic, as `double` have a mantissa of 52 bits, which means that 
the largest possible integer value that can be represented without loss is $5\cdot 10^{15}$.
This is 12 bits less than the largest possible integer value that can be represented 
by an `int64_t`. As a result, if you are doing an area calculation on a large chip 
that is more than $\sqrt{5\cdot 10^{15}} = 7\cdot 10^7\ DBU$ on a side, the mantissa of the 
double will overflow and the result will be truncated.

Not only is a `double` less capable than an `int64_t`, but using it
tells any reader of the code that the value can be a real number, such as
$104.23$. So it is extremely misleading.

### Use int only for LEF/DEF Distances

Circling back to LEF, we see that unlike DEF the distances are real
numbers like 1.3 even though LEF also has a distance unit statement. We
suspect this is a historical artifact of a mistake made in the early
definition of the LEF file format. The reason it is a mistake is because
decimal fractions cannot be represented exactly in binary floating-point.
For example, $1.1 = 1.00011001100110011...$, a continued fraction.

OpenDB uses `int` to represent LEF distances, just as with DEF. This solves
the problem by multiplying distances by a decimal constant (distance
units) to convert the distance to an integer. In the future I would like
to see OpenDB use a `dbu` typedef instead of `int` everywhere.

### Why not float?

We have also noticed RePlAce, OpenDP, TritonMacroPlace and OpenNPDN all using
`double` or `float` to represent distances. This can be problematic, as 
floating-point numbers cannot always represent exact fractions. As a result,
these tools need to `round` or `floor` the results of their calculations, which 
can introduce errors. Additionally, some of these tools reinvent the wheel 
by implementing their own rounding functions, as we shall see in the example below.
This can lead to inconsistencies and is highly discouraged.

```cpp
(int) x_coord + 0.5
```

Worse than using a `double` is using a `float`, because the mantissa
is only 23 bits, so the maximum exactly representable integer is $8\cdot 10^6$.
This makes it even less capable than an `int`.

When a value has to be snapped to a grid such as the pitch of a layer,
the calculation can be done with a simple divide using `int`, which
`floor` the result. For example, to snap a coordinate to the pitch of a
layer the following can be used:

``` cpp
int x, y;
inst->getOrigin(x, y);
int pitch = layer->getPitch();
int x_snap = (x / pitch) * pitch;
```

The use of rounding in existing code that uses floating-point
representations is to compensate for the inability to represent floating-point
fractions exactly. Results like $5.99999999992$ need to be "fixed".
This problem does not exist if fixed-point arithmetic is used.


