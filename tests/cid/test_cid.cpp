
#include <fstream>
#include <sstream>
#include <iostream>
#include <iomanip>

#include "ace/High_Res_Timer.h"
#include "ace/Sched_Params.h"

#include "madara/cid/CIDConvenience.h"
#include "madara/cid/CIDGenetic.h"
#include "madara/cid/CIDHeuristic.h"
#include "madara/utility/LogMacros.h"

double ga_time = 1.0;

void genetic_degree (Madara::Cid::Settings & settings)
{
  Madara::Cid::generate_worst_solution (settings);
  Madara::Cid::ga_degree (settings, ga_time);
}

void genetic_naive (Madara::Cid::Settings & settings)
{
  Madara::Cid::generate_worst_solution (settings);
  Madara::Cid::ga_naive (settings, ga_time);
}

void naive_cid_ga_naive (Madara::Cid::Settings & settings)
{  
  Madara::Cid::fill_by_highest_degree (settings);
  Madara::Cid::ga_naive (settings, ga_time);
}

void naive_cid_ga_degree (Madara::Cid::Settings & settings)
{
  Madara::Cid::fill_by_highest_degree (settings);
  Madara::Cid::ga_degree (settings, ga_time);
}

void cid_ga_naive (Madara::Cid::Settings & settings)
{
  Madara::Cid::approximate (settings);
  Madara::Cid::ga_naive (settings, ga_time);
}

void cid_ga_degree (Madara::Cid::Settings & settings)
{
  Madara::Cid::approximate (settings);
  Madara::Cid::ga_degree (settings, ga_time);
}

void hand_coded (Madara::Cid::Settings & settings)
{
  Madara::Cid::Workflow & deployment = settings.target_deployment;
  Madara::Cid::SummationsMap & averages = settings.network_summations;
  Madara::Cid::SolutionMap & lookup = settings.solution_lookup;

  unsigned int fill_starting = deployment.size ();
  unsigned int first_degree = deployment[0].size ();
  unsigned int second_degree = deployment[1].size ();

  if (first_degree > 1 && second_degree > 1)
  {
    // set node 1 to the best average latency from the highest degree
    settings.solution[deployment[0][0].first] = 
      averages[first_degree][0].first;

    lookup[averages[first_degree][0].first] = deployment[0][0].first;

    // set node 0 to either the best latency from degree 4 or to the
    // second best latency in 4, if solution[1] was the same
    if (averages[first_degree][0].first != averages[second_degree][0].first)
    {
      settings.solution[deployment[1][0].first] = 
        averages[second_degree][0].first;

      lookup[averages[second_degree][0].first] = deployment[1][0].first;
    }
    else
    {
      settings.solution[deployment[1][0].first] = 
        averages[first_degree][1].first;

      lookup[averages[first_degree][0].first] = deployment[1][0].first;
    }
  }
  else
  {
    fill_starting = 0;
  }

  Madara::Cid::fill_by_highest_degree (settings, fill_starting);
}

bool use_csv_format (true);

#define NUM_TESTS           9
#define PREPARATION         9
#define MIN_TEST_RANGE      0
#define MAX_TEST_RANGE      9

char * testnames [] = {
  "CID Heuristic",
  "Blind CID Heuristic",
  "Blind Genetic Algorithm",
  "Guided Genetic Algorithm",
  "CID-BGA",
  "CID-GGA",
  "BCID-BGA",
  "BCID-GGA",
  "Random Deployment",
};

void (* test_impls []) (Madara::Cid::Settings &) = {
  Madara::Cid::approximate,
  Madara::Cid::fill_by_highest_degree,
  genetic_naive,
  genetic_degree,
  ::cid_ga_naive,
  ::cid_ga_degree,
  ::naive_cid_ga_naive,
  ::naive_cid_ga_degree,
  Madara::Cid::generate_worst_solution,

};

uint64_t clocks [NUM_TESTS + 1];
uint64_t latencies [NUM_TESTS];

void print_averages (std::ostream & output, 
                     Madara::Cid::Settings & settings)
{
  std::locale loc(""); 
  output.imbue (loc); 

  std::string divider (79, '=');

  std::vector <unsigned int> degrees;
  degrees.resize (settings.network_summations.size ());
  unsigned int size = settings.solution.size ();

  output << "\nPrinting averages by degree for tests.\n";
  output << divider << std::endl;

  Madara::Cid::SummationsMap & averages = settings.network_summations;

  unsigned int j = 0;

  for (Madara::Cid::SummationsMap::iterator i = averages.begin ();
                                           i != averages.end (); ++i)
  {
    if (i->first)
    {
      degrees[j++] = i->first;

      output << std::setw (15) << i->first;
    }
  }

  output << std::endl;

  for (unsigned int i = 0; i < size; ++i)
  {
    for (j = 0; j < degrees.size (); ++j)
    {
      if (averages[j].size () > i)
      {
        std::stringstream buffer;
        buffer << averages[j][i].first << "=" << averages[j][i].second;

        output << std::setw (15) << buffer.str ();
      }
      else
        output << std::setw (15) << "N/A";
    }

    output << std::endl;
  }
}

void print_stats (std::ostream & output, const std::string & id,
                  unsigned int size)
{
  if (use_csv_format)
  {
    output << id;
    output << "," << clocks[PREPARATION];
    output << "," << clocks[PREPARATION] / size;

    for (int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      output << "," << clocks[i];
      output << "," << latencies[i];
    }
    output << "\n";
  }
  else
  {
    std::string divider (79, '=');

    std::locale loc(""); 
    output.imbue (loc); 

    output << "\n Statistics for tests on size " << size << "\n";

    output << divider << "\n";

    output << std::setw (30) << "            " << 
              std::setw (25) << "Single Node (ns)" << 
              std::setw (24) << "Distributed (ns)" << "\n";

    output << divider << "\n";

    output << std::setw (30) << "  Prep time " << 
              std::setw (25) << clocks[PREPARATION] <<
              std::setw (24) << clocks[PREPARATION] / size << "\n\n";


    output << divider << "\n";
    output << std::setw (30) << " Technique" << 
              std::setw (25) << "Time (ns)" << 
              std::setw (24) << "Total Latency (us)" << "\n"; 

    output << divider << "\n";

    for (int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      output << std::setw (30) << testnames[i] << 
                std::setw (25) << clocks[i] <<
                std::setw (24) << latencies[i] << "\n";
    }

    output << divider << "\n";
  }
}

void prepare_latencies (Madara::Cid::Settings & settings)
{
  // high res timers provided by ACE
  ACE_hrtime_t measured;
  ACE_High_Res_Timer timer;

  timer.start ();

  // sort and average the latencies from each vertex
  Madara::Cid::prepare_latencies (settings);

  timer.stop ();
  timer.elapsed_time (measured);

  clocks[PREPARATION] = measured;
}

void print_solutions (std::ostream & output, Madara::Cid::Settings & settings,
       unsigned int test_type, unsigned int size)
{
  output << std::setw (30) << testnames[test_type];
  output << ":\n";
  for (unsigned int i = 0; i < size; ++i)
  {
    //unsigned int degree = settings.target_deployment[i].size ();

    //if (degree > 0)
    //{
    //  unsigned int source = settings.target_deployment[i][0].first;

      output << "  solution[" << i << "] is " << settings.solution[i] << "\n"; 
    //}
    //else
    //{
    //  break;
    //}
  }
  output << "\n";
}

void test_cid (unsigned int size, std::ostream & output)
{
  if (!use_csv_format)
    output << "\n\nTesting deployment of size " << size << std::endl;

  Madara::Cid::Settings settings;

  // initialize network
  Madara::Cid::init (size, settings);
  Madara::Cid::generate_random_network (size, settings);

  unsigned int quarter = size / 4;
  unsigned int mid = size / 2;
  unsigned int third = size / 3;

  //// first element of the deployment sends stuff to 0-3
  //settings.target_deployment[0].resize (mid);
  //for (unsigned int i = 0; i < mid; ++i)
  //{
  //  settings.target_deployment[0][i].first = 0;
  //  settings.target_deployment[0][i].second = i + quarter;
  //}

  std::string test_divider (79, '*');

  if (!use_csv_format)
  {
    output << test_divider << std::endl;
    output << "  Testing full-sized, complete overlap fanouts\n";
    output << test_divider << std::endl;
  }

  Madara::Cid::Workflow & deployment = settings.target_deployment;

  deployment.resize (size);

  for (unsigned int i = 0; i < deployment.size (); ++i)
  {
    deployment[i].clear ();
  }

  // test full-sized fanouts from 1-3 source nodes targeting the same nodes
  for (unsigned int fan_outs = 1; fan_outs <= 4; ++fan_outs)
  {
    if (!use_csv_format)
    {
      output << test_divider << std::endl;
      output << "  Testing " << fan_outs << 
        " full-sized, complete overlap fanouts\n";
      output << test_divider << std::endl;
    }
    std::cout << "  Testing " << fan_outs << 
      " full-sized, complete overlap fanouts\n";

    // create a 100% fanout in the targeted workflows
    for (unsigned int source = 0; source < fan_outs; ++source)
    {
      deployment[source].resize (size);
      unsigned int actual = 0;
      for (unsigned int i = 0; i < deployment[source].size (); ++i)
      {
        deployment[source][i].first = source;
        deployment[source][i].second = i;
      }
    }

    if (!use_csv_format)
      output << "  Preparing deployment\n";\
    Madara::Cid::prepare_deployment (settings);

    if (!use_csv_format)
      output << "  Preparing latencies\n";
    settings.network_summations.clear ();
    ::prepare_latencies (settings);

    // iterate through all tests on this constraint satisfaction problem
    for (unsigned int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      settings.solution_lookup.clear ();

      // high res timers provided by ACE
      ACE_hrtime_t measured;
      ACE_High_Res_Timer timer;

      if (!use_csv_format)
        output << "  Approximating with " << testnames[i] << std::endl;
      std::cout << "    Approximating with " << testnames[i] << std::endl;

      timer.start ();

      // sort and average the latencies from each vertex
      test_impls [i] (settings);

      timer.stop ();
      timer.elapsed_time (measured);

      // save wall clock time that passed during the test
      clocks[i] = measured;

      // save the overall utility of the solution (less is better)
      latencies[i] = Madara::Cid::calculate_latency (settings);


      if (Madara::Cid::check_solution (settings))
        std::cerr << "  " << testnames[i] << " appears to be broken. " <<
                  " Please report this at madara.googlecode.com\n";

      //print_solutions (output, settings, i, 1);
    }

    std::stringstream testid;
    testid << size << " with " << fan_outs << " max degrees";
    print_stats (output, testid.str (), size);
  } // end full-sized fanouts

  if (!use_csv_format)
  {
    output << test_divider << std::endl;
    output << "  Testing half-sized, complete overlap fanouts\n";
    output << test_divider << std::endl;
  }

  for (unsigned int i = 0; i < deployment.size (); ++i)
  {
    deployment[i].clear ();
  }

  // test half-sized fanouts from 1-3 source nodes targeting the same nodes
  for (unsigned int fan_outs = 1; fan_outs <= 4; ++fan_outs)
  {
    if (!use_csv_format)
    {
      output << test_divider << std::endl;
      output << "  Testing " << fan_outs << 
        " half-sized, complete overlap fanouts\n";
      output << test_divider << std::endl;
    }
    std::cout << "  Testing " << fan_outs << 
      " half-sized, complete overlap fanouts\n";

    // create a 100% fanout in the targeted workflows
    for (unsigned int source = 0; source < fan_outs; ++source)
    {
      deployment[source].resize (mid);
      for (unsigned int i = 0; i < deployment[source].size (); ++i)
      {
        deployment[source][i].first = source;
        deployment[source][i].second = quarter + i;
      }
    }

    if (!use_csv_format)
      output << "  Preparing deployment\n";\
    Madara::Cid::prepare_deployment (settings);

    if (!use_csv_format)
      output << "  Preparing latencies\n";
    settings.network_summations.clear ();
    ::prepare_latencies (settings);


    // iterate through all tests on this constraint satisfaction problem
    for (unsigned int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      settings.solution_lookup.clear ();

      // high res timers provided by ACE
      ACE_hrtime_t measured;
      ACE_High_Res_Timer timer;

      if (!use_csv_format)
        output << "  Approximating with " << testnames[i] << std::endl;
      std::cout << "    Approximating with " << testnames[i] << std::endl;

      timer.start ();

      // sort and average the latencies from each vertex
      test_impls [i] (settings);

      timer.stop ();
      timer.elapsed_time (measured);

      // save wall clock time that passed during the test
      clocks[i] = measured;

      // save the overall utility of the solution (less is better)
      latencies[i] = Madara::Cid::calculate_latency (settings);

      if (Madara::Cid::check_solution (settings))
        std::cerr << "  " << testnames[i] << " appears to be broken. " <<
                  " Please report this at madara.googlecode.com\n";

      //print_solutions (output, settings, i, 1);
    }

    std::stringstream testid;
    testid << size << " with " << fan_outs << " half overlapping degrees";
    print_stats (output, testid.str (), size);
  } // end half-sized fanouts


  // test even-sized fanouts from 1-4 source nodes targeting disjoint nodes
  for (unsigned int fan_outs = 1; fan_outs <= 4; ++fan_outs)
  {
    // create filename 
    std::stringstream filename;
    filename << getenv ("MADARA_ROOT");
    filename << "/configs/cid/deployments/test_cid/";
    filename << fan_outs << "_even_fans_disjoint.template";

    if (!use_csv_format)
    {
      // notify user of current est
      output << test_divider << std::endl;
      output << "  Testing " << fan_outs << 
        " even-sized, disjoint fanouts\n";
      output << test_divider << std::endl;
    }
    std::cout << "  Testing " << fan_outs << 
      " even-sized, disjoint fanouts\n";

    settings.solution.resize (size);
    Madara::Cid::read_deployment (settings, filename.str ());

    if (!use_csv_format)
      output << "  Preparing latencies\n";
    settings.network_summations.clear ();
    ::prepare_latencies (settings);


    // iterate through all tests on this constraint satisfaction problem
    for (unsigned int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      settings.solution_lookup.clear ();

      // high res timers provided by ACE
      ACE_hrtime_t measured;
      ACE_High_Res_Timer timer;

      if (!use_csv_format)
        output << "  Approximating with " << testnames[i] << std::endl;
      std::cout << "    Approximating with " << testnames[i] << std::endl;

      timer.start ();

      // sort and average the latencies from each vertex
      test_impls [i] (settings);

      timer.stop ();
      timer.elapsed_time (measured);

      // save wall clock time that passed during the test
      clocks[i] = measured;

      // save the overall utility of the solution (less is better)
      latencies[i] = Madara::Cid::calculate_latency (settings);

      if (Madara::Cid::check_solution (settings))
        std::cerr << "  " << testnames[i] << " appears to be broken. " <<
                  " Please report this at madara.googlecode.com\n";

      //print_solutions (output, settings, i, 1);
    }

    std::stringstream testid;
    testid << size << " with " << fan_outs << " even disjoint degrees";
    print_stats (output, testid.str (), size);
  } // end even-sized fanouts


}

void verify_algorithms (std::ostream & output)
{

  // *************************************************************

  #define CHECKED_ALGORITHMS      6
  #define WORST_LATENCY           0
  #define BEST_LATENCY            1
  #define CID                     2
  #define NAIVE_CID               3
  #define DEGREED_GA              4
  #define NAIVE_GA                5

  uint64_t alg_latencies[CHECKED_ALGORITHMS] = {0,0,0,0,0,0};

  std::string divider (79, '*');
  output << divider << std::endl;
  output << "Verifying that the algorithms are correct.\n";
  output << divider << std::endl;

  Madara::Cid::Settings settings;

  Madara::Cid::init (10, settings);

  Madara::Cid::Workflow & deployment = settings.target_deployment;
  Madara::Cid::LVVector & network_latencies = settings.network_latencies;
  Madara::Cid::LatencyVector & averages = settings.network_summations[10];
  Madara::Cid::Deployment & solution = settings.solution;
  Madara::Cid::SolutionMap & lookup = settings.solution_lookup;

  deployment.resize (10);
  deployment[0].resize (10);
  averages.resize (10);

  for (unsigned int i = 0; i < 10; ++i)
  {
    deployment[0][i].first = 0;
    deployment[0][i].second = i;
  }

  for (unsigned int i = 0; i < 10; ++i)
  {
    Madara::Cid::LatencyVector & source_latencies = network_latencies[i];
    averages[i].first = i;

    for (unsigned int j = 0; j < 10; ++j)
    {
      if (network_latencies[i][j].second == 0)
      {
        Madara::Cid::LatencyVector & dest_latencies = network_latencies[j];

        // make the latencies bidirectionally equivalent for realism
        source_latencies[j].first = j;
        source_latencies[j].second = j + 1;
        dest_latencies[i].first = i;
        dest_latencies[i].second = source_latencies[j].second;
      }
      averages[i].second += source_latencies[j].second;
    }
    averages[i].second /= 10;

    std::sort (source_latencies.begin (), source_latencies.end ());
  }

  output << "\nPrinting the deployment\n\n";
  output << "  ";

  for (unsigned int i = 0; i < 10; ++i)
  {
    std::stringstream buffer;
    buffer << "0->" << deployment[0][i].second;

    output << std::setw (5) << buffer.str ();

    if (i != 9)
      output << " ";
    else
      output << "\n";
  }

  output << "\nPrinting the network latencies\n\n  ";

  for (unsigned int i = 0; i < 10; ++i)
  {
    Madara::Cid::LatencyVector & cur_latencies = network_latencies[i];

    for (unsigned int j = 0; j < 10; ++j)
    {
      output << "[" << i << "]" << "[" << j << "]=" <<
        std::setw (3) << cur_latencies[j].second;

      if (j % 5 == 4)
        output << "\n  ";
      else
        output << " ";
    }
  }

  output << "\nPrinting the averages\n";

  for (unsigned int i = 0; i < 10; ++i)
  {
    output << i << "->" << averages[i].second << " ";
  }

  output << "\n";

  Madara::Cid::generate_worst_solution (settings);

//  uint64_t actual_latency, ideal_latency = 0, worst_latency;

  alg_latencies[WORST_LATENCY] = Madara::Cid::calculate_latency (settings);

  Madara::Cid::LatencyVector & cur_latencies = network_latencies[0];

  for (unsigned int i = 0; i < 10; ++i)
  {
    alg_latencies[BEST_LATENCY] += cur_latencies[i].second;
  }

  output << "\nWorst case latency: " << alg_latencies[WORST_LATENCY] << "\n";
  output << "\nBest case latency: " << alg_latencies[BEST_LATENCY] << "\n";

  // Test CID

  Madara::Cid::generate_worst_solution (settings);
  lookup.clear ();

  Madara::Cid::approximate (settings);

  alg_latencies[CID] = 
    Madara::Cid::calculate_latency (settings);

  output << "Cid solution latency: " << alg_latencies[CID] << "\n";

  // Test Naive CID

  Madara::Cid::generate_worst_solution (settings);
  lookup.clear ();

  Madara::Cid::fill_by_highest_degree (settings);

  alg_latencies[NAIVE_CID] = Madara::Cid::calculate_latency (settings);

  output << "Naive Cid solution latency: " << alg_latencies[NAIVE_CID] << "\n";

  // Test Degreed-GA

  Madara::Cid::generate_worst_solution (settings);

  for (unsigned int i = 0; i < 10; ++i)
    Madara::Cid::ga_degree (settings, 5);

  alg_latencies[DEGREED_GA] = Madara::Cid::calculate_latency (settings);

  output << "Degreed GA solution latency: " << alg_latencies[DEGREED_GA] << "\n";

  // Test Naive-GA

  Madara::Cid::generate_worst_solution (settings);

  for (unsigned int i = 0; i < 10; ++i)
    Madara::Cid::ga_naive (settings, 5);

  alg_latencies[NAIVE_GA] = Madara::Cid::calculate_latency (settings);

  output << "Naive GA solution latency: " << alg_latencies[NAIVE_GA] << "\n";

  output << "The following algorithms are incorrect: \n";
  
  if (alg_latencies[WORST_LATENCY] != 100)
    output << "  generate_worst_solution\n";

  if (alg_latencies[CID] != 55)
    output << "  approximate (i.e. CID heuristic)\n";

  if (alg_latencies[NAIVE_CID] != 55)
    output << "  fill_by_highest_degree (i.e. Naive CID heuristic)\n";

  if (alg_latencies[NAIVE_GA] > 56)
    output << "  ga_naive (i.e. Naive Genetic Algorithm)\n";

  if (alg_latencies[DEGREED_GA] != 55)
    output << "  ga_degreed (i.e. Degreed Genetic Algorithm)\n";

  return;

  // *************************************************************

}

int main (int argc, char *argv[])
{
  MADARA_debug_level = 1;

  unsigned int begin = 1000;
  unsigned int end = 10000;
  unsigned int increment = 1000;
  unsigned int repeat = 1;
  std::string output_file ("test_cid_results.txt");

  for (int i = 1; i < argc; ++i)
  {
    std::string arg (argv[i]);
    std::stringstream buffer;

    if      (i + 1 < argc && arg == "-b" || arg == "--begin")
    {
      buffer << argv[i + 1];
      buffer >> begin;
      ++i;
    }
    else if (arg == "-c" || arg == "--csv")
    {
      // toggle the use_csv_format flag
      use_csv_format = !use_csv_format;
    }
    else if (i + 1 < argc && arg == "-e" || arg == "--end")
    {
      buffer << argv[i + 1];
      buffer >> end;
      ++i;
    }
    else if (i + 1 < argc && arg == "-i" || arg == "--increment")
    {
      buffer << argv[i + 1];
      buffer >> increment;
      ++i;
    }
    else if (i + 1 < argc && arg == "-o" || arg == "--output-file")
    {
      buffer << argv[i + 1];
      buffer >> output_file;
      ++i;
    }
    else if (i + 1 < argc && arg == "-r" || arg == "--repeat")
    {
      buffer << argv[i + 1];
      buffer >> repeat;
      ++i;
    }
    else if (i + 1 < argc && arg == "-t" || arg == "--max-time")
    {
      buffer << argv[i + 1];
      buffer >> ga_time;
      ++i;
    }
    else
    {
      buffer << "\nHelp for " << argv[0] << "\n\n";
      buffer << "  This application tests the CID heuristics and genetic\n";
      buffer << "  algorithms with a range of sizes and deployment types.\n\n";
      buffer << "Usage information for " << argv[0] << "\n\n";
      buffer << "  -b <value> || --begin <value>\n";
      buffer << "      First size to try. Default is 1000 (1,000)\n\n";
      buffer << "  -c || --csv\n";
      buffer << "      Toggle csv formatting (on by default)\n\n";
      buffer << "  -e <value> || --end <value>\n";
      buffer << "      Last size to try. Default is 10000 (10,000)\n\n";
      buffer << "  -h  || --help\n";
      buffer << "      Print this help menu\n\n";
      buffer << "  -i <value> || --increment <value>\n";
      buffer << "      Increment size by. Default is 1000 (1,000)\n\n";
      buffer << "  -o <filename> || --output-file <filename>\n";
      buffer << "      Print output to the filename.\n\n";
      buffer << "  -r <filename> || --repeat <filename>\n";
      buffer << "      Repeats every test a certain number of times.\n"
             << "      Useful when combined with --csv for results.\n\n";
      buffer << "  -t <value> || --max-time <value>\n";
      buffer << "      Maximum time to give genetic algorithms\n\n";

      std::cout << buffer.str ();
      exit (0);
    }
  }

  std::ofstream output (output_file.c_str ());

  // use ACE real time scheduling class
  int prio  = ACE_Sched_Params::next_priority
    (ACE_SCHED_FIFO,
     ACE_Sched_Params::priority_max (ACE_SCHED_FIFO),
     ACE_SCOPE_THREAD);
  ACE_OS::thr_setprio (prio);

  srand ((unsigned int) time (0));

  if (!use_csv_format)
    verify_algorithms (output);

  std::cout << "\nSaving results to " << output_file << "\n";

  if (use_csv_format)
  {
    output << "Test ID,Preparation (all),Preparation (self)";

    for (int i = MIN_TEST_RANGE; i < MAX_TEST_RANGE; ++i)
    {
      output << "," << testnames[i] << " (clock)";
      output << "," << testnames[i] << " (latency)";
    }
    output << "\n";
  }

  for (unsigned int i = begin; i <= end; i += increment)
  {
    std::cout << "  Running tests for size " << i << "\n";
    for (unsigned int j = 0; j < repeat; ++j)
      test_cid (i, output);
  }

  output.close ();

  return 0;
}