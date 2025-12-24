#include <numeric>
#include <simgrid/s4u.hpp>

namespace sg4 = simgrid::s4u;

typedef struct partition {
    std::string name;
    std::string speed;
    std::string bw;
    std::string lat;
    int count;
} partition_t;

/**
 * @brief Create a partition with N machines
 *
 * This function creates the any partition, adding the hosts and links properly.
 *
 * @param root Root netzone
 * @param switch_fatpipe Switch link for the rack to model the connection
 * @param partition Information about the hosts in the partition
 */
void create_partition(sg4::NetZone* rack,
                      sg4::Link* switch_fatpipe,
		      partition_t partition)
{
  for (int id = 1; id <= partition.count; id++) {
    std::string hostname = partition.name + std::to_string(id);

    auto* host = rack->add_host(hostname, partition.speed)->seal();
    auto* node_cable = rack->add_link(hostname + "_cable", partition.bw)->set_latency(partition.lat)->seal();

    // Looks like StarZone automatically calculates the path to the netzone gateway when the second argument is a nullptr
    rack->add_route(host,
                    nullptr,
                    std::vector<const sg4::Link*>{node_cable, switch_fatpipe});
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
			  std::string switch_capacity, std::string switch_latency,
                          std::vector<partition_t> partitions)
    {

      auto* rack_zone = root->add_netzone_star(rack_name);
      auto* rack_router = rack_zone->add_router(rack_name + "_router");
      rack_zone->set_gateway(rack_router); // rack gateaway

      // Create Fatpipe to simulat the switch
      auto* rack_switch_fatpipe = rack_zone->add_link(rack_name + "_fatpipe", switch_capacity)
	->set_latency(switch_latency)
	->set_sharing_policy(sg4::Link::SharingPolicy::FATPIPE)
	->seal();

      // Nodes partitions (nodes)
      for(const auto& partition : partitions){
	create_partition(rack_zone, rack_switch_fatpipe, partition);

      }

      rack_zone->seal();
      return rack_zone;
}

/** @brief Programmatic version of PCAD */
extern "C" void load_platform(sg4::Engine& e);
void load_platform(sg4::Engine& e)
{
  auto* root = e.get_netzone_root();

  // =-=-=-=-=-=-=-=-=-=-=-= RACK 2 =-=-=-=-=-=-=-=-=-=-=-=
  partition_t cei = {"cei", "100kf", "1Gbps", "50us", 6};
  partition_t draco = {"draco", "50kf", "1Gbps", "50us", 6};
  std::vector<partition_t> rack2_partitions = {cei, draco};

  sg4::NetZone *rack2_zone = create_rack(root, "rack2", "10Gbps", "100ns", rack2_partitions);

  // =-=-=-=-=-=-=-=-=-=-=-= RACK 4 =-=-=-=-=-=-=-=-=-=-=-=
  partition_t tupi = {"tupi", "500kf", "1Gbps", "50us", 6};
  partition_t poti = {"poti", "400kf", "1Gbps", "50us", 6};
  std::vector<partition_t> rack4_partitions = {poti, tupi};

  sg4::NetZone *rack4_zone = create_rack(root, "rack4", "10Gbps", "100ns", rack4_partitions);


  // =-=-=-=-=-=-=-=-=-=-=-= CONNECTION BETWEEN THE RACKS (switchs) =-=-=-=-=-=-=-=-=-=-=-=
  std::string  inter_switch_bw = "10Gbps";
  std::string inter_switch_lat = "50us";

  auto* rack_link = root->add_link("r2_to_r4_cable", inter_switch_bw)
    ->set_latency(inter_switch_lat)
    ->seal();

  // connect the zones
  root->add_route(rack2_zone, rack4_zone, {rack_link});

  root->seal();
}
