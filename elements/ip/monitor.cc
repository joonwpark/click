/*
 * monitor.{cc,hh} -- counts packets clustered by src/dst addr.
 * Thomer M. Gil
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology.
 *
 * This software is being provided by the copyright holders under the GNU
 * General Public License, either version 2 or, at your discretion, any later
 * version. For more information, see the `COPYRIGHT' file in the source
 * distribution.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include "monitor.hh"
#include "confparse.hh"
#include "click_ip.h"
#include "error.hh"
#include "glue.hh"

Monitor::Monitor()
  : Element(1,1), _pb(COUNT_PACKETS), _base(NULL)
{
}

Monitor::~Monitor()
{
}

int
Monitor::configure(const String &conf, ErrorHandler *errh)
{
#if IPVERSION == 4
  Vector<String> args;
  cp_argvec(conf, args);

  // Enough args?
  if(args.size() < 2) {
    errh->error("too few arguments");
    return -1;
  }

  // PACKETS/BYTES
  if(args[0] == "PACKETS")
    _pb = COUNT_PACKETS;
  else if(args[0] == "BYTES")
    _pb = COUNT_BYTES;
  else {
    errh->error("first argument should be \"PACKETS\" or \"BYTES\"");
    return -1;
  }

  // THRESH
  if(!cp_integer(args[1], _thresh)) {
    errh->error("second argument expected THRESH. Not found.");
    return -1;
  }

  _base = new struct _stats;
  if(!_base) {
    errh->error("oops");
    return -1;
  }
  clean(_base);
  _base->base = 0;

  // SD1 VAL1, ..., SDx VALx
  int change;
  String srcdst;
  struct _inp *inp;
  for (int i = 2; i < args.size(); i++) {
    String arg = args[i];
    if(cp_word(arg, srcdst, &arg) &&
       cp_eat_space(arg) &&
       cp_integer(arg, change)) {

      inp = new struct _inp;
      inp->change = change;

      if(srcdst == "SRC")
        inp->srcdst = SRC;
      else if(srcdst == "DST")
        inp->srcdst = DST;
      else {
        errh->error("SDx should be \"SRC\" or \"DST\"");
        return -1;
      }

      _inputs.push_back(inp);
    } else
        errh->error("Expecting \"THRESH, [, SD1 VAR1 [, SD2 VAR2 [, ... [, SDn VARn]]]]\"");
  }

  // Add default if not supplied.
  if(_inputs.size()) {
    set_ninputs(_inputs.size());
    set_noutputs(_inputs.size());
  } else {
    inp = new struct _inp;
    inp->change = 1;
    inp->srcdst = DST;
    _inputs.push_back(inp);
  }

  set_resettime();
  return 0;
#else
  click_chatter("Monitor doesn't know how to handle non-IPv4!");
  return -1;
#endif
}


Monitor *
Monitor::clone() const
{
  return new Monitor;
}

void
Monitor::push(int port, Packet *p)
{
  IPAddress a;

  assert(_inputs[port]->srcdst == SRC || _inputs[port]->srcdst == DST);

  click_ip *ip = (click_ip *) p->data();
  if(_inputs[port]->srcdst == SRC)
    a = IPAddress(ip->ip_src);
  else
    a = p->dst_ip_anno();

  // Measuring # of packets or # of bytes?
  int val = _inputs[port]->change;
  if(_pb == COUNT_BYTES)
    val *= ip->ip_len;

  p->set_siblings_anno(update(a, val));
  output(port).push(p);
}


//
// Dives in tables based on a and raises the appropriate entry by val.
//
// XXX: Make this interrupt driven.
//
int
Monitor::update(IPAddress a, int val)
{
  assert(_base != NULL);
  int ret;
  unsigned int saddr = a.saddr();

#define CONST_MAX_SHIFT ((BYTES-1)*8)
  int MAX_SHIFT = CONST_MAX_SHIFT;      // avoid calculation

  struct _stats *s = _base;
  struct _counter *c = NULL;
  int bitshift;
  for(bitshift = 0; bitshift <= MAX_SHIFT; bitshift += 8) {
    unsigned char byte = (saddr >> bitshift) & 0x000000ff;
    c = &(s->counter[byte]);

    if(c->flags & SPLIT)
      s = c->next_level;
    else
      break;
  }

  c->value += val;
  ret = c->value;

  // Did value get larger than THRESH within one second?
  if(c->value >= _thresh) {
    if((click_jiffies() - c->last_update) < CLICK_HZ) {
      if(bitshift < MAX_SHIFT) {        // can't split last level
        c->flags |= SPLIT;
        struct _stats *tmp = new struct _stats;
        clean(tmp);
        tmp->base = c->value;
        c->next_level = tmp;
      }
    } else {
      c->value = 0;
      ret = 0;
      c->last_update = click_jiffies();
    }
  }

  return ret;
}


void
Monitor::clean(_stats *s, int value = 0, bool recurse = false)
{
  int jiffs = click_jiffies();

  for(int i = 0; i < 256; i++) {
    if(recurse && (s->counter[i].flags & SPLIT)) {
      clean(s->counter[i].next_level, value, true);
      delete s->counter[i].next_level;
    }
    s->counter[i].flags = 0;
    s->counter[i].value = value;
    s->counter[i].last_update = jiffs;
  }
}


String
Monitor::print(_stats *s, String ip = "")
{
  String ret = "";
  for(int i = 0; i < 256; i++) {
    String this_ip;
    if(ip)
      this_ip = ip + "." + String(i);
    else
      this_ip = String(i);


    if(s->counter[i].flags & SPLIT) {
      ret += this_ip + "\t*\t" + String(s->counter[i].next_level->base) + "\n";
      ret += print(s->counter[i].next_level, "\t" + this_ip);
    } else if(s->counter[i].value != 0)
      ret += this_ip + "\t" + String(s->counter[i].value) + "\n";
  }
  return ret;
}


inline void
Monitor::set_resettime()
{
  _resettime = click_jiffies();
}


// First line: number of jiffies since last reset.
// Other lines:
// tabs address ['*' number | number]
//
// tabs = 0 - 3 tabs
// address = string of form v[.w[.x[.y]]] denoting a (partial) IP address
// number = integer denoting the value associated with this IP address group
String
Monitor::look_read_handler(Element *e, void *)
{
  Monitor *me = (Monitor*) e;

  String ret = String(click_jiffies() - me->_resettime) + "\n";
  return ret + me->print(me->_base);
}


String
Monitor::thresh_read_handler(Element *e, void *)
{
  Monitor *me = (Monitor *) e;
  return String(me->_thresh) + "\n";
}

String
Monitor::what_read_handler(Element *e, void *)
{
  Monitor *me = (Monitor *) e;
  return (me->_pb == COUNT_PACKETS ? "PACKETS\n" : "BYTES\n");
}


int
Monitor::thresh_write_handler(const String &conf, Element *e, void *, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  Monitor* me = (Monitor *) e;

  if(args.size() != 1) {
    errh->error("expecting 1 integer");
    return -1;
  }
  int thresh;
  if(!cp_integer(args[0], thresh)) {
    errh->error("not an integer");
    return -1;
  }
  me->_thresh = thresh;
  return 0;
}



int
Monitor::reset_write_handler(const String &conf, Element *e, void *, ErrorHandler *errh)
{
  Vector<String> args;
  cp_argvec(conf, args);
  Monitor* me = (Monitor *) e;

  if(args.size() != 1) {
    errh->error("expecting 1 integer");
    return -1;
  }
  int init;
  if(!cp_integer(args[0], init)) {
    errh->error("not an integer");
    return -1;
  }
  me->clean(me->_base, init, true);
  me->set_resettime();
  return 0;
}


void
Monitor::add_handlers()
{
  add_read_handler("thresh", thresh_read_handler, 0);
  add_write_handler("thresh", thresh_write_handler, 0);

  add_read_handler("what", what_read_handler, 0);
  add_read_handler("look", look_read_handler, 0);

  add_write_handler("reset", reset_write_handler, 0);
}

EXPORT_ELEMENT(Monitor)
