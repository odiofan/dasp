/*
 * Graph.hpp
 *
 *  Created on: May 18, 2012
 *      Author: david
 */

#ifndef DASP_GRAPH_HPP_
#define DASP_GRAPH_HPP_

#include "Point.hpp"
#include "graphseg/as_range.hpp"
#include "graphseg/Spectral.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <Eigen/Dense>

namespace dasp
{
	/** Undirected graph */
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS> UndirectedGraph;

	/** Undirected, weighted (type=float) graph */
	typedef graphseg::SpectralGraph UndirectedWeightedGraph;

	/** Weighted graph of superpoints */
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::directedS,
		Point,
		boost::property<boost::edge_weight_t, float>
	> DaspGraph;

}

#endif
