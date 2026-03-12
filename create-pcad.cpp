#include <simgrid/s4u.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <map>

namespace sg4 = simgrid::s4u;

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

Switch build_switch(sg4::NetZone* root_zone, const std::string& name, const std::string& bw, const std::string& lat) {
  auto* sw_zone = root_zone->add_netzone_dijkstra(name + "_zone", true);
  std::string router_name = name + " router";
  auto* sw_router = sw_zone->add_router(router_name);
  sw_zone->set_gateway(sw_router);

  return {sw_zone, router_name, bw, lat};
}

void build_node(Switch& sw, Node node) {

  std::string router_name =  node.name + " router";
  auto* node_router = sw.zone->add_router(router_name);

  auto* link_switch = sw.zone->add_split_duplex_link(node.name + " -- " + sw.router_name, sw.bw)
    ->set_latency(sw.lat);

  sw.zone->add_route(node_router, sw.zone->get_gateway(),
                     {sg4::LinkInRoute(link_switch, sg4::LinkInRoute::Direction::UP)}, false);
  sw.zone->add_route(sw.zone->get_gateway(), node_router,
                     {sg4::LinkInRoute(link_switch, sg4::LinkInRoute::Direction::DOWN)}, false);

  // sw.zone->add_route(node_router, sw.zone->get_gateway(), {sg4::LinkInRoute(link_switch)});

  std::vector<sg4::Host*> cores;

  auto calib_props = load_calibration(node.calib_file);
  for (int i = 0; i < node.cores; ++i) {

    std::string core_name = node.name + "/" + std::to_string(i);
    auto* core = sw.zone->add_host(core_name, node.speed);

    for (const auto& [key, value] : calib_props) {
      core->set_property(key, value);
    }

    cores.push_back(core);

    auto* link_router = sw.zone->add_split_duplex_link(core_name + " -- " + router_name, "1000Gbps")
      ->set_latency("0ns");

    sw.zone->add_route(core->get_netpoint(), node_router,
                       {sg4::LinkInRoute(link_router, sg4::LinkInRoute::Direction::UP)}, false);

    sw.zone->add_route(node_router, core->get_netpoint(),
                       {sg4::LinkInRoute(link_router, sg4::LinkInRoute::Direction::DOWN)}, false);
  }
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
  switches.push_back(build_switch(root_zone, "switch_10g_rack2", "9.413Gbps", "30us"));

  // Cria os CEIs dentro da hierarquia do switch
  build_cei(switches, 6);

  e.seal_platform();
}
