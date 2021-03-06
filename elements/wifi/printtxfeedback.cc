/*
 * printtxfeedback.{cc,hh} -- print Wifi packets, for debugging.
 * John Bicket
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
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
#include <click/ipaddress.hh>
#include <click/confparse.hh>
#include <click/error.hh>
#include <click/glue.hh>
#include <click/straccum.hh>
#include <click/packet_anno.hh>
#include <clicknet/wifi.h>
#include <click/etheraddress.hh>
#include "printtxfeedback.hh"
CLICK_DECLS

PrintTXFeedback::PrintTXFeedback()
  : _print_anno(false),
    _print_checksum(false)
{
  _label = "";
}

PrintTXFeedback::~PrintTXFeedback()
{
}

int
PrintTXFeedback::configure(Vector<String> &conf, ErrorHandler* errh)
{
  int ret;
  _offset = 0;
  ret = cp_va_kparse(conf, this, errh,
		     "LABEL", cpkP, cpString, &_label,
		     "OFFSET", 0, cpUnsigned, &_offset,
		     cpEnd);
  return ret;
}

Packet *
PrintTXFeedback::simple_action(Packet *p)
{

  if (!p) {
    return p;
  }
  uint8_t *dst_ptr = (uint8_t *) p->data() + _offset;
  EtherAddress dst = EtherAddress(dst_ptr);


  StringAccum sa;
  if (_label[0] != 0) {
    sa << _label.c_str() << ":";
  } else {
      sa << "PrintTXFeedback";
  }
  struct click_wifi_extra *ceh = WIFI_EXTRA_ANNO(p);
  sa << " " << p->timestamp_anno();
  sa << " " << dst;
  sa << " flags " << (int) ceh->flags;
  sa << " rate " << (int) ceh->rate;
  sa << " max_retries " << (int) ceh->max_tries;
  sa << " alt_rate " << (int) ceh->rate1;
  sa << " alt_retries " << (int) ceh->max_tries1;
  sa << " retries " << (int) ceh->retries;
  sa << " virt_col " << (int) ceh->virt_col;

  click_chatter("%s\n", sa.c_str());


  return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(PrintTXFeedback)
