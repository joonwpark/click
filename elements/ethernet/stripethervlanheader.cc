/*
 * stripethervlanheader.{cc,hh} -- encapsulates packet in Ethernet header
 *
 * Copyright (c) 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "stripethervlanheader.hh"
#include <click/etheraddress.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/packet_anno.hh>
CLICK_DECLS

StripEtherVlanHeader::StripEtherVlanHeader()
{
}

StripEtherVlanHeader::~StripEtherVlanHeader()
{
}

int
StripEtherVlanHeader::configure(Vector<String> &conf, ErrorHandler *errh)
{
    int native_vlan = 0;
    if (cp_va_kparse(conf, this, errh,
		     "NATIVE_VLAN_ID", cpkP, cpInteger, &native_vlan,
		     cpEnd) < 0)
	return -1;
    if (native_vlan >= 0xFFF)
	return errh->error("bad NATIVE_VLAN_ID");
    _has_native_vlan = native_vlan >= 0;
    _native_vlan = htons(native_vlan);
    return 0;
}

Packet *
StripEtherVlanHeader::simple_action(Packet *p)
{
    const click_ether_vlan *ethh = reinterpret_cast<const click_ether_vlan *>(p->data());
    if (ethh->ether_vlan_proto == htons(ETHERTYPE_8021Q)) {
	SET_VLAN_ANNO(p, ethh->ether_vlan_tci);
	p->pull(sizeof(click_ether_vlan));
    } else if (_has_native_vlan) {
	SET_VLAN_ANNO(p, _native_vlan);
	p->pull(sizeof(click_ether));
    } else {
	checked_output_push(1, p);
	p = 0;
    }
    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(StripEtherVlanHeader)
