import sys
import pprint as pp

CHROM = str(22)
CHROM_LEN = str(51304566)

def load_ksnps(filename):
	'''
	Map position to a tuple of REF and ALTs
	POS : (REF, [(ALT, AF), ...])
	'''
	alts = {}
	with open(filename, 'r') as f:
		for line in f:
			s = line.split()
			if len(s) != 8:
				print("Invalid number of fields (8) in line:")
				print(line)
				exit(1)
			pos = int(s[1])
			ref = s[2]
			alt = s[3]
			af = float(s[4])
			if pos in alts:
				alts[pos][1].append(tuple((alt, af)))
			else:
				alts[pos] = tuple((ref, [tuple((alt,af))]))
	return alts

def to_vcf_record(ksnps, pos, num_samples):
	line = ""
	line += CHROM + '\t' + str(pos) + '\t' + '*' + '\t' + str(ksnps[pos][0]) + '\t'
	af_str = ""
	gt_str = "0|0\t"
	for i in range(len(ksnps[pos][1])):
		line += ksnps[pos][1][i][0]
		af_str += str(ksnps[pos][1][i][1])
		gt_str += str(i+1) + '|' + str(i+1)
		if i != len(ksnps[pos][1]) - 1:
			line += ','
			af_str += ','
			gt_str += '\t'
	for i in range(len(ksnps[pos][1]), num_samples, 1):
		gt_str += '\t0|0'

	line += '\t' + '100' + '\t' + 'PASS' + '\t' + "AF=" + af_str + '\tGT\t' + gt_str

	return line	

def main():
	'''
	python ksnp_to_vcf.py snps.ksnp sortfile 1,2,4,8,16 output
	'''

	if (len(sys.argv) < 5):
		print("Format should be:")
		print("python ksnp_to_vcf.py snps.ksnp sortfile 1,2,4,8,16 output")
		exit(1)

	ksnp_file = sys.argv[1]
	ksnp_sort = None

	ksnp_sort = sys.argv[2]

	num_k = sorted([int(i) for i in sys.argv[3].split(',')])

	prefix = sys.argv[4]

	ksnps = load_ksnps(ksnp_file)

	max_alts = 0
	for k in ksnps:
		if len(ksnps[k][1]) > max_alts:
			max_alts = len(ksnps[k][1])

	vcf_header = "##fileformat=VCFv4\n##KSNPFILE=" + ksnp_file 
	vcf_header += "\n##KSNP_SORT=" + ksnp_sort + "\n##KSNP_NUM="
	vcf_header += pp.pformat(num_k).replace('\n', '') + '\n'
	vcf_header += "##contig=<ID=" + CHROM + ",assembly=b37,length="+ CHROM_LEN + ">\n"
	vcf_header += "##FORMAT=<ID=GT,Number=1,Type=String,Description=\"Genotype\">\n"
	vcf_header += "##FILTER=<ID=PASS,Description=\"All filters passed\">\n"
	vcf_header += "##INFO=<ID=AF,Number=A,Type=Float,Description=\"Estimated allele frequency in the range (0,1)\">\n"
	vcf_header += "#CHROM\tPOS\tID\tREF\tALT\tQUAL\tFILTER\tINFO\t"
	for i in range(max_alts):
		vcf_header += str(i)
		if i != max_alts - 1:
			vcf_header += '\t'
	vcf_header += '\n'

	
	sorting = []
	with open(ksnp_sort, 'r') as file:
		lines = file.readlines()[0]
		sorting = [int(i) for i in lines.split(',')]

	rec_num = 0
	file_core = {}
	added_pos = set()
	num_k_i = 0

	for p in ksnps:
		file_core[p] = to_vcf_record(ksnps, p, max_alts)

	for p in sorting:
		if p not in ksnps:
			print(str(p) + " does not exist in KSNP dict.")
			exit(1)

		added_pos.add(p)
		rec_num += 1

		if rec_num == num_k[num_k_i]:
			with open(prefix + '_' + str(num_k[num_k_i]) + '.vcf', 'w') as o:
				with open(prefix + '_' + str(num_k[num_k_i]) + '_out.vcf', 'w') as oout:
					o.write(vcf_header)
					oout.write(vcf_header)
					for k in file_core:
						if k in added_pos:
							o.write(file_core[k] + '\n')
						else:
							oout.write(file_core[k] + '\n')

			if num_k_i == len(num_k) - 1:
				exit(0) # Done
			num_k_i += 1


if __name__ == '__main__':
	main()