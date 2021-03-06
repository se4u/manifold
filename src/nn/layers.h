/*
* @Author: kmrocki@us.ibm.com
* @Date:   2017-03-03 15:06:37
* @Last Modified by:   Kamil Rocki
* @Last Modified time: 2017-04-21 18:04:04
*/

#ifndef __LAYERS_H__
#define __LAYERS_H__

#include <nn/nn_utils.h>
#include <nn/opt.h>

//abstract
class Layer {

	public:
	
		std::string name;
		
		//used in forward pass
		
		Matrix x; //inputs
		Matrix y; //outputs
		
		//grads, used in backward pass
		Matrix dx;
		Matrix dy;
		
		float current_dy_norm = 0.0f;
		float current_dx_norm = 0.0f;
		
		Layer ( size_t inputs, size_t outputs, size_t batch_size, std::string _label = "layer" ) : name ( _label ) {
		
			x = Matrix ( inputs, batch_size );
			y = Matrix ( outputs, batch_size );
			dx = Matrix ( inputs, batch_size );
			dy = Matrix ( outputs, batch_size );
			
			// avg_batch_activations.resize(y.rows());
			// avg_hidden_activations.resize(y.cols());
			// sparsity_correction.resize(y.rows());
			
		};
		
		//need to override these
		virtual void forward() = 0;
		virtual void backward ( bool accumulate = false ) = 0;
		virtual void resetGrads() {};
		virtual void applyGrads ( float alpha, float decay ) {};
		
		virtual void reset() {};
		virtual void kick ( float amount = 1 ) {};
		
		virtual void layer_info() { std:: cout << name << std::endl; }
		virtual ~Layer() {};
		
		virtual void save ( nanogui::Serializer &s ) const {
		
			s.set ( "x", x );
			s.set ( "y", y );
			s.set ( "dx", dx );
			s.set ( "dy", dy );
			s.set ( "name", name );
			
		};
		
		virtual bool load ( nanogui::Serializer &s ) {
		
			if ( !s.get ( "x", x ) ) return false;
			if ( !s.get ( "y", y ) ) return false;
			if ( !s.get ( "dx", dx ) ) return false;
			if ( !s.get ( "dy", dy ) ) return false;
			if ( !s.get ( "name", name ) ) return false;
			
			return true;
		}
		
		/*      -- penalties (L1 and L2):
		      if opt.coefL1 ~= 0 or opt.coefL2 ~= 0 then
		        local norm,sign= torch.norm,torch.sign
		        -- Loss:
		        f = f + opt.coefL1 * norm(parameters_D,1)
		        f = f + opt.coefL2 * norm(parameters_D,2)^2/2
		        -- Gradients:
		        gradParameters_D:add( sign(parameters_D):mul(opt.coefL1) + parameters_D:clone():mul(opt.coefL2) )
		      end*/
		
		virtual float w_norm( ) { return 0.0f; }
		virtual float dw_norm() { return 0.0f; }
		virtual float mw_norm() { return 0.0f; }
		
		virtual void sparsify ( float sparsity_penalty = 0.005f, float sparsity_target = 0.1f ) {
		
		
		
			/*		//if ( adjust_sparsity && sparsity_penalty > 1e-9f ) {
					float eps = 1e-3f;
			
					Eigen::VectorXf avg_batch_activations = y.rowwise().mean().array() + eps; // hidden num x 1
					Eigen::VectorXf avg_hidden_activations = y.colwise().mean().array() + eps;
					sparsity_target += eps;
			
					Eigen::VectorXf sparsity_correction =
					    sparsity_penalty *
					    (
					        ( -sparsity_target / avg_batch_activations.array() ) +
					        ( ( 1.0f - sparsity_target ) / ( 1.0f - avg_batch_activations.array() + eps ) ) );
			
					// filter out NaNs and INFs just in case
					checkNaNInf ( sparsity_correction );
					dy.colwise() += sparsity_correction;
			
					sparsity_target -= eps;
			
			*/
			
		}
		
		virtual float dx_norm() {
		
			return current_dx_norm;
			
		}
		
		virtual float dy_norm() {
		
			return current_dy_norm;
			
		}
		
		virtual void collect_statistics() {
		
			if ( x_avg_activity ) {
			
				x_avg_activity->head ( x_avg_activity->size() - 1 ) = x_avg_activity->tail ( x_avg_activity->size() - 1 );
				x_avg_activity->tail ( 1 ) ( 0 ) = x.mean();
				
			}
			
			if ( y_avg_activity ) {
			
				y_avg_activity->head ( y_avg_activity->size() - 1 ) = y_avg_activity->tail ( y_avg_activity->size() - 1 );
				y_avg_activity->tail ( 1 ) ( 0 ) = y.mean();
				
			}
			
			if ( x_hist_activity )
			
				histogram ( x, x_hist_activity );
				
				
			if ( y_hist_activity )
			
				histogram ( y, y_hist_activity );
				
				
			if ( dx_avg_activity ) {
			
				dx_avg_activity->head ( dx_avg_activity->size() - 1 ) = dx_avg_activity->tail ( dx_avg_activity->size() - 1 );
				dx_avg_activity->tail ( 1 ) ( 0 ) = dx.mean();
				
			}
			
			if ( dy_avg_activity ) {
			
				dy_avg_activity->head ( dy_avg_activity->size() - 1 ) = dy_avg_activity->tail ( dy_avg_activity->size() - 1 );
				dy_avg_activity->tail ( 1 ) ( 0 ) = dy.mean();
				
			}
			
			if ( dx_hist_activity )
			
				histogram ( dx, dx_hist_activity );
				
				
			if ( dy_hist_activity )
			
				histogram ( dy, dy_hist_activity );
				
			current_dx_norm = dx.norm();
			current_dy_norm = dy.norm();
			
			
		}
		
		Eigen::VectorXf *x_avg_activity = nullptr;
		Eigen::VectorXf *y_avg_activity = nullptr;
		Eigen::VectorXf *x_hist_activity = nullptr;
		Eigen::VectorXf *y_hist_activity = nullptr;
		Eigen::VectorXf *dx_avg_activity = nullptr;
		Eigen::VectorXf *dy_avg_activity = nullptr;
		Eigen::VectorXf *dx_hist_activity = nullptr;
		Eigen::VectorXf *dy_hist_activity = nullptr;
		
		//}
		
		// // sparsity
		// 	bool adjust_sparsity = false;
		// 	float sparsity_penalty = 0.0000f;
		// 	float sparsity_target = 0.2f;
		
		
		
		// count number of operations for perf counters
		long ops;
		
};

class Linear : public Layer {

	public:
	
		Matrix W;
		Matrix b;
		
		Matrix dW, d2W;
		Matrix db, d2b;
		
		Matrix mW;
		Matrix mb;
		
		// ADAM
		Matrix vW;
		Matrix vb;
		Matrix eW;
		Matrix eb;
		float t = 0.0f;
		
		float current_dw_norm = 0.0f;
		float current_w_norm = 0.0f;
		float current_mw_norm = 0.0f;
		
		// Matrix gaussian_noise;
		// bool add_gaussian_noise = false;
		// float bias_leakage = 0.0000001f;
		
		// void forward() {
		
		// 	y.setZero();// = b.replicate ( 1, x.cols() );
		
		// 	if ( add_gaussian_noise ) {
		
		// 		gaussian_noise.resize ( x.rows(), x.cols() );
		// 		matrix_randn ( gaussian_noise, 0, 0.1 );
		// 		x += gaussian_noise;
		// 	}
		
		// 	BLAS_mmul ( y, W, x );
		
		// }
		
		void forward () {
		
			y = b.replicate ( 1, x.cols() );
			BLAS_mmul ( y, W, x );
			
		}
		
		void backward ( bool accumulate = false ) {
		
			if ( !accumulate ) {
				dW.setZero();
				BLAS_mmul ( dW, dy, x, false, true );
				db = dy.rowwise().sum();
				dx.setZero();
				BLAS_mmul ( dx, W, dy, true, false );
			}
			else {
				BLAS_mmul ( dW, dy, x, false, true );
				db += dy.rowwise().sum();
				dx.setZero();
				BLAS_mmul ( dx, W, dy, true, false );
			}
			
			
		}
		
		Linear ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "linear" ) {
			
			reset();
			
			W = Matrix ( outputs, inputs );
			b = Vector::Zero ( outputs );
			double range = sqrt ( 6.0 / double ( inputs + outputs ) );
			
			mW = Matrix::Zero ( W.rows(), W.cols() );
			mb = Vector::Zero ( b.rows() );
			dW = Matrix::Zero ( W.rows(), W.cols() );
			db = Vector::Zero ( b.rows() );
			d2W = Matrix::Zero ( W.rows(), W.cols() );
			d2b = Vector::Zero ( b.rows() );
			
			// ADAM
			vW = Matrix::Zero ( W.rows(), W.cols() );
			vb = Vector::Zero ( b.rows() );
			eW = Matrix::Zero ( W.rows(), W.cols() );
			eb = Vector::Zero ( b.rows() );
			
			//matrix_rand ( W, -range, range );
			//matrix_randn ( W, 0, 0.01f );
			matrix_randn ( W, 0, ( 1.0f ) / sqrtf ( W.rows() + W.cols() ) );
			
		};
		
		virtual void collect_statistics() {
		
			Layer::collect_statistics();
			current_w_norm = W.norm();
			current_dw_norm = dW.norm();
			current_mw_norm = mW.norm();
			
		}
		virtual float w_norm() {
		
			return current_w_norm;
			
		}
		
		virtual float dw_norm() {
		
			return current_dw_norm;
			
		}
		
		virtual float mw_norm() {
		
			return current_mw_norm;
			
		}
		
		virtual void reset() {
		
			matrix_randn ( W, 0, ( 1.0f ) / sqrtf ( W.rows() + W.cols() ) );
			mW = Matrix::Zero ( W.rows(), W.cols() );
			mb = Vector::Zero ( b.rows() );
			vW = Matrix::Zero ( W.rows(), W.cols() );
			vb = Vector::Zero ( b.rows() );
			eW = Matrix::Zero ( W.rows(), W.cols() );
			eb = Vector::Zero ( b.rows() );
			
			resetGrads();
		}
		
		void resetGrads() {
		
			dW = Matrix::Zero ( W.rows(), W.cols() );
			db = Vector::Zero ( b.rows() );
		}
		
		virtual void kick ( float amount = 1 ) {
		
			if ( amount > 0 ) {
				Matrix gaussian_noise = Matrix ( W.rows(), W.cols() );
				matrix_randn ( gaussian_noise, 0, ( amount ) / sqrtf ( W.rows() + W.cols() ) );
				W += gaussian_noise;
			}
		}
		
		virtual void layer_info() { std:: cout << name << ": " << W.rows() << ", " << W.cols() << std::endl; }
		
		void adam ( float alpha, float decay = 0 ) {
		
			// ADAM
			float lr = 0.00005f;
			float beta1 = 0.9f;
			float beta2 = 0.999f;
			float epsilon = 1e-8f;
			
			checkNaNInf ( dW );
			checkNaNInf ( db );
			d2W = dW.cwiseProduct ( dW );
			d2b = db.cwiseProduct ( db );
			
			mW = mW * beta1 + ( 1 - beta1 ) * dW;
			vW = vW * beta2 + ( 1 - beta2 ) * d2W;
			eW = vW.unaryExpr ( std::ptr_fun ( sqrtf ) ).array() + epsilon;
			checkNaNInf ( eW );
			
			mb = mb * beta1 + ( 1 - beta1 ) * db;
			vb = vb * beta2 + ( 1 - beta2 ) * d2b;
			eb = vb.unaryExpr ( std::ptr_fun ( sqrtf ) ).array() + epsilon;
			checkNaNInf ( eb );
			
			t = t + 1.0f;
			float biasCorrection1 = 1.0f - powf ( beta1, t );
			float biasCorrection2 = 1.0f - powf ( beta2, t );
			
			W.noalias() = ( 1.0f - decay ) * W;
			b.noalias() = ( 1.0f - decay ) * b;
			
			d2W = ( sqrtf ( biasCorrection2 ) * mW.array() ) / ( eW.array() * biasCorrection1 ).array();
			d2b = ( sqrtf ( biasCorrection2 ) * mb.array() ) / ( eb.array() * biasCorrection1 ).array();
			
			checkNaNInf ( d2W );
			checkNaNInf ( d2b );
			W = W.array() + lr * d2W.array();
			b = b.array() + lr * d2b.array();
			
		}
		
		void rmsprop ( float alpha, float decay = 0 ) {
		
			//rmsprop
			
			float memory_loss = 1e-1f;
			
			d2W = dW.cwiseProduct ( dW );
			d2b = db.cwiseProduct ( db );
			checkNaNInf ( d2W );
			checkNaNInf ( d2b );
			
			mW.noalias() = mW * ( 1.0f - memory_loss ) + d2W;
			mb.noalias() = mb * ( 1.0f - memory_loss ) + d2b;
			
			W.noalias() = ( 1.0f - decay ) * W;
			b.noalias() = ( 1.0f - decay ) * b;
			t = t + 1.0f;
			
			// W.noalias() = ( 1.0f - decay ) * W + alpha * dW.cwiseQuotient ( mW.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			// b.noalias() = ( 1.0f - decay ) * b + alpha * db.cwiseQuotient ( mb.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			d2W = dW.cwiseQuotient ( mW.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			d2b = db.cwiseQuotient ( mb.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			checkNaNInf ( d2W );
			checkNaNInf ( d2b );
			
			W.noalias() = W + ( alpha ) * d2W;
			b.noalias() = b + ( alpha ) * d2b;
			
			
			// 'plain' fixed learning rate update
			// b.noalias() += alpha * db;
			// W.noalias() += alpha * dW;
			
			// flops_performed += 10 * ( dW.cols() * dW.rows() + db.cols() * db.rows() );
			// bytes_read += 7 * ( dW.cols() * dW.rows() + db.cols() * db.rows() );
			
		}
		
		void adagrad ( float alpha, float decay = 0 ) {
		
			float epsilon = 1e-8f;
			d2W = dW.cwiseProduct ( dW );
			d2b = db.cwiseProduct ( db );
			checkNaNInf ( d2W );
			checkNaNInf ( d2b );
			
			mW.noalias() = mW + d2W;
			mb.noalias() = mb + d2b;
			
			W.noalias() = ( 1.0f - decay ) * W;
			b.noalias() = ( 1.0f - decay ) * b;
			
			t = t + 1.0f;
			d2W = dW.cwiseQuotient ( mW.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			d2b = db.cwiseQuotient ( mb.unaryExpr ( std::ptr_fun ( sqrt_eps ) ) );
			checkNaNInf ( d2W );
			checkNaNInf ( d2b );
			
			W.noalias() = W + ( alpha ) * d2W ;
			b.noalias() = b + ( alpha ) * d2b ;
			
		}
		
		void sgd ( float alpha, float decay = 0 ) {
		
			W.noalias() = ( 1.0f - decay ) * W;
			b.noalias() = ( 1.0f - decay ) * b;
			
			b += alpha * db;
			W += alpha * dW;
			
		}
		
		void applyGrads ( float alpha, float decay = 0.0f ) {
		
			//sgd ( alpha, decay );
			//adagrad ( alpha, decay );
			rmsprop ( alpha, decay );
			//adam ( alpha, decay );
			
		}
		
		// // sgd
		// void sgd ( float alpha, float decay = 0.0f ) {
		
		// 	W *= ( 1.0f - decay );
		// 	b += alpha * db;
		// 	W += alpha * dW;
		// 	flops_performed += W.size() * 4 + 2 * b.size();
		// 	bytes_read += W.size() * sizeof ( dtype ) * 3;
		// }
		
		// // sgd
		// void sgd_momentum ( float alpha, float decay = 0.0f ) {
		
		// 	float momentum = 0.5f;
		
		// 	mW.array() = momentum * mW.array() + (1 - momentum) * dW.array();
		// 	mb.array() = momentum * mb.array() + (1 - momentum) * db.array();
		
		// 	W *= ( 1.0f - decay );
		
		// 	b += alpha * mb;
		// 	W += alpha * mW;
		
		// 	flops_performed += W.size() * 4 + 2 * b.size();
		// 	bytes_read += W.size() * sizeof ( dtype ) * 3;
		// }
		
		// // adagrad
		// void adagrad ( float alpha, float decay ) {
		
		// 	mW.array() += dW.array() * dW.array();
		// 	mb.array() += db.array() * db.array();
		
		// 	W *= ( 1.0f - decay );
		
		// 	b.array() += alpha * db.array() / (( mb.array() + 1e-6 )).sqrt().array();
		// 	W.array() += alpha * dW.array() / (( mW.array() + 1e-6 )).sqrt().array();
		
		// 	flops_performed += W.size() * 6 + 2 * b.size();
		// 	bytes_read += W.size() * sizeof ( dtype ) * 4;
		// }
		
		// // pseudo adadelta
		// void pseudo_adadelta ( float alpha, float decay ) {
		
		// 	float rho = 0.9f;
		
		// 	mW.array() = rho * mW.array() + (1 - rho) * dW.array() * dW.array();
		// 	mb.array() = rho * mb.array() + (1 - rho) * db.array() * db.array();
		
		// 	W *= ( 1.0f - decay );
		
		// 	b.array() += alpha * db.array() / (( mb.array() + 1e-6 )).sqrt().array();
		// 	W.array() += alpha * dW.array() / (( mW.array() + 1e-6 )).sqrt().array();
		
		// 	flops_performed += W.size() * 6 + 2 * b.size();
		// 	bytes_read += W.size() * sizeof ( dtype ) * 4;
		// }
		
		
		virtual void save ( nanogui::Serializer &s ) const {
		
			Layer::save ( s );
			s.set ( "W", W );
			s.set ( "b", b );
			s.set ( "dW", dW );
			s.set ( "db", db );
			// s.set("add_gaussian_noise", add_gaussian_noise);
			// s.set("gaussian_noise", gaussian_noise);
			
		};
		
		virtual bool load ( nanogui::Serializer &s ) {
		
			Layer::load ( s );
			if ( !s.get ( "W", W ) ) return false;
			if ( !s.get ( "b", b ) ) return false;
			if ( !s.get ( "dW", dW ) ) return false;
			if ( !s.get ( "db", db ) ) return false;
			// if (!s.get("add_gaussian_noise", add_gaussian_noise)) return false;
			// if (!s.get("gaussian_noise", gaussian_noise)) return false;
			
			return true;
		}
		
		~Linear() {};
		
};

class Sigmoid : public Layer {

	public:
	
		void forward() {
		
			y = logistic ( x );
			
		}
		
		void backward ( bool accumulate = false ) {
		
			dx.array() = dy.array() * y.array() * ( 1.0 - y.array() ).array();
			flops_performed += dx.size() * 3;
			bytes_read += x.size() * sizeof ( dtype ) * 2;
			
		}
		
		virtual void layer_info() { std:: cout << "sigm " << std::endl; }
		
		Sigmoid ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "sigmoid" ) {};
		~Sigmoid() {};
		
};

class Softmax : public Layer {

	public:
	
		void forward () {
		
			y = softmax ( x );
			
		}
		
		void backward ( bool accumulate = false ) {
		
			dx = dy - y;
		}
		
		
		Softmax ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "softmax" ) {};
		~Softmax() {};
		
};

class Identity : public Layer {

	public:
	
		void forward () {
		
			y = x;
			
		}
		
		void backward ( bool accumulate = false ) {
		
			dx = dy;
			
		}
		
		Identity ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "noop" ) {};
		~Identity() {};
		
};

class ReLU : public Layer {

	public:
	
		void forward () {
		
			y = rectify ( x );
			
		}
		
		void backward ( bool accumulate = false ) {
		
			dx.array() = derivative_ReLU ( y ).array() * dy.array();
			
		}
		
		ReLU ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "relu" ) {};
		~ReLU() {};
		
};

// Exponential Linear Unit
// http://arxiv.org/pdf/1511.07289v5.pdf

class ELU : public Layer {

	public:
	
		void forward () {
		
			y = activation_ELU ( x );
			
		}
		
		void backward ( bool accumulate = false ) {
		
			dx.array() = derivative_ELU ( y ).array() * dy.array();
			
		}
		
		ELU ( size_t inputs, size_t outputs, size_t batch_size ) :
			Layer ( inputs, outputs, batch_size, "elu" ) {};
		~ELU() {};
		
};

class Dropout : public Layer {

	public:
	
		float keep_ratio;
		Matrix dropout_mask;
		
		void forward () {
		
			// if ( test ) // skip at test time
			
			y = x;
			
			// else {
			
			Matrix rands = Matrix::Zero ( y.rows(), y.cols() );
			matrix_rand ( rands, 0.0f, 1.0f );
			
			//dropout mask - 1s - preserved elements
			dropout_mask = ( rands.array() < keep_ratio ).cast <float> ();
			
			// y = y .* dropout_mask, discard elements where mask is 0
			y.array() = x.array() * dropout_mask.array();
			
			// normalize, so that we don't have to do anything at test time
			y /= keep_ratio;
			
			// }
		}
		
		void backward ( bool accumulate = false ) {
		
			dx.array() = dy.array() * dropout_mask.array();
		}
		
		virtual void save ( nanogui::Serializer &s ) const {
		
			Layer::save ( s );
			s.set ( "keep_ratio", keep_ratio );
			
		};
		
		virtual bool load ( nanogui::Serializer &s ) {
		
			Layer::load ( s );
			if ( !s.get ( "keep_ratio", keep_ratio ) ) return false;
			
			return true;
		}
		
		Dropout ( size_t inputs, size_t outputs, size_t batch_size, float _ratio ) :
			Layer ( inputs, outputs, batch_size, "dropout" ),  keep_ratio ( _ratio ) {};
		~Dropout() {};
		
};

// template <class A>
// void process_one_type() {
// 	std::cout << typeid(A).name() << ' ';
// }


// template<class ... Layers>
// class ComplexLayer {

//   public:
// 	ComplexLayer(size_t inputs) {

// 		int _[] = {0, (process_one_type<Layers>(), 0)...};
// 		(void)_;
// 		std::cout << '\n';

// 	};
// 	~ComplexLayer() = default;


// };

// template<class ... Args> void layer_combo(Args ... args) {

// 	int dummy[] = { 0, ( (void) bar(std::forward<Args>(args)), 0) ... };
// }

// ComplexLayer<Sigmoid, Linear>(64)
// ComplexLayer<Sigmoid, Linear>(3)
// ComplexLayer<Sigmoid, Linear>(64)
// ComplexLayer<Sigmoid>(input_width)

// 	layer<input_width, Dropout, Linear>,
// 	layer<64, Sigmoid, Linear>,
// 	layer<3, Sigmoid, Linear>,
// 	layer<64, Sigmoid, Linear>,
// 	layer<input_width, Sigmoid>
// };

// auto net = {ComplexLayer<Dropout, Linear>(input_width)};
// std::cout << typeid(net).name() << ' ';

// ({size_t inputs, std::initializer_list<Layer> layers})

// std::string net_definition = "x 784 dropout(0.5) linsigm(64) linsigm(3) linsigm(64) linsigm(784) y";
// std::vector<std::string> tokens = split(net_definition);
// for (int i = 0; i < tokens.size(); i++)
// 	std::cout << tokens[i]  << std::endl;

#endif
