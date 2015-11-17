//
// Created by gaddra on 11/12/15.
//

#ifndef VMATCH_GRAPH_H
#define VMATCH_GRAPH_H

#include "../gssw/src/gssw.h"
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "../include/utils.h"

namespace vmatch {

class Graph {

 public:

  struct GraphParams {
    uint32_t maxNodeLen = 50000; // Maximum length of a single node graph
    int32_t ingroup = 100; // percent of individuals to include
    std::string region = ""; // Specify a specific region to build, a:b
    std::string buildfile = ""; // existing buildfile to build the complement
    bool genComplement = false; // Generate a graph with individuals not in buildfile
    bool maxAF = false; // Linear graph with only the variants/ref with maximum allele frequency
    int8_t *nt_table = NULL; // Table of nt mappings
    int8_t *mat = NULL; // table of scores
    int32_t match = 2, mismatch = 2; // default scores
    uint8_t gap_open = 3, gap_extension = 1; // default gap scores
  };

  Graph() {
    params.nt_table = gssw_create_nt_table();
    params.mat = gssw_create_score_matrix(params.match, params.mismatch);
  }

  Graph(GraphParams p) {
    params = p;
  }

  //  Build a graph from a FASTA and a VCF
  Graph(std::istream reference, std::istream vcf) {
    Graph();
    buildGraph(buildGraph(reference, vcf));
  }

  // Build a graph from a buildfile
  Graph(std::ifstream buildfile) {
    Graph();
    buildGraph(buildfile);
  }

  // Delete gssw graph
  ~Graph() {
    if (graph != NULL)
    gssw_graph_destroy(graph);
  }

  void exportDOT();
  void exportBuildfile(std::ofstream &out);
  std::iostream &buildGraph(std::istream &reference, std::istream &variants);
  void buildGraph(std::istream &buildfile);
  gssw_graph *getGSSWGraph() const {
    return graph;
  }
  GraphParams getParamsCopy() const {
    return params;
  }
  void setParams(GraphParams p) {
    params = p;
  }
  void setScores(int32_t m = 2, int32_t mm = 2, uint8_t go = 3, uint8_t ge = 1) {
    params.match = m;
    params.mismatch = mm;
    params.gap_extension = ge;
    params.gap_open = go;
  }


 protected:
  GraphParams params;
  gssw_graph *graph = NULL; // The graph

  void parseRegion(std::string region, uint32_t *min, uint32_t *max);

};
}
#endif //VMATCH_GRAPH_H
