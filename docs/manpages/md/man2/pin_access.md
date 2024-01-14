---
title: pin_access(2)
author: Jack Luar (TODO@TODO.com)
date: 24/01/14
---

# NAME

pin_access - pin access

# SYNOPSIS

pin_access
    [-db_process_node name]
    [-bottom_routing_layer layer]
    [-top_routing_layer layer]
    [-min_access_points count]
    [-verbose level]
    [-distributed]
    [-remote_host rhost]
    [-remote_port rport]
    [-shared_volume vol]
    [-cloud_size sz]


# DESCRIPTION

This function checks pin access.

# OPTIONS

`-db_process_node`:  Specify process node.

`-bottom_routing_layer`:  Bottommost routing layer.

`-top_routing_layer`:  Topmost routing layer.

`-min_access_points`:  Minimum number of access points per pin.

`-verbose`:  Sets verbose mode if the value is greater than 1, else non-verbose mode (must be integer, or error will be triggered.)

`-distributed`:  Refer to distributed arguments [here](#distributed-arguments).

# ARGUMENTS

`-distributed`:  Enable distributed mode with Kubernetes and Google Cloud.

`-remote_host`:  The host IP.

`-remote_port`:  The value of the port to access from.

`-shared_volume`:  The mount path of the nfs shared folder.

`-cloud_size`:  The number of workers.

# EXAMPLES

# SEE ALSO

# COPYRIGHT

Copyright (c) 2024, The Regents of the University of California. All rights reserved.
