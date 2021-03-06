/*
* @Author: kmrocki@us.ibm.com
* @Date:   2017-03-20 21:17:46
* @Last Modified by:   kmrocki@us.ibm.com
* @Last Modified time: 2017-03-28 12:27:28
*/

#ifndef __NBODY_H__
#define __NBODY_H__

namespace nbody {

void calculate_forces(Eigen::MatrixXf& points, Eigen::MatrixXf& velocities, float softening = 1e-7f, float dt = 0.001f) {

	//#pragma omp parallel for schedule(dynamic)
	for (size_t i = 0; i < (size_t) points.cols(); i++) {

		Eigen::Vector3f p = points.col(i);
		Eigen::Vector3f f = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
		Eigen::Vector3f d;

		for (size_t j = 0; j < (size_t) points.cols(); j++) {

			d = points.col(j) - p;
			float dist = d[0] * d[0] + d[1] * d[1] + d[2] * d[2] + softening;
			float invDist = 1.0f / sqrtf(dist);
			float invDist3 = invDist * invDist * invDist;

			f += invDist3 * d;

		}

		// integrate v
		velocities.col(i).noalias() += dt * f;

		// integrate position
		points.col(i).noalias() += velocities.col(i) * dt;

	}

}
}
#endif