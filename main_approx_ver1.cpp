#include <iostream>
#include <fstream>
#include <ctime>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <unordered_set>
#include <unordered_map>

#include "read_data.cpp"
#include "motif_id.cpp"

using namespace std;

inline long long convert_id(int hyperedge_a, int hyperedge_b){
	return hyperedge_a * (1LL << 31) + hyperedge_b;
}

int main(int argc, char *argv[])
{
	clock_t start;
	clock_t run_start;
	int progress;

	int sampling_size = stoi(argv[1]);

	string graphFile = "dblp_graph.txt";

	cout << "Sampling size: " << sampling_size << endl << endl;

	cout << graphFile << "\n\n";

	// Read data
	start = clock();
	vector< vector<int> > node2hyperedge;
	vector< vector<int> > hyperedge2node;
	vector< unordered_set<int> > hyperedge2node_set;
	read_data(graphFile, node2hyperedge, hyperedge2node, hyperedge2node_set);

	int V = node2hyperedge.size(), E = hyperedge2node.size();
	cout << "# of nodes: " << V << '\n';
	cout << "# of hyperedges: " << E << '\n';
	cout << "Reading data done: "
		<< (double)(clock() - start) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "------------------------------------------" << endl << endl;

	
	// Adjacency list construction
	start = clock(); run_start = clock();
	hyperedge2node.resize(E); 
	hyperedge2node_set.resize(E);
	vector< vector< pair<int, int> > > hyperedge_adj;
	vector< unordered_map<int, int> > hyperedge_inter;
	hyperedge_adj.resize(E);
	hyperedge_inter.resize(E);
	vector<long long> upd_time(E, -1LL);
		
	for (int hyperedge_a = 0; hyperedge_a < E; hyperedge_a++){
		long long l_hyperedge_a = (long long)hyperedge_a;
		for (const int &node: hyperedge2node[hyperedge_a]){
			for (const int &hyperedge_b: node2hyperedge[node]){
				if (hyperedge_b == hyperedge_a) continue;
				if ((upd_time[hyperedge_b] >> 31) ^ hyperedge_a){
					upd_time[hyperedge_b] = (l_hyperedge_a << 31) + (long long)hyperedge_adj[hyperedge_b].size();
					hyperedge_adj[hyperedge_b].push_back({hyperedge_a, 0});
				}
				hyperedge_adj[hyperedge_b][(int)(upd_time[hyperedge_b] & 0x7FFFFFFFLL)].second++;
			}
		}
	}

	for (int hyperedge_a = 0; hyperedge_a < E; hyperedge_a++){
		int deg_a = hyperedge_adj[hyperedge_a].size();
		for (int i = 0; i < deg_a; i++){
			int hyperedge_b = hyperedge_adj[hyperedge_a][i].first;
			int C_ab = hyperedge_adj[hyperedge_a][i].second;
			hyperedge_inter[hyperedge_a].insert({hyperedge_b, C_ab});
		}
	}

	cout << "Adjacency list construction done: "
		<< (double)(clock() - start) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "------------------------------------------" << endl << endl;


	// Hyperedge sampling
	start = clock();
	vector<long long> h_motif(30, 0);
	vector<int> intersection(E, 0); 
	std::fill(upd_time.begin(), upd_time.end(), -1LL);
	
	random_device rd;
	mt19937 gen(2020);
	uniform_int_distribution<> dist(0, ((int)hyperedge2node.size())-1);

	for (int sample = 0; sample < sampling_size; sample++){
		if (sample % 1000 == 0)
			cout << "Sampling: " << sample << " / " << sampling_size << endl;
		
		int hyperedge_a = dist(gen);
		int size_a = hyperedge2node[hyperedge_a].size();
		int deg_a = (int)hyperedge_adj[hyperedge_a].size();

		for (int i = 0; i < deg_a; i++){
			int hyperedge_b = hyperedge_adj[hyperedge_a][i].first, C_ab = hyperedge_adj[hyperedge_a][i].second;
			int size_b = hyperedge2node[hyperedge_b].size();
			int deg_b = (int)hyperedge_adj[hyperedge_b].size();

			long long ab_id = convert_id(hyperedge_a, hyperedge_b);
			upd_time[hyperedge_b] = ab_id;
			
			const auto &nodes = hyperedge2node_set[hyperedge_b]; auto it_end = nodes.end(); int cnt = 0;
			for (const int &node: hyperedge2node[hyperedge_a]){ if(nodes.find(node) != it_end) intersection[cnt++] = node;}
			
			for (int j = 0; j < deg_b; j++){
				int hyperedge_c = hyperedge_adj[hyperedge_b][j].first, C_bc = hyperedge_adj[hyperedge_b][j].second;
				if (hyperedge_a == hyperedge_c || hyperedge_b == hyperedge_c) continue;
				upd_time[hyperedge_c] = ab_id;
				
				int size_c = hyperedge2node[hyperedge_c].size();
				int C_ca = 0, g_abc = 0;
				const auto &nodes = hyperedge2node_set[hyperedge_c]; auto it_end = nodes.end();
				C_ca = hyperedge_inter[hyperedge_a][hyperedge_c];
				//for (const int &node: hyperedge2node[hyperedge_a]){ if(nodes.find(node) != it_end) C_ca++; }
				for (int k = 0; k < C_ab; k++){ if(nodes.find(intersection[k]) != it_end) g_abc++; }
				
				int motif_index = get_motif_index_new(size_a, size_b, size_c, C_ab, C_bc, C_ca, g_abc);
				if (C_ca) h_motif[motif_index] += (int)(hyperedge_b < hyperedge_c);
				else h_motif[motif_index] ++;
			}
			for (int j = i + 1; j < deg_a; j++){
				int hyperedge_c = hyperedge_adj[hyperedge_a][j].first, C_ca = hyperedge_adj[hyperedge_a][j].second;
				if (hyperedge_a == hyperedge_c || hyperedge_b == hyperedge_c) continue;
				if(upd_time[hyperedge_c] == ab_id) continue;
				
				int size_c = hyperedge2node[hyperedge_c].size();
				int C_bc = 0, g_abc = 0;
				int motif_index = get_motif_index_new(size_a, size_b, size_c, C_ab, C_bc, C_ca, g_abc);
				h_motif[motif_index]++;
			}
		}
	}

	int index = 0;
	vector<long double> h_motif_final(30, 0);
	for (int i = 0; i < 30; i++)
	{
		h_motif_final[i] = h_motif[i];
		h_motif_final[i] *= (long double)E / (sampling_size);
		h_motif_final[i] /= 3;
		if (i == 0 || i == 1 || i == 4 || i == 6) continue;
		cout << "h-motif " << ++index << ": " << h_motif_final[i] << endl;
	}

	double runtime = (double)(clock() - run_start) / CLOCKS_PER_SEC;

	cout << "\nHypergraph motif counting done: "
		<< (double)(clock() - start) / CLOCKS_PER_SEC << " sec" << endl;
	cout << "Total runtime: " << runtime << endl;
	cout << "-----------------------------------------" << endl << endl;
	node2hyperedge.clear();
	hyperedge2node.clear();

	return 0;
}
