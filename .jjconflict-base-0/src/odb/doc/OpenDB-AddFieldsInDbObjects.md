# Manually adding new fields in DB Object

For example `add_pitchDiag` in object `DbTechLayer`.

*Modifications in these steps need to be done inside user regions, delimited like the below, to avoid being rewritten by the code generator
```cpp
// User Code Begin <something>

...

// User Code End <something>
```
<table>
  <tr>
    <th></th><th>Action</th><th>File</th><th>Source Code</th>
  </tr>
  <tr>
    <td>1</td>
    <td>Add Fields at the .h file</td>
    <td><code>dbTechLayer.h</code></td>
    <td>In the class <code>_dbTechLayer</code>:<br><pre lang="cpp">int _pitchDiag;</pre></td>
  </tr>
  <tr>
    <td>2</td>
    <td>Increase the current rev number by one </td>
    <td><code>dbDatabase.h</code></td>
    <td>

``` cpp
const uint db_schema_minor = 52;
```
  </td></tr>
    <tr>
    <td>3</td>
    <td>Define a keyword for the new db rev number</td>
    <td><code>dbDatabase.h</code></td>
    <td>

``` cpp
const uint db_schema_add_pitchDiag = 52;
```
  </td></tr>
  </td></tr>
    <tr>
    <td>4*</td>
    <td>Stream in new fields Conditionally upon Schema number</td>
    <td><code>dbTechLayer.cpp</code></td>
    <td>In the method <code>dbIStream& operator>></code>:

``` cpp
if (obj.getDatabase()->isSchema(db_schema_add_pitchDiag)) {
    stream >> obj._pitchDiag;
}
```
  </td></tr>
  <tr>
    <td>5*</td>
    <td>Stream out new fields Conditionally upon Schema number</td>
    <td><code>dbTechLayer.cpp</code></td>
    <td>
      In the method <code>dbOStream& operator<<</code>:

``` cpp
if (obj.getDatabase()->isSchema(db_schema_add_pitchDiag)) {
    stream << obj._pitchDiag;
}
```
  </td></tr>
  <tr>
    <td>6*</td>
    <td>Diff new fields</td>
    <td><code>dbTechLayer.cpp</code></td>
    <td>
      In the method <code>void _dbTechLayer::differences</code>:

``` cpp
DIFF_FIELD(_pitchDiag);
```
  </td></tr>
  <tr>
    <td>7*</td>
    <td>Diff Out new fields</td>
    <td><code>dbTechLayer.cpp</code></td>
    <td>
      In the method <code>void _dbTechLayer::out</code>:

``` cpp
DIFF_OUT_FIELD(_pitchDiag);
```
  </td></tr>
  <tr>
    <td>8*</td>
    <td>Created access APIs to the fields </td>
    <td><code>dbTechLayer.cpp</code></td>
    <td>

``` cpp
int dbTechLayer::getPitchDiag() {...}
void dbTechLayer::setPitchDiag( int pitch ) {...}
```
  </td></tr>
  <tr>
    <td>9*</td>
    <td> Add new APIs</td>
    <td><code>include/db.h</code></td>
    <td>In the class <code>dbTechLayer</code>

``` cpp
int getPitchDiag();
void setPitchDiag( int pitch );
```
  </td></tr>
</table>