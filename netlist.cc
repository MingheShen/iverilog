/*
 * Copyright (c) 1998-1999 Stephen Williams (steve@icarus.com)
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */
#if !defined(WINNT)
#ident "$Id: netlist.cc,v 1.101 1999/12/17 03:38:46 steve Exp $"
#endif

# include  <cassert>
# include  <typeinfo>
# include  "netlist.h"
# include  "netmisc.h"

ostream& operator<< (ostream&o, NetNet::Type t)
{
      switch (t) {
	  case NetNet::IMPLICIT:
	    o << "wire /*implicit*/";
	    break;
	  case NetNet::IMPLICIT_REG:
	    o << "reg /*implicit*/";
	    break;
	  case NetNet::INTEGER:
	    o << "integer";
	    break;
	  case NetNet::REG:
	    o << "reg";
	    break;
	  case NetNet::SUPPLY0:
	    o << "supply0";
	    break;
	  case NetNet::SUPPLY1:
	    o << "supply1";
	    break;
	  case NetNet::TRI:
	    o << "tri";
	    break;
	  case NetNet::TRI0:
	    o << "tri0";
	    break;
	  case NetNet::TRI1:
	    o << "tri1";
	    break;
	  case NetNet::TRIAND:
	    o << "triand";
	    break;
	  case NetNet::TRIOR:
	    o << "trior";
	    break;
	  case NetNet::WAND:
	    o << "wand";
	    break;
	  case NetNet::WOR:
	    o << "wor";
	    break;
	  case NetNet::WIRE:
	    o << "wire";
	    break;
      }
      return o;
}


void connect(NetObj::Link&l, NetObj::Link&r)
{
      assert(&l != &r);
      assert(l.next_->prev_ == &l);
      assert(l.prev_->next_ == &l);
      assert(r.next_->prev_ == &r);
      assert(r.prev_->next_ == &r);

      NetObj::Link* cur = &l;
      do {
	    NetObj::Link*tmp = cur->next_;

	      // If I stumble on r in the nexus, then stop now because
	      // we are already connected.
	    if (tmp == &r) break;

	      // Pull cur out of left list...
	    cur->prev_->next_ = cur->next_;
	    cur->next_->prev_ = cur->prev_;

	      // Put cur in right list
	    cur->next_ = r.next_;
	    cur->prev_ = &r;
	    cur->next_->prev_ = cur;
	    cur->prev_->next_ = cur;

	      // Go to the next item in the left list.
	    cur = tmp;
      } while (cur != &l);

      assert(l.next_->prev_ == &l);
      assert(l.prev_->next_ == &l);
      assert(r.next_->prev_ == &r);
      assert(r.prev_->next_ == &r);
}

NetObj::Link::Link()
: dir_(PASSIVE), inst_(0), next_(this), prev_(this)
{
}

NetObj::Link::~Link()
{
      unlink();
}

void NetObj::Link::unlink()
{
      next_->prev_ = prev_;
      prev_->next_ = next_;
      next_ = prev_ = this;
}

bool NetObj::Link::is_linked() const
{
      return next_ != this;
}

bool NetObj::Link::is_linked(const NetObj&that) const
{
      for (const Link*idx = next_ ; this != idx ;  idx = idx->next_)
	    if (idx->node_ == &that)
		  return true;

      return false;
}

bool NetObj::Link::is_linked(const NetObj::Link&that) const
{
      for (const Link*idx = next_ ; this != idx ;  idx = idx->next_)
	    if (idx == &that)
		  return true;

      return false;
}

void NetObj::Link::next_link(NetObj*&net, unsigned&pin)
{
      assert(next_->prev_ == this);
      assert(prev_->next_ == this);
      net = next_->node_;
      pin = next_->pin_;
}

void NetObj::Link::next_link(const NetObj*&net, unsigned&pin) const
{
      assert(next_->prev_ == this);
      assert(prev_->next_ == this);
      net = next_->node_;
      pin = next_->pin_;
}

NetObj::Link* NetObj::Link::next_link()
{
      assert(next_->prev_ == this);
      assert(prev_->next_ == this);
      return next_;
}

const NetObj::Link* NetObj::Link::next_link() const
{
      assert(next_->prev_ == this);
      assert(prev_->next_ == this);
      return next_;
}

const NetObj*NetObj::Link::get_obj() const
{
      return node_;
}

NetObj*NetObj::Link::get_obj()
{
      return node_;
}

unsigned NetObj::Link::get_pin() const
{
      return pin_;
}

void NetObj::Link::set_name(const string&n, unsigned i)
{
      name_ = n;
      inst_ = i;
}

const string& NetObj::Link::get_name() const
{
      return name_;
}

unsigned NetObj::Link::get_inst() const
{
      return inst_;
}

bool connected(const NetObj&l, const NetObj&r)
{
      for (unsigned idx = 0 ;  idx < l.pin_count() ;  idx += 1)
	    if (! l.pin(idx).is_linked(r))
		  return false;

      return true;
}

unsigned count_inputs(const NetObj::Link&pin)
{
      unsigned count = (pin.get_dir() == NetObj::Link::INPUT)? 1 : 0;
      const NetObj*cur;
      unsigned cpin;
      pin.next_link(cur, cpin);
      while (cur->pin(cpin) != pin) {
	    if (cur->pin(cpin).get_dir() == NetObj::Link::INPUT)
		  count += 1;
	    cur->pin(cpin).next_link(cur, cpin);
      }

      return count;
}

unsigned count_outputs(const NetObj::Link&pin)
{
      unsigned count = (pin.get_dir() == NetObj::Link::OUTPUT)? 1 : 0;
      const NetObj*cur;
      unsigned cpin;
      pin.next_link(cur, cpin);
      while (cur->pin(cpin) != pin) {
	    if (cur->pin(cpin).get_dir() == NetObj::Link::OUTPUT)
		  count += 1;
	    cur->pin(cpin).next_link(cur, cpin);
      }

      return count;
}

unsigned count_signals(const NetObj::Link&pin)
{
      unsigned count = 0;
      if (dynamic_cast<const NetNet*>(pin.get_obj()))
	    count += 1;

      const NetObj*cur;
      unsigned cpin;
      pin.next_link(cur, cpin);
      while (cur->pin(cpin) != pin) {
	    if (dynamic_cast<const NetNet*>(cur))
		  count += 1;

	    cur->pin(cpin).next_link(cur, cpin);
      }

      return count;
}

const NetNet* find_link_signal(const NetObj*net, unsigned pin, unsigned&bidx)
{
      const NetObj*cur;
      unsigned cpin;
      net->pin(pin).next_link(cur, cpin);

      while (cur != net) {
	    const NetNet*sig = dynamic_cast<const NetNet*>(cur);
	    if (sig) {
		  bidx = cpin;
		  return sig;
	    }
	    cur->pin(cpin).next_link(cur, cpin);
      }

      return 0;
}

NetObj::Link* find_next_output(NetObj::Link*lnk)
{
      for (NetObj::Link*cur = lnk->next_link()
		 ;  cur != lnk ;  cur = cur->next_link())
	    if (cur->get_dir() == NetObj::Link::OUTPUT)
		  return cur;

      return 0;
}

NetObj::NetObj(const string&n, unsigned np)
: name_(n), npins_(np), delay1_(0), delay2_(0), delay3_(0), mark_(false)
{
      pins_ = new Link[npins_];
      for (unsigned idx = 0 ;  idx < npins_ ;  idx += 1) {
	    pins_[idx].node_ = this;
	    pins_[idx].pin_  = idx;
      }
}

NetObj::~NetObj()
{
      delete[]pins_;
}

void NetObj::set_attributes(const map<string,string>&attr)
{
      assert(attributes_.size() == 0);
      attributes_ = attr;
}

string NetObj::attribute(const string&key) const
{
      map<string,string>::const_iterator idx = attributes_.find(key);
      if (idx == attributes_.end())
	    return "";

      return (*idx).second;
}

void NetObj::attribute(const string&key, const string&value)
{
      attributes_[key] = value;
}

bool NetObj::has_compat_attributes(const NetObj&that) const
{
      map<string,string>::const_iterator idx;
      for (idx = that.attributes_.begin()
		 ; idx != that.attributes_.end() ;  idx ++) {
	   map<string,string>::const_iterator cur;
	   cur = attributes_.find((*idx).first);

	   if (cur == attributes_.end())
		 return false;
	   if ((*cur).second != (*idx).second)
		 return false;
      }

      return true;
}

NetObj::Link& NetObj::pin(unsigned idx)
{
      assert(idx < npins_);
      return pins_[idx];
}

const NetObj::Link& NetObj::pin(unsigned idx) const
{
      assert(idx < npins_);
      return pins_[idx];
}

NetNode::~NetNode()
{
      if (design_)
	    design_->del_node(this);
}

NetNet::NetNet(NetScope*s, const string&n, Type t, unsigned npins)
: NetObj(n, npins), sig_next_(0), sig_prev_(0), design_(0), scope_(s),
    type_(t), port_type_(NOT_A_PORT), msb_(npins-1), lsb_(0),
    local_flag_(false), eref_count_(0)
{
      ivalue_ = new verinum::V[npins];
      for (unsigned idx = 0 ;  idx < npins ;  idx += 1) {
	    pin(idx).set_name("P", idx);
	    ivalue_[idx] = verinum::Vz;
      }
}

NetNet::NetNet(NetScope*s, const string&n, Type t, long ms, long ls)
: NetObj(n, ((ms>ls)?ms-ls:ls-ms) + 1), sig_next_(0),
    sig_prev_(0), design_(0), scope_(s), type_(t),
    port_type_(NOT_A_PORT), msb_(ms), lsb_(ls), local_flag_(false),
    eref_count_(0)
{
      ivalue_ = new verinum::V[pin_count()];
      for (unsigned idx = 0 ;  idx < pin_count() ;  idx += 1) {
	    pin(idx).set_name("P", idx);
	    ivalue_[idx] = verinum::Vz;
      }
}

NetNet::~NetNet()
{
      assert(eref_count_ == 0);
      if (design_)
	    design_->del_signal(this);
}

NetScope* NetNet::scope()
{
      return scope_;
}

const NetScope* NetNet::scope() const
{
      return scope_;
}

unsigned NetNet::sb_to_idx(long sb) const
{
      if (msb_ >= lsb_)
	    return sb - lsb_;
      else
	    return lsb_ - sb;
}

void NetNet::incr_eref()
{
      eref_count_ += 1;
}

void NetNet::decr_eref()
{
      assert(eref_count_ > 0);
      eref_count_ -= 1;
}

unsigned NetNet::get_eref() const
{
      return eref_count_;
}

NetTmp::NetTmp(const string&name, unsigned npins)
: NetNet(0, name, IMPLICIT, npins)
{
      local_flag(true);
}

NetProc::NetProc()
: next_(0)
{
}

NetProc::~NetProc()
{
}

NetProcTop::NetProcTop(Type t, NetProc*st)
: type_(t), statement_(st)
{
}

NetProcTop::~NetProcTop()
{
      delete statement_;
}

NetProc* NetProcTop::statement()
{
      return statement_;
}

const NetProc* NetProcTop::statement() const
{
      return statement_;
}

/*
 * The NetFF class represents an LPM_FF device. The pinout is assigned
 * like so:
 *    0  -- Clock
 *    1  -- Enable
 *    2  -- Aload
 *    3  -- Aset
 *    4  -- Aclr
 *    5  -- Sload
 *    6  -- Sset
 *    7  -- Sclr
 *
 *    8  -- Data[0]
 *    9  -- Q[0]
 *     ...
 */

NetFF::NetFF(const string&n, unsigned wid)
: NetNode(n, 8 + 2*wid)
{
      pin_Clock().set_dir(Link::INPUT);
      pin_Clock().set_name("Clock", 0);
      pin_Enable().set_dir(Link::INPUT);
      pin_Enable().set_name("Enable", 0);
      pin_Aload().set_dir(Link::INPUT);
      pin_Aload().set_name("Aload", 0);
      pin_Aset().set_dir(Link::INPUT);
      pin_Aset().set_name("Aset", 0);
      pin_Aclr().set_dir(Link::INPUT);
      pin_Aclr().set_name("Aclr", 0);
      pin_Sload().set_dir(Link::INPUT);
      pin_Sload().set_name("Sload", 0);
      pin_Sset().set_dir(Link::INPUT);
      pin_Sset().set_name("Sset", 0);
      pin_Sclr().set_dir(Link::INPUT);
      pin_Sclr().set_name("Sclr", 0);
      for (unsigned idx = 0 ;  idx < wid ;  idx += 1) {
	    pin_Data(idx).set_dir(Link::INPUT);
	    pin_Data(idx).set_name("Data", idx);
	    pin_Q(idx).set_dir(Link::OUTPUT);
	    pin_Q(idx).set_name("Q", idx);
      }
}

NetFF::~NetFF()
{
}

unsigned NetFF::width() const
{
      return (pin_count() - 8) / 2;
}

NetObj::Link& NetFF::pin_Clock()
{
      return pin(0);
}

const NetObj::Link& NetFF::pin_Clock() const
{
      return pin(0);
}

NetObj::Link& NetFF::pin_Enable()
{
      return pin(1);
}

const NetObj::Link& NetFF::pin_Enable() const
{
      return pin(1);
}

NetObj::Link& NetFF::pin_Aload()
{
      return pin(2);
}

NetObj::Link& NetFF::pin_Aset()
{
      return pin(3);
}

NetObj::Link& NetFF::pin_Aclr()
{
      return pin(4);
}

NetObj::Link& NetFF::pin_Sload()
{
      return pin(5);
}

NetObj::Link& NetFF::pin_Sset()
{
      return pin(6);
}

NetObj::Link& NetFF::pin_Sclr()
{
      return pin(7);
}

NetObj::Link& NetFF::pin_Data(unsigned w)
{
      unsigned pn = 8 + 2*w;
      assert(pn < pin_count());
      return pin(pn);
}

const NetObj::Link& NetFF::pin_Data(unsigned w) const
{
      unsigned pn = 8 + 2*w;
      assert(pn < pin_count());
      return pin(pn);
}

NetObj::Link& NetFF::pin_Q(unsigned w)
{
      unsigned pn = 9 + w*2;
      assert(pn < pin_count());
      return pin(pn);
}

const NetObj::Link& NetFF::pin_Q(unsigned w) const
{
      unsigned pn = 9 + w*2;
      assert(pn < pin_count());
      return pin(pn);
}


/*
 * The NetAddSub class represents an LPM_ADD_SUB device. The pinout is
 * assigned like so:
 *    0  -- Add_Sub
 *    1  -- Aclr
 *    2  -- Clock
 *    3  -- Cin
 *    4  -- Cout
 *    5  -- Overflow
 *    6  -- DataA[0]
 *    7  -- DataB[0]
 *    8  -- Result[0]
 */
NetAddSub::NetAddSub(const string&n, unsigned w)
: NetNode(n, w*3+6)
{
      pin(0).set_dir(NetObj::Link::INPUT); pin(0).set_name("Add_Sub", 0);
      pin(1).set_dir(NetObj::Link::INPUT); pin(1).set_name("Aclr", 0);
      pin(2).set_dir(NetObj::Link::INPUT); pin(2).set_name("Clock", 0);
      pin(3).set_dir(NetObj::Link::INPUT); pin(3).set_name("Cin", 0);
      pin(4).set_dir(NetObj::Link::OUTPUT); pin(4).set_name("Cout", 0);
      pin(5).set_dir(NetObj::Link::OUTPUT); pin(5).set_name("Overflow", 0);
      for (unsigned idx = 0 ;  idx < w ;  idx += 1) {
	    pin_DataA(idx).set_dir(NetObj::Link::INPUT);
	    pin_DataB(idx).set_dir(NetObj::Link::INPUT);
	    pin_Result(idx).set_dir(NetObj::Link::OUTPUT);
	    pin_DataA(idx).set_name("DataA", idx);
	    pin_DataB(idx).set_name("DataB", idx);
	    pin_Result(idx).set_name("Result", idx);
      }
}

NetAddSub::~NetAddSub()
{
}

unsigned NetAddSub::width()const
{
      return (pin_count() - 6) / 3;
}

NetObj::Link& NetAddSub::pin_Cout()
{
      return pin(4);
}

const NetObj::Link& NetAddSub::pin_Cout() const
{
      return pin(4);
}

NetObj::Link& NetAddSub::pin_DataA(unsigned idx)
{
      idx = 6 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

const NetObj::Link& NetAddSub::pin_DataA(unsigned idx) const
{
      idx = 6 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

NetObj::Link& NetAddSub::pin_DataB(unsigned idx)
{
      idx = 7 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

const NetObj::Link& NetAddSub::pin_DataB(unsigned idx) const
{
      idx = 7 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

NetObj::Link& NetAddSub::pin_Result(unsigned idx)
{
      idx = 8 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

const NetObj::Link& NetAddSub::pin_Result(unsigned idx) const
{
      idx = 8 + idx*3;
      assert(idx < pin_count());
      return pin(idx);
}

/*
 * The pinout for the NetCLShift is:
 *    0   -- Direction
 *    1   -- Underflow
 *    2   -- Overflow
 *    3   -- Data(0)
 *    3+W -- Result(0)
 *    3+2W -- Distance(0)
 */
NetCLShift::NetCLShift(const string&n, unsigned width, unsigned width_dist)
: NetNode(n, 3+2*width+width_dist), width_(width), width_dist_(width_dist)
{
      pin(0).set_dir(NetObj::Link::INPUT); pin(0).set_name("Direction", 0);
      pin(1).set_dir(NetObj::Link::OUTPUT); pin(1).set_name("Underflow", 0);
      pin(2).set_dir(NetObj::Link::OUTPUT); pin(2).set_name("Overflow", 0);

      for (unsigned idx = 0 ;  idx < width_ ;  idx += 1) {
	    pin(3+idx).set_dir(NetObj::Link::INPUT);
	    pin(3+idx).set_name("Data", idx);

	    pin(3+width_+idx).set_dir(NetObj::Link::OUTPUT);
	    pin(3+width_+idx).set_name("Result", idx);
      }

      for (unsigned idx = 0 ;  idx < width_dist_ ;  idx += 1) {
	    pin(3+2*width_+idx).set_dir(NetObj::Link::INPUT);
	    pin(3+2*width_+idx).set_name("Distance", idx);
      }
}

NetCLShift::~NetCLShift()
{
}

unsigned NetCLShift::width() const
{
      return width_;
}

unsigned NetCLShift::width_dist() const
{
      return width_dist_;
}

NetObj::Link& NetCLShift::pin_Direction()
{
      return pin(0);
}

const NetObj::Link& NetCLShift::pin_Direction() const
{
      return pin(0);
}

NetObj::Link& NetCLShift::pin_Underflow()
{
      return pin(1);
}

const NetObj::Link& NetCLShift::pin_Underflow() const
{
      return pin(1);
}

NetObj::Link& NetCLShift::pin_Overflow()
{
      return pin(2);
}

const NetObj::Link& NetCLShift::pin_Overflow() const
{
      return pin(2);
}

NetObj::Link& NetCLShift::pin_Data(unsigned idx)
{
      assert(idx < width_);
      return pin(3+idx);
}

const NetObj::Link& NetCLShift::pin_Data(unsigned idx) const
{
      assert(idx < width_);
      return pin(3+idx);
}

NetObj::Link& NetCLShift::pin_Result(unsigned idx)
{
      assert(idx < width_);
      return pin(3+width_+idx);
}

const NetObj::Link& NetCLShift::pin_Result(unsigned idx) const
{
      assert(idx < width_);
      return pin(3+width_+idx);
}

NetObj::Link& NetCLShift::pin_Distance(unsigned idx)
{
      assert(idx < width_dist_);
      return pin(3+2*width_+idx);
}

const NetObj::Link& NetCLShift::pin_Distance(unsigned idx) const
{
      assert(idx < width_dist_);
      return pin(3+2*width_+idx);
}

NetCompare::NetCompare(const string&n, unsigned wi)
: NetNode(n, 8+2*wi), width_(wi)
{
      pin(0).set_dir(NetObj::Link::INPUT); pin(0).set_name("Aclr");
      pin(1).set_dir(NetObj::Link::INPUT); pin(1).set_name("Clock");
      pin(2).set_dir(NetObj::Link::OUTPUT); pin(2).set_name("AGB");
      pin(3).set_dir(NetObj::Link::OUTPUT); pin(3).set_name("AGEB");
      pin(4).set_dir(NetObj::Link::OUTPUT); pin(4).set_name("AEB");
      pin(5).set_dir(NetObj::Link::OUTPUT); pin(5).set_name("ANEB");
      pin(6).set_dir(NetObj::Link::OUTPUT); pin(6).set_name("ALB");
      pin(7).set_dir(NetObj::Link::OUTPUT); pin(7).set_name("ALEB");
      for (unsigned idx = 0 ;  idx < width_ ;  idx += 1) {
	    pin(8+idx).set_dir(NetObj::Link::INPUT);
	    pin(8+idx).set_name("DataA", idx);
	    pin(8+width_+idx).set_dir(NetObj::Link::INPUT);
	    pin(8+width_+idx).set_name("DataB", idx);
      }
}

NetCompare::~NetCompare()
{
}

unsigned NetCompare::width() const
{
      return width_;
}

NetObj::Link& NetCompare::pin_Aclr()
{
      return pin(0);
}

const NetObj::Link& NetCompare::pin_Aclr() const
{
      return pin(0);
}

NetObj::Link& NetCompare::pin_Clock()
{
      return pin(1);
}

const NetObj::Link& NetCompare::pin_Clock() const
{
      return pin(1);
}

NetObj::Link& NetCompare::pin_AGB()
{
      return pin(2);
}

const NetObj::Link& NetCompare::pin_AGB() const
{
      return pin(2);
}

NetObj::Link& NetCompare::pin_AGEB()
{
      return pin(3);
}

const NetObj::Link& NetCompare::pin_AGEB() const
{
      return pin(3);
}

NetObj::Link& NetCompare::pin_AEB()
{
      return pin(4);
}

const NetObj::Link& NetCompare::pin_AEB() const
{
      return pin(4);
}

NetObj::Link& NetCompare::pin_ANEB()
{
      return pin(5);
}

const NetObj::Link& NetCompare::pin_ANEB() const
{
      return pin(5);
}

NetObj::Link& NetCompare::pin_ALB()
{
      return pin(6);
}

const NetObj::Link& NetCompare::pin_ALB() const
{
      return pin(6);
}

NetObj::Link& NetCompare::pin_ALEB()
{
      return pin(7);
}

const NetObj::Link& NetCompare::pin_ALEB() const
{
      return pin(7);
}

NetObj::Link& NetCompare::pin_DataA(unsigned idx)
{
      return pin(8+idx);
}

const NetObj::Link& NetCompare::pin_DataA(unsigned idx) const
{
      return pin(8+idx);
}

NetObj::Link& NetCompare::pin_DataB(unsigned idx)
{
      return pin(8+width_+idx);
}

const NetObj::Link& NetCompare::pin_DataB(unsigned idx) const
{
      return pin(8+width_+idx);
}

/*
 * The NetMux class represents an LPM_MUX device. The pinout is assigned
 * like so:
 *    0   -- Aclr (optional)
 *    1   -- Clock (optional)
 *    2   -- Result[0]
 *    2+N -- Result[N]
 */

NetMux::NetMux(const string&n, unsigned wi, unsigned si, unsigned sw)
: NetNode(n, 2+wi+sw+wi*si), width_(wi), size_(si), swidth_(sw)
{
      pin(0).set_dir(NetObj::Link::INPUT); pin(0).set_name("Aclr",  0);
      pin(1).set_dir(NetObj::Link::INPUT); pin(1).set_name("Clock", 0);

      for (unsigned idx = 0 ;  idx < width_ ;  idx += 1) {
	    pin_Result(idx).set_dir(NetObj::Link::OUTPUT);
	    pin_Result(idx).set_name("Result", idx);

	    for (unsigned jdx = 0 ;  jdx < size_ ;  jdx += 1) {
		  pin_Data(idx,jdx).set_dir(Link::INPUT);
		  pin_Data(idx,jdx).set_name("Data", jdx*width_+idx);
	    }
      }

      for (unsigned idx = 0 ;  idx < swidth_ ;  idx += 1) {
	    pin_Sel(idx).set_dir(Link::INPUT);
	    pin_Sel(idx).set_name("Sel", idx);
      }
}

NetMux::~NetMux()
{
}

unsigned NetMux::width()const
{
      return width_;
}

unsigned NetMux::size() const
{
      return size_;
}

unsigned NetMux::sel_width() const
{
      return swidth_;
}

NetObj::Link& NetMux::pin_Aclr()
{
      return pin(0);
}

const NetObj::Link& NetMux::pin_Aclr() const
{
      return pin(0);
}

NetObj::Link& NetMux::pin_Clock()
{
      return pin(1);
}

const NetObj::Link& NetMux::pin_Clock() const
{
      return pin(1);
}

NetObj::Link& NetMux::pin_Result(unsigned w)
{
      assert(w < width_);
      return pin(2+w);
}

const NetObj::Link& NetMux::pin_Result(unsigned w) const
{
      assert(w < width_);
      return pin(2+w);
}

NetObj::Link& NetMux::pin_Sel(unsigned w)
{
      assert(w < swidth_);
      return pin(2+width_+w);
}

const NetObj::Link& NetMux::pin_Sel(unsigned w) const
{
      assert(w < swidth_);
      return pin(2+width_+w);
}

NetObj::Link& NetMux::pin_Data(unsigned w, unsigned s)
{
      assert(w < width_);
      assert(s < size_);
      return pin(2+width_+swidth_+s*width_+w);
}

const NetObj::Link& NetMux::pin_Data(unsigned w, unsigned s) const
{
      assert(w < width_);
      assert(s < size_);
      return pin(2+width_+swidth_+s*width_+w);
}


NetRamDq::NetRamDq(const string&n, NetMemory*mem, unsigned awid)
: NetNode(n, 3+2*mem->width()+awid), mem_(mem), awidth_(awid)
{
      pin(0).set_dir(NetObj::Link::INPUT); pin(0).set_name("InClock", 0);
      pin(1).set_dir(NetObj::Link::INPUT); pin(1).set_name("OutClock", 0);
      pin(2).set_dir(NetObj::Link::INPUT); pin(2).set_name("WE", 0);

      for (unsigned idx = 0 ;  idx < awidth_ ;  idx += 1) {
	    pin(3+idx).set_dir(NetObj::Link::INPUT);
	    pin(3+idx).set_name("Address", idx);
      }

      for (unsigned idx = 0 ;  idx < width() ;  idx += 1) {
	    pin(3+awidth_+idx).set_dir(NetObj::Link::INPUT);
	    pin(3+awidth_+idx).set_name("Data", idx);
      }

      for (unsigned idx = 0 ;  idx < width() ;  idx += 1) {
	    pin(3+awidth_+width()+idx).set_dir(NetObj::Link::OUTPUT);
	    pin(3+awidth_+width()+idx).set_name("Q", idx);
      }

      next_ = mem_->ram_list_;
      mem_->ram_list_ = this;
}

NetRamDq::~NetRamDq()
{
      if (mem_->ram_list_ == this) {
	    mem_->ram_list_ = next_;

      } else {
	    NetRamDq*cur = mem_->ram_list_;
	    while (cur->next_ != this) {
		  assert(cur->next_);
		  cur = cur->next_;
	    }
	    assert(cur->next_ == this);
	    cur->next_ = next_;
      }
}

unsigned NetRamDq::width() const
{
      return mem_->width();
}

unsigned NetRamDq::awidth() const
{
      return awidth_;
}

unsigned NetRamDq::size() const
{
      return mem_->count();
}

const NetMemory* NetRamDq::mem() const
{
      return mem_;
}

unsigned NetRamDq::count_partners() const
{
      unsigned count = 0;
      for (NetRamDq*cur = mem_->ram_list_ ;  cur ;  cur = cur->next_)
	    count += 1;

      return count;
}

void NetRamDq::absorb_partners()
{
      NetRamDq*cur, *tmp;
      for (cur = mem_->ram_list_, tmp = 0
		 ;  cur||tmp ;  cur = cur? cur->next_ : tmp) {
	    tmp = 0;
	    if (cur == this) continue;

	    bool ok_flag = true;
	    for (unsigned idx = 0 ;  idx < awidth() ;  idx += 1)
		  ok_flag &= pin_Address(idx).is_linked(cur->pin_Address(idx));

	    if (!ok_flag) continue;

	    if (pin_InClock().is_linked()
		&& cur->pin_InClock().is_linked()
		&& ! pin_InClock().is_linked(cur->pin_InClock()))
		  continue;

	    if (pin_OutClock().is_linked()
		&& cur->pin_OutClock().is_linked()
		&& ! pin_OutClock().is_linked(cur->pin_OutClock()))
		  continue;

	    if (pin_WE().is_linked()
		&& cur->pin_WE().is_linked()
		&& ! pin_WE().is_linked(cur->pin_WE()))
		  continue;

	    for (unsigned idx = 0 ;  idx < width() ;  idx += 1) {
		  if (!pin_Data(idx).is_linked()) continue;
		  if (! cur->pin_Data(idx).is_linked()) continue;

		  ok_flag &= pin_Data(idx).is_linked(cur->pin_Data(idx));
	    }

	    if (! ok_flag) continue;

	    for (unsigned idx = 0 ;  idx < width() ;  idx += 1) {
		  if (!pin_Q(idx).is_linked()) continue;
		  if (! cur->pin_Q(idx).is_linked()) continue;

		  ok_flag &= pin_Q(idx).is_linked(cur->pin_Q(idx));
	    }

	    if (! ok_flag) continue;

	      // I see no other reason to reject cur, so link up all
	      // my pins and delete it.
	    connect(pin_InClock(), cur->pin_InClock());
	    connect(pin_OutClock(), cur->pin_OutClock());
	    connect(pin_WE(), cur->pin_WE());

	    for (unsigned idx = 0 ;  idx < awidth() ;  idx += 1)
		  connect(pin_Address(idx), cur->pin_Address(idx));

	    for (unsigned idx = 0 ;  idx < width() ;  idx += 1) {
		  connect(pin_Data(idx), cur->pin_Data(idx));
		  connect(pin_Q(idx), cur->pin_Q(idx));
	    }

	    tmp = cur->next_;
	    delete cur;
	    cur = 0;
      }
}

NetObj::Link& NetRamDq::pin_InClock()
{
      return pin(0);
}

const NetObj::Link& NetRamDq::pin_InClock() const
{
      return pin(0);
}

NetObj::Link& NetRamDq::pin_OutClock()
{
      return pin(1);
}

const NetObj::Link& NetRamDq::pin_OutClock() const
{
      return pin(1);
}

NetObj::Link& NetRamDq::pin_WE()
{
      return pin(2);
}

const NetObj::Link& NetRamDq::pin_WE() const
{
      return pin(2);
}

NetObj::Link& NetRamDq::pin_Address(unsigned idx)
{
      assert(idx < awidth_);
      return pin(3+idx);
}

const NetObj::Link& NetRamDq::pin_Address(unsigned idx) const
{
      assert(idx < awidth_);
      return pin(3+idx);
}

NetObj::Link& NetRamDq::pin_Data(unsigned idx)
{
      assert(idx < width());
      return pin(3+awidth_+idx);
}

const NetObj::Link& NetRamDq::pin_Data(unsigned idx) const
{
      assert(idx < width());
      return pin(3+awidth_+idx);
}

NetObj::Link& NetRamDq::pin_Q(unsigned idx)
{
      assert(idx < width());
      return pin(3+awidth_+width()+idx);
}

const NetObj::Link& NetRamDq::pin_Q(unsigned idx) const
{
      assert(idx < width());
      return pin(3+awidth_+width()+idx);
}

/*
 * NetAssign
 */

NetAssign_::NetAssign_(const string&n, unsigned w)
: NetNode(n, w), rval_(0), bmux_(0)
{
      for (unsigned idx = 0 ;  idx < pin_count() ;  idx += 1) {
	    pin(idx).set_dir(NetObj::Link::OUTPUT);
	    pin(idx).set_name("P", idx);
      }

}

NetAssign_::~NetAssign_()
{
      if (rval_) delete rval_;
      if (bmux_) delete bmux_;
}

void NetAssign_::set_rval(NetExpr*r)
{
      assert(rval_ == 0);
      rval_ = r;
}

void NetAssign_::set_bmux(NetExpr*r)
{
      assert(bmux_ == 0);
      bmux_ = r;
}

NetExpr* NetAssign_::rval()
{
      return rval_;
}

const NetExpr* NetAssign_::rval() const
{
      return rval_;
}

const NetExpr* NetAssign_::bmux() const
{
      return bmux_;
}

NetAssign::NetAssign(const string&n, Design*des, unsigned w, NetExpr*rv)
: NetAssign_(n, w)
{
      set_rval(rv);
}

NetAssign::NetAssign(const string&n, Design*des, unsigned w,
		     NetExpr*mu, NetExpr*rv)
: NetAssign_(n, w)
{
      bool flag = rv->set_width(1);
      if (flag == false) {
	    cerr << rv->get_line() << ": Expression bit width" <<
		  " conflicts with l-value bit width." << endl;
	    des->errors += 1;
      }

      set_rval(rv);
      set_bmux(mu);
}

NetAssign::~NetAssign()
{
}

NetAssignNB::NetAssignNB(const string&n, Design*des, unsigned w, NetExpr*rv)
: NetAssign_(n, w)
{
      if (rv->expr_width() < w) {
	    cerr << rv->get_line() << ": Expression bit width (" <<
		  rv->expr_width() << ") conflicts with l-value "
		  "bit width (" << w << ")." << endl;
	    des->errors += 1;
      }

      set_rval(rv);
}

NetAssignNB::NetAssignNB(const string&n, Design*des, unsigned w,
			 NetExpr*mu, NetExpr*rv)
: NetAssign_(n, w)
{
      bool flag = rv->set_width(1);
      if (flag == false) {
	    cerr << rv->get_line() << ": Expression bit width" <<
		  " conflicts with l-value bit width." << endl;
	    des->errors += 1;
      }

      set_rval(rv);
      set_bmux(mu);
}

NetAssignNB::~NetAssignNB()
{
}


NetAssignMem_::NetAssignMem_(NetMemory*m, NetNet*i, NetExpr*r)
: mem_(m), index_(i), rval_(r)
{
      index_->incr_eref();
}

NetAssignMem_::~NetAssignMem_()
{
      index_->decr_eref();
}

NetAssignMem::NetAssignMem(NetMemory*m, NetNet*i, NetExpr*r)
: NetAssignMem_(m, i, r)
{
}

NetAssignMem::~NetAssignMem()
{
}

NetAssignMemNB::NetAssignMemNB(NetMemory*m, NetNet*i, NetExpr*r)
: NetAssignMem_(m, i, r)
{
}

NetAssignMemNB::~NetAssignMemNB()
{
}


NetBlock::~NetBlock()
{
}

void NetBlock::append(NetProc*cur)
{
      if (last_ == 0) {
	    last_ = cur;
	    cur->next_ = cur;
      } else {
	    cur->next_ = last_->next_;
	    last_->next_ = cur;
	    last_ = cur;
      }
}

NetBUFZ::NetBUFZ(const string&n)
: NetNode(n, 2)
{
      pin(0).set_dir(Link::OUTPUT);
      pin(1).set_dir(Link::INPUT);
      pin(0).set_name("O", 0);
      pin(1).set_name("I", 0);
}

NetBUFZ::~NetBUFZ()
{
}


NetCase::NetCase(NetCase::TYPE c, NetExpr*ex, unsigned cnt)
: type_(c), expr_(ex), nitems_(cnt)
{
      assert(expr_);
      items_ = new Item[nitems_];
      for (unsigned idx = 0 ;  idx < nitems_ ;  idx += 1) {
	    items_[idx].statement = 0;
      }
}

NetCase::~NetCase()
{
      delete expr_;
      for (unsigned idx = 0 ;  idx < nitems_ ;  idx += 1) {
	    delete items_[idx].guard;
	    if (items_[idx].statement) delete items_[idx].statement;
      }
      delete[]items_;
}

NetCase::TYPE NetCase::type() const
{
      return type_;
}

void NetCase::set_case(unsigned idx, NetExpr*e, NetProc*p)
{
      assert(idx < nitems_);
      items_[idx].guard = e;
      items_[idx].statement = p;
      if (items_[idx].guard)
	    items_[idx].guard->set_width(expr_->expr_width());
}

NetCaseCmp::NetCaseCmp(const string&n)
: NetNode(n, 3)
{
      pin(0).set_dir(Link::OUTPUT); pin(0).set_name("O",0);
      pin(1).set_dir(Link::INPUT); pin(1).set_name("I",0);
      pin(2).set_dir(Link::INPUT); pin(2).set_name("I",1);
}

NetCaseCmp::~NetCaseCmp()
{
}

NetCondit::NetCondit(NetExpr*ex, NetProc*i, NetProc*e)
: expr_(ex), if_(i), else_(e)
{
}

NetCondit::~NetCondit()
{
      delete expr_;
      if (if_) delete if_;
      if (else_) delete else_;
}

const NetExpr* NetCondit::expr() const
{
      return expr_;
}

NetExpr* NetCondit::expr()
{
      return expr_;
}

NetProc* NetCondit::if_clause()
{
      return if_;
}

NetProc* NetCondit::else_clause()
{
      return else_;
}

NetConst::NetConst(const string&n, verinum::V v)
: NetNode(n, 1)
{
      pin(0).set_dir(Link::OUTPUT);
      pin(0).set_name("O", 0);
      value_ = new verinum::V[1];
      value_[0] = v;
}

NetConst::NetConst(const string&n, const verinum&val)
: NetNode(n, val.len())
{
      value_ = new verinum::V[pin_count()];
      for (unsigned idx = 0 ;  idx < pin_count() ;  idx += 1) {
	    pin(idx).set_dir(Link::OUTPUT);
	    pin(idx).set_name("O", idx);
	    value_[idx] = val.get(idx);
      }
}

NetConst::~NetConst()
{
      delete[]value_;
}

verinum::V NetConst::value(unsigned idx) const
{
      assert(idx < pin_count());
      return value_[idx];
}

NetFuncDef::NetFuncDef(const string&n, const svector<NetNet*>&po)
: name_(n), statement_(0), ports_(po)
{
}

NetFuncDef::~NetFuncDef()
{
}

const string& NetFuncDef::name() const
{
      return name_;
}

void NetFuncDef::set_proc(NetProc*st)
{
      assert(statement_ == 0);
      assert(st != 0);
      statement_ = st;
}

const NetProc* NetFuncDef::proc() const
{
      return statement_;
}

unsigned NetFuncDef::port_count() const
{
      return ports_.count();
}

const NetNet* NetFuncDef::port(unsigned idx) const
{
      assert(idx < ports_.count());
      return ports_[idx];
}

NetNEvent::NetNEvent(const string&ev, unsigned wid, Type e, NetPEvent*pe)
: NetNode(ev, wid), sref<NetPEvent,NetNEvent>(pe), edge_(e)
{
      for (unsigned idx = 0 ;  idx < wid ; idx += 1) {
	    pin(idx).set_name("P", idx);
      }
}

NetNEvent::~NetNEvent()
{
}

NetPEvent::NetPEvent(const string&n, NetProc*st)
: name_(n), statement_(st)
{
}

NetPEvent::~NetPEvent()
{
      svector<NetNEvent*>*back = back_list();
      if (back) {
	    for (unsigned idx = 0 ;  idx < back->count() ;  idx += 1) {
		  NetNEvent*ne = (*back)[idx];
		  delete ne;
	    }
	    delete back;
      }

      delete statement_;
}

NetProc* NetPEvent::statement()
{
      return statement_;
}

const NetProc* NetPEvent::statement() const
{
      return statement_;
}

NetSTask::NetSTask(const string&na, const svector<NetExpr*>&pa)
: name_(na), parms_(pa)
{
      assert(name_[0] == '$');
}

NetSTask::~NetSTask()
{
      for (unsigned idx = 0 ;  idx < parms_.count() ;  idx += 1)
	    delete parms_[idx];

}

const NetExpr* NetSTask::parm(unsigned idx) const
{
      return parms_[idx];
}

NetEUFunc::NetEUFunc(NetFuncDef*def, NetESignal*res, svector<NetExpr*>&p)
: func_(def), result_(res), parms_(p)
{
      expr_width(result_->expr_width());
}

NetEUFunc::~NetEUFunc()
{
      for (unsigned idx = 0 ;  idx < parms_.count() ;  idx += 1)
	    delete parms_[idx];
}

const string& NetEUFunc::name() const
{
      return func_->name();
}

const NetESignal*NetEUFunc::result() const
{
      return result_;
}

unsigned NetEUFunc::parm_count() const
{
      return parms_.count();
}

const NetExpr* NetEUFunc::parm(unsigned idx) const
{
      assert(idx < parms_.count());
      return parms_[idx];
}

const NetFuncDef* NetEUFunc::definition() const
{
      return func_;
}

NetEUFunc* NetEUFunc::dup_expr() const
{
      assert(0);
      return 0;
}

NetUTask::NetUTask(NetTaskDef*def)
: task_(def)
{
}

NetUTask::~NetUTask()
{
}

NetExpr::NetExpr(unsigned w)
: width_(w)
{
}

NetExpr::~NetExpr()
{
}

NetEBAdd::NetEBAdd(char op, NetExpr*l, NetExpr*r)
: NetEBinary(op, l, r)
{
      if (l->expr_width() > r->expr_width())
	    r->set_width(l->expr_width());

      if (r->expr_width() > l->expr_width())
	    l->set_width(r->expr_width());

      if (l->expr_width() < r->expr_width())
	    r->set_width(l->expr_width());

      if (r->expr_width() < l->expr_width())
	    l->set_width(r->expr_width());

      if (r->expr_width() != l->expr_width())
	    expr_width(0);
      else
	    expr_width(l->expr_width());
}

NetEBAdd::~NetEBAdd()
{
}

NetEBAdd* NetEBAdd::dup_expr() const
{
      NetEBAdd*result = new NetEBAdd(op_, left_->dup_expr(),
				     right_->dup_expr());
      return result;
}

NetEBBits::NetEBBits(char op, NetExpr*l, NetExpr*r)
: NetEBinary(op, l, r)
{
	/* First try to naturally adjust the size of the
	   expressions to match. */
      if (l->expr_width() > r->expr_width())
	    r->set_width(l->expr_width());

      if (r->expr_width() > l->expr_width())
	    l->set_width(r->expr_width());

      if (l->expr_width() < r->expr_width())
	    r->set_width(l->expr_width());

      if (r->expr_width() < l->expr_width())
	    l->set_width(r->expr_width());

	/* If the expressions cannot be matched, pad them to fit. */
      if (l->expr_width() > r->expr_width())
	    right_ = pad_to_width(r, l->expr_width());

      if (r->expr_width() > l->expr_width())
	    left_ = pad_to_width(l, r->expr_width());

      assert(left_->expr_width() == right_->expr_width());
      expr_width(left_->expr_width());
}

NetEBBits::~NetEBBits()
{
}

NetEBBits* NetEBBits::dup_expr() const
{
      NetEBBits*result = new NetEBBits(op_, left_->dup_expr(),
				       right_->dup_expr());
      return result;
}

NetEBComp::NetEBComp(char op, NetExpr*l, NetExpr*r)
: NetEBinary(op, l, r)
{
      expr_width(1);
}

NetEBComp::~NetEBComp()
{
}

NetEBComp* NetEBComp::dup_expr() const
{
      NetEBComp*result = new NetEBComp(op_, left_->dup_expr(),
				       right_->dup_expr());
      return result;
}

NetEBinary::NetEBinary(char op, NetExpr*l, NetExpr*r)
: op_(op), left_(l), right_(r)
{
}

NetEBinary::~NetEBinary()
{
      delete left_;
      delete right_;
}

NetEBinary* NetEBinary::dup_expr() const
{
      assert(0);
}

NetEBLogic::NetEBLogic(char op, NetExpr*l, NetExpr*r)
: NetEBinary(op, l, r)
{
      expr_width(1);
}

NetEBLogic::~NetEBLogic()
{
}

NetEBLogic* NetEBLogic::dup_expr() const
{
      NetEBLogic*result = new NetEBLogic(op_, left_->dup_expr(),
					 right_->dup_expr());
      return result;
}

NetEBShift::NetEBShift(char op, NetExpr*l, NetExpr*r)
: NetEBinary(op, l, r)
{
      expr_width(l->expr_width());
}

NetEBShift::~NetEBShift()
{
}

NetEBShift* NetEBShift::dup_expr() const
{
      NetEBShift*result = new NetEBShift(op_, left_->dup_expr(),
					 right_->dup_expr());
      return result;
}

NetEConcat::NetEConcat(unsigned cnt, unsigned r)
: parms_(cnt), repeat_(r)
{
      expr_width(0);
}

NetEConcat::~NetEConcat()
{
      for (unsigned idx = 0 ;  idx < parms_.count() ;  idx += 1)
	    delete parms_[idx];
}

void NetEConcat::set(unsigned idx, NetExpr*e)
{
      assert(idx < parms_.count());
      assert(parms_[idx] == 0);
      parms_[idx] = e;
      expr_width( expr_width() + repeat_*e->expr_width() );
}

NetEConcat* NetEConcat::dup_expr() const
{
      NetEConcat*dup = new NetEConcat(parms_.count(), repeat_);
      for (unsigned idx = 0 ;  idx < parms_.count() ;  idx += 1)
	    if (parms_[idx]) {
		  assert(parms_[idx]->dup_expr());
		  dup->parms_[idx] = parms_[idx]->dup_expr();
	    }


      dup->expr_width(expr_width());
      return dup;
}

NetEConst::NetEConst(const verinum&val)
: NetExpr(val.len()), value_(val)
{
}

NetEConst::~NetEConst()
{
}

NetEConst* NetEConst::dup_expr() const
{
      NetEConst*tmp = new NetEConst(value_);
      tmp->set_line(*this);
      return tmp;
}

NetEIdent* NetEIdent::dup_expr() const
{
      assert(0);
}

NetEMemory::NetEMemory(NetMemory*m, NetExpr*i)
: NetExpr(m->width()), mem_(m), idx_(i)
{
}

NetEMemory::~NetEMemory()
{
}

NetMemory::NetMemory(const string&n, long w, long s, long e)
: name_(n), width_(w), idxh_(s), idxl_(e), ram_list_(0)
{
}

unsigned NetMemory::count() const
{
      if (idxh_ < idxl_)
	    return idxl_ - idxh_ + 1;
      else
	    return idxh_ - idxl_ + 1;
}

unsigned NetMemory::index_to_address(long idx) const
{
      if (idxh_ < idxl_)
	    return idx - idxh_;
      else
	    return idx - idxl_;
}


void NetMemory::set_attributes(const map<string,string>&attr)
{
      assert(attributes_.size() == 0);
      attributes_ = attr;
}

NetEMemory* NetEMemory::dup_expr() const
{
      assert(0);
}

NetEParam::NetEParam()
: des_(0)
{
}

NetEParam::NetEParam(Design*d, const string&p, const string&n)
: des_(d), path_(p), name_(n)
{
}

NetEParam::~NetEParam()
{
}

NetEParam* NetEParam::dup_expr() const
{
      return 0;
}

NetEScope::NetEScope(NetScope*s)
: scope_(s)
{
}

NetEScope::~NetEScope()
{
}

const NetScope* NetEScope::scope() const
{
      return scope_;
}

NetESignal::NetESignal(NetNet*n)
: NetExpr(n->pin_count()), net_(n)
{
      net_->incr_eref();
      set_line(*n);
}

NetESignal::~NetESignal()
{
      net_->decr_eref();
}

const string& NetESignal::name() const
{
      return net_->name();
}

unsigned NetESignal::pin_count() const
{
      return net_->pin_count();
}

NetObj::Link& NetESignal::pin(unsigned idx)
{
      return net_->pin(idx);
}

NetESignal* NetESignal::dup_expr() const
{
      assert(0);
}

NetESubSignal::NetESubSignal(NetESignal*sig, NetExpr*ex)
: sig_(sig), idx_(ex)
{
	// This supports mux type indexing of an expression, so the
	// with is by definition 1 bit.
      expr_width(1);
}

NetESubSignal::~NetESubSignal()
{
      delete idx_;
}

NetESubSignal* NetESubSignal::dup_expr() const
{
      assert(0);
}

NetETernary::NetETernary(NetExpr*c, NetExpr*t, NetExpr*f)
: cond_(c), true_val_(t), false_val_(f)
{
      expr_width(true_val_->expr_width());
}

NetETernary::~NetETernary()
{
      delete cond_;
      delete true_val_;
      delete false_val_;
}

const NetExpr* NetETernary::cond_expr() const
{
      return cond_;
}

const NetExpr* NetETernary::true_expr() const
{
      return true_val_;
}

const NetExpr* NetETernary::false_expr() const
{
      return false_val_;
}

NetETernary* NetETernary::dup_expr() const
{
      assert(0);
}

NetEUnary::NetEUnary(char op, NetExpr*ex)
: NetExpr(ex->expr_width()), op_(op), expr_(ex)
{
      switch (op_) {
	  case '!': // Logical not
	  case '&': // Reduction and
	  case '|': // Reduction or
	  case '^': // Reduction XOR
	  case 'A': // Reduction NAND (~&)
	  case 'N': // Reduction NOR (~|)
	  case 'X': // Reduction NXOR (~^)
	    expr_width(1);
	    break;
      }
}

NetEUnary::~NetEUnary()
{
      delete expr_;
}

NetEUnary* NetEUnary::dup_expr() const
{
      assert(0);
}

NetEUBits::NetEUBits(char op, NetExpr*ex)
: NetEUnary(op, ex)
{
}

NetEUBits::~NetEUBits()
{
}

NetForever::NetForever(NetProc*p)
: statement_(p)
{
}

NetForever::~NetForever()
{
      delete statement_;
}

NetLogic::NetLogic(const string&n, unsigned pins, TYPE t)
: NetNode(n, pins), type_(t)
{
      pin(0).set_dir(Link::OUTPUT);
      pin(0).set_name("O", 0);
      for (unsigned idx = 1 ;  idx < pins ;  idx += 1) {
	    pin(idx).set_dir(Link::INPUT);
	    pin(idx).set_name("I", idx-1);
      }
}

NetRepeat::NetRepeat(NetExpr*e, NetProc*p)
: expr_(e), statement_(p)
{
}

NetRepeat::~NetRepeat()
{
      delete expr_;
      delete statement_;
}

const NetExpr* NetRepeat::expr() const
{
      return expr_;
}

NetScope::NetScope(const string&n)
: type_(NetScope::MODULE), name_(n)
{
}

NetScope::NetScope(const string&p, NetScope::TYPE t)
: type_(t), name_(p)
{
}

NetScope::~NetScope()
{
}

NetScope::TYPE NetScope::type() const
{
      return type_;
}

string NetScope::name() const
{
      return name_;
}

NetTaskDef::NetTaskDef(const string&n, const svector<NetNet*>&po)
: name_(n), proc_(0), ports_(po)
{
}

NetTaskDef::~NetTaskDef()
{
      delete proc_;
}

void NetTaskDef::set_proc(NetProc*p)
{
      assert(proc_ == 0);
      proc_ = p;
}

NetNet* NetTaskDef::port(unsigned idx)
{
      assert(idx < ports_.count());
      return ports_[idx];
}

NetUDP::NetUDP(const string&n, unsigned pins, bool sequ)
: NetNode(n, pins), sequential_(sequ), init_('x')
{
      pin(0).set_dir(Link::OUTPUT);
      for (unsigned idx = 1 ;  idx < pins ;  idx += 1)
	    pin(idx).set_dir(Link::INPUT);

}

NetUDP::state_t_* NetUDP::find_state_(const string&str)
{
      map<string,state_t_*>::iterator cur = fsm_.find(str);
      if (cur != fsm_.end())
	    return (*cur).second;

      state_t_*st = fsm_[str];
      if (st == 0) {
	    st = new state_t_(pin_count());
	    st->out = str[0];
	    fsm_[str] = st;
      }

      return st;
}

/*
 * This method takes the input string, which contains exactly one
 * edge, and connects it to the correct output state. The output state
 * will be generated if needed, and the value compared.
 */
bool NetUDP::set_sequ_(const string&input, char output)
{
      if (output == '-')
	    output = input[0];

      string frm = input;
      string to  = input;
      to[0] = output;

      unsigned edge = frm.find_first_not_of("01x");
      assert(frm.find_last_not_of("01x") == edge);

      switch (input[edge]) {
	  case 'r':
	    frm[edge] = '0';
	    to[edge] = '1';
	    break;
	  case 'R':
	    frm[edge] = 'x';
	    to[edge] = '1';
	    break;
	  case 'f':
	    frm[edge] = '1';
	    to[edge] = '0';
	    break;
	  case 'F':
	    frm[edge] = 'x';
	    to[edge] = '0';
	    break;
	  case 'P':
	    frm[edge] = '0';
	    to[edge] = 'x';
	    break;
	  case 'N':
	    frm[edge] = '1';
	    to[edge] = 'x';
	    break;
	  default:
	    assert(0);
      }

      state_t_*sfrm = find_state_(frm);
      state_t_*sto  = find_state_(to);

      switch (to[edge]) {
	  case '0':
	      // Notice that I might have caught this edge already
	    if (sfrm->pins[edge].zer != sto) {
		  assert(sfrm->pins[edge].zer == 0);
		  sfrm->pins[edge].zer = sto;
	    }
	    break;
	  case '1':
	      // Notice that I might have caught this edge already
	    if (sfrm->pins[edge].one != sto) {
		    assert(sfrm->pins[edge].one == 0);
		    sfrm->pins[edge].one = sto;
	    }
	    break;
	  case 'x':
	      // Notice that I might have caught this edge already
	    if (sfrm->pins[edge].xxx != sto) {
		    assert(sfrm->pins[edge].xxx == 0);
		    sfrm->pins[edge].xxx = sto;
	    }
	    break;
      }

      return true;
}

bool NetUDP::sequ_glob_(string input, char output)
{
      for (unsigned idx = 0 ;  idx < input.length() ;  idx += 1)
	    switch (input[idx]) {
		case '0':
		case '1':
		case 'x':
		case 'r':
		case 'R':
		case 'f':
		case 'F':
		case 'P':
		case 'N':
		  break;

		case '?': // Iterate over all the levels
		  input[idx] = '0';
		  sequ_glob_(input, output);
		  input[idx] = '1';
		  sequ_glob_(input, output);
		  input[idx] = 'x';
		  sequ_glob_(input, output);
		  return true;

		case 'n': // Iterate over (n) edges
		  input[idx] = 'f';
		  sequ_glob_(input, output);
		  input[idx] = 'F';
		  sequ_glob_(input, output);
		  input[idx] = 'N';
		  sequ_glob_(input, output);
		  return true;

		case 'p': // Iterate over (p) edges
		  input[idx] = 'r';
		  sequ_glob_(input, output);
		  input[idx] = 'R';
		  sequ_glob_(input, output);
		  input[idx] = 'P';
		  sequ_glob_(input, output);
		  return true;

		case '_': // Iterate over (?0) edges
		  input[idx] = 'f';
		  sequ_glob_(input, output);
		  input[idx] = 'F';
		  sequ_glob_(input, output);
		  return true;

		case '*': // Iterate over all the edges
		  input[idx] = 'r';
		  sequ_glob_(input, output);
		  input[idx] = 'R';
		  sequ_glob_(input, output);
		  input[idx] = 'f';
		  sequ_glob_(input, output);
		  input[idx] = 'F';
		  sequ_glob_(input, output);
		  input[idx] = 'P';
		  sequ_glob_(input, output);
		  input[idx] = 'N';
		  sequ_glob_(input, output);
		  return true;

		default:
		  assert(0);
	    }

      return set_sequ_(input, output);
}

bool NetUDP::set_table(const string&input, char output)
{
      assert((output == '0') || (output == '1') || (sequential_ &&
						    (output == '-')));

      if (sequential_) {
	    assert(input.length() == pin_count());
	      /* XXXX Need to check to make sure that the input vector
		 contains a legal combination of characters. */
	    return sequ_glob_(input, output);

      } else {
	    assert(input.length() == (pin_count()-1));
	      /* XXXX Need to check to make sure that the input vector
		 contains a legal combination of characters. In
		 combinational UDPs, only 0, 1 and x are allowed. */
	    cm_[input] = output;

	    return true;
      }
}

void NetUDP::cleanup_table()
{
      for (FSM_::iterator idx = fsm_.begin() ;  idx != fsm_.end() ; idx++) {
	    string str = (*idx).first;
	    state_t_*st = (*idx).second;
	    assert(str[0] == st->out);

	    for (unsigned pin = 0 ;  pin < pin_count() ;  pin += 1) {
		  if (st->pins[pin].zer && st->pins[pin].zer->out == 'x')
			st->pins[pin].zer = 0;
		  if (st->pins[pin].one && st->pins[pin].one->out == 'x')
			st->pins[pin].one = 0;
		  if (st->pins[pin].xxx && st->pins[pin].xxx->out == 'x')
			st->pins[pin].xxx = 0;
	    }
      }

      for (FSM_::iterator idx = fsm_.begin() ;  idx != fsm_.end() ; ) {
	    FSM_::iterator cur = idx;
	    idx ++;

	    state_t_*st = (*cur).second;

	    if (st->out != 'x')
		  continue;

	    for (unsigned pin = 0 ;  pin < pin_count() ;  pin += 1) {
		  if (st->pins[pin].zer)
			goto break_label;
		  if (st->pins[pin].one)
			goto break_label;
		  if (st->pins[pin].xxx)
			goto break_label;
	    }

		    //delete st;
	    fsm_.erase(cur);

      break_label:;
      }
}

char NetUDP::table_lookup(const string&from, char to, unsigned pin) const
{
      assert(pin <= pin_count());
      assert(from.length() == pin_count());
      FSM_::const_iterator idx = fsm_.find(from);
      if (idx == fsm_.end())
	    return 'x';

      state_t_*next;
      switch (to) {
	  case '0':
	    next = (*idx).second->pins[pin].zer;
	    break;
	  case '1':
	    next = (*idx).second->pins[pin].one;
	    break;
	  case 'x':
	    next = (*idx).second->pins[pin].xxx;
	    break;
	  default:
	    assert(0);
	    next = 0;
      }

      return next? next->out : 'x';
}

void NetUDP::set_initial(char val)
{
      assert(sequential_);
      assert((val == '0') || (val == '1') || (val == 'x'));
      init_ = val;
}

Design:: Design()
: errors(0), signals_(0), nodes_(0), procs_(0), lcounter_(0)
{
}

Design::~Design()
{
}

NetScope* Design::make_root_scope(const string&root)
{
      NetScope*scope = new NetScope(root);
      scopes_[root] = scope;
      return scope;
}

NetScope* Design::make_scope(const string&path,
			     NetScope::TYPE t,
			     const string&name)
{
      string npath = path + "." + name;
      NetScope*scope = new NetScope(npath, t);
      scopes_[npath] = scope;
      return scope;
}

NetScope* Design::find_scope(const string&key)
{
      map<string,NetScope*>::const_iterator tmp = scopes_.find(key);
      if (tmp == scopes_.end())
	    return 0;
      else
	    return (*tmp).second;
}

void Design::set_parameter(const string&key, NetExpr*expr)
{
      parameters_[key] = expr;
}

/*
 * Find a parameter from within a specified context. If the name is
 * not here, keep looking up until I run out of up to look at.
 */
const NetExpr* Design::find_parameter(const string&path,
				      const string&name) const
{
      string root = path;

      for (;;) {
	    string fulname = root + "." + name;
	    map<string,NetExpr*>::const_iterator cur
		  = parameters_.find(fulname);

	    if (cur != parameters_.end())
		  return (*cur).second;

	    unsigned pos = root.rfind('.');
	    if (pos > root.length())
		  break;

	    root = root.substr(0, pos);
      }

      return 0;
}

string Design::get_flag(const string&key) const
{
      map<string,string>::const_iterator tmp = flags_.find(key);
      if (tmp == flags_.end())
	    return "";
      else
	    return (*tmp).second;
}

void Design::add_signal(NetNet*net)
{
      assert(net->design_ == 0);
      if (signals_ == 0) {
	    net->sig_next_ = net;
	    net->sig_prev_ = net;
      } else {
	    net->sig_next_ = signals_->sig_next_;
	    net->sig_prev_ = signals_;
	    net->sig_next_->sig_prev_ = net;
	    net->sig_prev_->sig_next_ = net;
      }
      signals_ = net;
      net->design_ = this;
}

void Design::del_signal(NetNet*net)
{
      assert(net->design_ == this);
      if (signals_ == net)
	    signals_ = net->sig_prev_;

      if (signals_ == net) {
	    signals_ = 0;
      } else {
	    net->sig_prev_->sig_next_ = net->sig_next_;
	    net->sig_next_->sig_prev_ = net->sig_prev_;
      }
      net->design_ = 0;
}

/*
 * This method looks for a string given a current context as a
 * starting point.
 */
NetNet* Design::find_signal(const string&path, const string&name)
{
      if (signals_ == 0)
	    return 0;

      string root = path;
      for (;;) {

	    string fulname = root + "." + name;
	    NetNet*cur = signals_;
	    do {
		  if (cur->name() == fulname)
			return cur;

		  cur = cur->sig_prev_;
	    } while (cur != signals_);

	    unsigned pos = root.rfind('.');
	    if (pos > root.length())
		  break;

	    root = root.substr(0, pos);
      }

      return 0;
}

void Design::add_memory(NetMemory*mem)
{
      memories_[mem->name()] = mem;
}

NetMemory* Design::find_memory(const string&path, const string&name)
{
      string root = path;

      for (;;) {
	    string fulname = root + "." + name;
	    map<string,NetMemory*>::const_iterator cur
		  = memories_.find(fulname);

	    if (cur != memories_.end())
		  return (*cur).second;

	    unsigned pos = root.rfind('.');
	    if (pos > root.length())
		  break;

	    root = root.substr(0, pos);
      }

      return 0;
}

void Design::add_function(const string&key, NetFuncDef*def)
{
      funcs_[key] = def;
}

NetFuncDef* Design::find_function(const string&path, const string&name)
{
      string root = path;
      for (;;) {
	    string key = root + "." + name;
	    map<string,NetFuncDef*>::const_iterator cur = funcs_.find(key);
	    if (cur != funcs_.end())
		  return (*cur).second;

	    unsigned pos = root.rfind('.');
	    if (pos > root.length())
		  break;

	    root = root.substr(0, pos);
      }

      return 0;
}

NetFuncDef* Design::find_function(const string&key)
{
      map<string,NetFuncDef*>::const_iterator cur = funcs_.find(key);
      if (cur != funcs_.end())
	    return (*cur).second;
      return 0;
}

void Design::add_task(const string&key, NetTaskDef*def)
{
      tasks_[key] = def;
}

NetTaskDef* Design::find_task(const string&path, const string&name)
{
      string root = path;
      for (;;) {
	    string key = root + "." + name;
	    map<string,NetTaskDef*>::const_iterator cur = tasks_.find(key);
	    if (cur != tasks_.end())
		  return (*cur).second;

	    unsigned pos = root.rfind('.');
	    if (pos > root.length())
		  break;

	    root = root.substr(0, pos);
      }

      return 0;
}

NetTaskDef* Design::find_task(const string&key)
{
      map<string,NetTaskDef*>::const_iterator cur = tasks_.find(key);
      if (cur == tasks_.end())
	    return 0;

      return (*cur).second;
}

void Design::add_node(NetNode*net)
{
      assert(net->design_ == 0);
      if (nodes_ == 0) {
	    net->node_next_ = net;
	    net->node_prev_ = net;
      } else {
	    net->node_next_ = nodes_->node_next_;
	    net->node_prev_ = nodes_;
	    net->node_next_->node_prev_ = net;
	    net->node_prev_->node_next_ = net;
      }
      nodes_ = net;
      net->design_ = this;
}

void Design::del_node(NetNode*net)
{
      assert(net->design_ == this);
      if (nodes_ == net)
	    nodes_ = net->node_prev_;

      if (nodes_ == net) {
	    nodes_ = 0;
      } else {
	    net->node_next_->node_prev_ = net->node_prev_;
	    net->node_prev_->node_next_ = net->node_next_;
      }

      net->design_ = 0;
}

void Design::add_process(NetProcTop*pro)
{
      pro->next_ = procs_;
      procs_ = pro;
}

void Design::delete_process(NetProcTop*top)
{
      assert(top);
      if (procs_ == top) {
	    procs_ = top->next_;

      } else {
	    NetProcTop*cur = procs_;
	    while (cur->next_ != top) {
		  assert(cur->next_);
		  cur = cur->next_;
	    }

	    cur->next_ = top->next_;
      }

      if (procs_idx_ == top)
	    procs_idx_ = top->next_;

      delete top;
}

void Design::clear_node_marks()
{
      if (nodes_ == 0)
	    return;

      NetNode*cur = nodes_;
      do {
	    cur->set_mark(false);
	    cur = cur->node_next_;
      } while (cur != nodes_);
}

void Design::clear_signal_marks()
{
      if (signals_ == 0)
	    return;

      NetNet*cur = signals_;
      do {
	    cur->set_mark(false);
	    cur = cur->sig_next_;
      } while (cur != signals_);
}

NetNode* Design::find_node(bool (*func)(const NetNode*))
{
      if (nodes_ == 0)
	    return 0;

      NetNode*cur = nodes_->node_next_;
      do {
	    if ((cur->test_mark() == false) && func(cur))
		  return cur;

	    cur = cur->node_next_;
      } while (cur != nodes_->node_next_);

      return 0;
}

NetNet* Design::find_signal(bool (*func)(const NetNet*))
{
      if (signals_ == 0)
	    return 0;

      NetNet*cur = signals_->sig_next_;
      do {
	    if ((cur->test_mark() == false) && func(cur))
		  return cur;

	    cur = cur->sig_next_;
      } while (cur != signals_->sig_next_);

      return 0;
}

/*
 * $Log: netlist.cc,v $
 * Revision 1.101  1999/12/17 03:38:46  steve
 *  NetConst can now hold wide constants.
 *
 * Revision 1.100  1999/12/16 02:42:15  steve
 *  Simulate carry output on adders.
 *
 * Revision 1.99  1999/12/05 19:30:43  steve
 *  Generate XNF RAMS from synthesized memories.
 *
 * Revision 1.98  1999/12/05 02:24:09  steve
 *  Synthesize LPM_RAM_DQ for writes into memories.
 *
 * Revision 1.97  1999/12/02 16:58:58  steve
 *  Update case comparison (Eric Aardoom).
 *
 * Revision 1.96  1999/11/28 23:42:02  steve
 *  NetESignal object no longer need to be NetNode
 *  objects. Let them keep a pointer to NetNet objects.
 *
 * Revision 1.95  1999/11/28 01:16:18  steve
 *  gate outputs need to set signal values.
 *
 * Revision 1.94  1999/11/27 19:07:57  steve
 *  Support the creation of scopes.
 *
 * Revision 1.93  1999/11/24 04:01:59  steve
 *  Detect and list scope names.
 *
 * Revision 1.92  1999/11/21 18:03:35  steve
 *  Fix expression width of memory references.
 *
 * Revision 1.91  1999/11/21 17:35:37  steve
 *  Memory name lookup handles scopes.
 *
 * Revision 1.90  1999/11/21 00:13:08  steve
 *  Support memories in continuous assignments.
 *
 * Revision 1.89  1999/11/19 05:02:37  steve
 *  handle duplicate connect to a nexus.
 *
 * Revision 1.88  1999/11/19 03:02:25  steve
 *  Detect flip-flops connected to opads and turn
 *  them into OUTFF devices. Inprove support for
 *  the XNF-LCA attribute in the process.
 *
 * Revision 1.87  1999/11/18 03:52:19  steve
 *  Turn NetTmp objects into normal local NetNet objects,
 *  and add the nodangle functor to clean up the local
 *  symbols generated by elaboration and other steps.
 *
 * Revision 1.86  1999/11/14 23:43:45  steve
 *  Support combinatorial comparators.
 *
 * Revision 1.85  1999/11/14 20:24:28  steve
 *  Add support for the LPM_CLSHIFT device.
 *
 * Revision 1.84  1999/11/13 03:46:52  steve
 *  Support the LPM_MUX in vvm.
 *
 * Revision 1.83  1999/11/05 04:40:40  steve
 *  Patch to synthesize LPM_ADD_SUB from expressions,
 *  Thanks to Larry Doolittle. Also handle constants
 *  in expressions.
 *
 *  Synthesize adders in XNF, based on a patch from
 *  Larry. Accept synthesis of constants from Larry
 *  as is.
 *
 * Revision 1.82  1999/11/04 03:53:26  steve
 *  Patch to synthesize unary ~ and the ternary operator.
 *  Thanks to Larry Doolittle <LRDoolittle@lbl.gov>.
 *
 *  Add the LPM_MUX device, and integrate it with the
 *  ternary synthesis from Larry. Replace the lpm_mux
 *  generator in t-xnf.cc to use XNF EQU devices to
 *  put muxs into function units.
 *
 *  Rewrite elaborate_net for the PETernary class to
 *  also use the LPM_MUX device.
 *
 * Revision 1.81  1999/11/04 01:12:42  steve
 *  Elaborate combinational UDP devices.
 *
 * Revision 1.80  1999/11/02 04:55:34  steve
 *  Add the synthesize method to NetExpr to handle
 *  synthesis of expressions, and use that method
 *  to improve r-value handling of LPM_FF synthesis.
 *
 *  Modify the XNF target to handle LPM_FF objects.
 *
 * Revision 1.79  1999/11/01 02:07:40  steve
 *  Add the synth functor to do generic synthesis
 *  and add the LPM_FF device to handle rows of
 *  flip-flops.
 *
 * Revision 1.78  1999/10/31 04:11:27  steve
 *  Add to netlist links pin name and instance number,
 *  and arrange in vvm for pin connections by name
 *  and instance number.
 *
 * Revision 1.77  1999/10/10 23:29:37  steve
 *  Support evaluating + operator at compile time.
 *
 * Revision 1.76  1999/10/10 01:59:55  steve
 *  Structural case equals device.
 *
 * Revision 1.75  1999/10/07 05:25:34  steve
 *  Add non-const bit select in l-value of assignment.
 *
 * Revision 1.74  1999/10/06 05:06:16  steve
 *  Move the rvalue into NetAssign_ common code.
 *
 * Revision 1.73  1999/10/05 04:02:10  steve
 *  Relaxed width handling for <= assignment.
 *
 * Revision 1.72  1999/09/30 21:28:34  steve
 *  Handle mutual reference of tasks by elaborating
 *  task definitions in two passes, like functions.
 *
 * Revision 1.71  1999/09/29 18:36:03  steve
 *  Full case support
 *
 * Revision 1.70  1999/09/28 03:11:29  steve
 *  Get the bit widths of unary operators that return one bit.
 *
 * Revision 1.69  1999/09/23 03:56:57  steve
 *  Support shift operators.
 *
 * Revision 1.68  1999/09/23 00:21:54  steve
 *  Move set_width methods into a single file,
 *  Add the NetEBLogic class for logic expressions,
 *  Fix error setting with of && in if statements.
 *
 * Revision 1.67  1999/09/21 00:13:40  steve
 *  Support parameters that reference other paramters.
 *
 * Revision 1.66  1999/09/20 02:21:10  steve
 *  Elaborate parameters in phases.
 *
 * Revision 1.65  1999/09/18 01:53:08  steve
 *  Detect constant lessthen-equal expressions.
 *
 * Revision 1.64  1999/09/16 04:18:15  steve
 *  elaborate concatenation repeats.
 *
 * Revision 1.63  1999/09/16 00:33:45  steve
 *  Handle implicit !=0 in if statements.
 *
 * Revision 1.62  1999/09/15 01:55:06  steve
 *  Elaborate non-blocking assignment to memories.
 *
 * Revision 1.61  1999/09/13 03:10:59  steve
 *  Clarify msb/lsb in context of netlist. Properly
 *  handle part selects in lval and rval of expressions,
 *  and document where the least significant bit goes
 *  in NetNet objects.
 *
 * Revision 1.60  1999/09/12 01:16:51  steve
 *  Pad r-values in certain assignments.
 *
 * Revision 1.59  1999/09/11 04:43:17  steve
 *  Support ternary and <= operators in vvm.
 *
 * Revision 1.58  1999/09/08 04:05:30  steve
 *  Allow assign to not match rvalue width.
 *
 * Revision 1.57  1999/09/04 01:57:15  steve
 *  Generate fake adder code in vvm.
 *
 * Revision 1.56  1999/09/03 04:28:38  steve
 *  elaborate the binary plus operator.
 *
 * Revision 1.55  1999/09/01 20:46:19  steve
 *  Handle recursive functions and arbitrary function
 *  references to other functions, properly pass
 *  function parameters and save function results.
 *
 * Revision 1.54  1999/08/31 22:38:29  steve
 *  Elaborate and emit to vvm procedural functions.
 *
 * Revision 1.53  1999/08/25 22:22:41  steve
 *  elaborate some aspects of functions.
 *
 * Revision 1.52  1999/08/06 04:05:28  steve
 *  Handle scope of parameters.
 *
 * Revision 1.51  1999/08/01 21:48:11  steve
 *  set width of procedural r-values when then
 *  l-value is a memory word.
 *
 * Revision 1.50  1999/07/31 19:14:47  steve
 *  Add functions up to elaboration (Ed Carter)
 *
 * Revision 1.49  1999/07/31 03:16:54  steve
 *  move binary operators to derived classes.
 *
 * Revision 1.48  1999/07/24 02:11:20  steve
 *  Elaborate task input ports.
 *
 * Revision 1.47  1999/07/18 21:17:50  steve
 *  Add support for CE input to XNF DFF, and do
 *  complete cleanup of replaced design nodes.
 *
 * Revision 1.46  1999/07/18 05:52:46  steve
 *  xnfsyn generates DFF objects for XNF output, and
 *  properly rewrites the Design netlist in the process.
 *
 * Revision 1.45  1999/07/17 19:51:00  steve
 *  netlist support for ternary operator.
 *
 * Revision 1.44  1999/07/17 18:06:02  steve
 *  Better handling of bit width of + operators.
 *
 * Revision 1.43  1999/07/17 03:08:31  steve
 *  part select in expressions.
 *
 * Revision 1.42  1999/07/16 04:33:41  steve
 *  set_width for NetESubSignal.
 *
 * Revision 1.41  1999/07/03 02:12:51  steve
 *  Elaborate user defined tasks.
 *
 * Revision 1.40  1999/06/24 05:02:36  steve
 *  Properly terminate signal matching scan.
 *
 * Revision 1.39  1999/06/24 04:24:18  steve
 *  Handle expression widths for EEE and NEE operators,
 *  add named blocks and scope handling,
 *  add registers declared in named blocks.
 *
 * Revision 1.38  1999/06/19 21:06:16  steve
 *  Elaborate and supprort to vvm the forever
 *  and repeat statements.
 *
 * Revision 1.37  1999/06/13 23:51:16  steve
 *  l-value part select for procedural assignments.
 *
 * Revision 1.36  1999/06/13 16:30:06  steve
 *  Unify the NetAssign constructors a bit.
 *
 * Revision 1.35  1999/06/10 05:33:28  steve
 *  Handle a few more operator bit widths.
 *
 * Revision 1.34  1999/06/09 03:00:06  steve
 *  Add support for procedural concatenation expression.
 *
 * Revision 1.33  1999/06/07 02:23:31  steve
 *  Support non-blocking assignment down to vvm.
 *
 * Revision 1.32  1999/06/06 20:45:38  steve
 *  Add parse and elaboration of non-blocking assignments,
 *  Replace list<PCase::Item*> with an svector version,
 *  Add integer support.
 *
 * Revision 1.31  1999/06/03 05:16:25  steve
 *  Compile time evalutation of constant expressions.
 *
 * Revision 1.30  1999/06/02 15:38:46  steve
 *  Line information with nets.
 *
 * Revision 1.29  1999/05/30 01:11:46  steve
 *  Exressions are trees that can duplicate, and not DAGS.
 *
 * Revision 1.28  1999/05/27 04:13:08  steve
 *  Handle expression bit widths with non-fatal errors.
 *
 * Revision 1.27  1999/05/20 05:07:37  steve
 *  Line number info with match error message.
 *
 * Revision 1.26  1999/05/16 05:08:42  steve
 *  Redo constant expression detection to happen
 *  after parsing.
 *
 *  Parse more operators and expressions.
 *
 * Revision 1.25  1999/05/13 04:02:09  steve
 *  More precise handling of verinum bit lengths.
 *
 * Revision 1.24  1999/05/12 04:03:19  steve
 *  emit NetAssignMem objects in vvm target.
 *
 * Revision 1.23  1999/05/10 00:16:58  steve
 *  Parse and elaborate the concatenate operator
 *  in structural contexts, Replace vector<PExpr*>
 *  and list<PExpr*> with svector<PExpr*>, evaluate
 *  constant expressions with parameters, handle
 *  memories as lvalues.
 *
 *  Parse task declarations, integer types.
 *
 * Revision 1.22  1999/05/01 02:57:53  steve
 *  Handle much more complex event expressions.
 *
 * Revision 1.21  1999/04/25 00:44:10  steve
 *  Core handles subsignal expressions.
 *
 * Revision 1.20  1999/04/19 01:59:36  steve
 *  Add memories to the parse and elaboration phases.
 *
 * Revision 1.19  1999/03/15 02:43:32  steve
 *  Support more operators, especially logical.
 *
 * Revision 1.18  1999/03/01 03:27:53  steve
 *  Prevent the duplicate allocation of ESignal objects.
 *
 * Revision 1.17  1999/02/21 17:01:57  steve
 *  Add support for module parameters.
 *
 * Revision 1.16  1999/02/08 02:49:56  steve
 *  Turn the NetESignal into a NetNode so
 *  that it can connect to the netlist.
 *  Implement the case statement.
 *  Convince t-vvm to output code for
 *  the case statement.
 *
 * Revision 1.15  1999/02/03 04:20:11  steve
 *  Parse and elaborate the Verilog CASE statement.
 */

