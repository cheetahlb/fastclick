/*
 * fromdump.{cc,hh} -- element reads packets from tcpdump file
 * John Jannotti, Eddie Kohler
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
#include "fromdump.hh"
#include "confparse.hh"
#include "router.hh"
#include "elements/standard/scheduleinfo.hh"
#include "error.hh"
#include "glue.hh"

FromDump::FromDump()
  : _pcap(0), _pending_packet(0)
{
  add_output();
}

FromDump::~FromDump()
{
  assert(!_pcap && !_pending_packet);
}

FromDump*
FromDump::clone() const
{
  return new FromDump;
}

int
FromDump::configure(const String &conf, ErrorHandler *errh)
{
  _timing = true;
  return cp_va_parse(conf, this, errh,
		     cpString, "dump file name", &_filename,
		     cpOptional,
		     cpBool, "use original packet timing", &_timing,
		     0);
}

int
FromDump::initialize(ErrorHandler *errh)
{
  if (!_filename)
    return errh->error("filename not set");

  timerclear(&_offset);
  
#ifdef HAVE_PCAP
  char ebuf[PCAP_ERRBUF_SIZE];
  _pcap = pcap_open_offline((char*)(const char*)_filename, ebuf);
  if (!_pcap)
    return errh->error("pcap error: %s\n", ebuf);
  pcap_dispatch(_pcap, 1, &pcap_packet_hook, (u_char*)this);
  if (!_pending_packet)
    errh->warning("dump contains no packets");
#else
  errh->warning("can't read packets: not compiled with pcap support");
#endif

  if (_pending_packet)
    ScheduleInfo::join_scheduler(this, errh);
  return 0;
}

void
FromDump::uninitialize()
{
  if (_pcap) {
    pcap_close(_pcap);
    _pcap = 0;
  }
  if (_pending_packet) {
    _pending_packet->kill();
    _pending_packet = 0;
  }
}

#ifdef HAVE_PCAP
void
FromDump::pcap_packet_hook(u_char* clientdata,
			   const struct pcap_pkthdr* pkthdr,
			   const u_char* data)
{
  FromDump *e = (FromDump *)clientdata;

  // If first time called, set up offset for syncing up real time with
  // the time of the dump.
  if (!timerisset(&e->_offset)) {
    click_gettimeofday(&e->_offset);
    timersub(&e->_offset, &pkthdr->ts, &e->_offset);
  }

  e->_pending_packet = Packet::make(data, pkthdr->caplen);
  
  // Fill pkthdr and bump the timestamp by offset
  memcpy(&e->_pending_pkthdr, pkthdr, sizeof(pcap_pkthdr));
  timeradd(&e->_pending_pkthdr.ts, &e->_offset, &e->_pending_pkthdr.ts);
}
#endif

void
FromDump::run_scheduled()
{
#ifdef HAVE_PCAP
  timeval now;
  click_gettimeofday(&now);
  if (!_timing || timercmp(&now, &_pending_pkthdr.ts, >)) {
    output(0).push(_pending_packet);
    _pending_packet = 0;
    pcap_dispatch(_pcap, 1, &pcap_packet_hook, (u_char *)this);
  }
  if (_pending_packet)
    reschedule();
  else
    router()->please_stop_driver();
#endif
}

EXPORT_ELEMENT(FromDump)
