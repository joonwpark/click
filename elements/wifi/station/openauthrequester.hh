#ifndef CLICK_OPENAUTHREQUESTER_HH
#define CLICK_OPENAUTHREQUESTER_HH
#include <click/element.hh>
#include <clicknet/ether.h>
CLICK_DECLS

/*
=c

OpenAuthRequester(CHANNEL)

=s decapsulation, Wifi -> Ethernet

Turns 80211 packets into ethernet packets encapsulates packets in Ethernet header

=d


If channel is 0, it doesn't filter any beacons.
If channel is < 0, it doesn't look at any beconds
if channel is > 0, it looks at only beacons with on channel.
=e


  wifi_cl :: Classifier (0/00%0c, 
                         0/04%0c,
                         0/08%0c);

  wifi_cl [0] -> Discard; //mgt 
  wifi_cl [1] -> Discard; //ctl
  wifi_cl [2] -> wifi_decap :: OpenAuthRequester() -> ...

=a

EtherEncap */

class OpenAuthRequester : public Element { public:
  
  OpenAuthRequester();
  ~OpenAuthRequester();

  const char *class_name() const	{ return "OpenAuthRequester"; }
  const char *processing() const	{ return PUSH; }
  
  int configure(Vector<String> &, ErrorHandler *);
  bool can_live_reconfigure() const	{ return true; }
  void add_handlers();

  void send_auth_request();
  void push(int, Packet *);


  bool _debug;
  EtherAddress _bssid;
  EtherAddress _eth;
 private:


};

CLICK_ENDDECLS
#endif
