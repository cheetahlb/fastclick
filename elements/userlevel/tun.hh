#ifndef TUN_HH
#define TUN_HH

/*
 * =c
 * Tun(address, netmask [, default-gw])
 * =s
 * user-level interface to /dev/tun or ethertap
 * V<devices>
 * =d
 * Reads and writes packets from/to a /dev/tun* or /dev/tap* device.
 * This allows a user-level Click to hand packets to the
 * ordinary kernel IP packet processing code.
 * A Tun will also transfer packets from the kernel IP
 * code if the kernel routing table has entries pointing
 * at the tun device.
 *
 * Tun produces and expects Ethernet packets, much like ToLinux.
 * The only point is to look at the type field, so that e.g.
 * IPv6 packets can be sent into a Tun.
 * 
 * Tun allocates a /dev/tun* or tap* device (this might fail) and
 * runs ifconfig to set the interface's local (i.e. kernel) address to
 * address and the netmask to netmask.  If a default-gw IP address
 * (which must be on the same network as the tun) is specified (that
 * is not 0.0.0.0), Tun tries to set up a default route through that
 * host.
 *
 * When cleaning up, Tun attempts to bring down the device via
 * ifconfig.
 *
 * =a
 * ToLinux */

#include "element.hh"
#include "ipaddress.hh"

class Tun : public Element {
 public:
  Tun();
  ~Tun();
  
  const char *class_name() const	{ return "Tun"; }
  const char *processing() const	{ return PUSH; }
  int configure(const Vector<String> &, ErrorHandler *);
  int initialize(ErrorHandler *);
  void uninitialize();
  Tun *clone() const;

  void selected(int fd);

  void push(int port, Packet *);
  void run_scheduled();

 private:
  String _dev_name;
  IPAddress _near;
  IPAddress _mask;
  IPAddress _gw;
  int _fd;

  int alloc_tun(struct in_addr near, struct in_addr far, ErrorHandler *errh);
  void dealloc_tun();
};

#endif
