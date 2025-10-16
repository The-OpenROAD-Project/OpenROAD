// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

%{
#include "dst/Distributed.h"
#include "ord/OpenRoad.hh"
%}

%include "../../Exception.i"

%inline %{

void run_worker_cmd(
    const char* host, unsigned short port, bool interactive)
{
  auto* distributed = ord::OpenRoad::openRoad()->getDistributed();
  distributed->runWorker(host, port, interactive);
}

void run_load_balancer(
    const char* host, unsigned short port, const char* workers_domain)
{
  auto* distributed = ord::OpenRoad::openRoad()->getDistributed();
  distributed->runLoadBalancer(host, port, workers_domain);
}

void add_worker_address(
    const char* address, unsigned short ip)
{
  auto* distributed = ord::OpenRoad::openRoad()->getDistributed();
  distributed->addWorkerAddress(address, ip);
}

%} // inline
