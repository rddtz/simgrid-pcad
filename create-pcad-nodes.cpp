#include <simgrid/s4u.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>

namespace sg4 = simgrid::s4u;

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
  int cores;
};

// ================ Helpers ================
std::map<std::string, std::string> load_calibration(const std::string& filename) {
  std::map<std::string, std::string> props;
  if (filename.empty()) return props;
  std::ifstream file(filename);
  std::string line;
  while (std::getline(file, line)) {
    auto delim = line.find('=');
    if (delim != std::string::npos) {
      props[line.substr(0, delim)] = line.substr(delim + 1);
    }
  }
  return props;
}

// ================ Switch (NetZone) ================
Switch build_switch(sg4::NetZone* root_zone, const std::string& name, const std::string& bw, const std::string& lat) {
  auto* sw_zone = root_zone->add_netzone_dijkstra(name + "_zone", true);
  std::string router_name = "router_from_" + name;
  auto* sw_router = sw_zone->add_router(router_name);
  sw_zone->set_gateway(sw_router);

  return {sw_zone, router_name, bw, lat};
}

// ================ Node creation ================
void build_node(Switch& sw, Node node) {

  auto* host = sw.zone->add_host(node.name, node.speed);

  host->set_core_count(node.cores);


  auto calib_props = load_calibration(node.calib_file);
  for (const auto& [key, value] : calib_props) {
    host->set_property(key, value);
  }

  std::string link_name = "link_" + node.name;
  auto* link_nic = sw.zone->add_link(link_name, sw.bw)->set_latency(sw.lat);

  sw.zone->add_route(host->get_netpoint(), sw.zone->get_gateway(), {sg4::LinkInRoute(link_nic)});
}

void build_cei(std::vector<Switch>& switches, int num_nodes) {
  Node cei = {"cei", "", "21.125Gf", 24};

  for(int i = 1; i <= num_nodes; i++) {
    cei.name = "cei" + std::to_string(i);
    build_node(switches[0], cei);
  }
}

extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e) {
  auto* root_zone = e.get_netzone_root();

  std::vector<Switch> switches;
  switches.push_back(build_switch(root_zone, "switch_10g_rack2", "9.413Gbps", "124.5us"));

  build_cei(switches, 6);

  e.seal_platform();
}
