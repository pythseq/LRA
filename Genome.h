#ifndef GENOME_H_
#define GENOME_H_
#include "htslib/kseq.h"

class Header {
 public:
	vector<string> names;
	vector<uint64_t> pos;
	Header() {
		pos.push_back(0);
	}
	int Find(uint64_t query) { 
		vector<uint64_t>::iterator it = lower_bound(pos.begin(), pos.end(), query);
		int i = it - pos.begin();
		assert(i > 0);
		return i-1;
	}
	uint64_t GetOffset(uint64_t query) {
		int i = Find(query);
		return pos[i];
	}

	uint64_t GetNextOffset(uint64_t query) {
		int i = Find(query);
		assert(i+1 < pos.size());
		return pos[i+1];
	}
		
	int GetChromStart(uint64_t query) {
		vector<uint64_t>::iterator it = lower_bound(pos.begin(), pos.end(), query);
		return query - *it;
	}

	void Add(const char* name, uint64_t p) {
		names.push_back(string(name));
		pos.push_back(p);
	}
		
	void Write(ofstream &out) {
		int idxLen = names.size();
		out.write((char*) &idxLen, sizeof(int));
		for(int i=0; i < names.size();i++) {
			int nameLen=names[i].size();
			out.write((char*) &nameLen, sizeof(int));
			out.write((char*) names[i].c_str(), names[i].size());
		}
		out.write((char*) &pos[0], sizeof(int64_t)*pos.size());
	}

	void Read(ifstream &in) {
		int idxLen;
		in.read((char*) &idxLen, sizeof(int));
		names.resize(idxLen);
		pos.resize(idxLen+1);		
		for(int i=0; i < names.size(); i++) {			
			int nameLen;
			in.read((char*) &nameLen, sizeof(int));
			char *name = new char[nameLen+1];
			name[nameLen] = '\0';
			in.read((char*) name, nameLen);
			names[i] = name;
		}
		in.read((char*) &pos[0],sizeof(int64_t)*pos.size());
	}
	void WriteSAMHeader(ostream &out) {
		for (int i=0; i < names.size(); i++) {
			out << "@SQ\tSN:"<<names[i]<<"\tLN:"<<pos[i+1]-pos[i]<<endl;
		}
	}
};

class Genome {
 public:
	vector<char*> seqs;
	vector<int>   lengths;
	uint64_t GetSize() {
		if (header.pos.size() == 0) {
			return 0;
		}
		else {
			return header.pos[header.pos.size()-1];
		}
	}

	Header header;
	void Read(string &genome) {
		gzFile f = gzopen(genome.c_str(), "r");
		kseq_t *ks = kseq_init(f);
		uint64_t offset=0;
		
		while (kseq_read(ks) >= 0) { // each kseq_read() call reads one query sequence
			char *seq = new char[ks->seq.l];
			memcpy(seq, ks->seq.s, ks->seq.l);
			seqs.push_back(seq);
			lengths.push_back(ks->seq.l);
			offset+=ks->seq.l;
			header.Add(ks->name.s, offset);
		}
		kseq_destroy(ks);
	}
	~Genome() {
		for (int i = 0; i < seqs.size(); i++) {
			delete[] seqs[i];
			seqs[i] = NULL;
		}
	}
	char *OffsetToChrom(GenomePos offset) {
		int chromIndex = header.Find(offset);
		assert(chromIndex < seqs.size());
		return seqs[chromIndex];
	}
};

#endif
