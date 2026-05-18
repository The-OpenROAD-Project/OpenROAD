#pragma once

namespace sta {
class Network;
}

namespace utl {
class Logger;
}

namespace syn {

class Graph;

void mapCombinationals(Graph& g, sta::Network* network, utl::Logger* logger);

}  // namespace syn
