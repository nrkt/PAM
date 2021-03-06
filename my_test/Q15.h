// suppkey, s_name, s_address, s_phone, total_revenue
using Q15_elt = tuple<dkey_t, char*, char*, char*, double>;
using Q15_rtype = pbbs::sequence<Q15_elt>;

Q15_rtype Q15(maps m,
	      const char* start_date,
	      const char* end_date) {
  ship_map& sm = m.sm;
  ship_map ship_range = ship_map::range(sm,
					Date(start_date),
					Date(end_date));

  using ftype = double;
  using sr_type = pair<dkey_t, float>;
  
  auto map_lineitems = [&] (li_map::E& e) -> sr_type {
    float v = e.e_price*(1 - e.discount.val());
    return make_pair(e.suppkey, v);
  };

  pbbs::sequence<sr_type> elts = flatten<sr_type>(ship_range, map_lineitems);  
  
  int max_supp_id = SUPPLIER_NUM+1;
  
  auto get_index = [] (sr_type& a) {return a.first;};
  auto get_val = [] (sr_type& a) -> ftype {return (ftype) a.second;};

  pbbs::sequence<ftype> supp_sums =
    pbbs::collect_reduce(elts, get_index, get_val, pbbs::addm<ftype>(), max_supp_id);

  ftype max_revenue = pbbs::reduce(supp_sums, pbbs::maxm<ftype>());
  
  auto x = [&] (size_t i) -> Q15_elt {
    Supplier& s = static_data.all_supp[i];
    return Q15_elt(i, s.name(), s.address(), s.phone(), supp_sums[i]);};
  auto is_in = [&] (size_t i) -> bool {
    return supp_sums[i] >= max_revenue;};
  return pbbs::pack(pbbs::delayed_seq<Q15_elt>(max_supp_id, x),
		    pbbs::delayed_seq<bool>(max_supp_id, is_in));
}

double Q15time(maps m, bool verbose) {
  timer t;
  t.start();
  const char start[] = "1996-01-01";
  const char end[] = "1996-03-31"; //"1996-04-01";

  Q15_rtype result = Q15(m, start, end);
  double ret_tm = t.stop();
  if (query_out) cout << "Q15 : " << ret_tm << endl;
  
  if (verbose) {
    Q15_elt r = result[0];
    cout << "Q15:" << endl
	 << get<0>(r) << ", "
      	 << get<1>(r) << ", "
      	 << get<2>(r) << ", "
      	 << get<3>(r) << ", "
      	 << get<4>(r) << endl;
  }
  return ret_tm;
}
