//
// Created by gaddra on 12/28/16.
//

#ifndef VARGAS_SCORING_H
#define VARGAS_SCORING_H

#include "utils.h"
#include <vector>
#include <cstdint>
#include <cstdio>
#include <string>
#include <sstream>

namespace vargas {

  /**
   * @brief
   * Aligner scoring parameters
   */
  struct ScoreProfile {
      ScoreProfile() {};

      /**
       * @param match Match bonus
       * @param mismatch Mismatch penalty
       * @param gopen Read and ref gap open penalty
       * @param gext Read and ref gap extension penalty
       */
      ScoreProfile(uint8_t match, uint8_t mismatch, uint8_t gopen, uint8_t gext) :
      match(match), mismatch(mismatch), read_gopen(gopen), read_gext(gext), ref_gopen(gopen), ref_gext(gext) {}

      /**
       * @param match Match bonus
       * @param mismatch Mismatch penalty
       * @param rd_gopen Read gap open penalty
       * @param rd_gext Read gap extension penalty
       * @param ref_gopen Ref gap open penalty
       * @param ref_gext Ref gap extension penalty
       */
      ScoreProfile(uint8_t match, uint8_t mismatch,
                   uint8_t rd_gopen, uint8_t rd_gext, uint8_t ref_gopen, uint8_t ref_gext) :
      match(match), mismatch(mismatch),
      read_gopen(rd_gopen), read_gext(rd_gext), ref_gopen(ref_gopen), ref_gext(ref_gext) {}

      uint8_t match = 2, /**< Match bonus */
      mismatch = 2, /**< Mismatch penalty */
      read_gopen = 3, /**< Read gap open penalty */
      read_gext = 1, /**< Read gap extension penalty */
      ref_gopen = 3, /**< Ref gap open penalty */
      ref_gext = 1, /**< Ref gap extension penalty */
      ambig = 0; /**< Ambigious base penalty */
      size_t tol = 5; /**< Classify as correct if pos == target +- tol */
      bool end_to_end = false; /**< End to end alignment */

      std::string to_string() const;

      void from_string(std::string s);
  };

  /**
 * @brief
 * Aligner results
 */
  struct Results {
      std::vector<size_t> max_pos; /**< Best positions, 1 indexed */
      std::vector<size_t> sub_pos; /**< Second best positions, 1 indexed */

      std::vector<size_t> max_count; /**< # of max_score */
      std::vector<size_t> sub_count; /**< # of sub_score */

      std::vector<int> max_score; /**< Best scores */
      std::vector<int> sub_score; /**< Second best scores */
      std::vector<int> target_score; /**< Score at the target position */

      std::vector<uint8_t> correct; /**< 1 for target matching best score, 2 for matching sub score, else 0 */

      ScoreProfile profile;

      size_t size() const {
          return max_pos.size();
      }

      /**
       * @brief
       * Resize all result vectors.
       */
      void resize(size_t size);
  };

  const std::vector<std::string> supported_pgid = {"bowtie2", "bwa"};

  std::vector<std::string> tokenize_cl(std::string cl);

  ScoreProfile bwt2(const std::string &cl);

  ScoreProfile bwa_mem(const std::string &cl);

  ScoreProfile program_profile(const std::string &cl);

}

#endif //VARGAS_SCORING_H