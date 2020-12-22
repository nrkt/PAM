#include <math.h>
#include <vector>
#include "readCSV.h"
#include "lineitem.h"
#include "pbbslib/get_time.h"
#include "pbbslib/parse_command_line.h"
#include "pbbslib/collect_reduce.h"
#include "pbbslib/random_shuffle.h"
#include "pbbslib/monoid.h"
#include "pam.h"
#include "utils.h"
#include "tables.h"
vector<maps> history;
#include "new_orders.h"

size_t new_lineitem = 0;
size_t shipped_lineitem = 0;
size_t keep_versions = 1000000;
bool if_persistent = false;
bool if_collect = true;

void test_all(bool verbose, bool if_query, bool if_update,
	      int scale, int num_txns, string data_directory) {

  history.push_back(make_maps(data_directory, verbose));
  memory_stats();
  history.clear();
}
	
int main(int argc, char** argv) {
  commandLine P(argc, argv, "./test [-v] [-q] [-u] [-c] [-s size] [-t txns] [-d directory] [-y keep_versions] [-p]");
  bool verbose = P.getOption("-v");
  bool if_query = P.getOption("-q");
  bool if_update = P.getOption("-u");
  if_persistent = P.getOption("-p");
  keep_versions = P.getOptionIntValue("-y", 1000000);
  if_collect = P.getOption("-c");
  string default_directory = "/ssd1/tpch/S10/";
  int scale = P.getOptionIntValue("-s", 10);
  int num_txns = P.getOptionIntValue("-t", 10000);
  if (scale == 100) default_directory = "/ssd1/tpch/S100/";
  if (scale == 1) default_directory = "/ssd1/tpch/S1/";
   
  string data_directory = P.getOptionValue("-d", default_directory);
  cout << "Num Workers = " << num_workers() << endl;
  test_all(verbose, if_query, if_update,
	   scale, num_txns, data_directory);
  
  memory_stats();
  return 0;
}
