#include "mbff.h"
#include <iostream>
#include <algorithm>
#include <queue>
#include <stack>
#include <map>
#include <set>
#include <ctime>
#include <chrono>
#include <memory>
#include <random>
#include <omp.h>
#include "ortools/linear_solver/linear_solver.h"
#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include <lemon/list_graph.h>
#include <lemon/network_simplex.h>
#include <lemon/maps.h>
using namespace std;
using namespace lemon;
using namespace operations_research;
using namespace sat;


using Graph = ListDigraph;
using Node = ListDigraph::Node;
using Edge = ListDigraph::Arc;

auto seed = chrono::high_resolution_clock::now().time_since_epoch().count();
std::mt19937 mt(seed);

/* hard coded values for height, width, power consumption, etc. */
float WIDTH = 2.448, HEIGHT = 1.2;
float RATIOS[6] = {1, 0.875, 0.854, 0.854, 0.844, 0.844}, POWER[6] = {1, 0.875, 0.854, 0.854, 0.844, 0.844};
int BITCNT[6] = {1, 4, 8, 16, 32, 64};
namespace gpl {
	// begin or-tools usage 

		float MBFF::RunLP(vector<Flop> flops, vector<Tray> &trays, vector<pair<int, int> > clusters, int sz) {
			int num_flops = (int)flops.size(), num_trays = (int)trays.size();

			unique_ptr<MPSolver> solver(MPSolver::CreateSolver("GLOP"));
			const double inf = solver->infinity();

			vector<const MPVariable*> tray_x(num_trays), tray_y(num_trays);
			for (int i = 0; i < num_trays; i++) {
				tray_x[i] = solver->MakeNumVar(0, inf, "");
				tray_y[i] = solver->MakeNumVar(0, inf, "");
			}

			vector<const MPVariable*> d_x(num_flops), d_y(num_flops);


			for (int i = 0; i < num_flops; i++) {	

				d_x[i] = solver->MakeNumVar(0, inf, "");
				d_y[i] = solver->MakeNumVar(0, inf, "");


				int tray_idx = clusters[i].first, slot_idx = clusters[i].second;

				Point flop = flops[i].pt;
				Point tray = trays[tray_idx].pt;
				Point slot = trays[tray_idx].slots[slot_idx]; 


				float shift_x = slot.x - tray.x, shift_y = slot.y - tray.y;

				MPConstraint* const c1 = solver->MakeRowConstraint(shift_x - flop.x, inf, "");
				c1->SetCoefficient(d_x[i], 1);
				c1->SetCoefficient(tray_x[tray_idx], -1);


				MPConstraint* const c2 = solver->MakeRowConstraint(flop.x - shift_x, inf, "");
				c2->SetCoefficient(d_x[i], 1);
				c2->SetCoefficient(tray_x[tray_idx], 1);

				MPConstraint* const c3 = solver->MakeRowConstraint(shift_y - flop.y, inf, "");
				c3->SetCoefficient(d_y[i], 1);
				c3->SetCoefficient(tray_y[tray_idx], -1);

				MPConstraint* const c4 = solver->MakeRowConstraint(flop.y - shift_y, inf, "");
				c4->SetCoefficient(d_y[i], 1);
				c4->SetCoefficient(tray_y[tray_idx], 1);
			}

			MPObjective* const objective = solver->MutableObjective();
			for (int i = 0; i < num_flops; i++) {
				objective->SetCoefficient(d_x[i], 1);
				objective->SetCoefficient(d_y[i], 1);
			}
			objective->SetMinimization();


			const MPSolver::ResultStatus result_status = solver->Solve();


			float tot_d = 0;
			for (int i = 0; i < num_trays; i++) {
				Tray new_tray;
				new_tray.pt.x = tray_x[i]->solution_value();
				new_tray.pt.y = tray_y[i]->solution_value();
			    tot_d += MBFF::GetDist(trays[i].pt, new_tray.pt);
		        trays[i] = new_tray;
			 }

			return tot_d;
		}


		/* 
			NOTE: CP-SAT constraints only work with INTEGERS
			so, all coefficients (shift_x and shift_y) are multiplied by 100
		*/


			void MBFF::RunILP(vector<Flop> flops, vector<Path> paths, vector<vector<Tray> > all_trays, float ALPHA, float BETA) {
				vector<Tray> trays;
		        for (int i = 0; i < 6; i++) {
		            for (int j = 0; j < (int)all_trays[i].size(); j++) trays.push_back(all_trays[i][j]);
		        }

				int num_flops = (int)flops.size(), num_paths = (int)paths.size(), num_trays = (int)trays.size();	


				CpModelBuilder cp_model;
				const int inf = 2000000000;

				vector<BoolVar> B[num_flops][num_trays];
				vector<IntVar> d_x[num_flops][num_trays], d_y[num_flops][num_trays];


				for (int i = 0; i < num_flops; i++) {
						for (int j = 0; j < num_trays; j++) {
								d_x[i][j].resize((int)trays[j].slots.size()), d_y[i][j].resize((int)trays[j].slots.size()), B[i][j].resize((int)trays[j].slots.size());
								for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
										d_x[i][j][k] = cp_model.NewIntVar(Domain(0, inf));
										d_y[i][j][k] = cp_model.NewIntVar(Domain(0, inf));
										B[i][j][k] = cp_model.NewBoolVar();
								}
						}
				}


				// add constraints for D
				float maxD = 0;
				for (int i = 0; i < num_flops; i++) {
						for (int j = 0; j < num_trays; j++) {

								vector<Point> slots = trays[j].slots;


								for (int k = 0; k < (int)slots.size(); k++) if (trays[j].cand[k] == i) {
										
										float shift_x = slots[k].x - flops[i].pt.x, shift_y = slots[k].y - flops[i].pt.y;
										maxD = max(maxD, max(shift_x, -shift_x) + max(shift_y, -shift_y));

										// absolute value constraints for x
										cp_model.AddLessOrEqual(0, d_x[i][j][k] - int(100 * shift_x) * B[i][j][k]);
										cp_model.AddLessOrEqual(0, d_x[i][j][k] + int(100 * shift_x) * B[i][j][k]);


										// absolute value constraints for y
										cp_model.AddLessOrEqual(0, d_y[i][j][k] - int(100 * shift_y) * B[i][j][k]);
										cp_model.AddLessOrEqual(0, d_y[i][j][k] + int(100 * shift_y) * B[i][j][k]);

										
								}
						}
				} 

				for (int i = 0; i < num_flops; i++) {
					for (int j = 0; j < num_trays; j++) {
							for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
									cp_model.AddLessOrEqual(d_x[i][j][k] + d_y[i][j][k], int(100 * maxD));
							}
					}
				}


				// add constraints for Z 
				vector<IntVar> z_x(num_paths), z_y(num_paths);
				for (int i = 0; i < num_paths; i++) {
					z_x[i] = cp_model.NewIntVar(Domain(0, inf));
					z_y[i] = cp_model.NewIntVar(Domain(0, inf));
				}

				for (int i = 0; i < num_paths; i++) {
					int flop_a_idx = paths[i].a, flop_b_idx = paths[i].b;

					// d_x_a represents the sum of d_x[a][j][k]
					
					sat::LinearExpr d_x_a, d_y_a;
					sat::LinearExpr d_x_b, d_y_b;

					for (int j = 0; j < num_trays; j++) {
	                        vector<Point> slots = trays[j].slots;
							for (int k = 0; k < (int)slots.size(); k++) {
									if (trays[j].cand[k] == flop_a_idx) {
	                                        float shift_x = slots[k].x - flops[flop_a_idx].pt.x, shift_y = slots[k].y - flops[flop_a_idx].pt.y;
											d_x_a += (int(100 * shift_x) * B[flop_a_idx][j][k]);
											d_y_a += (int(100 * shift_y) * B[flop_a_idx][j][k]);
									}
									if (trays[j].cand[k] == flop_b_idx) {
	                                        float shift_x = slots[k].x - flops[flop_b_idx].pt.x, shift_y = slots[k].y - flops[flop_b_idx].pt.y;
											d_x_b += (int(100 * shift_x) * B[flop_b_idx][j][k]);
											d_y_b += (int(100 * shift_y) * B[flop_b_idx][j][k]);
									}
							}
					}

					// absolute value constraints for x
					cp_model.AddLessOrEqual(0, z_x[i] + d_x_a - d_x_b);
					cp_model.AddLessOrEqual(0, z_x[i] - d_x_a + d_x_b);



					// absolute value constraints for y
					cp_model.AddLessOrEqual(0, z_y[i] + d_y_a - d_y_b);
					cp_model.AddLessOrEqual(0, z_y[i] - d_y_a + d_y_b);
				}




				// BEGIN CONSTRAINTS FOR B
				for (int i = 0; i < num_trays; i++) {
					for (int j = 0; j < (int)trays[i].slots.size(); j++) {
							sat::LinearExpr b_slot;
							for (int k = 0; k < num_flops; k++) if (trays[i].cand[j] == k) {
									b_slot += B[k][i][j];
							}
							cp_model.AddLessOrEqual(0, b_slot);
							cp_model.AddLessOrEqual(b_slot, 1);
							
					}
				}

				for (int i = 0; i < num_flops; i++) {
					sat::LinearExpr b_flop;
					for (int j = 0; j < num_trays; j++) {
							for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
									b_flop += B[i][j][k];
							}
					}
					cp_model.AddLessOrEqual(1, b_flop);
					cp_model.AddLessOrEqual(b_flop, 1);
				}
				// END CONSTRAINTS FOR B


				// CONSTRAINTS FOR E
				vector<BoolVar> e(num_trays);
				for (int i = 0; i < num_trays; i++) {
					e[i] = cp_model.NewBoolVar();
				}

				for (int i = 0; i < num_trays; i++) {
						sat::LinearExpr slots_used;
						for (int j = 0; j < (int)trays[i].slots.size(); j++) {
								for (int k = 0; k < num_flops; k++) if (trays[i].cand[j] == k) {
										cp_model.AddLessOrEqual(0, e[i] - B[k][i][j]);
										slots_used += B[k][i][j];
								}
						}
						cp_model.AddLessOrEqual(0, slots_used - e[i]);
				}
				// END CONSTRAINTS FOR E

				// calculate the cost of each tray 
				vector<float> w(num_trays);
				for (int i = 0; i < num_trays; i++) {
						int bit_idx = 0;
						for (int j = 0; j < 6; j++) {
								if (BITCNT[j] == (int)trays[i].slots.size()) bit_idx = j;
						}
						if (BITCNT[bit_idx] == 1) w[i] = 1.00;
						else w[i] = ((float)BITCNT[bit_idx]) * (POWER[bit_idx] - BITCNT[bit_idx] * 0.0015);
				}



				/* 
					- DoubleLinearExpr support double coefficients 
					- can only be used for the objective function
				*/

				sat::DoubleLinearExpr obj;

				// add B
				for (int i = 0; i < num_flops; i++) {
					for (int j = 0; j < num_trays; j++) {
							for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
									obj.AddTerm(d_x[i][j][k], 0.01);
									obj.AddTerm(d_y[i][j][k], 0.01);
							}
					}
				}

				// add Z
				for (int i = 0; i < num_paths; i++) {
					obj.AddTerm(z_x[i], BETA * 0.01);
					obj.AddTerm(z_y[i], BETA * 0.01);
				}

				// add E
				for (int i = 0; i < num_trays; i++) {
					obj.AddTerm(e[i], ALPHA * w[i]);
				}

				cp_model.Minimize(obj);

				const CpSolverResponse response = Solve(cp_model.Build());
				if (response.status() == CpSolverStatus::INFEASIBLE) {
	  				cout << "No solution found.\n";
	  				return;
				}

				cout << "Total = " << (response.objective_value()) << "\n";


				float TOT_D = 0;
				for (int i = 0; i < num_flops; i++) {
					for (int j = 0; j < num_trays; j++) {
							for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
									TOT_D += (0.01 * SolutionIntegerValue(response, d_x[i][j][k]));
									TOT_D += (0.01 * SolutionIntegerValue(response, d_y[i][j][k]));
							}
					}
				}
				cout << "D: " << TOT_D << "\n";


				float TOT_Z = 0;
				for (int i = 0; i < num_paths; i++) {
					TOT_Z += (0.01 * SolutionIntegerValue(response, z_x[i]));
					TOT_Z += (0.01 * SolutionIntegerValue(response, z_y[i]));
				}
				cout << "Z: " << TOT_Z << "\n";


				float TOT_W = 0;
				for (int i = 0; i < num_trays; i++) {
					if (SolutionIntegerValue(response, e[i])) {
						TOT_W += w[i];
					}
				}
				cout << "W: " << TOT_W << "\n";

				int tot_cnt = 0;
				set<int> tray_idx;
				for (int i = 0; i < num_flops; i++) {
						for (int j = 0; j < num_trays; j++) {
								for (int k = 0; k < (int)trays[j].slots.size(); k++) if (trays[j].cand[k] == i) {
										if (SolutionIntegerValue(response, B[i][j][k]) == 1) {
												tray_idx.insert(j);
												tot_cnt++;
										} 
								}
						}
				}


				map<int, int> tray_sz;
				for (auto x: tray_idx) {
						tray_sz[(int)trays[x].slots.size()]++;
				}

				for (auto x: tray_sz) {
						cout << "Tray size = " << x.first << ": " << x.second << "\n";
				}

				if (tot_cnt != num_flops) {
					cout << "ERROR -- NOT ALL FLOPS ARE MATCHED\n";
				}

				cout << "\n";

			} // end ILP 


	float MBFF::GetDist(Point a, Point b) {
		return (abs(a.x - b.x) + abs(a.y - b.y));
	}

	int MBFF::GetRows(int K) {
		if (K == 1 || K == 2 || K == 4) return 1;
		if (K == 8) return 2;
		else return 4;
	}


	vector<Point> MBFF::GetSlots(Point tray, int rows, int cols) {
		int bit_idx = 0;
		for (int i = 1; i < 6; i++) {
			if (rows * cols == BITCNT[i]) bit_idx = i;
		}

		float center_x = tray.x, center_y = tray.y;

		vector<Point> slots;
		for (int i = 0; i < rows * cols; i++) {
			int new_col = i % cols, new_row = i / cols;

			Point new_slot;
			new_slot.x = center_x + WIDTH * RATIOS[bit_idx] * ((new_col + 0.5) - (cols / 2.0));
			new_slot.y = center_y + HEIGHT * ((new_row + 0.5) - (rows / 2.0));


			if (new_slot.x >= 0 && new_slot.y >= 0) slots.push_back(new_slot);
		}

		return slots;
	}


	Flop MBFF::GetNewFlop(vector<Flop> prob_dist, float tot_dist) {
		float rand_num = (float)(mt() % 101), cum_sum = 0;
	    Flop new_flop;
		for (int i = 0; i < (int)prob_dist.size(); i++) {
			cum_sum += prob_dist[i].prob;
			new_flop = prob_dist[i];
			if (cum_sum * 100.0 >= rand_num * tot_dist) break;
		}

		return new_flop;
	}

	vector<Tray> MBFF::GetStartTrays(vector<Flop> &flops, int num_trays) {

		int num_flops = (int)flops.size();

		/* pick a random flop */
		int rand_idx = mt() % (num_flops);
		Tray tray_zero; tray_zero.pt = flops[rand_idx].pt;

		set<int> used_flops; used_flops.insert(rand_idx);
		vector<Tray> trays; trays.push_back(tray_zero);

	
		float tot_dist = 0;
		for (int i = 0; i < num_flops; i++) {
				float contr = MBFF::GetDist(flops[i].pt, tray_zero.pt);
				flops[i].prob = contr, tot_dist += contr;
		}

		while ((int)trays.size() < num_trays) {
			vector<Flop> prob_dist;
			for (int i = 0; i < num_flops; i++) if (!used_flops.count(i)) {
					prob_dist.push_back(flops[i]);
			}
        
			sort(begin(prob_dist), end(prob_dist));
         

				Flop new_flop = GetNewFlop(prob_dist, tot_dist);
				used_flops.insert(new_flop.idx);

				Tray new_tray; new_tray.pt = new_flop.pt;
				trays.push_back(new_tray);

				for (int i = 0; i < num_flops; i++) {
						float new_contr = MBFF::GetDist(flops[i].pt, new_tray.pt);
						flops[i].prob += new_contr, tot_dist += new_contr;
				}

		}

		return trays;
	}


	Tray MBFF::GetOneBit(Point pt) {
		Tray tray; 
		tray.pt.x = pt.x - WIDTH / 2.0, tray.pt.y = pt.y - HEIGHT / 2.0;
		tray.slots.push_back(pt);
		return tray;
	}

	vector<pair<int, int> > MBFF::MinCostFlow(vector<Flop> flops, vector<Tray> &trays, int sz) {
		
		int num_flops = (int)flops.size(), num_trays = (int)trays.size();


		Graph graph; 
		vector<Node> nodes; vector<Edge> edges;
		vector<int> cur_costs, cur_caps;

		// add edges from source to flop 
		Node src = graph.addNode(), sink = graph.addNode();
		for (int i = 0; i < num_flops; i++) {
				Node flop_node = graph.addNode();
				nodes.push_back(flop_node);

				Edge src_to_flop = graph.addArc(src, flop_node);
				edges.push_back(src_to_flop);
				cur_costs.push_back(0), cur_caps.push_back(1);
		}


	 	vector<pair<int, int> > slot_to_tray;

		for (int i = 0; i < num_trays; i++) {
				vector<Point> tray_slots = trays[i].slots; 
				
				for (int j = 0; j < (int)tray_slots.size(); j++) {

						Node slot_node = graph.addNode();
						nodes.push_back(slot_node);

						// add edges from flop to slot
						for (int k = 0; k < num_flops; k++) {
								Edge flop_to_slot = graph.addArc(nodes[k], slot_node);
								edges.push_back(flop_to_slot);

								int edge_cost = (100 * MBFF::GetDist(flops[k].pt, tray_slots[j]));
								cur_costs.push_back(edge_cost), cur_caps.push_back(1);
						}

						// add edges from slot to sink 
						Edge slot_to_sink = graph.addArc(slot_node, sink);
						edges.push_back(slot_to_sink);
						cur_costs.push_back(0), cur_caps.push_back(1);

						slot_to_tray.push_back(make_pair(i, j));
				}

		}

		// run min-cost flow
		Graph::ArcMap<int> costs(graph), caps(graph), flow(graph);
		Graph::NodeMap<int> labels(graph);
		NetworkSimplex<Graph, int, int> new_graph(graph);

		for (int i = 0; i < (int)edges.size(); i++) {
				costs[edges[i]] = cur_costs[i], caps[edges[i]] = cur_caps[i];
		}

		labels[src] = -1;
		for (int i = 0; i < (int)nodes.size(); i++) {
			labels[nodes[i]] = i;
		}
		labels[sink] = (int)nodes.size();

		new_graph.costMap(costs);
		new_graph.upperMap(caps);
		new_graph.stSupply(src, sink, num_flops);
		new_graph.run();
		new_graph.flowMap(flow);

		// get, and save, the clustering solution 
		vector<pair<int, int> > clusters(num_flops);
		for (Graph::ArcIt itr(graph); itr != INVALID; ++itr) {
				int u = labels[graph.source(itr)], v = labels[graph.target(itr)];
				if (flow[itr] != 0 && u < num_flops && v >= num_flops) {
						v -= num_flops; 
						int tray_idx = slot_to_tray[v].first, slot_idx = slot_to_tray[v].second;
						clusters[u] = make_pair(tray_idx, slot_idx), trays[tray_idx].cand[slot_idx] = u;
				}
		}

		return clusters;
	}


    float MBFF::GetSilh(vector<Flop> flops, vector<Tray> trays, vector<pair<int, int> > clusters) {
		int num_flops = (int)flops.size(), num_trays = (int)trays.size();
		
		float tot = 0;
		for (int i = 0; i < num_flops; i++) {
			float min_num = 2000000000;
			float max_den = MBFF::GetDist(flops[i].pt, trays[clusters[i].first].slots[clusters[i].second]);
			for (int j = 0; j < num_trays; j++) if (j != clusters[i].first) {
				max_den = max(max_den, MBFF::GetDist(flops[i].pt, trays[j].slots[clusters[i].second]));
				for (int k = 0; k < (int)trays[j].slots.size(); k++) {
					min_num = min(min_num, MBFF::GetDist(flops[i].pt, trays[j].slots[k]));
				}
			}

			tot += (min_num / max_den);
		}

		return tot;
	}

	// standard K-means++ implementation 
	vector<vector<Flop> > MBFF::KMeans(vector<Flop> flops, int K) {
		int num_flops = (int)flops.size();


		set<int> chosen;
		// choose initial center
		int seed = mt() % num_flops;
		chosen.insert(seed);

		vector<Flop> centers;
		centers.push_back(flops[seed]);

		float d[num_flops];
		for (int i = 0; i < num_flops; i++) d[i] = MBFF::GetDist(flops[i].pt, flops[seed].pt);

		// choose remaining K-1 centers 
		while ((int)chosen.size() < K) {

			double tot_sum = 0;
			for (int i = 0; i < num_flops; i++) if (!chosen.count(i)) {
				for (int j: chosen) {
					d[i] = min(d[i], MBFF::GetDist(flops[i].pt, flops[j].pt));
				}
				tot_sum += (double(d[i]) * double(d[i]));
			}

			int rnd = mt() % (int(tot_sum * 100));
			float prob = rnd / 100.0;

			double cum_sum = 0;
			for (int i = 0; i < num_flops; i++) if (!chosen.count(i)) {
				cum_sum += (double(d[i]) * double(d[i]));
				if (cum_sum >= prob) {
					chosen.insert(i);
					centers.push_back(flops[i]);
					break;
				}
			}
		}

		vector<vector<Flop> > clusters(K);

		float prev = -1;
		while (1) {

			for (int i = 0; i < K; i++) clusters[i].clear();

			// remap flops to clusters 
			for (int i = 0; i < num_flops; i++) {
				
				float min_cost = 2000000000;
				int idx = -1;


				for (int j = 0; j < K; j++) {
					if (MBFF::GetDist(flops[i].pt, centers[j].pt) < min_cost) {
						min_cost = MBFF::GetDist(flops[i].pt, centers[j].pt);
						idx = j;
					}
				}

				clusters[idx].push_back(flops[i]);
			}

			// find new center locations 
			for (int i = 0; i < K; i++) {

				int cur_sz = (int)clusters[i].size();
				float cX = 0, cY = 0;

				for (Flop f: clusters[i]) {
					cX += f.pt.x;
					cY += f.pt.y;
				}

				Flop new_flop; 
				new_flop.pt.x = cX / float(cur_sz);
				new_flop.pt.y = cY / float(cur_sz); 

				centers[i] = new_flop;
			}


			// get total displacement 
			float tot_disp = 0;
			for (int i = 0; i < K; i++) {
				for (Flop f: clusters[i]) {
					tot_disp += MBFF::GetDist(centers[i].pt, f.pt);
				}
			}

			if (tot_disp == prev) break;
			prev = tot_disp;

		}

		for (int i = 0; i < K; i++) {
			clusters[i].push_back(centers[i]);
		}

		return clusters;
	}



	/* 
		shreyas (august 2023): 
		method to decompose a pointset into multiple "mini"-pointsets of size <= MAX_SZ.
		basic implementation of K-means++ (with K = 4) is used. 
	*/
	vector<vector<Flop> > MBFF::KMeansDecomp(vector<Flop> flops, int MAX_SZ) {
		int num_flops = (int)flops.size();

		vector<vector<Flop> > ret;
	  

		if (num_flops <= MAX_SZ) {
		    ret.push_back(flops);
		    return ret;
		}



		vector<vector<Flop> > tmp_clusters[10];
		vector<float> tmp_costs(10);

		// multistart K-means++ 
		for (int i = 0; i < 10; i++) {

			vector<vector<Flop> > tmp = KMeans(flops, 4);

			// cur_cost = sum of distances between flops and its matching cluster's center
			float cur_cost = 0;
			for (int j = 0; j < 4; j++) {
				for (int k = 0; k + 1 < (int)tmp[j].size(); k++) {
					cur_cost += MBFF::GetDist(tmp[j][k].pt, tmp[j].back().pt);
				}
			}	

			tmp_clusters[i] = tmp;
			tmp_costs[i] = cur_cost;
		}	


		float best_cost = 2000000000;
		vector<vector<Flop> > k_means_ret;
		for (int i = 0; i < 10; i++) {
			if (tmp_costs[i] < best_cost) {
				best_cost = tmp_costs[i];
				k_means_ret = tmp_clusters[i];
			}
		}

		/* 
		create edges between all pairs of cluster centers 
		edge weight = distance between the two centers
		*/

		vector<pair<float, pair<int, int> > > cluster_pairs;
		for (int i = 0; i < 4; i++) {
			for (int j = i + 1; j < 4; j++) {
				cluster_pairs.push_back(make_pair(MBFF::GetDist(k_means_ret[i].back().pt, k_means_ret[j].back().pt), make_pair(i, j)));
			}
		}
		sort(begin(cluster_pairs), end(cluster_pairs));

		for (int i = 0; i < 4; i++) {
			k_means_ret[i].pop_back();
		}

		/* 
			lines 810-830: basic implementation of DSU
			keep uniting sets (== mini-pointsets with size <= MAX_SZ) while not exceeding MAX_SZ
		*/
		int id[4], sz[4];
		for (int i = 0; i < 4; i++) {
			id[i] = i, sz[i] = ((int)k_means_ret[i].size());
		}


		for (int i = 0; i < (int)cluster_pairs.size(); i++) {
			int idx1 = cluster_pairs[i].second.first, idx2 = cluster_pairs[i].second.second;
			if (sz[id[idx1]] < sz[id[idx2]]) swap(idx1, idx2);
			if (sz[id[idx1]] + sz[id[idx2]] > MAX_SZ) continue;

			sz[id[idx1]] += sz[id[idx2]];

			// merge the two clusters 
			int orig_id = id[idx2];
			for (int j = 0; j < 4; j++) {
				if (id[j] == orig_id) {
					id[j] = id[idx1];
				}
			}
		}


		vector<vector<Flop> > nxt_clusters(4);
		for (int i = 0; i < 4; i++) {
			for (Flop f: k_means_ret[i]) {
				nxt_clusters[id[i]].push_back(f);
			}
		}

		// recurse on each new cluster 
		vector<Flop> Q1 = nxt_clusters[0], Q2 = nxt_clusters[1], Q3 = nxt_clusters[2], Q4 = nxt_clusters[3];

		vector<vector<Flop> > R1, R2, R3, R4;
		if ((int)Q1.size()) R1 = KMeansDecomp(Q1, MAX_SZ);
		if ((int)Q2.size()) R2 = KMeansDecomp(Q2, MAX_SZ);
		if ((int)Q3.size()) R3 = KMeansDecomp(Q3, MAX_SZ);
		if ((int)Q4.size()) R4 = KMeansDecomp(Q4, MAX_SZ);

		for (auto x: R1) ret.push_back(x);
		for (auto x: R2) ret.push_back(x);
		for (auto x: R3) ret.push_back(x);
		for (auto x: R4) ret.push_back(x);

		return ret;
	}



	void MBFF::Run(int mx_sz, float ALPHA, float BETA) {
		/* 
			(1) Run k-means++ based pointset decomposition
			(2) Run Silhouette metric on each pointset from (1)
			(3) Run capacitated k-means clustering on each pointset from (1)
			(4) Merge pointsets from (1)
			(5) Run ILP
		*/

		omp_set_num_threads(NUM_THREADS);

		int num_flops = (int)FLOPS.size(), num_trays = (int)PATHS.size();
		vector<Flop> flops = FLOPS;
		vector<Path> paths = PATHS;


		// Run k-means++ based pointset decomposition
		vector<vector<Flop> > pointsets = KMeansDecomp(flops, mx_sz);

		// all_trays[i] = all trays of size BITCNT[i]
		vector<vector<Tray> > all_trays(6);



		for (int T = 0; T < (int)pointsets.size(); T++) {

				vector<vector<Tray> > trays(6);
				vector<vector<pair<int, int> > > clusters(6);


				num_flops = (int)pointsets[T].size();

				// resize 
		        for (int i = 0; i < 6; i++) {
		                int num_trays = (num_flops + (BITCNT[i] - 1)) / BITCNT[i];
		                trays[i].resize(num_trays);
		                clusters[i].resize(num_flops);
		        }

		        // add 1-bit trays
				for (int i = 0; i < num_flops; i++) {
						Tray one_bit = GetOneBit(pointsets[T][i].pt);
						one_bit.cand[0] = i;
						trays[0][i] = one_bit;
				}



				// begin Silhoutte
				vector<vector<Tray> > start_trays[6];
				vector<float> res[6];
				vector<pair<int, int> > ind;

				for (int i = 1; i < 6; i++) {
					for (int j = 0; j < 5; j++) ind.push_back(make_pair(i, j));
					start_trays[i].resize(5);
					res[i].resize(5);
				}

			
				#pragma omp parallel for
				for (int i = 0; i < 25; i++) {

					int bit_idx = ind[i].first, tray_idx = ind[i].second;

					int rows = GetRows(BITCNT[bit_idx]), cols = BITCNT[bit_idx] / rows;
					float AR = (cols * WIDTH * RATIOS[bit_idx]) / (rows * HEIGHT);

					int num_trays = (num_flops + (BITCNT[bit_idx] - 1)) / BITCNT[bit_idx];

					start_trays[bit_idx][tray_idx] = GetStartTrays(pointsets[T], num_trays);
					for (int j = 0; j < num_trays; j++) {
						start_trays[bit_idx][tray_idx][j].slots = GetSlots(start_trays[bit_idx][tray_idx][j].pt, rows, cols);
					}

		      		float delta = 0;
					vector<pair<int, int> > tmp_cluster; 
					for (int j = 0; j < 8; j++) {
						
		        		tmp_cluster = MinCostFlow(pointsets[T], start_trays[bit_idx][tray_idx], BITCNT[bit_idx]);
		         		delta = MBFF::RunLP(pointsets[T], start_trays[bit_idx][tray_idx], tmp_cluster, BITCNT[bit_idx]);

						for (int k = 0; k < num_trays; k++) {
								start_trays[bit_idx][tray_idx][k].slots = GetSlots(start_trays[bit_idx][tray_idx][k].pt, rows, cols);
						}

		        		if (delta < 0.5) break;
		      
					}

					tmp_cluster = MinCostFlow(pointsets[T], start_trays[bit_idx][tray_idx], BITCNT[bit_idx]);
					res[bit_idx][tray_idx] = GetSilh(pointsets[T], start_trays[bit_idx][tray_idx], tmp_cluster);
				}
				// end Silhoutte 


				// begin capacitated k-means clustering
				#pragma omp parallel for
				for (int i = 1; i < 6; i++) {

						int rows = GetRows(BITCNT[i]), cols = BITCNT[i] / rows;
						float AR = (cols * WIDTH * RATIOS[i]) / (rows * HEIGHT);

						int num_trays = (num_flops + (BITCNT[i] - 1)) / BITCNT[i];

						int opt_idx = 0;
						float opt_val = -1;
						for (int j = 0; j < 5; j++) {
							if (res[i][j] > opt_val) {
								opt_val = res[i][j];
								opt_idx = j;
							}
						}

						trays[i] = start_trays[i][opt_idx];



						for (int j = 0; j < num_trays; j++) trays[i][j].slots = GetSlots(trays[i][j].pt, rows, cols);

						// clusters[i] = (tray that flop i belongs to, slot that flop i belongs to)
						float delta = 0;
						for (int j = 0; j < 35; j++) {
		            		clusters[i] = MinCostFlow(pointsets[T], trays[i], BITCNT[i]);
							delta = MBFF::RunLP(pointsets[T], trays[i], clusters[i], BITCNT[i]);
							for (int k = 0; k < num_trays; k++) {
								trays[i][k].slots = GetSlots(trays[i][k].pt, rows, cols);
							}

		            		if (delta < 0.5) break;
						}

						 clusters[i] = MinCostFlow(pointsets[T], trays[i], BITCNT[i]);

						for (int j = 0; j < num_trays; j++) {
							trays[i][j].slots = GetSlots(trays[i][j].pt, rows, cols);
						}
				}
				// end capacitated k-means clustering


				// remap the smaller pointset to the larger one
				map<int, int> small_to_large;
				for (int i = 0; i < num_flops; i++) {
					for (int j = 0; j < (int)flops.size(); j++) {
						if (pointsets[T][i].pt.x == flops[j].pt.x && pointsets[T][i].pt.y == flops[j].pt.y) {
							small_to_large[i] = j;
							break;
						}
					}
				}

				for (int i = 0; i < 6; i++) {
					for (int j = 0; j < (int)trays[i].size(); j++) {
						for (int k = 0; k < 70; k++) {
							trays[i][j].cand[k] = small_to_large[trays[i][j].cand[k]];
						}
					}
				}

				// append trays 
				for (int i = 0; i < 6; i++) {
					for (int j = 0; j < (int)trays[i].size(); j++) {
						all_trays[i].push_back(trays[i][j]);
					}
				}
    	}

    	MBFF::RunILP(flops, paths, all_trays, ALPHA, BETA);
	}


	// ctor
	MBFF::MBFF(int num_flops, int num_paths, vector<float> x, vector<float> y, vector<pair<int, int> > paths, int threads) {
		FLOPS.resize(num_flops);
		for (int i = 0; i < num_flops; i++) {
			Point new_pt; 
			new_pt.x = x[i] + 70.0, new_pt.y = y[i] + 70.0;

			Flop new_flop; 
			new_flop.pt = new_pt, new_flop.idx = i;
			FLOPS[i] = new_flop;
		}

		PATHS.resize(num_paths);
		for (int i = 0; i < num_paths; i++) {
			Path new_path;
			new_path.a = paths[i].first, new_path.b = paths[i].second;
			PATHS[i] = new_path;
		}

		NUM_THREADS = threads;
	}

	// dector
	MBFF::~MBFF() {
		FLOPS.clear();
		PATHS.clear();
		NUM_THREADS = 0;
	}


} // end namespace gpl 













