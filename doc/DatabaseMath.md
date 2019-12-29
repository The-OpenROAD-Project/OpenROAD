Database Math 101

DEF defines the units it uses with the units command.

```
UNITS DISTANCE MICRONS 1000 ;
```

Typically the units are 1000 or 2000 database units (DBU) per micron.
DBUs are integers, so the distance resolution is typically 1/1000u
or 1nm.

OpenDB uses an `int` to represent a DBU, which on most hardware is 4
bytes.  This means a database coordinate can be +/-2147483647, which
is about 2 billion, or 2000 microns or 2 meters.

Since chip coordinates cannot be negative, it would make sense to use
an `unsigned int` to represent a distance. This conveys the fact that
it can never be negative and doubles the maximum possible distance
that can be represented. The problem is doing subtraction with
unsigned numbers is dangerous because the differences can be
negative. An unsigned negative number looks like a very very big
number. So this is a very bad idea and leads to bugs.

Note that calculating an area with `int` values is problematic.  An
`int * int` does not fit in an `int`. My suggestion is to use
`int64_t` in this situation. Although `long` "works", it's size is
implementation dependent.

Unfortunately I have seen multiple instances of programs using a
`double` for distance calculations. A double is 8 bytes, with 52 bits
used for the mantissa. So the largest possible integer value that can
be represented without loss is 5e+15, 12 bits less than using a
`int64_t`.  Doing an area calculation on a large chip that is more
than `sqrt(5e+15) = 7e+7 DBU` will overflow the mantissa and truncate
the result.

Not only is a `double` less capable than an `int64_t`, using it the
tells any reader of the code that the value can be real number, such
as 104.23. So it is extremely misleading.

Circling back to LEF, we see that unlike DEF the distances are real
numbers like 1.3 even though LEF also has a distance unit statement.
I suspect this is a historical artifact of a mistake made in the early
definition of the LEF file format. The reason it is a mistake is because
decimal fractions cannot be represented exactly in binary floating point.
For example, 1.1 = 1.00011001100110011..., a continued fracion.

OpenDB uses `int` to represent LEF distances, just like DEF. This
solves the problem by multiplying distances by a decimal constant
(distance units) to convert the distance to an integer. In the future
I would like to see OpenDB use a `dbu` typedef instead of `int`
everywhere.

Unfortunately, I see RePlAce, OpenDP, TritonMacroPlace and OpenNPDN
all using `double` or `float` to represent distances and converting
back and forth between DBUs and microns everywhere. This means they
also need to `round` or `floor` the results of every calculation
because the floating point representation of the LEF distances is a
fraction that cannot be exactly represented in binary. Even worse
is the practice of reinventing round in the following idiom.

```(int) x_coord + 0.5```

Even worse than using a `double` is using `float` because the mantissa
is only 23 bits, so the maximum exactly representable integer is
8e+6. This makes it even less capable than an `int`.

When a value has to be snapped to a grid such as the pitch of a layer
the calculation can be done with a simple divide using `int`s, which
`floor`s the result. For example, to snap a coordinate to the pitch
of a layer the following can be used.

```
int x, y;
inst->getOrigin(x, y);
int pitch = layer->getPitch();
int x_snap = (x / pitch) * pitch;
```

The use of rounding in existing code that uses floating point
representations is to compensate for the inability to represent
floating point fractions exactly. Results like 5.99999999992 need to
be "fixed". This problem does not exist if fixed point arithmetic is
used.

The **only** place that the database distance units should appear in
any program should be in the user interface, because humans like
microns more than DBUs. Internally code should use `int` for all
database units and `int64_t` for all area calculations.

James Cherry, 2019
