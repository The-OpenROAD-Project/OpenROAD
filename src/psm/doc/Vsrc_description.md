# Voltage source location file description
This file specifies the description of the C4 bump configurations file.
The file is a csv as described below:

<x_coordinate>, <y_coordinate>, <octagonal_c4_bump_edge_length>, <voltage_value>

The x and y coordinate specify the center location of the voltage C4 bumps in
micro meter.

The octagonal c4_edge_length specifies the edge length of the C4 to determine
the pitch of the RDL layer in micrometer

Voltage_value specifies the value of voltage source at the C4 bump. In case
there is a need to specify voltage drop in micrometer

## Example file
250,250,20,1.1
130,170,20,1.1
370,410,10,1.1
410,450,10,1.1
