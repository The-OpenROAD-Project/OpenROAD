# Authors: Osama
#
# Copyright (c) 2021, The Regents of the University of California
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of the University nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

sta::define_cmd_args "run_worker" {
    [-host host]
    [-port port]
    [-i]
}
proc run_worker { args } {
  sta::parse_key_args "run_worker" args \
    keys {-host -port -threads} \
    flags {-i}
  sta::check_argc_eq0 "run_worker" $args
  if { [info exists keys(-host)] } {
    set host $keys(-host)
  } else {
    utl::error DST 2 "-host is required in run_worker cmd."
  }
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  } else {
    utl::error DST 3 "-port is required in run_worker cmd."
  }
  set interactive [info exists flags(-i)]
  dst::run_worker_cmd $host $port $interactive
}

sta::define_cmd_args "run_load_balancer" {
    [-host host]
    [-port port]
    [-workers_domain workers_domain]
}
proc run_load_balancer { args } {
  sta::parse_key_args "run_load_balancer" args \
    keys {-host -port -workers_domain} \
    flags {}
  sta::check_argc_eq0 "run_load_balancer" $args
  if { [info exists keys(-host)] } {
    set host $keys(-host)
  } else {
    utl::error DST 10 "-host is required in run_load_balancer cmd."
  }
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  } else {
    utl::error DST 11 "-port is required in run_load_balancer cmd."
  }
  if { [info exists keys(-workers_domain)] } {
    set workers_domain $keys(-workers_domain)
  } else {
    set workers_domain ""
  }
  dst::run_load_balancer $host $port $workers_domain
}
sta::define_cmd_args "add_worker_address" {
    [-host host]
    [-port port]
}
proc add_worker_address { args } {
  sta::parse_key_args "add_worker_address" args \
    keys {-host -port -threads} \
    flags {}
  if { [info exists keys(-host)] } {
    set host $keys(-host)
  } else {
    utl::error DST 16 "-host is required in add_worker_address cmd."
  }
  if { [info exists keys(-port)] } {
    set port $keys(-port)
  } else {
    utl::error DST 17 "-port is required in add_worker_address cmd."
  }
  dst::add_worker_address $host $port
}
