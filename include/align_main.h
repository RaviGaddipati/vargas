/**
 * Ravi Gaddipati
 * Dec 23, 2016
 * rgaddip1@jhu.edu
 *
 * @brief
 * Main aligner interface
 *
 * @copyright
 * Distributed under the MIT Software License.
 * See accompanying LICENSE or https://opensource.org/licenses/MIT
 *
 * @file
 */

#ifndef VARGAS_ALIGN_MAIN_H
#define VARGAS_ALIGN_MAIN_H



// Assign all reads without a read group to a RG:ID of:
#define UNGROUPED_READGROUP "VAUGRP"

// Output alignment tags
#define ALIGN_SAM_MAX_POS_TAG "mp"
#define ALIGN_SAM_SUB_POS_TAG "sp"
#define ALIGN_SAM_SUB_SCORE_TAG "ss"
#define ALIGN_SAM_MAX_COUNT_TAG "mc"
#define ALIGN_SAM_SUB_COUNT_TAG "sc"
#define ALIGN_SAM_SUB_STRAND_TAG "st"
#define ALIGN_SAM_SUB_SEQ "su"
#define ALIGN_SAM_PG_GDF "gd"

#include "cxxopts.hpp"
#include "sam.h"
#include "graphman.h"

#include <stdexcept>


// Forward decl to prevent main.cpp recompilation for alignment.h changes
namespace vargas {
  class AlignerBase;
  struct ScoreProfile;
}

/**
 * Align given reads to specified target graphs.
 * @param argc command line argument count
 * @param argv command line arguments
 */
int align_main(int argc, char *argv[]);

/**
 * @brief
 * Align tasks to their graphs.
 * @param gm GraphMan hosting target graphs
 * @param task_list Parallel execution tasks
 * @param output SAM
 * @param aligners
 * @param fwdonly
 * @param primary
 * @param msonly
 * @param maxonly
 * @param notraceback
 * @param phred_offset
 */
void align(vargas::GraphMan &gm,
           std::vector<std::pair<std::string, std::vector<vargas::SAM::Record>>> &task_list,
           vargas::osam &out,
           std::ofstream &reportall_out,
           const std::vector<std::unique_ptr<vargas::AlignerBase>> &aligners,
           bool fwdonly, bool msonly, bool maxonly, bool notraceback, char phred_offset);

/**
 * @brief
 * Create a list of alignment jobs.
 * @param reads input read SAM stream
 * @param align_targets List of targets : RG:Subgraph
 * @param read_len Max readlen encountered
 * @param chunk_size Limit task size to N alignments
 * @return List of jobs of the form <subgraph label, [reads]>
 */
std::vector<std::pair<std::string, std::vector<vargas::SAM::Record>>>
create_tasks(vargas::isam &reads, std::string &align_targets, int chunk_size, size_t &read_len);

/**
 * @brief
 * Create a new aligner with given parameters
 * @param prof Score profile
 * @param node_len Maximum node length of graph
 * @param read_len Read length
 * @param use_wide use 16 bit cell elements instead of 8 bit
 * @param end_to_end End to end alignment
 * @return pointer to new aligner
 */
std::unique_ptr<vargas::AlignerBase>
make_aligner(const vargas::ScoreProfile &prof, size_t read_len, bool use_wide, bool msonly, bool maxonly);

/**
 * @brief
 * Load a FASTA or FASTQ file into a SAM structure
 * @param file FAST file name
 * @param fastq
 * @param ret
 * @param p64 Phred+64 encoding
 */
void load_fast(std::string &file, bool fastq, vargas::isam &ret, bool p64=false);

/**
 * Read file format type.
 */
enum class ReadFmt {SAM, FASTQ, FASTA};

/**
 * @brief
 * Identity read file type
 * @param filename
 * @return SAM, FASTA, or FASTQ
 */
ReadFmt read_fmt(const std::string& filename);

void align_help(const cxxopts::Options &opts);


#endif //VARGAS_ALIGN_MAIN_H
