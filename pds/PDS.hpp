#ifndef INCLUDED_PDS_PDS_HPP
#define INCLUDED_PDS_PDS_HPP

#include <Eigen/Dense>
#include <vector>

namespace pds
{

	std::vector<Eigen::Vector2f> RectGrid(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> HexGrid(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> SimplifiedPDSOld(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> SimplifiedPDS(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> FloydSteinberg(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> FloydSteinbergExpo(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> FloydSteinbergMultiLayer(const Eigen::MatrixXf& density);

	std::vector<Eigen::Vector2f> Fattal(const Eigen::MatrixXf& density, unsigned int max_steps=0);

}

#endif
