#include <numeric>
#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

/**
 * @brief Create a partition with N machines
 *
 * This function creates the any partition, adding the hosts and links properly.
 * See figure below for more details of each cabinet
 *
 * @param root Root netzone
 * @param switch_fatpipe Switch link for the rack to model the connection
 * @param N Number of hosts
 * @param name Host partition name
 * @param host_speed Host computing speed
 * @param host_bw Host Bandwidth
 * @param host_lat Host Latency
 */

void create_partition(sg4::NetZone* rack,
                      sg4::Link* switch_fatpipe,
                      int N,
                      const std::string& name, const std::string& host_speed,
                      const std::string& host_bw, const std::string& host_lat)
{
  for (int id = 1; id <= N; id++) {
    std::string hostname = name + std::to_string(id);

    auto* host = rack->add_host(hostname, host_speed)->seal();

    auto* node_cable = rack->add_link(hostname + "_cable", host_bw)->set_latency(host_lat)->seal();

    // Looks like StarZone automatically calculates the path to the netzone gateway when the second argument is a nullptr
    rack->add_route(host,
                    nullptr,
                    std::vector<const sg4::Link*>{node_cable, switch_fatpipe});
  }
}

/** @brief Programmatic version of PCAD */
extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e)
{
  auto* root = e.get_netzone_root();

  // =-=-=-=-=-=-=-=-=-=-=-= RACK 2 =-=-=-=-=-=-=-=-=-=-=-=
  std::string rack_name = "rack2";
  auto* rack2_zone = root->add_netzone_star(rack_name);
  auto* rack2_router = rack2_zone->add_router(rack_name + "_router");
  rack2_zone->set_gateway(rack2_router); // rack gateaway

  std::string switch_capacity = "10Gbps";
  std::string switch_latency  = "100ns";

  // Create Fatpipe to simulate the switch
  auto* rack2_switch_fatpipe = rack2_zone->add_link(rack_name + "_fatpipe", switch_capacity)
    ->set_latency(switch_latency)
    ->set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)
    ->seal();

  // Nodes partitions (nodes)
  create_partition(rack2_zone, rack2_switch_fatpipe, 6, "cei", "100kf", "1Gbps", "50us");
  create_partition(rack2_zone, rack2_switch_fatpipe, 6, "draco", "50kf", "1Gbps", "50us");

  rack2_zone->seal();

  // =-=-=-=-=-=-=-=-=-=-=-= RACK 4 =-=-=-=-=-=-=-=-=-=-=-=
  rack_name = "rack4";
  auto* rack4_zone = root->add_netzone_star(rack_name); // StarZone
  auto* rack4_router = rack4_zone->add_router(rack_name + "_router");
  rack4_zone->set_gateway(rack4_router);

  switch_capacity = "10Gbps";
  switch_latency  = "100ns";

  auto* rack4_switch_fatpipe = rack4_zone->add_link(rack_name + "_fatpipe", switch_capacity)
    ->set_latency(switch_latency)
    ->set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)
    ->seal();

  create_partition(rack4_zone, rack4_switch_fatpipe, 5, "poti", "400kf", "1Gbps", "50us");
  create_partition(rack4_zone, rack4_switch_fatpipe, 4, "tupi", "500kf", "1Gbps", "50us");

  rack4_zone->seal();


  // =-=-=-=-=-=-=-=-=-=-=-= CONNECTION BETWEEN THE RACKS (switchs) =-=-=-=-=-=-=-=-=-=-=-=
  std::string inter_switch_bw = "10Gbps";
  std::string inter_switch_lat = "50us";

  auto* rack_link = root->add_link("r2_to_r4_cable", inter_switch_bw)
    ->set_latency(inter_switch_lat)
    ->seal();

  // connect the zones
  root->add_route(rack2_zone, rack4_zone, {rack_link});

  root->seal();
}
