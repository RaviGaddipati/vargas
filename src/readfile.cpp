/**
 * Ravi Gaddipati
 * November 24, 2015
 * rgaddip1@jhu.edu
 *
 * Wrapper for a reads file that loads reads and meta information
 * into a Read structure.
 *
 * readfile.cpp
 */

#include <string>
#include <fstream>
#include <stdexcept>
#include <vector>
#include <iostream>
#include "utils.h"
#include "readfile.h"

bool vargas::ReadFile::update_read() {
  if (!_read_file || !std::getline(_read_file, line)) return false;
  if (line.at(0) == '#') {
    header += "\n" + line;
    return update_read();
  }

  if (line.at(0) != '>') {
    read.read = line;
    read.desc = "-";
    read.end_pos = 0;
    read.indiv = -1;
    read.sub_err = -1;
    read.indel_err = -1;
    read.var_nodes = -1;
    read.var_bases = -1;
    read.read_num = seq_to_num(line);
  } else {
    split(line.substr(1, std::string::npos), ',', _split_meta);
    for (auto &s : _split_meta) {
      auto split_label = split(s, '=');
      std::string tag = split_label[0];
      std::string val = split_label[1];
      if (tag == READ_META_END) {
        read.end_pos = std::stoi(val);
      }
      else if (tag == READ_META_MUT) {
        read.sub_err = std::stoi(val);
      }
      else if (tag == READ_META_INDEL) {
        read.indel_err = std::stoi(val);
      }
      else if (tag == READ_META_VARNODE) {
        read.var_nodes = std::stoi(val);
      }
      else if (tag == READ_META_VARBASE) {
        read.var_bases = std::stoi(val);
      }
      else if (tag == READ_META_DESC) {
        read.desc = val;
      }
    }

    if (!std::getline(_read_file, line)) throw std::invalid_argument("No Read after FASTA read label.");
    read.read = line;
    read.read_num = seq_to_num(read.read);
  }
  return true;
}

void vargas::ReadFile::resume_from(std::string read) {
  do {
    if (!update_read()) {
      std::cerr << "Warning: read not found in reads file. Starting from beginning." << std::endl;
      _read_file.clear();
      _read_file.seekg(0, std::ios::beg);
      break;
    }
  } while (get_read().read != read);
}