//
// Created by Ravi Gaddipati on 9/12/15.
// Contains common functions.
//

#include "../include/utils.h"
/****
int exportDOT(int argc, char *argv[]) {
  using std::string;
  // Scores
  int32_t match = 2, mismatch = 2;

  // Graph score and conversion table
  int8_t *nt_table = gssw_create_nt_table();
  int8_t *mat = gssw_create_score_matrix(match, mismatch);

  string buildfile = "";
  gssw_graph *graph;

  GetOpt::GetOpt_pp args(argc, argv);
  if ((args >> GetOpt::OptionPresent('h', "help"))) {
    std::cout << std::endl << "------------------- VMatch export, " << __DATE__
        << ". rgaddip1@jhu.edu -------------------"
        << std::endl;
    std::cout << "-b\t--buildfile    Graph to export to DOT." << std::endl;
    std::cout << std::endl << "DOT file printed to stdout." << std::endl;
    return 0;
  }
  if (!(args >> GetOpt::Option('b', "buildfile", buildfile))) {
    std::cerr << "Error: No buildfile defined." << std::endl << std::endl;
    return 1;
  }
  graph = buildGraph(buildfile, nt_table, mat);
  graphToDot(graph);

  return 0;
}

gssw_graph *buildGraph(std::string buildfile, int8_t *nt_table, int8_t *mat) {
  using namespace std;

  string line;
  ifstream graphDat(buildfile.c_str());
  vector<string> lineSplit(0);
  vector<gssw_node *> nodes(0);
  uint32_t curr = 0;

  // Build nodes and edges from buildfile
  cerr << "Building graph..." << endl;
  while (getline(graphDat, line)) {
    if (line.at(0) != '#') {
      if (line.at(0) == '[') {
        line = line.substr(1, line.length() - 2);
        lineSplit = split(line, ',');
        for (uint32_t i = 0; i < lineSplit.size(); i++) {
          gssw_node_add_indiv(nodes.back(), strtol(lineSplit[i].c_str(), NULL, 10));
        }
      } else {
        lineSplit = split(line, ',');
        switch (lineSplit.size()) {
          case 3: // New node
            curr = uint32_t(strtol(lineSplit[1].c_str(), NULL, 10));
            nodes.push_back(gssw_node_create(int32_t(strtol(lineSplit[0].c_str(), NULL, 10)),
                                             curr,
                                             lineSplit[2].c_str(), nt_table, mat));
            break;

          case 2: // New edge
            gssw_nodes_add_edge(nodes.end()[strtol(lineSplit[0].c_str(), NULL, 10)],
                                nodes.end()[strtol(lineSplit[1].c_str(), NULL, 10)]);
            break;

          default:
            cerr << "Unexpected line in buildfile: " << endl << line << endl;
            break;
        }
      }
    }
  }

  // Add nodes to graph
  gssw_graph *graph = gssw_graph_create(uint32_t(nodes.size()));
  for (uint32_t n = 0; n < nodes.size(); n++) {
    gssw_graph_add_node(graph, nodes[n]);
  }

  graphDat.close();
  return graph;
}

*/

std::vector<std::string> split(const std::string &s, char delim) {
  /** Split string with delim, return a vector **/
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> newElems(0);

  if (s.length() == 0) {
    return newElems;
  }
  else if (s.length() == 1 && s.at(0) != delim) {
    newElems.push_back(s.substr(0, 1));
    return newElems;
  }
  while (std::getline(ss, item, delim)) {
    if (item.size() != 0) newElems.push_back(item);
  }
  return newElems;
}

void split(const std::string &s, char delim, std::vector<std::string> &vec) {
  /** Split string with delim, return a vector **/
  std::stringstream ss(s);
  std::string item;
  vec.clear();

  if (s.length() == 0) {
    return;
  }
  else if (s.length() == 1 && s.at(0) != delim) {
    vec.push_back(s.substr(0, 1));
    return;
  }
  while (std::getline(ss, item, delim)) {
    if (item.size() != 0) vec.push_back(item);
  }
}