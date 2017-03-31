#ifndef ___FUNCTIONS_H___
#define ___FUNCTIONS_H___

#include <utils.h>
#include <rand/rngs.h>

namespace func1 {

	float normal ( float scale, size_t i = 0 ) {
	
		UNUSED ( i );
		return scale * rng.randn();
		
	}
	
	float uniform ( float scale, size_t i = 0 ) {
	
		UNUSED ( i );
		return rng.get ( std::uniform_real_distribution<> ( -scale, scale ) );
		
	}
	
}

namespace func3 {

	float hat ( float x, float y ) {
	
		float t = hypotf ( x, y ) * 1.0;
		float z = ( 1 - t * t ) * expf ( t * t / -2.0 );
		return z;
		
	}
	
	float normal ( float x, float y ) {
	
		UNUSED ( x, y );
		return func1::normal ( 1 );
		
	}
	
	float uniform ( float x, float y ) {
	
		UNUSED ( x, y );
		return func1::uniform ( 1 );
		
	}
	
}

template <class FX, class FY, class FZ>
void generate ( FX fx, FY fy, FZ fz, Eigen::MatrixXf &points, size_t N = 1 ) {

	points.resize ( 3, N );
	
	for ( size_t i = 0; i < N; i++ ) {
	
		float x = rng.get ( fx );
		float y = rng.get ( fy );
		float z = rng.get ( fz );
		
		points.col ( i ) << x, y, z;
		
	}
}

template <class Function, class Sampler>
void generate ( Function f, Sampler s, Eigen::MatrixXf &points, size_t N = 1, float scale = 1.0f ) {

	points.resize ( 3, N );
	
	for ( size_t i = 0; i < N; i++ ) {
	
		float x = s ( scale, i );
		float y = s ( scale, i );
		
		points.col ( i ) << x, y, scale *f ( x, y );
		
	}
}

void set ( const Eigen::Vector3f c, Eigen::MatrixXf &points, size_t N = 1 ) {

	points.resize ( 3, N );
	points.colwise() = c;
	
}

#endif