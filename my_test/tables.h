int CUSTOMER_NUM, PARTSUPP_NUM, CURR_ORDER, ORDER_NUM, NATION_NUM, REGION_NUM, SUPPLIER_NUM, PART_NUM, LINEITEM_NUM;

auto sep = [] (char c) -> bool { return c == '|'; };
int total_nation = 25;

template <class S>
sequence<S> read_and_parse(string filename, bool verbose) {
  csv_data d = readCSV(filename, sep, verbose);
  auto parseItem = [&] (size_t i) { return S(d.lines[i]);};
  auto x = sequence<S>(d.lines.size(), parseItem);
  return x;
}

struct li_entry {
  using key_t = Lineitem; //Lineitem is a struct(in lineitem.h)
  static bool comp(const key_t& a, const key_t& b) {
    return ((a.orderkey < b.orderkey) ||
	    (a.orderkey == b.orderkey && a.linenumber < b.linenumber));
  }
};

// li_map: create weight_balanced_tree with Lineitem data(key-only). Node->(Lineitem)
using li_map = pam_set<li_entry>; 
// order_map: create weight_balanced_tree with Orders and Lineitem. Node->(Orders.key, (Orders.value, Lineitem(li_map)))
using order_map = keyed_map<pair<Orders,li_map>>; 

struct maps {
  order_map om;
  int version = 0;
  maps() {}
  maps(const maps& m) {
	om = m.om;
	version = m.version+1;
  }
  maps& operator = (const maps& m) {
    if (this != &m) { 
	  om = m.om;
	  version = m.version+1;
	}
    return *this;
  }

  void clear() {
	  om.clear();
  }
};

void memory_stats() {
  cout << "lineitem map:" << endl;
  li_map::GC::print_stats();
  cout << "order map:" << endl;
  order_map::GC::print_stats();
}

maps make_maps(string dirname, bool verbose) {
  maps m;
  string lf = dirname;
 
  timer tx; tx.start();
  cout << sizeof(Lineitem) << endl;
  cout << sizeof(Orders) << endl;

  // lineitems
  cout << "-- lineitems" << endl;

  // first keyed on shipdate
  string lineitem_fname = lf.append("lineitem.tbl");
  sequence<Lineitem> items = read_and_parse<Lineitem>(lineitem_fname, verbose);
  cout << items.size() << endl;
  //for (int i = 0; i < items.size(); ++i) std::cout << "items = " << items[i].orderkey << std::endl;
  if (verbose) {nextTime("parse line items");}
  
  LINEITEM_NUM = items.size();

  // orders and customers
  { 
    if (verbose) cout << endl;
    cout << "-- orders" << endl;

    string of = dirname;
    string orders_fname = of.append("orders.tbl");
    sequence<Orders> orders = read_and_parse<Orders>(orders_fname, verbose);
	ORDER_NUM = orders.size();

  // for (int i = 0; i < ORDER_NUM; ++i) std::cout << "Order Key = " << orders[i].orderkey << endl;
	// CURR_ORDER: final data number of orders.tbl
  CURR_ORDER = orders[ORDER_NUM-1].orderkey; 
    if (verbose) nextTime("parse orders");
    // ordermap: create weight_balanced_tree with Orders data. Node->(Orders.key, Orders.value)
    using ordermap = keyed_map<Orders>;
    // co_li_map: create weight_balanced_tree with Paired key and Key-only data. Node->(Paired_key, li_map(tree structure))
    // ex.) comap<co_li_map> = {(custkey, orderkey) : lineitem(group by orderkey and tree structure)}
    using co_li_map = paired_key_map<li_map>; 
    {
      auto orders_primary = primary_index<ordermap>(orders, [&] (Orders o) {
	  return o.orderkey;});

    //std::cout << "orders_primary's root's key = " << orders_primary.root->entry.first << std::endl; 
    //std::cout << "orders_primary's root's value(orderdate) = " << orders_primary.root->entry.second.orderdate << std::endl; 

    // comap:= {(orders.custkey, lineitem.orderkey) : lineitem(group by orderkey and tree structure)}
      auto comap = secondary_index<co_li_map>(items, [&] (Lineitem l) {
	  return make_pair((*(orders_primary.find(l.orderkey))).custkey,
			   l.orderkey);});
      //std::cout << "comap root customer key = " << comap.root->entry.first.first << std::endl;
      //std::cout << "comap root order key = " << comap.root->entry.first.second << std::endl;
      //std::cout << "comap root inner tree's root order key = " << comap.root->entry.second.root->entry.orderkey << std::endl; 
      //std::cout << "comap root inner tree's root linenumber = " << comap.root->entry.second.root->entry.linenumber << std::endl; 
      
      // get_key: function (input: orders item, output: orderkey)
      auto get_key = [&] (Orders& o) -> dkey_t {
	return o.orderkey;};
      //std::cout << get_key(orders[0]) << std::endl;

      // get_val: function (input: orders item, output: li_map)
      // if (orders.custkey, orders.orderkey) in comap then return li_map(lineitem group by order key)
      // else return empty li_map
      auto get_val = [&] (Orders& o) -> li_map {
	maybe<li_map> x = comap.find(make_pair(o.custkey,o.orderkey));
	return x ? *x : li_map();
      };
      // std::cout << get_val(orders[0]).root->entry.orderkey << std::endl;

      // make_paired_index: function
      // input: orders(list), get_key(func), get_val(func)
      // output: order_map. Node->(Orders.key, (Orders.value, Lineitem(li_map)))
      m.om = make_paired_index<order_map>(orders, get_key, get_val);
      //std::cout << "make_paired_index's root's key = " << m.om.root->entry.first << std::endl;
      //std::cout << "m.om.root->entry.second.first.orderkey = " << m.om.root->entry.second.first.orderkey << std::endl;
      //std::cout << "m.om.root->entry.second.second.root->entry.orderkey = " << m.om.root->entry.second.second.root->entry.orderkey << std::endl;
    }
    ordermap::GC::finish(); // return memory
    co_li_map::GC::finish(); // return memory
  }
  m.version = 0;
  cout << "build all index in time: " << tx.stop() << endl;
  
  return m;
}

auto revenue = [] (li_map::E& l) -> float {
  return l.e_price * (1.0 - l.discount.val());
};

auto plus_float = [] (float a, float b) -> float {return float(a + b);};
auto plus_double = [] (double a, double b) -> double {return double(a + b);};
auto plus_size_t = [] (size_t a, size_t b) {return a + b;};
auto plus_int = [] (int a, int b) {return a + b;};
template <class P>
P plus_pair(P a, P b) {
  return make_pair(a.first+b.first, a.second+b.second);
}

template <class T>
T plus_f(T a, T b) {return a + b;}

template <class T>
T max_f(T a, T b) {return std::max(a,b);}

template <class T>
T min_f(T a, T b) {return std::min(a,b);}

auto min_float = [] (float a, float b) -> float {return std::min(a,b);};

