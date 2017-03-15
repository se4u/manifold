
#ifndef __PLOTPOINTS_H__
#define __PLOTPOINTS_H__

#include <nanogui/glcanvas.h>
#include <nanogui/checkbox.h>
#define POINT_SHADER_NAME "point_shader"
#define POINT_FRAG_FILE "./src/glsl/surf_point.f.glsl"
#define POINT_VERT_FILE "./src/glsl/surf_point.v.glsl"

#define GRID_SHADER_NAME "grid_shader"
#define GRID_FRAG_FILE "./src/glsl/surf_grid.f.glsl"
#define GRID_VERT_FILE "./src/glsl/surf_grid.v.glsl"

#define BOX_SHADER_NAME "box_shader"
#define BOX_FRAG_FILE "./src/glsl/surf_box.f.glsl"
#define BOX_VERT_FILE "./src/glsl/surf_box.v.glsl"

#include <colors.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/graph.h>
#include <nanogui/layout.h>
#include <nanogui/imagepanel.h>

// FPS
#include "fps.h"

class SurfWindow : public nanogui::Window {

	public:
	
		SurfWindow ( Widget *parent, const std::string &title ) : nanogui::Window ( parent, title ) {
		
			/* FPS GRAPH */
			m_window = new nanogui::Window ( this, "" );
			m_window->setLayout ( new nanogui::GroupLayout() );
			
			//legend
			nanogui::Graph *graph = new nanogui::Graph ( m_window, "", nanogui::GraphType::GRAPH_LEGEND );
			graph->values().resize ( 10 );
			graph->values() << 1, 1, 1, 1, 1, 1, 1, 1, 1, 1;
			
			m_grid = new nanogui::CheckBox ( this, "Grid" );
			m_polar = new nanogui::CheckBox ( this, "Polar" );
			
			m_grid->setChecked ( true );
			
		}
		
		nanogui::Window *m_window;
		nanogui::CheckBox *m_grid, *m_polar;
		
};

class SurfPlot : public nanogui::GLCanvas {
	public:
		SurfPlot ( Widget *parent, const Eigen::Vector2i &w_size,
				   SurfWindow &helper_window ) : widgets ( helper_window ) , nanogui::GLCanvas ( parent ) {
			using namespace nanogui;
			
			
			setSize ( w_size );
			
			setBackgroundColor ( nanogui::Color ( 0, 0, 0, 32 ) );
			setDrawBorder ( true );
			
			// init shaders
			m_pointShader = new nanogui::GLShader();
			m_pointShader->initFromFiles ( POINT_SHADER_NAME, POINT_VERT_FILE, POINT_FRAG_FILE );
			m_gridShader = new nanogui::GLShader();
			m_gridShader->initFromFiles ( GRID_SHADER_NAME, GRID_VERT_FILE, GRID_FRAG_FILE );
			m_boxShader = new nanogui::GLShader();
			m_boxShader->initFromFiles ( BOX_SHADER_NAME, BOX_VERT_FILE, BOX_FRAG_FILE );
			
			refresh();
			setVisible ( true );
			
			// set default view
			Eigen::Matrix3f mat;
			
			mat.setIdentity();
			// mat ( 0, 0 ) = 0.813283; mat ( 0, 1 ) =  -0.424654; mat ( 0, 2 ) = -0.397795;
			// mat ( 1, 0 ) = -0.0271025; mat ( 1, 1 ) =  -0.710554; mat ( 1, 2 ) = 0.70312;
			// mat ( 2, 0 ) = -0.581237; mat ( 2, 1 ) =  -0.561054; mat ( 2, 2 ) = -0.589391;
			
			Eigen::Quaternionf q ( mat );
			m_arcball.setState ( q );
			setSize ( w_size );
			m_arcball.setSize ( w_size );
			
			translation = Eigen::Vector3f::Zero();
			model_angle = Eigen::Vector3f::Zero();
			
			ray_wor.setOnes();
			
			drag_sensitivity = 5.0f;
			scroll_sensitivity = 10.0f;
			keyboard_sensitivity = 0.1f;
			
		}
		
		void generate_points() {
		
			m_pointCount = nn->codes.cols();
			
			positions.resize ( 3, m_pointCount );
			colors.resize ( 3, m_pointCount );
			
			for ( size_t i = 0; i < m_pointCount; i++ ) {
			
				Eigen::VectorXf coords;
				int label;
				
				coords = nn->codes.col ( i );
				label = nn->codes_idxs ( i );
				
				float x, y, z;
				
				//if (spherical) {
				
				// float phi = coords[2];
				
				// 3d
				// x =	r * cos ( theta ) * sin ( phi );
				// y = r * sin ( theta )  * cos ( phi );
				// z = r * cos ( phi );
				
				//}
				
				if ( widgets.m_polar->checked() ) {
				
					float r = coords[0];
					float theta = coords[1];
					x =	r * cos ( theta );
					y = r * sin ( theta );
				}
				else {
					x = coords[0];
					y = coords[1];
					
				}
				if ( coords.size() > 2 ) z = coords[2];
				else z = 0.0f;
				
				positions.col ( i ) << x / 30.0f, y / 30.0f, z / 30.0f;
				// std::cout << positions.col ( i ) << std::endl;
				nanogui::Color c = nanogui::parula_lut[label];
				colors.col ( i ) << c.r(), c.g(), c.b();
				
			}
			
		}
		
		bool mouseMotionEvent ( const Eigen::Vector2i &p, const Eigen::Vector2i &rel, int button, int modifiers ) override {
			// if ( !SurfPlot::mouseMotionEvent ( p, rel, button, modifiers ) )
			m_arcball.motion ( p );
			
			std::cout << p[0] << ", " << p[1] << " pos , " << mPos[0] << ", " << mPos[1] << std::endl;
			
			/* takes mouse position on screen and return ray in world coords */
			Eigen::Vector3f relative_pos = Eigen::Vector3f ( 2.0f * ( float ) ( p[0] - mPos[0] ) / ( float ) mSize[0] - 1.0f,
										   ( float ) ( -2.0f * ( p[1] - mPos[1] ) ) / ( float ) mSize[0] + 1.0f, 1.0f );
										   
			float x = relative_pos[0];
			float y = relative_pos[1];
			
			std::cout << x << ", " << y << ", " << std::endl;
			
			Eigen::Vector3f ray_nds ( x, y, -1.0f );
			Eigen::Vector4f ray_clip ( x, y, -1.0f, -1.0f );
			Eigen::Vector4f ray_eye = proj.inverse() * ray_clip;
			ray_eye[2] = -1.0f; ray_eye[3] = 0.0f;
			Eigen::Vector4f ray_world = view.inverse() * ray_eye;
			ray_wor[0] = ray_world[0]; ray_wor[1] = ray_world[1]; ray_wor[2] = ray_world[2];
			ray_wor.normalize();
			// near clip plane
			// Eigen::Vector3f eye = Eigen::Vector3f ( 0, 0, -1 );
			// Eigen::Vector3f ray_clip_near = Eigen::Vector3f ( x, y, 2.0f );
			// Eigen::Vector3f ray_dir = eye - ray_clip_near;
			// ray_dir /= ray_dir[2];
			
			// std::cout << std::endl << "ray_clip_near" << std::endl;
			// std::cout << ray_clip_near << std::endl;
			
			// std::cout << std::endl << "ray_dir" << std::endl;
			// std::cout << ray_dir << std::endl;
			
			// Eigen::Vector3f ray_clip_far = 100.0f * Eigen::Vector3f ( ray_dir );
			// std::cout << std::endl << "ray_clip_farr" << std::endl;
			// std::cout << ray_clip_far << std::endl;
			
			// Eigen::Vector3f ray_clip_zero = Eigen::Vector3f ( ray_dir );
			// std::cout << std::endl << "ray_clip_zero" << std::endl;
			// std::cout << ray_clip_zero << std::endl;
			
			// //Eigen::Matrix4f inversed_proj = proj;
			// //inversed_proj.inverse();
			// //Eigen::Matrix4f inversed_view = view;
			// //inversed_view.inverse();
			// // Eigen::Matrix4f inversed_model = model;
			// // inversed_model = model.inverse();
			
			// std::cout << model << std::endl;
			// std::cout << std::endl << model.inverse() << std::endl;
			
			// Eigen::Vector4f tmp = Eigen::Vector4f ( ray_clip_zero[0], ray_clip_zero[1], 0, 1.0f );
			// tmp = model.inverse() * tmp;
			// ray_wor = Eigen::Vector3f ( tmp[0], tmp[1], tmp[2] );
			
			
			
			return true;
		}
		
		bool mouseButtonEvent ( const Eigen::Vector2i &p, int button, bool down, int modifiers ) override {
		
			// if ( !SurfPlot::mouseButtonEvent ( p, button, down, modifiers ) ) {
			if ( button == GLFW_MOUSE_BUTTON_1 )
				m_arcball.button ( p, down );
			// }
			
			return true;
		}
		
		virtual bool keyboardEvent ( int key, int scancode, int action, int modifiers ) override {
		
			// if ( SurfPlot::keyboardEvent ( key, scancode, action, modifiers ) )
			// 	return true;
			
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS ) {
				setVisible ( false );
				return true;
			}
			
			return false;
		}
		
		void refresh() {
		
			m_pointCount = ( int ) m_pointCount;
			
			generate_points();
			
			/* Upload points to GPU */
			m_pointShader->bind();
			m_pointShader->uploadAttrib ( "position", positions );
			m_pointShader->uploadAttrib ( "color", colors );
			
			/* Upload lines to GPU */
			if ( widgets.m_grid->checked() ) {
			
				m_gridlineCount = 9 * 4 * 2;
				gridline_positions.resize ( 3, m_gridlineCount );
				
				size_t ii = 0;
				for ( float k = -1.0f; k <= 1.0f; k += 0.25f ) {
				
					gridline_positions.col ( ii + 0 ) << 0, 0, 0; gridline_positions.col ( ii + 1 ) << 0, k, 0; //L
					gridline_positions.col ( ii + 2 ) << k, 0, 0; gridline_positions.col ( ii + 3 ) << k, k, 0; //R
					gridline_positions.col ( ii + 4 ) << 0, 0, 0; gridline_positions.col ( ii + 5 ) << k, 0, 0; //B
					gridline_positions.col ( ii + 6 ) << 0, k, 0; gridline_positions.col ( ii + 7 ) << k, k, 0; //T
					
					// gridline_positions.col ( ii + 8 ) << 0, 0, 0; gridline_positions.col ( ii + 9 ) << 0, 0, k;
					// gridline_positions.col ( ii + 10 ) << k, 0, 0; gridline_positions.col ( ii + 11 ) << k, 0, k;
					// gridline_positions.col ( ii + 12 ) << 0, k, 0; gridline_positions.col ( ii + 13 ) << 0, k, k;
					// gridline_positions.col ( ii + 14 ) << k, k, 0; gridline_positions.col ( ii + 15 ) << k, k, k;
					
					// gridline_positions.col ( ii + 16 ) << 0, 0, k; gridline_positions.col ( ii + 17 ) << 0, k, k; //UL
					// gridline_positions.col ( ii + 18 ) << k, 0, k; gridline_positions.col ( ii + 19 ) << k, k, k; //UR
					// gridline_positions.col ( ii + 20 ) << 0, 0, k; gridline_positions.col ( ii + 21 ) << k, 0, k; //UB
					// gridline_positions.col ( ii + 22 ) << 0, k, k; gridline_positions.col ( ii + 23 ) << k, k, k; //UT
					
					ii += 4 * 2;
				}
				
			}
			
			// draw grid
			m_boxlineCount = 12 * 2;
			boxline_positions.resize ( 3, m_boxlineCount );
			
			boxline_positions.col ( 0 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 1 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f; //L
			boxline_positions.col ( 2 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 3 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f; //R
			boxline_positions.col ( 4 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 5 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f; //B
			boxline_positions.col ( 6 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 7 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f; //T
			boxline_positions.col ( 8 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 9 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 10 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 11 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 12 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 13 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 14 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] - 0.01f;
			boxline_positions.col ( 15 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 16 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 17 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f; //UL
			boxline_positions.col ( 18 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 19 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f; //UR
			boxline_positions.col ( 20 ) << ray_wor[0] - 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 21 ) << ray_wor[0] + 0.01f, ray_wor[1] - 0.01f, ray_wor[2] + 0.01f; //UB
			boxline_positions.col ( 22 ) << ray_wor[0] - 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f;
			boxline_positions.col ( 23 ) << ray_wor[0] + 0.01f, ray_wor[1] + 0.01f, ray_wor[2] + 0.01f; //UT
			
			m_gridShader->bind();
			m_gridShader->uploadAttrib ( "position", gridline_positions );
			
			m_boxShader->bind();
			m_boxShader->uploadAttrib ( "position", boxline_positions );
			
			
		}
		
		// smooth keys - TODO: move to nanogui
		void process_keyboard() {
		
			// keyboard management
			/*	TODO: move to nanogui - need to modify keyboardEvent to allow smooth opeartion */
			// translation
			if ( glfwGetKey ( screen->glfwWindow(), 'A' ) == GLFW_PRESS ) translation[0] -= 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'D' ) == GLFW_PRESS ) translation[0] += 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'S' ) == GLFW_PRESS ) translation[1] -= 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'W' ) == GLFW_PRESS ) translation[1] += 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'Z' ) == GLFW_PRESS ) translation[2] -= 0.5 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'C' ) == GLFW_PRESS ) translation[2] += 0.5 * keyboard_sensitivity;
			
			if ( glfwGetKey ( screen->glfwWindow(), '1' ) == GLFW_PRESS ) fov += 0.05;
			if ( glfwGetKey ( screen->glfwWindow(), '2' ) == GLFW_PRESS ) fov -= 0.05;
			
			// rotation around x, y, z axes
			if ( glfwGetKey ( screen->glfwWindow(), 'Q' ) == GLFW_PRESS ) model_angle[0] -= 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), 'E' ) == GLFW_PRESS ) model_angle[0] += 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), GLFW_KEY_UP ) == GLFW_PRESS ) model_angle[1] -= 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), GLFW_KEY_DOWN ) == GLFW_PRESS ) model_angle[1] += 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), GLFW_KEY_LEFT ) == GLFW_PRESS ) model_angle[2] -= 0.05 * keyboard_sensitivity;
			if ( glfwGetKey ( screen->glfwWindow(), GLFW_KEY_RIGHT ) == GLFW_PRESS ) model_angle[2] += 0.05 * keyboard_sensitivity;
			
		}
		
		void drawGL() override {
		
			process_keyboard();
			
			/* Set up a perspective camera matrix */
			
			view = nanogui::lookAt ( Eigen::Vector3f ( 0, 0, 1 ), Eigen::Vector3f ( 0, 0, 0 ), Eigen::Vector3f ( 0, 1, 0 ) );
			const float near = 0.01, far = 100;
			float fH = std::tan ( fov / 360.0f * M_PI ) * near;
			float fW = fH * ( float ) mSize.x() / ( float ) mSize.y();
			proj = nanogui::frustum ( -fW, fW, -fH, fH, near, far );
			
			model.setIdentity();
			model = m_arcball.matrix() * model;
			model = nanogui::translate ( Eigen::Vector3f ( translation ) ) * model;
			
			/* Render the point set */
			mvp = proj * view * model;
			
			m_pointShader->bind();
			m_pointShader->setUniform ( "mvp", mvp );
			
			glPointSize ( 1 );
			glEnable ( GL_DEPTH_TEST );
			
			m_pointShader->drawArray ( GL_POINTS, 0, m_pointCount );
			
			if ( widgets.m_grid->checked() ) {
				m_gridShader->bind();
				m_gridShader->setUniform ( "mvp", mvp );
				glEnable ( GL_BLEND );
				glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				m_gridShader->drawArray ( GL_LINES, 0, m_gridlineCount );
				glDisable ( GL_BLEND );
			}
			
			m_boxShader->bind();
			m_boxShader->setUniform ( "mvp", mvp );
			glEnable ( GL_BLEND );
			glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			m_boxShader->drawArray ( GL_LINES, 0, m_boxlineCount );
			glDisable ( GL_BLEND );
			
			
			// perf stats
			update_FPS ( *graph_data );
			
		}
		
		~SurfPlot() {
		
			delete m_pointShader;
			delete m_gridShader;
			delete m_boxShader;
			
		}
		
		size_t m_pointCount = 1;
		size_t m_gridlineCount = 12 * 2;
		size_t m_boxlineCount = 12 * 2;
		
		// shaders
		nanogui::GLShader *m_pointShader = nullptr;
		nanogui::GLShader *m_gridShader = nullptr;
		nanogui::GLShader *m_boxShader = nullptr;
		
		nanogui::Arcball m_arcball;
		
		Eigen::VectorXf *graph_data;
		
		Eigen::Vector3f translation;
		Eigen::Vector3f model_angle;
		
		// coords of points
		Eigen::MatrixXf positions;
		Eigen::MatrixXf gridline_positions;
		Eigen::MatrixXf boxline_positions;
		Eigen::MatrixXf colors;
		
		Eigen::Matrix4f view, proj, model, mvp;
		Eigen::Vector3f ray_wor;
		
		SurfWindow &widgets;
		
		float fov = 60;
		float drag_sensitivity, scroll_sensitivity, keyboard_sensitivity;
		
};

#endif
