/*
* Authors: Dylan Avery
* License: refer to the LICENSE file in the root of the project
*
* This utility aims to ease querying of specific facts about interfaces within shell scripts. 
* Many utilities do this, such as ip(8), though the output is not easily parsed.
*
* Utilising ioctl(2) and netdevice(7)
*
* TODO: Implement IPV6 support.
* TODO: Implement a CLI in main() for use within shell scripts.
*
* Approach:
* Query the kernel via IOCTL using the 'get' controls defined in netdevice(7).
* Construct some if_info structs for the available network interfaces.
* Some modification is neccessary for the likes of IPV4 addresses, to support CIDR notation.
* Provide an API for querying information about networks interfaces.
*
*/

#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>

/*
* Size: 5 bytes
*   (Assuming int is 4 bytes)
*   (Assuming char is 1 byte)
*/
struct if_addr_ip4 {
  int                   addr;
  char                  cidr_mask;
}

/*
* Minimum Size: 45 Bytes (assuming a single IPV4 address)
* Maximum Size: 1315 Bytes (and 26 bools/bits)
* (Assuming int is 4 bytes)
* (Assuming char is 1 byte)
* (Assuming bool is 1 bit)
*/
struct if_info {
  char                  name[IFNAMESIZ]; //16 chars (see linux/net/if.h)
  char                  mac_addr[6];
  
  struct if_addr_ip4    ip4_addrs[255];
  struct if_addr_ip4    ip4_brd_addr;
  struct if_addr_ip4    ip4_ptp_dst_addr;
  
  bool                  is_up;
  bool                  is_running;
  bool                  is_loopback;
  bool                  is_promiscuous;
  bool                  is_ptp;
  bool                  is_vlan;
  bool                  is_bridge;
  bool                  has_brd_addr;
  bool                  is_multicast_supported;
  bool                  rcv_all_multicast;
  bool                  is_addrs_dynamic;
  bool                  no_arp;
  bool                  avoid_trailers;
  bool                  is_master;
  bool                  is_master_8023ad;
  bool                  is_master_alb;
  bool                  is_slave;
  bool                  is_slave_inactive;
  bool                  is_slave_needarp;
  bool                  is_bonding;
  bool                  is_rfc4214_isatap;
  bool                  can_sel_media;
  bool                  auto_sel_media;
  bool                  sig_lower_up;
  bool                  sig_dormant;
  bool                  echo_pkts;
  int                   metric;
  int                   mtu;
}

#define size_if_infos_init                                     255
struct if_info                                                 if_infos[size_if_infos_init];

char*                get_if_name                               (int if_idx);

char*                get_if_mac_addr                           (int if_idx);

struct if_addr_ip4*  get_if_ipv4_addresses                     (int if_idx);

struct if_addr_ip4*  get_if_ptp_ipv4_dst_address               (int if_idx);

struct if_addr_ip4*  get_if_ipv4_broadcast_address             (int if_idx);

char*                convert_if_addr_ip4_to_cidr_str           (if_addr_ip4* if_addr_ip4);

int                  get_if_metric                             (int if_idx);

int                  get_if_mtu                                (int if_idx);

short                get_if_flags_public                       (int if_idx);

bool                 get_if_flags_is_up                        (short if_flags_public);

bool                 get_if_flags_is_running                   (short if_flags_public);

bool                 get_if_flags_is_loopback                  (short if_flags_public);

bool                 get_if_flags_is_promiscuous               (short if_flags_public);

bool                 get_if_flags_is_ptp                       (short if_flags_public);

bool                 get_if_flags_has_brd_addr                 (short if_flags_public);

bool                 get_if_flags_is_rcv_multicast             (short if_flags_public);

bool                 get_if_flags_is_addrs_dynamic             (short if_flags_public);

bool                 get_if_flags_no_arp                       (short if_flags_public);

bool                 get_if_flags_is_no_trailers               (short if_flags_public);

bool                 get_if_flags_is_master                    (short if_flags_public);

bool                 get_if_flags_is_slave                     (short if_flags_public);

bool                 get_if_flags_can_select_media             (short if_flags_public);

bool                 get_if_flags_auto_select_media            (short if_flags_public);

bool                 get_if_flags_is_layer_1_up                (short if_flags_public);

bool                 get_if_flags_is_dormant                   (short if_flags_public);

bool                 get_if_flags_are_pkts_echoed              (short if_flags_public);

short                get_if_flags_private                      (int if_idx);

bool                 get_if_flags_is_vlan                      (short if_flags_private);

bool                 get_if_flags_is_ether_bridge              (short if_flags_private);

bool                 get_if_flags_is_slave_inactive            (short if_flags_private);

bool                 get_if_flags_is_master_8023ad             (short if_flags_private);

bool                 get_if_flags_is_master_alb                (short if_flags_private);

bool                 get_if_flags_is_bonding                   (short if_flags_private);

bool                 get_if_flags_needs_arp_validation         (short if_flags_private);

bool                 get_if_flags_is_isatap                    (short if_flags_private);

if_info*             construct_if_info                         (int if_idx);

int                  populate_if_infos                         ();

int                  print_stdout_name_str                     (int if_idx);

int                  print_stdout_mac_addr_str                 (int if_idx);

int                  print_stdout_ipv4_addr_cidr_str           (int if_idx, int addr_idx);

int                  print_stdout_ipv4_addrs_cidr_str          (int if_idx);

int                  print_stdout_ipv4_brd_addr_str            (int if_idx);

int                  print_stdout_flag_is_up_bool              (int if_idx);

int                  print_stdout_flag_has_brd_addr_bool       (int if_idx);

int                  print_stdout_flag_is_loopback_bool        (int if_idx);

int                  print_stdout_flag_is_ptp_bool             (int if_idx);

int                  print_stdout_flag_is_running_bool         (int if_idx);

int                  print_stdout_flag_no_arp_bool             (int if_idx);

int                  print_stdout_flag_is_promisc_bool         (int if_idx);

int                  print_stdout_flag_avoid_trailers_bool     (int if_idx);

int                  print_stdout_flag_rcv_multicast_bool      (int if_idx);

int                  print_stdout_flag_is_master_bool          (int if_idx);

int                  print_stdout_flag_is_slave_bool           (int if_idx);

int                  print_stdout_flag_can_sel_media_bool      (int if_idx);

int                  print_stdout_flag_auto_sel_media_bool     (int if_idx);

int                  print_stdout_flag_is_addrs_dynamic_bool   (int if_idx);

int                  print_stdout_flag_sig_lower_up_bool       (int if_idx);

int                  print_stdout_flag_sig_dormant_bool        (int if_idx);

int                  print_stdout_flag_echo_pkts_bool          (int if_idx);

int                  print_stdout_flag_is_vlan_8021q_bool      (int if_idx);

int                  print_stdout_flag_is_bridge_bool          (int if_idx);

int                  print_stdout_flag_is_slave_inactive_bool  (int if_idx);

int                  print_stdout_flag_is_master_8023ad_bool   (int if_idx);

int                  print_stdout_flag_is_master_alb_bool      (int if_idx);

int                  print_stdout_flag_is_master_or_slave_bool (int if_idx);

int                  print_stdout_flag_is_slave_needarp_bool   (int if_idx);

int                  print_stdout_flag_is_rfc4214_isatap_bool  (int if_idx);

int                  print_stdout_metric_int                   (int if_idx);

int                  print_stdout_mtu_int                      (int if_idx);

//Free memory before program exit.
int                  destruct                                  ();
