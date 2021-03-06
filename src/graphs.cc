/*
* @Author: Kamil Rocki
* @Date:   2017-02-28 11:25:34
* @Last Modified by:   kmrocki@us.ibm.com
* @Last Modified time: 2017-03-10 05:25:35
*/

#include <thread>
#include <unistd.h>

/* A template for GUI + compute thread (with plotting) */

#include <colors.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/graph.h>
#include <nanogui/layout.h>

//helpers
#include <utils.h>

#define DEF_WIDTH 300
#define DEF_HEIGHT 600
#define SCREEN_NAME "GRAPHS"

#define NUM_WORKERS 10

class GUI : public nanogui::Screen {

	public:
	
		GUI ( ) : nanogui::Screen ( Eigen::Vector2i ( DEF_WIDTH, DEF_HEIGHT ), SCREEN_NAME ), vsync ( true ) { init(); }
		
		void init() {
		
			/* get physical GLFW screen size */
			glfwGetWindowSize ( glfwWindow(), &glfw_window_width, &glfw_window_height );
			
			// get GL capabilities info
			printf ( "GL_RENDERER: %s\n", glGetString ( GL_RENDERER ) );
			printf ( "GL_VERSION: %s\n", glGetString ( GL_VERSION ) );
			printf ( "GLSL_VERSION: %s\n\n", glGetString ( GL_SHADING_LANGUAGE_VERSION ) );
			
			/* * create widgets  * */
			this->setLayout ( new nanogui::BoxLayout() );
			
			nanogui::Window *window_graphs = new nanogui::Window ( this, "" );
			window_graphs->setLayout ( new nanogui::GroupLayout ( 3, 1, 0, 0 ) );
			
			int NUM_GRAPHS = 10;
			
			for ( int i = 0; i < NUM_GRAPHS; i++ ) {
			
				nanogui::Graph *graph = new nanogui::Graph ( window_graphs, string_format ( "%d", i ),
						nanogui::GraphType::GRAPH_NANOGUI_NOFILL, nanogui::parula_lut[i] );
				graph_data.push_back ( graph->values_ptr() );
			}
			
			window_graphs->setSize ( {glfw_window_width, glfw_window_height} );
			/* * * * * * * * * * * */
			
			drawAll();
			setVisible ( true );
			
			glfwSwapInterval ( vsync );
			
			performLayout();
			resizeEvent ( { glfw_window_width, glfw_window_height } );
			
		}
		
		/* event handlers */
		
		virtual bool keyboardEvent ( int key, int scancode, int action, int modifiers ) {
		
			/* process subcomponents */
			if ( Screen::keyboardEvent ( key, scancode, action, modifiers ) )
				return true;
				
			/* close */
			if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS ) {
			
				setVisible ( false );
				return true;
			}
			
			return false;
		}
		
		virtual bool resizeEvent ( const Eigen::Vector2i &size ) {
		
			UNUSED ( size );
			performLayout();
			return true;
			
		}
		
		~GUI() { /* free resources */}
		
	protected:
	
		int glfw_window_width, glfw_window_height;
		bool vsync;
		
	public:
	
		std::vector<Eigen::VectorXf *> graph_data;
		
};

GUI *screen;

int compute ( int i ) {

	/* work until main window is open */
	while ( screen->getVisible() ) {
	
		// generate some data
		if ( screen->graph_data[i] ) {
		
			screen->graph_data[i]->resize ( 100 );
			
			for ( int k = 0; k < screen->graph_data[i]->size(); ++k ) {
			
				screen->graph_data[i]->operator[] ( k ) = 0.5f * ( 0.5f * std::sin ( k / 10.f + glfwGetTime() ) +
						0.5f * std::cos ( i * k / 23.f ) + 1 );
			}
		}
		
		usleep ( 1000 );
	}
	
	return 0;
	
}

int main ( int /* argc */, char ** /* argv */ ) {

	try {
	
		/* init GUI */
		nanogui::init();
		screen = new GUI();
		
		std::thread compute_threads[NUM_WORKERS];
		
		for ( int i = 0; i < NUM_WORKERS; i++ )
			compute_threads[i] = std::thread ( compute, i );
			
		nanogui::mainloop ( 1 );
		
		delete screen;
		nanogui::shutdown();
		
		for ( int i = 0; i < NUM_WORKERS; i++ )
			compute_threads[i].join();
			
	}
	catch ( const std::runtime_error &e ) {
	
		std::string error_msg = std::string ( "Caught a fatal error: " ) + std::string ( e.what() );
		return -1;
		
	}
	
	return 0;
	
}
