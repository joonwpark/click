#ifndef CLICK_GatewaySelector_HH
#define CLICK_GatewaySelector_HH
#include <click/element.hh>
#include <click/glue.hh>
#include <click/timer.hh>
#include <click/ipaddress.hh>
#include <click/etheraddress.hh>
#include <click/vector.hh>
#include <click/hashmap.hh>
#include <click/dequeue.hh>
#include <elements/wifi/linktable.hh>
#include <elements/wifi/arptable.hh>
#include <elements/wifi/sr/path.hh>
CLICK_DECLS

/*
 * =c
 * GatewaySelector(IP, ETH, ETHERTYPE, LinkTable, ARPTable, 
 *                 [PERIOD timeout], [GW is_gateway], 
 *                 [METRIC GridGenericMetric])
 * =d

 * Input 0: packets from dev
 * Input 1: packets for gateway node
 * Output 0: packets to dev
 * Output 1: packets with dst_ip anno set
 *
 * This element provides proactive gateway selection.  
 * Each gateway broadcasts an ad every PERIOD seconds.  
 * Non-gateway nodes select the gateway with the best metric
 * and forward ads.
 * 
 * 
 *
 */


class GatewaySelector : public Element {
 public:
  
  GatewaySelector();
  ~GatewaySelector();
  
  const char *class_name() const		{ return "GatewaySelector"; }
  const char *processing() const		{ return PUSH; }
  int initialize(ErrorHandler *);
  int configure(Vector<String> &conf, ErrorHandler *errh);


  /* handler stuff */
  void add_handlers();

  String print_gateway_stats();

  static int write_param(const String &arg, Element *e,
				void *, ErrorHandler *errh); 
  
  static String read_param(Element *, void *);

  static int static_pick_new_gateway(const String &arg, Element *el,
				     void *, ErrorHandler *errh);
  void push(int, Packet *);
  void run_timer();

  int get_fwd_metric(IPAddress other);
  int get_rev_metric(IPAddress other);
  bool update_link(IPAddress from, IPAddress to, int metric);
  void forward_ad_hook();
  IPAddress best_gateway();
private:
    // List of query sequence #s that we've already seen.
  class Seen {
  public:
    IPAddress _gw;
    u_long _seq;
    int _fwd_metric;
    int _rev_metric;
    int _count;
    struct timeval _when; /* when we saw the first query */
    struct timeval _to_send;
    bool _forwarded;
    Vector<IPAddress> _hops;
    Vector<int> _fwd_metrics;
    Vector<int> _rev_metrics;
    Seen(IPAddress gw, u_long seq, int fwd, int rev) {
      _gw = gw; _seq = seq; _count = 0; _rev_metric = rev; _fwd_metric = fwd;
    }
    Seen();
  };
  
  DEQueue<Seen> _seen;



  typedef HashMap<IPAddress, IPAddress> IPTable;
  typedef IPTable::const_iterator IPIter;
  IPTable _ignore;
  IPTable _allow;
  
  class GWInfo {
  public:
    IPAddress _ip;
    struct timeval _last_update;
    GWInfo() {memset(this,0,sizeof(*this)); }
  };

  typedef HashMap<IPAddress, GWInfo> GWTable;
  typedef GWTable::const_iterator GWIter;
  GWTable _gateways;


  int MaxSeen;   // Max size of table of already-seen queries.
  int MaxHops;   // Max hop count for queries.
  struct timeval _gw_expire;
  u_long _seq;      // Next query sequence number to use.
  IPAddress _ip;    // My IP address.
  EtherAddress _en; // My ethernet address.
  uint32_t _et;     // This protocol's ethertype
  unsigned int _period; // msecs

  EtherAddress _bcast;
  bool _is_gw;

  class LinkTable *_link_table;
  class LinkMetric *_metric;
  class ARPTable *_arp_table;
  Timer _timer;





  int find_dst(IPAddress ip, bool create);
  EtherAddress find_arp(IPAddress ip);
  void got_arp(IPAddress ip, EtherAddress en);
  void got_sr_pkt(Packet *p_in);
  void start_ad();
  void process_query(struct sr_pkt *pk);
  void forward_ad(Seen *s);
  void send(WritablePacket *);
  void process_data(Packet *p_in);
  bool pick_new_gateway();
  bool valid_gateway(IPAddress);
  static void static_forward_ad_hook(Timer *, void *e) { 
    ((GatewaySelector *) e)->forward_ad_hook(); 
  }
};


CLICK_ENDDECLS
#endif