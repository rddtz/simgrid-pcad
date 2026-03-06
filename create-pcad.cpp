#include <numeric>
#include <simgrid/s4u.hpp>
#include <fstream>

namespace sg4 = simgrid::s4u;

typedef struct partition {
  std::string name;
  std::string calib_file;
  std::string speed;
  std::string bw;
  std::string lat;
  std::string switch_ref;
  int count;
  int cores; // Only counting phisical cores
} partition_t;

typedef struct switch_info {
  std::string name;
  std::string capacity;
  std::string latency;
} switch_t;

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
      props[key] = value;
    }
  }
  return props;
}

/*
 @brief Create a partition with N machines

 This function creates the any partition, adding the hosts and links properly.
 @param root Root netzone
 @param switch_fatpipe Switch link for the rack to model the connection
 @param partition Information about the hosts in the partition
*/

void create_partition(sg4::NetZone* rack,
                      sg4::Link* switch_cable,
		      partition_t partition)
{

  auto calib_props = load_calibration(partition.calib_file);

  for (int id = 1; id <= partition.count; id++) {
    std::string hostname = partition.name + std::to_string(id);

    auto* host = rack->add_host(hostname, partition.speed)->set_core_count(partition.cores);

    for (const auto& [key, value] : calib_props) {
        host->set_property(key, value);
    }

    host->seal();

    auto* node_cable = rack->add_link(hostname + "_cable", partition.bw)->set_latency(partition.lat)->seal();

    // Looks like StarZone automatically calculates the path to the netzone gateway when the second argument is a nullptr
    rack->add_route(host,
                    nullptr,
                    std::vector<const sg4::Link*>{node_cable, switch_cable});
  }
}

/**
 * @brief Create a rack with N partitions
 *
 * This function creates the a rack, adding the internal partitions linked correctly.
 *
 * @param root Root netzone
 * @param rack_name Rack name
 * @param partitions Vector with the partitions information
 */
sg4::NetZone *create_rack(sg4::NetZone *root, std::string rack_name,
			  std::vector<switch_t> switches,
                          std::vector<partition_t> partitions)
    {

      auto* rack_zone = root->add_netzone_star(rack_name);
      auto* rack_router = rack_zone->add_router(rack_name + "_router");
      rack_zone->set_gateway(rack_router); // rack gateaway


      std::map<std::string, sg4::Link*> switch_links;

      for(const auto& sw : switches) {
	auto* cable = rack_zone->add_link(rack_name + "_" + sw.name, sw.capacity)
	  ->set_latency(sw.latency)
	  ->set_sharing_policy(sg4::Link::SharingPolicy::SHARED)
	  ->seal();

	switch_links[sw.name] = cable;
      }

      // // Create Fatpipe to simulat the switch
      // auto* rack_switch_fatpipe = rack_zone->add_link(rack_name + "_fatpipe", switch_capacity)
      // 	->set_latency(switch_latency)
      // 	->set_sharing_policy(sg4::Link::SharingPolicy::SHARED)
      // 	->seal();

      // Nodes partitions (nodes)
      for(const auto& partition : partitions){
	create_partition(rack_zone, switch_links[partition.switch_ref], partition);

      }

      rack_zone->seal();
      return rack_zone;
}

/** @brief Programmatic version of PCAD */
extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e)
{
  auto* root = e.get_netzone_root();

  switch_t r2_sw_10gb = {"switch_10gb", "10Gbps", "100ns"};
  switch_t r2_sw_1gb  = {"switch_1gb", "1Gbps", "100ns"};
  std::vector<switch_t> rack2_switches = {r2_sw_10gb, r2_sw_1gb};

  // partition_t cei = {"cei", "", "19.125Gf", "9.413Gbps", "124us", 6, 24};
  partition_t cei = {"cei", "", "19.125Gf", "9.413Gbps", "124us", "switch_10gb", 6, 24};
  // partition_t draco = {"draco", "draco.txt", "14.125Gf", "941Mbps", "115us", 6, 16};
  partition_t draco = {"draco", "", "14.125Gf", "941Mbps", "115us", "switch_1gb", 6, 16};
  std::vector<partition_t> rack2_partitions = {cei, draco};

  sg4::NetZone *rack2_zone = create_rack(root, "rack2", rack2_switches, rack2_partitions);

  // =-=-=-=-=-=-=-=-=-=-=-= RACK 4 =-=-=-=-=-=-=-=-=-=-=-=

  switch_t r4_sw_1gb = {"switch_1gb", "1Gbps", "100ns"};
  std::vector<switch_t> rack4_switches = {r4_sw_1gb};

  partition_t poti = {"poti", "", "20.2Gf", "940Mbps", "117us", "switch_1gb", 5, 20};
  // partition_t tupi = {"tupi", "tupi.txt", "58.375Gf", "942Mbps", "87us", 6, 8};
  partition_t tupi = {"tupi", "", "58.375Gf", "942Mbps", "87us", "switch_1gb", 6, 8};
  std::vector<partition_t> rack4_partitions = {poti, tupi};

  sg4::NetZone *rack4_zone = create_rack(root, "rack4", rack4_switches, rack4_partitions);

  // =-=-=-=-=-=-=-=-=-=-=-= CONNECTION BETWEEN THE RACKS (switchs) =-=-=-=-=-=-=-=-=-=-=-=
  std::string inter_switch_bw = "10Gbps";
  std::string inter_switch_lat = "50us";

  auto* link_rack2_gw = root->add_link("link_rack2_gateway", inter_switch_bw)->set_latency(inter_switch_lat)->seal();
  auto* link_rack4_gw = root->add_link("link_rack4_gateway", inter_switch_bw)->set_latency(inter_switch_lat)->seal();

  // auto* rack_link = root->add_link("r2_to_r4_cable", inter_switch_bw)
  //   ->set_latency(inter_switch_lat)
  //   ->seal();

  root->add_route(rack2_zone, rack4_zone, std::vector<const sg4::Link*>{link_rack2_gw, link_rack4_gw});
  // connect the zones
  // root->add_route(rack2_zone, rack4_zone, {rack_link});

  root->seal();
}
