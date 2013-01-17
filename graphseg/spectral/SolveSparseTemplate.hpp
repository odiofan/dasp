/*
 * SolveSparseTemplate.hpp
 *
 *  Created on: Okt 20, 2012
 *      Author: david
 */

#ifndef DASP_SPECTRAL_SOLVESPARSETEMPLATE_HPP_
#define DASP_SPECTRAL_SOLVESPARSETEMPLATE_HPP_

#define SPECTRAL_VERBOSE
#include "../Common.hpp"
#include "../as_range.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <arpack++/arlssym.h>
#include <iostream>
#include <vector>
#include <cmath>

namespace graphseg { namespace detail {

template<typename Graph>
std::vector<EigenComponent> SolveSparseTemplate(const Graph& graph, unsigned int num_ev)
{
#ifdef SPECTRAL_VERBOSE
	std::cout << "Sparse Solver: started" << std::endl;
#endif

	// We want to solve the EV problem: (D - W) x = \lamda D x.
	// Each edge of the graph defines two entries into the symmetric matrix W.
	// The diagonal matrix D is defined via d_i = sum_j{w_ij}.

	// As D is a diagonal matrix the the general problem can be easily transformed
	// into a normal eigenvalue problem by decomposing D = L L^t, which yields L = sqrt(D).
	// Thus the EV problem is: L^{-1} (D - W) L^{-T} y = \lambda y.
	// Eigenvectors can be transformed using x = L^{-T} y.

#ifdef SPECTRAL_VERBOSE
	std::cout << "Sparse Solver: preparing problem" << std::endl;
#endif

	// The dimension of the problem
	const int n = boost::num_vertices(graph);

	struct Entry {
		int i, j;
		Real value;
	};

	// Each edge defines two entries (one in the upper and one in the lower).
	// In addition all diagonal entries are non-zero.
	// Thus the number of non-zero entries in the lower triangle is equal to
	// the number of edges plus the number of nodes.
	// This is not entirely true as some connections are possibly rejected.
	// Additionally some connections may be added to assure global connectivity.
	const int nnz_guess = boost::num_edges(graph) + n;

	// collect all non-zero elements
	std::vector<Entry> entries;
	entries.reserve(nnz_guess);

	// also collect diagonal entries
	std::vector<Real> diag(n);

	// no collect entries
	for(auto eid : as_range(boost::edges(graph))) {
		int ea = static_cast<int>(boost::source(eid, graph));
		int eb = static_cast<int>(boost::target(eid, graph));
		float ew = boost::get(boost::edge_weight, graph, eid);
		// assure correct edge weight
		if(std::isnan(ew)) {
			std::cerr << "Weight for edge (" << ea << "," << eb << ") is nan!" << std::endl;
			continue;
		}
		if(ew < 0) {
			std::cerr << "Weight for edge (" << ea << "," << eb << ") is negative!" << std::endl;
			continue;
		}
		// assure that no vertices is connected to self
		if(ea == eb) {
			std::cerr << "Vertex " << ea << " is connected to self!" << std::endl;
			continue;
		}
		// In the lower triangle the row index i is bigger or equal than the column index j.
		// The next statement fullfills this requirement.
		if(ea < eb) {
			std::swap(ea, eb);
		}
		entries.push_back(Entry{ea, eb, ew});
		diag[ea] += ew;
		diag[eb] += ew;
	}

	// do the conversion to a normal ev problem
	// assure global connectivity
	for(unsigned int i=0; i<diag.size(); i++) {
		Real& v = diag[i];
		if(v == 0) {
			// connect the disconnected cluster to all other clusters with a very small weight
			v = static_cast<Real>(1);
			Real q = static_cast<Real>(1) / static_cast<Real>(n-1);
			for(unsigned int j=0; j<i; j++) {
				auto it = std::find_if(entries.begin(), entries.end(), [i, j](const Entry& e) { return e.i == i && e.j == j; });
				if(it == entries.end()) {
					entries.push_back(Entry{i, j, q});
				}
			}
			for(unsigned int j=i+1; j<n; j++) {
				auto it = std::find_if(entries.begin(), entries.end(), [j, i](const Entry& e) { return e.i == j && e.j == i; });
				if(it == entries.end()) {
					entries.push_back(Entry{j, i, q});
				}
			}
			std::cerr << "Diagonal is 0! (i=" << i << ")" << std::endl;
		}
		else {
			v = static_cast<Real>(1) / std::sqrt(v);
		}
	}

	// a_ij for the transformed "normal" EV problem
	//		A x = \lambda x
	// is computed as follow from the diagonal matrix D and the weight
	// matrix W of the general EV problem
	//		(D - W) x = \lambda D x
	// as follows:
	//		a_ij = - w_ij / sqrt(d_i * d_j) if i != j
	//		a_ii = 1
	for(Entry& e : entries) {
		e.value = - e.value * diag[e.i] * diag[e.j];
	}
	for(unsigned int i=0; i<n; i++) {
		entries.push_back(Entry{i, i, static_cast<Real>(1)});
	}

	// sort entries to form a lower triangle matrix
	std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
		return (a.j != b.j) ? (a.j < b.j) : (a.i < b.i);
	});

	// define ARPACK matrix (see p. 119 in ARPACK++ manual)
#ifdef SPECTRAL_VERBOSE
	std::cout << "Sparse Solver: defining matrix" << std::endl;
#endif
	int nnz = entries.size();
	std::vector<Real> nzval(nnz);
	std::vector<int> irow(nnz);
	std::vector<int> pcol;
	pcol.reserve(n + 1);
	std::ofstream ofs("/tmp/sparse.tsv");
	// Assumes that each column has at least one non-zero element.
	int current_col = -1;
	for(unsigned int i=0; i<entries.size(); i++) {
		const Entry& e = entries[i];
		ofs << e.i << "\t" << e.j << "\t" << e.value << std::endl;
		nzval[i] = e.value;
		irow[i] = e.i;
		if(e.j == current_col + 1) {
			pcol.push_back(i);
			current_col++;
		}
	}
	ofs.close();
	pcol.push_back(nnz);
//	// check CRC
//	{
//		int i, j, k;
//
//		// Checking if pcol is in ascending order.
//
//		i = 0;
//		while ((i!=n)&&(pcol[i]<=pcol[i+1])) i++;
//		if (i!=n) {
//		  std::cout << "Error 1" << std::endl;
//		  std::cout << i << std::endl;
//		  throw 0;
//		}
//
//		// Checking if irow components are in order and within bounds.
//
//		for (i=0; i!=n; i++) {
//		j = pcol[i];
//		k = pcol[i+1]-1;
//		if (j<=k) {
//		  if ((irow[j]<i)||(irow[k]>=n)) {
//			  std::cout << "Error 2" << std::endl;
//			  std::cout << i << std::endl;
//			  throw 0;
//		  }
//		  while ((j!=k)&&(irow[j]<irow[j+1])) j++;
//		  if (j!=k) {
//			  std::cout << "Error 3" << std::endl;
//			  std::cout << i << ", " << irow[j] << " -> " << irow[j+1] << std::endl;
//			  std::cout << j << " -> " << k << std::endl;
//			  throw 0;
//		  }
//		}
//		}
//	}
	ARluSymMatrix<Real> mat(n, nnz, nzval.data(), irow.data(), pcol.data());

	// solve ARPACK problem (see p. 82 in ARPACK++ manual)
#ifdef SPECTRAL_VERBOSE
	std::cout << "Sparse Solver: solving ..." << std::flush;
#endif
	num_ev = std::min<unsigned int>(num_ev, n);
	if(static_cast<float>(num_ev)/static_cast<float>(n) > 0.1f) {
		std::cout << "Warning: Using sparse eigensolver, but trying to get a huge number of eigenvectors!" << std::endl;
	}
	ARluSymStdEig<Real> solv(num_ev, mat, "SM");
	std::vector<Real> v_ew(num_ev);
	std::vector<Real> v_ev(num_ev * n);
	Real* p_ew = v_ew.data();
	Real* p_ev = v_ev.data();
	solv.EigenValVectors(p_ev, p_ew, false);
#ifdef SPECTRAL_VERBOSE
	std::cout << " finished." << std::endl;
	std::cout << "Sparse Solver: collecting results" << std::endl;
#endif
	std::vector<EigenComponent> solution(num_ev);
	for(unsigned int i=0; i<num_ev; i++) {
		EigenComponent& cmp = solution[i];
		cmp.eigenvalue = p_ew[i];
#ifdef SPECTRAL_VERBOSE
		std::cout << "Eigenvalue " << i << ": " << cmp.eigenvalue << std::endl;
		cmp.eigenvector = Vec(n);
#endif
		for(unsigned int j=0; j<n; j++) {
			Real v = p_ev[i*n + j];
			// convert back to generalized eigenvalue problem!
			v *= diag[j];
			cmp.eigenvector[j] = v;
		}
	}
#ifdef SPECTRAL_VERBOSE
	std::cout << "Sparse Solver: returning" << std::endl;
#endif
	return solution;
}

}}

#endif
