#include <simgrid/s4u.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

namespace sg4 = simgrid::s4u;
namespace sgr = simgrid::kernel::routing;


std::ofstream arquivo_dot;

// ================ Visualization ================

void init_dot(const std::string& filename) {
  arquivo_dot.open(filename);
  arquivo_dot << "graph PlataformaS4U {\n";
  arquivo_dot << "  node [shape=box, style=filled, fillcolor=lightgrey, fontname=\"Helvetica\"];\n";
}

void close_dot() {
  arquivo_dot << "}\n";
  arquivo_dot.close();
}

void log_conexao(const std::string& origem, const std::string& destino, const std::string& cor = "black", const std::string& estilo = "solid", int espessura = 1) {
  if (arquivo_dot.is_open()) {
    arquivo_dot << "  \"" << origem << "\" -- \"" << destino
		<< "\" [color=" << cor << ", style=" << estilo << ", penwidth=" << espessura << "];\n";
  }
}


// ================ Structs ================
struct Switch {
  sg4::NetZone* zone;
  std::string router_name;
  std::string bw;
  std::string lat;
};


struct Node {
  std::string name;
  std::string calib_file;
  std::string speed;
  int cores; // Only counting physical cores
};

// ================ Calibration Helper ================
std::map<std::string, std::string> load_calibration(const std::string& filename) {
  std::map<std::string, std::string> props;
  if (filename.empty()) return props; // Retorna vazio se não houver arquivo

  std::ifstream file(filename);
  std::string line;

  while (std::getline(file, line)) {
    auto delim = line.find('=');
    if (delim != std::string::npos) {
      std::string key = line.substr(0, delim);
      std::string value = line.substr(delim + 1);
      // std::cout << key << " -> " << value;
      props[key] = value;
    }
  }
  return props;
}

// ================ Switch Creation and Linking ================
Switch build_switch(sg4::NetZone* root_zone, const std::string& name, const std::string& bw, const std::string& lat) {

  auto* sw_zone = root_zone->add_netzone_star(name + "_Zone");
  std::string router_name = "Router_" + name;
  auto* sw_router = sw_zone->add_router(router_name);
  sw_zone->set_gateway(sw_router);

  // router in the .dot
  arquivo_dot << "  \"" << router_name << "\" [shape=circle, fillcolor=white];\n";

  return {sw_zone, router_name, bw, lat};
}

// Connect two switches
void connect_switches(sg4::NetZone* root_zone, const Switch& sw1, const Switch& sw2, const std::string& bw = "40GBps", const std::string& lat = "5us") {

  std::string link_name = "Link_" + sw1.router_name + "_to_" + sw2.router_name;
  auto* link = root_zone->add_link(link_name, bw)->set_latency(lat);

  root_zone->add_route(sw1.zone, sw2.zone, std::vector<const sg4::Link*>{link});

  log_conexao(sw1.router_name, sw2.router_name, "black", "bold", 4);
}

// Connect ALL switches
void connect_switch_mesh(sg4::NetZone* root_zone, const std::vector<Switch>& switches, const std::string& bw = "40GBps", const std::string& lat = "5us") {
  for (size_t i = 0; i < switches.size(); ++i) {
    for (size_t j = i + 1; j < switches.size(); ++j) {
      connect_switches(root_zone, switches[i], switches[j], bw, lat);
    }
  }
}

// ================ Node creation ================
sg4::NetZone* build_node(sg4::NetZone* root_zone,
			 Node node,
			 const std::vector<Switch>& parent_switches) {

  auto* node_zone = root_zone->add_netzone_full(node.name);

  std::string router_name = "Router_" + node.name;
  auto* node_router = node_zone->add_router(router_name);
  node_zone->set_gateway(node_router);

  // cluster for .dot
  arquivo_dot << "  subgraph cluster_" << node.name << " {\n";
  arquivo_dot << "    label=\"Nodo " << node.name << "\"; style=dashed; color=gray; fontname=\"Helvetica\";\n";
  // router
  arquivo_dot << "    \"" << router_name << "\" [shape=ellipse, fillcolor=orange];\n";

  std::vector<sg4::Host*> cores;

  // create cores and link it to router
  auto calib_props = load_calibration(node.calib_file);
  for (int i = 0; i < node.cores; ++i) {

    std::string core_name = node.name + "/" + std::to_string(i);
    auto* core = node_zone->add_host(core_name, node.speed);

    for (const auto& [key, value] : calib_props) {
      core->set_property(key, value);
    }

    cores.push_back(core);

    auto* link_to_router = node_zone->add_link("Link_" + core_name + "_to_Router", "500GBps")->set_latency("1ns");
    node_zone->add_route(core->get_netpoint(), node_router, std::vector<const sg4::Link*>{link_to_router});

    log_conexao(core_name, router_name, "blue");
  }

  // connections between cores
  for (size_t i = 0; i < cores.size(); ++i) {
    for (size_t j = i + 1; j < cores.size(); ++j) {
      std::string p2p_link_name = "Link_cpu_" + cores[i]->get_name() + "_" + cores[j]->get_name();
      auto* link_p2p = node_zone->add_link(p2p_link_name, "500GBps")->set_latency("1ns");
      node_zone->add_route(cores[i], cores[j], std::vector<const sg4::Link*>{link_p2p});

      log_conexao(cores[i]->get_name(), cores[j]->get_name(), "red", "dashed");
    }
  }

  // end .dot for the node
  arquivo_dot << "  }\n";

  // connect the router to the switchs
  for (const auto& sw : parent_switches) {
    std::string uplink_name = "Uplink_" + node.name + "_to_" + sw.router_name;
    auto* link_uplink = root_zone->add_link(uplink_name, sw.bw)->set_latency(sw.lat);

    root_zone->add_route(node_zone, sw.zone, std::vector<const sg4::Link*>{link_uplink});

    log_conexao(router_name, sw.router_name, "green", "solid", 2);
  }

  return node_zone;
}

// Construct one cei partition
void build_cei(sg4::NetZone* root_zone, const std::vector<Switch>& parent_switches, int num_nodes) {

  // Host speed is already divided by number of cores.
  Node cei = {"cei", "cei.txt", "21.125Gf", 24};

  for(int i = 1; i <= num_nodes; i++){
    cei.name = "cei" + std::to_string(i);
    build_node(root_zone, cei, parent_switches);
  }

}

extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e){
  init_dot("topology.dot");

  auto* implicit_root = e.get_netzone_root();
  auto* root_zone = implicit_root->add_netzone_dijkstra("PCAD", true);

  // create the switchs
  std::vector<Switch> switches;
  switches.push_back(build_switch(root_zone, "switch_1g_rack2", "1Gbps", "100us"));
  switches.push_back(build_switch(root_zone, "switch_10g_rack2", "10Gbps", "100us"));
  // core_switches.push_back(build_switch(root_zone, "switch_1g_rack4"));

  // connect all the switchs
  connect_switch_mesh(root_zone, switches, "500GBps", "0.1us");

  // Create the cei partition
  build_cei(root_zone, {switches[0], switches[1]}, 6);

  close_dot();
  e.seal_platform();
}

// int main(int argc, char* argv[]) {
//   sg4::Engine e(&argc, argv);
//   create_platform(e);

//   // std::cout << "[SIM] platform created, .dot generated ---\n";
//   return 0;
// }
