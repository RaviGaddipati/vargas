/**
 * @author Ravi Gaddipati (rgaddip1@jhu.edu)
 * @date Nov 4, 2016
 *
 * @brief
 * Provides a C++ wrapper for htslib handling SAM/BAM files.
 * @details
 * Both file types are handled transparently by htslib. Records
 * are loaded into Reads.
 *
 * Implementation.
 *
 * @file
 */

#include "sam.h"
void vargas::SAM::Optional::add(std::string a) {
    if (a.at(2) != ':') return;
    if (a.at(4) != ':') return;
    std::string tag = a.substr(0, 2);
    aux[tag] = a.substr(5);
    aux_fmt[tag] = a.at(3);
}

void vargas::SAM::Optional::set(std::string tag, char val) {
    aux[tag] = std::string(1, val);
    aux_fmt[tag] = 'A';
}

void vargas::SAM::Optional::set(std::string tag, int val) {
    aux[tag] = std::to_string(val);
    aux_fmt[tag] = 'i';
}

void vargas::SAM::Optional::set(std::string tag, float val) {
    aux[tag] = std::to_string(val);
    aux_fmt[tag] = 'f';
}

void vargas::SAM::Optional::set(std::string tag, std::string val) {
    aux[tag] = val;
    aux_fmt[tag] = 'Z';
}

bool vargas::SAM::Optional::get(std::string tag, char &val) const {
    if (aux.count(tag) == 0 || aux_fmt.at(tag) != 'A') return false;
    val = aux.at(tag).at(0);
    return true;
}

bool vargas::SAM::Optional::get(std::string tag, int &val) const {
    if (aux.count(tag) == 0 || aux_fmt.at(tag) != 'i') return false;
    val = std::stoi(aux.at(tag));
    return true;
}

bool vargas::SAM::Optional::get(std::string tag, float &val) const {
    if (aux.count(tag) == 0 || aux_fmt.at(tag) != 'f') return false;
    val = std::stof(aux.at(tag));
    return true;
}

bool vargas::SAM::Optional::get(std::string tag, std::string &val) const {
    if (aux.count(tag) == 0) return false;
    val = aux.at(tag);
    return true;
}

std::string vargas::SAM::Optional::to_string() const {
    std::ostringstream ss;
    for (auto &pair : aux) {
        ss << '\t' << pair.first
           << ':' << aux_fmt.at(pair.first)
           << ':' << pair.second;
    }
    return ss.str();
}

std::string vargas::SAM::Header::Sequence::to_string() const {
    std::ostringstream ss;
    ss << "@SQ" <<
       "\tSN:" << name <<
       "\tLN:" << std::to_string(len) <<
       (genome_assembly.length() > 0 ? "\tAS:" + genome_assembly : "") <<
       (md5.length() > 0 ? "\tM5:" + md5 : "") <<
       (species.length() > 0 ? "\tSP:" + species : "") <<
       (URI.length() > 0 ? "\tUR:" + URI : "") << aux.to_string();
    return ss.str();
}

void vargas::SAM::Header::Sequence::parse(std::string line) {
    genome_assembly = "";
    md5 = "";
    species = "";
    URI = "";
    aux.clear();

    std::vector<std::string> tags = split(line, '\t');
    for (auto &p : tags) {
        std::string tag = p.substr(0, 2);
        std::string val = p.substr(3);
        if (tag == "SN") {
            name = val;
        } else if (tag == "LN") {
            len = std::stoi(val);
        } else if (tag == "AS") {
            genome_assembly = val;
        } else if (tag == "M5") {
            md5 = val;
        } else if (tag == "SP") {
            species = val;
        } else if (tag == "UR") {
            URI = val;
        } else {
            aux.add(p);
        }
    }
}

std::string vargas::SAM::Header::ReadGroup::to_string() const {
    std::ostringstream ss;
    ss << "@RG" <<
       "\tID:" << id <<
       (seq_center.length() > 0 ? "\tCN:" + seq_center : "") <<
       (desc.length() > 0 ? "\tDS:" + desc : "") <<
       (date.length() > 0 ? "\tDT:" + date : "") <<
       (flow_order.length() > 0 ? "\tFO:" + flow_order : "") <<
       (key_seq.length() > 0 ? "\tKS:" + key_seq : "") <<
       (library.length() > 0 ? "\tLB:" + library : "") <<
       (programs.length() > 0 ? "\tPG:" + programs : "") <<
       (insert_size.length() > 0 ? "\tPI:" + insert_size : "") <<
       (platform.length() > 0 ? "\tPL:" + platform : "") <<
       (platform_model.length() > 0 ? "\tPM:" + platform_model : "") <<
       (platform_unit.length() > 0 ? "\tPU:" + platform_unit : "") <<
       (sample.length() > 0 ? "\tSM:" + sample : "") << aux.to_string();
    return ss.str();
}

void vargas::SAM::Header::ReadGroup::parse(std::string line) {
    seq_center = "";
    desc = "";
    date = "";
    flow_order = "";
    key_seq = "";
    library = "";
    programs = "";
    insert_size = "";
    platform = "";
    platform_model = "";
    platform_unit = "";
    sample = "";
    aux.clear();

    std::vector<std::string> tags = split(line, '\t');
    for (auto &p : tags) {
        std::string tag = p.substr(0, 2);
        std::string val = p.substr(3);
        if (tag == "ID") {
            id = val;
        } else if (tag == "CN") {
            seq_center = val;
        } else if (tag == "DS") {
            desc = val;
        } else if (tag == "DT") {
            date = val;
        } else if (tag == "FO") {
            flow_order = val;
        } else if (tag == "KS") {
            key_seq = val;
        } else if (tag == "LB") {
            library = val;
        } else if (tag == "PG") {
            programs = val;
        } else if (tag == "PI") {
            insert_size = val;
        } else if (tag == "PL") {
            platform = val;
        } else if (tag == "PM") {
            platform_model = val;
        } else if (tag == "PU") {
            platform_unit = val;
        } else if (tag == "SM") {
            sample = val;
        } else {
            aux.add(p);
        }
    }
}

std::string vargas::SAM::Header::Program::to_string() const {
    std::ostringstream ss;
    ss << "@PG" <<
       "\tID:" << id <<
       (name.length() > 0 ? "\tPN:" + name : "") <<
       (command_line.length() > 0 ? "\tCL:" + command_line : "") <<
       (prev_pg.length() > 0 ? "\tPP:" + prev_pg : "") <<
       (desc.length() > 0 ? "\tDS:" + desc : "") <<
       (version.length() > 0 ? "\tVN:" + version : "") << aux.to_string();
    return ss.str();
}

void vargas::SAM::Header::Program::parse(std::string line) {
    name = "";
    command_line = "";
    prev_pg = "";
    desc = "";
    version = "";
    aux.clear();

    std::vector<std::string> tags = split(line, '\t');
    for (auto &p : tags) {
        std::string tag = p.substr(0, 2);
        std::string val = p.substr(3);
        if (tag == "ID") {
            id = val;
        } else if (tag == "PN") {
            name = val;
        } else if (tag == "CL") {
            command_line = val;
        } else if (tag == "PP") {
            prev_pg = val;
        } else if (tag == "DS") {
            desc = val;
        } else if (tag == "VN") {
            version = val;
        } else {
            aux.add(p);
        }
    }
}

std::string vargas::SAM::Header::to_string() const {
    std::ostringstream ret;
    ret << "@HD" <<
        "\tVN:" << version <<
        ((sorting_order.length() > 0) ? std::string("\tSO:") + sorting_order : "") <<
        ((grouping.length() > 0) ? std::string("\tGO:") + grouping : "") <<
        "\n";
    for (auto &seq : sequences) {
        ret << seq.second.to_string() << "\n";
    }
    for (auto &rg : read_groups) {
        ret << rg.second.to_string() << "\n";
    }
    for (auto &pg : programs) {
        ret << pg.second.to_string() << "\n";
    }
    return ret.str();
}

void vargas::SAM::Header::parse(std::string hdr) {
    sorting_order = "";
    grouping = "";
    sequences.clear();
    read_groups.clear();
    programs.clear();

    std::vector<std::string> lines = split(hdr, '\n');

    // @HD line
    std::vector<std::string> tags = split(lines[0], '\t');
    if (tags[0] != "@HD") throw std::invalid_argument("First line must start with \"@HD\"");
    for (auto &p : tags) {
        std::string tag = p.substr(0, 2);
        std::string val = p.substr(3);
        if (tag == "VN") {
            version = val;
        } else if (tag == "SO") {
            sorting_order = val;
        } else if (tag == "GO") {
            grouping = val;
        }
    }

    // Other lines
    for (auto &l : lines) {
        std::string tag = l.substr(0, 3);
        if (tag == "@SQ") add(Sequence(l));
        else if (tag == "@RG") add(ReadGroup(l));
        else if (tag == "@PG") add(Program(l));
    }
}

int vargas::SAM::Record::Flag::encode() const {
    return (multiple ? 0x001 : 0) +
        (aligned ? 0x002 : 0) +
        (unmapped ? 0x004 : 0) +
        (next_unmapped ? 0x008 : 0) +
        (rev_complement ? 0x010 : 0) +
        (next_rev_complement ? 0x020 : 0) +
        (first ? 0x040 : 0) +
        (last ? 0x080 : 0) +
        (secondary ? 0x100 : 0) +
        (pass_fail ? 0x200 : 0) +
        (duplicate ? 0x400 : 0) +
        (supplementary ? 0x800 : 0);
}

void vargas::SAM::Record::Flag::decode(unsigned int f) {
    multiple = ((f & 0x001) != 0u);
    aligned = ((f & 0x002) != 0u);
    unmapped = ((f & 0x004) != 0u);
    next_unmapped = ((f & 0x008) != 0u);
    rev_complement = ((f & 0x010) != 0u);
    next_rev_complement = ((f & 0x020) != 0u);
    first = ((f & 0x040) != 0u);
    last = ((f & 0x080) != 0u);
    secondary = ((f & 0x100) != 0u);
    pass_fail = ((f & 0x200) != 0u);
    duplicate = ((f & 0x400) != 0u);
    supplementary = ((f & 0x800) != 0u);
}

std::string vargas::SAM::Record::to_string() const {
    std::ostringstream ss;
    ss << query_name << '\t'
       << flag.encode() << '\t'
       << ref_name << '\t'
       << pos << '\t'
       << mapq << '\t'
       << cigar << '\t'
       << ref_next << '\t'
       << pos_next << '\t'
       << tlen << '\t'
       << seq << '\t'
       << qual << aux.to_string();
    return ss.str();
}

void vargas::SAM::Record::parse(std::string line) {
    std::vector<std::string> cols = split(line, '\t');
    if (cols.size() < 11) throw std::invalid_argument("Record should have at least 11 columns");
    query_name = cols[0];
    flag = std::stoi(cols[1]);
    ref_name = cols[2];
    pos = std::stoi(cols[3]);
    mapq = std::stoi(cols[4]);
    cigar = cols[5];
    ref_next = cols[6];
    pos_next = std::stoi(cols[7]);
    tlen = std::stoi(cols[8]);
    seq = cols[9];
    qual = cols[10];
    aux.clear();

    // Aux fields
    for (size_t i = 11; i < cols.size(); ++i) {
        aux.add(cols[i]);
    }

}

void vargas::isam::open(std::string file_name) {
    close();
    if (file_name.length() == 0) _use_stdio = true;
    else {
        _use_stdio = false;
        in.open(file_name);
        if (!in.good()) throw std::invalid_argument("Error opening file \"" + file_name + "\"");
    }
    std::ostringstream hdr;
    while (std::getline((_use_stdio ? std::cin : in), _curr_line) && _curr_line.at(0) == '@') {
        hdr << _curr_line << '\n';
    }
    if (hdr.str().length() > 0) _hdr << hdr.str();
    _pprec << _curr_line;
}

void vargas::osam::open(std::string file_name) {
    close();
    if (file_name.length() == 0) _use_stdio = true;
    else {
        _use_stdio = false;
        out.open(file_name);
        if (!out.good()) throw std::invalid_argument("Error opening output file \"" + file_name + "\"");
    }
    (_use_stdio ? std::cout : out) << _hdr.to_string() << std::flush;
}