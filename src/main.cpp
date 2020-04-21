/**
 * Ravi Gaddipati
 * November 25, 2015
 * rgaddip1@jhu.edu
 *
 * @brief
 * Interface for simulating and aligning reads from/to a DAG.
 *
 * @copyright
 * Distributed under the MIT Software License.
 * See accompanying LICENSE or https://opensource.org/licenses/MIT
 *
 * @file
 */

#ifndef DOCTEST_CONFIG_DISABLE
#define DOCTEST_CONFIG_IMPLEMENT // User controlled test execution
#endif


#include "main.h"
#include "align_main.h"
#include "graphman.h"
#include "threadpool.h"

#include <iostream>
#include <algorithm>
#include <scoring.h>
#include <mutex>

int main(int argc, char *argv[]) {
    srand(time(nullptr)); // Rand used in profiles and sim

    try {
        if (argc > 1) {
#ifndef DOCTEST_CONFIG_DISABLE
            if (!strcmp(argv[1], "test")) {
                doctest::Context doc(argc, argv);
                doc.setOption("abort-after", 5);
                doc.setOption("no-version", true);
                std::cerr << "Running tests...\n\n";
                return doc.run();
            } else
#endif
            if (!strcmp(argv[1], "profile")) {
                return profile(argc - 1, argv + 1);
            } else if (!strcmp(argv[1], "define")) {
                return define_main(argc - 1, argv + 1);
            } else if (!strcmp(argv[1], "sim")) {
                return sim_main(argc - 1, argv + 1);
            } else if (!strcmp(argv[1], "align")) {
                return align_main(argc - 1, argv + 1);
            } else if (!strcmp(argv[1], "convert")) {
                return convert_main(argc - 1, argv + 1);
            } else if (!strcmp(argv[1], "query")) {
                return query_main(argc - 1, argv + 1);
            }
        }
    } catch (std::exception &e) {
        std::cerr << "\033[1;31m"
                  << "\nFatal Error: " << e.what()
                  << "\033[0m\n" << std::endl;
        return 1;
    }

    std::cerr << "Define a valid mode of operation." << std::endl;
    main_help();
    return 1;

}

int define_main(int argc, char *argv[]) {
    std::string fasta_file, varfile, region, out_file, sample_filter, subdef;
    bool not_contig = false;
    size_t varlim = 0;

    cxxopts::Options opts("vargas define", "Define subgraphs deriving from a reference and VCF file.");
    try {
        opts.add_options("Input")
        ("f,fasta", "<str> *Reference FASTA filename.", cxxopts::value(fasta_file));

        opts.add_options("Optional")
        ("v,vcf", "<str> Variant file (vcf, vcf.gz, or bcf).", cxxopts::value<std::string>(varfile))
        ("t,out", "<str> Output filename. (default: stdout)", cxxopts::value(out_file))
        ("g,region", "<CHR[:MIN-MAX];...> list of regions. (default: all)", cxxopts::value(region))
        ("s,subgraph", "<str> Subgraph definitions, see below.", cxxopts::value(subdef))
        ("p,filter", "<str> Filter by sample names in file.", cxxopts::value(sample_filter))
        ("n,limvar", "<N> Limit to the first N variant records", cxxopts::value(varlim))
        ("c,notcontig", "VCF records for a given contig are not contiguous.", cxxopts::value(not_contig)->implicit_value("true"));

        opts.add_options()("h,help", "Display this message.");
        opts.parse(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        throw std::invalid_argument("Error parsing options."); }
    if (opts.count("h")) {
        define_help(opts);
        return 0;
    }
    if (!opts.count("f")) {
        define_help(opts);
        throw std::invalid_argument("FASTA file required.");
    }

    vargas::GraphMan gm;
    gm.print_progress();
    if (sample_filter.length()) {
        std::ifstream in(sample_filter);
        if (!in.good()) throw std::invalid_argument("Error opening file: \"" + sample_filter + "\"");
        std::stringstream ss;
        ss << in.rdbuf();
        sample_filter = ss.str();
    }

    std::vector<vargas::Region> region_vec;
    if (!region.empty()) {
        region.erase(std::remove_if(region.begin(), region.end(), isspace), region.end());
        region.erase(std::remove(region.begin(), region.end(), ','), region.end());
        auto v = rg::split(region, ';');
        std::transform(v.begin(), v.end(), std::back_inserter(region_vec), vargas::parse_region);
    }

    if (!not_contig) gm.assume_contig_chr();
    gm.create_base(fasta_file, varfile, region_vec, sample_filter, varlim);

    if (!subdef.empty()) {
        auto defs = rg::split(subdef, ';');
        for (auto &def : defs) {
            std::cerr << "Deriving subgraph \"" << def << "\"...\n";
            std::cerr << gm.at(gm.derive(def))->statistics() << '\n';
        }
    }

    std::cerr << "Writing to \"" << out_file << "\"...\n";
    gm.write(out_file);
    return 0;
}

struct main_helper {
    std::vector<std::pair<std::string, // Graph label
                          std::pair<std::string, // RG ID
                                    vargas::Sim::Profile>>> // sim prof
     &task_list;
    vargas::GraphMan &gm;
    int num_reads;
    std::mutex &m;
    vargas::osam &out;
};
void main_helper_func(void *data, long index, int) {
    main_helper &help = *(main_helper *)data;
    auto &task_list = help.task_list;
    auto &gm = help.gm;
    const std::string &label = task_list.at(index).first;
    auto subgraph_ptr = gm.at(label);
    vargas::Sim sim(*subgraph_ptr, task_list[index].second.second);
    auto results = sim.get_batch(help.num_reads, gm.resolver());
    for(auto &r: results) r.aux.set("RG", task_list[index].second.first);
    {
        std::lock_guard<std::mutex> lock(help.m);
        for(const auto &r: results) help.out.add_record(r);
    }
}

int sim_main(int argc, char *argv[]) {
    std::string cl;
    {
        std::ostringstream ss;
        for (int i = 0; i < argc; ++i) ss << std::string(argv[i]) << " ";
        cl = ss.str();
    }

    int read_len, num_reads, threads;
    std::string mut, indel, vnodes, vbases, gdf_file, out_file, sim_src;
    bool use_rate = false, sim_src_isfile = false;

    cxxopts::Options opts("vargas sim", "Simulate reads from genome graphs.");
    try {
        opts.add_options("Input")
        ("g,graph", "<str> *Graph definition file.", cxxopts::value(gdf_file));

        opts.add_options("Optional")
        ("t,out", "<str> Output file. (default: stdout)", cxxopts::value(out_file))
        ("s,sub", "<S1,...> Subgraphs to simulate from. (default: base)", cxxopts::value(sim_src))
        ("f,file", "-s specifies a filename.", cxxopts::value(sim_src_isfile))
        ("l,rlen", "<N> Read length.", cxxopts::value(read_len)->default_value("50"))
        ("n,numreads", "<N> Number of reads to generate.", cxxopts::value(num_reads)->default_value("1000"))
        ("j,threads", "<N> Number of threads.", cxxopts::value(threads)->default_value("1"));

        opts.add_options("Stratum")
        ("v,vnodes", "<N1,...> Variant nodes. \'*\' for any.", cxxopts::value(vnodes)->default_value("*"))
        ("b,vbases", "<N1,...> Variant bases. \'*\' for any.", cxxopts::value(vbases)->default_value("*"))
        ("m,mut", "<N1,...> Mutations. \'*\' for any.", cxxopts::value(mut)->default_value("0"))
        ("i,indel", "<N1,...> Insertions/deletions. \'*\' for any.", cxxopts::value(indel)->default_value("0"))
        ("a,rate", "Interpret -m, -i as rates.", cxxopts::value(use_rate));
        opts.add_options()
        ("h,help", "Display this message.");
        opts.parse(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        throw std::invalid_argument("Error parsing options."); }
    if (opts.count("h")) {
        sim_help(opts);
        return 0;
    }
    if (!opts.count("g")) {
        sim_help(opts);
        throw std::invalid_argument("Graph definition file required.");
    }

    vargas::SAM::Header sam_hdr;
    {
        vargas::SAM::Header::Program pg;
        pg.command_line = cl;
        pg.name = "vargas_sim";
        pg.id = "VS";
        pg.version = __DATE__;
        std::replace_if(pg.version.begin(), pg.version.end(), isspace, ' '); // rm tabs
        sam_hdr.add(pg);
    }

    vargas::GraphMan gm;

    const std::vector<std::string>
    mut_split = rg::split(mut, ','),
    indel_split = rg::split(indel, ','),
    vnode_split = rg::split(vnodes, ','),
    vbase_split = rg::split(vbases, ',');

    if (sim_src_isfile) {
        std::ifstream in(sim_src);
        if (!in.good()) throw std::invalid_argument("Error opening file: \"" + sim_src + "\".");
        std::stringstream ss;
        ss << in.rdbuf();
        sim_src = ss.str();
    }

    std::cerr << "Loading base graph... " << std::flush;
    auto start_time = std::chrono::steady_clock::now();
    gm.open(gdf_file);
    std::cerr << rg::chrono_duration(start_time) << " seconds." << std::endl;

    std::vector<std::string> subdef_split;
    if (sim_src.length() == 0) {
        subdef_split.emplace_back("base");
    } else {
        std::replace(sim_src.begin(), sim_src.end(), '\n', ',');
        sim_src.erase(std::remove_if(sim_src.begin(), sim_src.end(), isspace), sim_src.end());
        subdef_split = rg::split(sim_src, ',');

        // validate graph labels
        for (const std::string &l : subdef_split) {
            gm.at(l);
        }

    }

    std::cerr << "Building profiles... " << std::flush;
    start_time = std::chrono::steady_clock::now();

    // Map a graph label to a vector of read group ID's/profiles
    std::unordered_map<std::string, // Graph label
                       std::vector<std::pair<std::string, // Read Group ID
                                             vargas::Sim::Profile>>> // Sim profile
    queue;

    int rg_id = 0;
    vargas::SAM::Header::ReadGroup rg;
    rg.seq_center = "vargas_sim";
    rg.date = rg::current_date();
    rg.aux.set(SIM_SAM_GRAPH_TAG, gdf_file);

    vargas::Sim::Profile prof;
    prof.len = read_len;
    prof.rand = use_rate;
    for (const std::string &vbase : vbase_split) {
        for (const std::string &vnode : vnode_split) {
            for (const std::string &ind : indel_split) {
                for (const std::string &m : mut_split) {
                    try {
                        prof.mut = m == "*" ? -1 : std::stof(m);
                        prof.indel = ind == "*" ? -1 : std::stof(ind);
                        prof.var_bases = vbase == "*" ? -1 : std::stoi(vbase);
                        prof.var_nodes = vnode == "*" ? -1 : std::stoi(vnode);
                    } catch (std::exception &e) {
                        std::cerr << "Invalid profile argument: " << e.what() << std::endl;
                        return 1;
                    }

                    rg.aux.set(SIM_SAM_INDEL_ERR_TAG, prof.indel);
                    rg.aux.set(SIM_SAM_VAR_NODES_TAG, prof.var_nodes);
                    rg.aux.set(SIM_SAM_VAR_BASE_TAG, prof.var_bases);
                    rg.aux.set(SIM_SAM_SUB_ERR_TAG, prof.mut);
                    rg.aux.set(SIM_SAM_USE_RATE_TAG, (int) prof.rand);

                    // Each profile and subgraph combination is a unique set of reads
                    for (const std::string &p : subdef_split) {
                        rg.aux.set(SIM_SAM_SRC_TAG, p);
                        rg.id = std::to_string(++rg_id);
                        sam_hdr.add(rg);
                        queue[p].emplace(queue[p].end(), rg.id, prof);
                    }
                }
            }
        }
    }
    std::cerr << rg::chrono_duration(start_time)
              << " seconds.\n"
              << sam_hdr.read_groups.size() << " read group(s) over "
              << subdef_split.size() << " subgraph(s). " << std::endl;

    vargas::osam out(out_file, sam_hdr);
    if (!out.good()) throw std::invalid_argument("Error opening output file \"" + out_file + "\"");

    std::cerr << "Simulating... " << std::flush;

    start_time = std::chrono::steady_clock::now();

    std::vector<std::pair<std::string, // Graph label
                          std::pair<std::string, // RG ID
                                    vargas::Sim::Profile>>> // sim prof
    task_list;

    for (auto & k : subdef_split) {
        for (size_t i = 0; i < queue.at(k).size(); ++i) {
            auto &p = queue.at(k).at(i);
            task_list.emplace_back(k, std::pair<std::string, vargas::Sim::Profile>(p.first, p.second));
        }
    }

    const size_t num_tasks = task_list.size();
    rg::ForPool fp(threads);
    std::mutex mutex;
    main_helper data{task_list, gm, num_reads, mutex, out};
    fp.forpool(&main_helper_func, (void *)&data, num_tasks);

    std::cerr << rg::chrono_duration(start_time) << " seconds." << std::endl;

    return 0;
}

int convert_main(int argc, char **argv) {
    std::string sam_file, format;
    std::vector<std::string> files;
    cxxopts::Options opts("vargas convert", "Export a SAM file as a CSV file.");
    try {
        opts.add_options()
        ("f,format", "<str> Output format.", cxxopts::value<std::string>(format))
        ("files", "SAM files, default stdin.", cxxopts::value<std::vector<std::string>>(files))
        ("h,help", "Display this message.");
        opts.parse_positional(std::vector<std::string>{"files"});
        opts.parse(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        throw std::invalid_argument("Error parsing options: " + std::string(e.what())); }
    if (opts.count("h")) {
        convert_help(opts);
        return 0;
    }
    if (format.empty()) {
        convert_help(opts);
        throw std::invalid_argument("Format specifier required.");
    }

    auto start_time = std::chrono::steady_clock::now();

    format.erase(std::remove(format.begin(), format.end(), ' '), format.end());
    std::vector<std::string> fmt_split = rg::split(format, ',');
    std::unordered_set<std::string> warned;

    if (files.empty()) files.resize(1);

    for (const auto &f : files) {
        vargas::isam input(f);
        std::string buff, val;
        do {
            if (files.size() > 1) buff = f + ",";
            else buff = "";
            for (auto &tag : fmt_split) {
                val = "*";
                if (!input.record().get(input.header(), tag, val) && warned.count(tag) == 0) {
                    std::cerr << "WARN: Tag \"" << tag << "\" not present." << std::endl;
                    warned.insert(tag);
                }
                buff += "\"" + val + "\",";
            }
            buff.resize(buff.size() - 1);
            std::cout << buff << '\n'; // Crop trailing comma
        } while (input.next());
    }

    std::cerr << rg::chrono_duration(start_time) << " seconds." << std::endl;

    return 0;
}

int profile(int argc, char *argv[]) {
    std::string bcf, fasta, region;
    size_t  ingroup;

    cxxopts::Options opts("vargas profile", "Run profiles.");
    try {
        opts.add_options()
        ("f,fasta", "<str> *Reference FASTA.", cxxopts::value(fasta))
        ("v,vcf", "<str> *Variant file (vcf, vcf.gz, or bcf)", cxxopts::value(bcf))
        ("g,region", R"(<str> *Region of format "CHR:MIN-MAX". "CHR:0-0" for all.)", cxxopts::value(region))
        ("i,ingroup", "<N> Ingroup percentage.", cxxopts::value(ingroup)->default_value("100"))
        ("h,help", "Display this message.");
        opts.parse(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        throw std::invalid_argument("Error parsing options: " + std::string(e.what())); }
    if (opts.count("h")) {
        profile_help(opts);
        return 0;
    }
    if (!opts.count("f")) {
        profile_help(opts);
        throw std::invalid_argument("FASTA file required.");
    }


    vargas::GraphFactory gb(fasta);
    gb.open_vcf(bcf);
    gb.set_region(region);

    auto start = std::clock();

    std::cerr << "Initial Graph Build:\n\t";
    vargas::Graph g;
    gb.build(g);
    std::vector<bool> filter;
    for (size_t i = 0; i < g.pop_size(); ++i) filter.push_back(rand() % 100 > 95); //TODO what is this doing?
    std::cerr << (std::clock() - start) / (double) (CLOCKS_PER_SEC) << " s, " << "Nodes: " << g.node_map()->size()
              << std::endl;

    size_t num = 0;

    {
        std::cerr << "Insertion order traversal:\n\t";
        start = std::clock();
        for (auto i = g.begin(); i != g.end(); ++i) {
            ++num;
        }
        std::cerr << (std::clock() - start) / (double) (CLOCKS_PER_SEC) << " s" << std::endl;
    }

    {
        std::cerr << "Filter constructor (" << ingroup << "):\n\t";
        auto pop_filt = g.subset(ingroup);
        start = std::clock();
        vargas::Graph g2(g, pop_filt);
        std::cerr << (std::clock() - start) / (double) (CLOCKS_PER_SEC) << " s" << std::endl;
    }

    {
        std::cerr << "REF constructor:\n\t";
        start = std::clock();
        vargas::Graph g2(g, vargas::Graph::Type::REF);
        std::cerr << (std::clock() - start) / (double) (CLOCKS_PER_SEC) << " s" << std::endl;
    }

    {
        std::cerr << "MAXAF constructor:\n\t";
        start = std::clock();
        vargas::Graph g2(g, vargas::Graph::Type::MAXAF);
        std::cerr << (std::clock() - start) / (double) (CLOCKS_PER_SEC) << " s" << std::endl;
    }

    return 0;
}

int query_main(int argc, char *argv[]) {
    std::string gdef, dot, stat, meta, out;

    cxxopts::Options opts("vargas query", "Query a graph and export a DOT graph.");
    try {
        opts.add_options()
        ("g,graph", "*<str> Graph file to query.", cxxopts::value(gdef))
        ("d,dot", "<str> Subgraph to export as a DOT graph.", cxxopts::value(dot))
        ("t,out", "<str> DOT output file.", cxxopts::value(out)->default_value("stdout"))
        ("a,stat", "<str> Print statistics about a subgraph \'-\' for all.", cxxopts::value(stat)->implicit_value("-"))
        ("h,help", "Display this message.");
        opts.parse(argc, argv);
    } catch (std::exception &e) {
        std::cerr << e.what() << '\n';
        throw std::invalid_argument("Error parsing options: " + std::string(e.what())); }
    if (opts.count("h")) {
        query_help(opts);
        return 0;
    }

    if (!opts.count("g")) {
        query_help(opts);
        throw std::invalid_argument("No graph specified.");
    }

    vargas::GraphMan gg;
    if (!dot.empty() || stat.size()) gg.open(gdef);
    else gg.open(gdef);

    if (!dot.empty()) {
        if (out == "stdout") std::cout << gg.at(dot)->to_DOT(dot);
        else gg.at(dot)->to_DOT(out, dot);
    }

    if (!stat.empty()) {
        if (stat == "-") {
            for (const auto& lab : gg.labels()) std::cerr << lab << " : " << gg.at(lab)->statistics() << '\n';
        }
        else std::cerr << gg.at(stat)->statistics() << '\n';
    }

    if (!meta.empty()) {
        throw std::domain_error("Not implemented.");
    }

    return 0;

}

void main_help() {
    using std::cerr;
    using std::endl;
    cerr << "\nVargas version " << VARGAS_VERSION << " \nby Ravi Gaddipati, Charlotte Darby, Daniel Baker, Ben Langmead (langmea@cs.jhu.edu, www.langmead-lab.org)\n";
    cerr << "\tdefine          Define a set of graphs for use with sim and align.\n";
    cerr << "\tsim             Simulate reads from a set of graphs.\n";
    cerr << "\talign           Align reads to a set of graphs.\n";
    cerr << "\tconvert         Convert a SAM file to a CSV file.\n";
    cerr << "\tquery           Convert a graph to DOT format.\n";
    cerr << "\ttest            Run unit tests.\n\n";

}

void query_help(const cxxopts::Options &opts) {
    using std::cerr;
    using std::endl;
    cerr << opts.help() << endl;
}

void define_help(const cxxopts::Options &opts) {
    using std::cerr;
    using std::endl;

    cerr << opts.help(opts.groups()) << "\n\n"
         << "Subgraphs are defined using the format \"label=N[%]\",\n"
         << "where \'N\' is the number of samples or percentage of samples to select.\n"
         << "The samples are selected from the parent graph, scoped with \':\'.\n"
         << "The BASE graph is implied as the root for all labels. Example:\n"
         << "\ta=50;a:b=10%;a:c=5\n\n";
}

void profile_help(const cxxopts::Options &opts) {
    using std::cerr;
    using std::endl;
    cerr << opts.help() << "\n" << endl;
}

void sim_help(const cxxopts::Options &opts) {
    using std::cerr;
    using std::endl;

    cerr << opts.help(opts.groups()) << "\n\n";
    cerr << "-n reads are produced for each -m, -i, -v, -b combination.\nIf set to \'*\', any value is accepted.\n"
         << endl;
}

void convert_help(const cxxopts::Options &opts) {
    using std::cerr;
    using std::endl;

    cerr << opts.help() << "\n\n";
    cerr << "Required column names:\n\tQNAME, FLAG, RNAME, POS, MAPQ, CIGAR, RNEXT, PNEXT, TLEN, SEQ, QUAL\n";
    cerr << "Prefix with \"RG:\" to obtain a value from the associated read group.\n";
    cerr << "Ex. vargas convert -f \"RG:ID,ms\" a.sam b.sam" << endl;

}

TEST_CASE("Vargas CLI") {
    const std::string fa = R"(>chrA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
>chrB
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC
)";
    const std::string vcf = R"(##fileformat=VCFv4.0
##contig=<ID=chrA>
##contig=<ID=chrB>
##INFO=<ID=AF,Number=A,Type=Float,Description="Allele Frequency">
#CHROM	POS	ID	REF	ALT	QUAL	FILTER	INFO	FORMAT
chrA	40	rs775809821	A	T,C	.	.	.	.
chrA	41	rs768019142	A	TA,G	.	.	.	.
chrA	60	rs62651026	A	T,A	.	.	.	.
chrB	40	rs376007522	C	T,G	.	.	.	.
chrB	80	rs796688738	C	AC,<CN0>	.	.	.	.)";
    {
        std::ofstream o("tmpfa.vatmp");
        o << fa;
    }
    {
        std::ofstream o("tmpvcf.vatmp");
        o << vcf;
    }

    {
        // vargas define -f tmpfa.vatmp -v tmpvcf.vatmp -t tmpgdef.vatmp
        const int argc = 8;
        const char *argv[] = {"vargas", "define", "-f", "tmpfa.vatmp", "-v", "tmpvcf.vatmp", "-t", "tmpgdef.vatmp"};
        define_main(argc, (char **) argv);
        vargas::GraphMan gm("tmpgdef.vatmp");
        REQUIRE(gm.labels().size() == 1);
        CHECK(gm.labels().at(0) == "base");

        auto stat = gm.at("base")->statistics();
        CHECK(stat.total_length == 320);
        CHECK(stat.num_nodes == 20);
        CHECK(stat.num_edges == 31);
        CHECK(stat.num_snps == 6);
        CHECK(stat.num_dels == 1);
    }

    {
        // vargas sim -g tmpgdef.vatmp -t tmpreads.vatmp -n 1 -l 10 -v 0,1 -m 0,1 -i 0,1
        const int argc = 16;
        const char *argv[] = {"vargas", "sim", "-g", "tmpgdef.vatmp", "-t", "tmpreads.vatmp", "-n", "1", "-l", "10", "-v", "0,1", "-m", "0,1", "-i", "0,1"};
        sim_main(argc, (char **) argv);
        vargas::isam in("tmpreads.vatmp");
        REQUIRE(in.header().read_groups.size() == 8);
        size_t cnt = 0;
        do { ++cnt; } while (in.next());
        CHECK(cnt == 8);
    }

    {
        // vargas align -g tmpgdef.vatmp -U tmpreads.vatmp -S tmpreads.vatmp -f
        const int argc = 9;
        const char *argv[] = {"vargas", "align", "-g", "tmpgdef.vatmp", "-U", "tmpreads.vatmp", "-S", "tmpsam.vatmp", "-f"};
        align_main(argc, (char **) argv);
        vargas::isam in("tmpsam.vatmp");
        REQUIRE(in.header().read_groups.size() == 8);
        const auto &hd = in.header();
        do {
            const auto &rec = in.record();
            unsigned ni, se, score;
            REQUIRE(rec.get(hd, "ni", ni) == true);
            REQUIRE(rec.get(hd, "se", se) == true);
            if (ni == 0 && se == 0) {
                REQUIRE(rec.get(hd, "AS", score) == true);
                CHECK(score == 20);
            }

        } while (in.next());
    }


    remove("tmpfa.vatmp");
    remove("tmpfa.vatmp.fai");
    remove("tmpvcf.vatmp");
    remove("tmpgdef.vatmp");
    remove("tmpreads.vatmp");
    remove("tmpsam.vatmp");
}
