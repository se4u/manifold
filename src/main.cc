/*
* @Author: Kamil Rocki
* @Date:   2017-02-28 11:25:34
* @Last Modified by:   kmrocki@us.ibm.com
* @Last Modified time: 2017-03-07 14:45:57
*/

#include <iostream>
#include <vector>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <iomanip>
#include <thread>
#include <unistd.h>

#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/theme.h>
#include <nanogui/graph.h>
#include <nanogui/matrix.h>
#include <nanogui/console.h>
#include <nanogui/imageview.h>
#include <nanogui/imagepanel.h>
#include <nanogui/checkbox.h>

//for NN
#include <io/import.h>
#include <nn/nn_utils.h>
#include <nn/layers.h>
#include <nn/nn.h>

//performance counters
#include "fps.h"
#include "perf.h"

// rand
#include "aux.h"

//helpers
#include <utils.h>
#include <gl/tex.h>

#define DEF_WIDTH 800
#define DEF_HEIGHT 600
#define SCREEN_NAME "vae"

bool quit = false;

NN* nn;

#define N 100
#define L 64

int w = 28;
int h = 28;

int nvgCreateImageA(NVGcontext* ctx, int w, int h, int imageFlags, const unsigned char* data) {

	return nvgInternalParams(ctx)->renderCreateTexture(nvgInternalParams(ctx)->userPtr, NVG_TEXTURE_ALPHA, w, h, imageFlags, data);
}

class Manifold : public nanogui::Screen {

  public:

	Manifold ( ) : nanogui::Screen ( Eigen::Vector2i ( DEF_WIDTH, DEF_HEIGHT ), SCREEN_NAME ) {

		init();

	}

	void console ( const char *pMsg, ... ) {

		char buffer[4096];
		std::va_list arg;
		va_start ( arg, pMsg );
		std::vsnprintf ( buffer, 4096, pMsg, arg );
		va_end ( arg );
		log_str.append ( buffer );

	}

	void init() {

		int glfw_window_width, glfw_window_height;

		glfwGetWindowSize ( glfwWindow(), &glfw_window_width, &glfw_window_height );

		// get GL debug info
		console ( "GL_RENDERER: %s\n", glGetString ( GL_RENDERER ) );
		console ( "GL_VERSION: %s\n", glGetString ( GL_VERSION ) );
		console ( "GLSL_VERSION: %s\n\n", glGetString ( GL_SHADING_LANGUAGE_VERSION ) );

		//NN loss graph
		graph_loss = new nanogui::Graph ( this, "" );
		graph_loss->setGraphColor ( nanogui::Color ( 160, 160, 160, 255 ) );
		graph_loss->setBackgroundColor ( nanogui::Color ( 0, 0, 0, 32 ) );

		graphs = new nanogui::Window ( this, "" );
		graphs->setPosition({5, size()[1] - graphs->size()[1] - 5});
		nanogui::GridLayout *layout = new nanogui::GridLayout(
		    nanogui::Orientation::Horizontal, 1, nanogui::Alignment::Middle, 1, 1);
		layout->setColAlignment( { nanogui::Alignment::Maximum, nanogui::Alignment::Fill });
		layout->setSpacing(0, 5);
		graphs->setLayout(layout);
		nanogui::Theme *t = graphs->theme();
		t->mWindowFillUnfocused = nanogui::Color (0, 0, 0, 0 );
		t->mWindowFillFocused = nanogui::Color (128, 128, 128, 16 );
		t->mDropShadow = nanogui::Color (0, 0, 0, 0 );
		graphs->setTheme ( t );

		int graph_width = 100;
		int graph_height = 15;

		//FPS graph
		graph_fps = graphs->add<nanogui::Graph> ( "" );
		graph_fps->setGraphColor ( nanogui::Color ( 0, 160, 192, 255 ) );
		graph_fps->setBackgroundColor ( nanogui::Color ( 0, 0, 0, 8 ) );
		graph_fps->setSize ( {graph_width, graph_height } );

		//CPU graph
		graph_cpu = graphs->add<nanogui::Graph> ( "" );
		graph_cpu->values().resize ( cpu_util.size() );
		graph_cpu->setGraphColor ( nanogui::Color ( 192, 0, 0, 255 ) );
		graph_cpu->setBackgroundColor ( nanogui::Color ( 0, 0, 0, 8 ) );
		graph_cpu->setSize ( {graph_width, graph_height } );

		// FlOP/s
		graph_flops = graphs->add<nanogui::Graph> ( "" );
		graph_flops->values().resize ( cpu_flops.size() );
		graph_flops->setGraphColor ( nanogui::Color ( 0, 192, 0, 255 ) );
		graph_flops->setBackgroundColor ( nanogui::Color ( 0, 0, 0, 8 ) );
		graph_flops->setSize ( {graph_width, graph_height } );

		auto chk_windows = new nanogui::Window(this, "");
		chk_windows->setPosition({5, 25 });
		chk_windows->setSize({25, 100 });
		nanogui::GridLayout *hlayout = new nanogui::GridLayout(
		    nanogui::Orientation::Horizontal, 1, nanogui::Alignment::Middle, 1, 1);
		hlayout->setColAlignment( { nanogui::Alignment::Maximum, nanogui::Alignment::Fill });
		hlayout->setSpacing(0, 5);
		chk_windows->setLayout(hlayout);
		chk_windows->setTheme ( t );

		//inputs checkbox
		m_showInputsCheckBox = new nanogui::CheckBox(chk_windows, "show xs");
		m_showInputsCheckBox->setCallback([&](bool) { });

		while (!nn) { std::cout << "Waiting for compute thread...\n"; usleep(10000); }

		rgba_image.resize(w, h);

		for (size_t i = 0; i < N; i++) {

			int im = nvgCreateImageA(nvgContext(), w, h, NVG_IMAGE_NEAREST, (unsigned char*) nullptr);
			mImagesData.emplace_back(std::pair<int, std::string>(im, ""));

		}

		for (size_t i = 0; i < L; i++) {

			layer_image[i].resize(w, h);
			int l_im = nvgCreateImageA(nvgContext(), w, h, NVG_IMAGE_NEAREST, (unsigned char*) nullptr);
			layerImagesData.emplace_back(std::pair<int, std::string>(l_im, ""));

		}

		auto imageWindow2 = new nanogui::Window(this, "inputs");
		imageWindow2->setPosition(Eigen::Vector2i(5, 45));
		imgPanel = new nanogui::ImagePanel(imageWindow2, 16, 3, 5, {10, 10});
		imgPanel->setImages(mImagesData);
		imgPanel->setPosition({0, 20});

		Eigen::Vector2i w_size = imgPanel->preferredSize() + Eigen::Vector2i({0, 20});
		imageWindow2->setSize(w_size);
		nanogui::Theme *th = imageWindow2->theme();
		th->mWindowFillUnfocused = nanogui::Color ( 0, 0, 0, 32 );
		th->mWindowFillFocused = nanogui::Color ( 32, 32, 32, 32 );
		th->mWindowHeaderSepTop = nanogui::Color ( 0, 0, 0, 32 );
		imageWindow2->setTheme ( th );

		auto nn_window = new nanogui::Window(this, "Layers");
		nn_window->setPosition({255, 15 });
		nn_imgPanel = new nanogui::ImagePanel(nn_window, 60, 2, 3, {8, 8});
		nn_imgPanel->setImages(layerImagesData);
		nn_imgPanel->setPosition({0, 20});
		nanogui::Theme *th_nn = nn_imgPanel->theme();
		th_nn->mWindowFillUnfocused = nanogui::Color ( 0, 0, 0, 32 );
		th_nn->mWindowFillFocused = nanogui::Color ( 0, 0, 0, 128 );;
		nn_imgPanel->setTheme(th_nn);

		w_size = nn_imgPanel->preferredSize() + Eigen::Vector2i({0, 20});
		nn_window->setSize(w_size);

		show_console = false;

		console_window = new nanogui::Window ( this, "" );

		// console_window->setLayout ( new GroupLayout ( 5, 5, 0, 0 ) );
		console_panel = new nanogui::Console ( console_window );
		console_panel->setFontSize ( 12 );

		// debug footer message
		footer_message = new nanogui::Console ( this );
		footer_message->setFontSize ( 12 );
		footer_message_string = "";

		m_showInputsCheckBox->setChecked(true);
		m_showInputsCheckBox->setFontSize ( 12 );

		drawAll();
		setVisible(true);

		resizeEvent ( { glfw_window_width, glfw_window_height } );

		performLayout();
	}

	~Manifold() { }

	virtual bool keyboardEvent ( int key, int scancode, int action, int modifiers ) {

		if ( Screen::keyboardEvent ( key, scancode, action, modifiers ) )
			return true;

		if ( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS ) {
			setVisible ( false ); quit = true;
			return true;
		}

		if ( key == GLFW_KEY_TAB && action == GLFW_PRESS ) {

			show_console = !show_console;
			console_window->setVisible ( show_console );

		}

		return false;
	}

	void draw_inputs() {

		if (nn) {

			for (size_t i = 0; i < N; i++) {

				Eigen::MatrixXf float_image = nn->layers[0]->x.col(i);

				float_image.resize(w, h);
				float_image *= 255.0f;

				rgba_image = float_image.cast<unsigned char>();
				nvgUpdateImage(nvgContext(), mImagesData[i].first, (unsigned char*) rgba_image.data());

			}
		}
	}

	void draw_layers() {

		if (nn) {

			for (size_t i = 0; i < 1; i++) {

				for (size_t j = 0; j < L; j++) {

					Eigen::MatrixXf float_image = ((Linear*)(nn->layers[i]))->W.row(j);
					float_image.resize(w, h);
					float l2 = 1.0;//float_image.norm();
					float_image = float_image.unaryExpr([&](float x) { return 255.0f * (x / l2 + 0.5f); });
					layer_image[j] = float_image.cast<unsigned char>();
					nvgUpdateImage(nvgContext(), layerImagesData[j].first, (unsigned char*) layer_image[j].data());

				}

			}
		}
	}

	void drawContents() {

		console_window->setVisible(show_console);
		console_panel->setValue ( log_str );

		// debug
		std::stringstream ss;
		ss << size() [0] << "x" << size() [1] << "\n";

		footer_message_string = ss.str();
		footer_message->setValue ( footer_message_string );

		if (m_showInputsCheckBox->checked()) {

			imgPanel->setVisible(true);
			draw_inputs();

		} else {

			imgPanel->setVisible(false);
		}

		draw_layers();

		update_FPS(graph_fps);
		update_graph (graph_cpu, cpu_util, 1000.0f, "ms" );
		update_graph (graph_flops, cpu_flops, 1.0f, "GF/s"  );

		if (nn)
			if (nn->clock) {
				nn->clock = false;
				update_graph (graph_loss, nn->current_loss );
			}

	}

	virtual bool resizeEvent ( const Eigen::Vector2i &size ) {

		// FPS graph
		graphs->setPosition ( {1, size[1] - 50} );

		// loss
		graph_loss->setSize ( {size[0] - 109, 45 } );
		graph_loss->setPosition( {105, size[1] - 48 } );

		// console
		int console_width = 250;
		int console_height = size[1] - 55;

		console_window->setPosition ( {size[0] - console_width - 5, 5} );
		console_window->setSize ( {console_width, console_height} );
		console_panel->setPosition ( {5, 5} );
		console_panel->setWidth ( console_width - 10 );
		console_panel->setHeight ( console_height - 10 );

		// footer
		int footer_height = 40;
		int footer_width = 550;

		footer_message->setPosition ( {5, 5} );
		footer_message->setSize ( {footer_width, footer_height} );

		performLayout();

		return true;

	}

	nanogui::Graph *graph_fps, *graph_cpu, *graph_flops, *graph_loss;
	nanogui::Window *console_window, *graphs;
	nanogui::Console *console_panel, *footer_message;
	nanogui::CheckBox *m_showInputsCheckBox;

	using imagesDataType = std::vector<std::pair<int, std::string>>;
	imagesDataType mImagesData;
	imagesDataType layerImagesData;
	nanogui::ImagePanel *imgPanel, *nn_imgPanel;

	std::string log_str, footer_message_string;

	Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> rgba_image;
	Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> layer_image[L];

	bool show_console;

};

int compute() {

	// TODO: option to suspend or kill this thread from GUI

	size_t epochs = 100;
	size_t batch_size = 100;
	double learning_rate = 1e-3;

	nn = new NN(batch_size);

	nn->layers.push_back(new Linear(28 * 28, 64, batch_size));
	nn->layers.push_back(new Sigmoid(64, 64, batch_size));
	// nn->layers.push_back(new Linear(64, 25, batch_size));
	// nn->layers.push_back(new ReLU(25, 25, batch_size));
	nn->layers.push_back(new Linear(64, 10, batch_size));
	nn->layers.push_back(new Softmax(10, 10, batch_size));

	//[60000, 784]
	std::deque<datapoint> train_data =
	    MNISTImporter::importFromFile("data/mnist/train-images-idx3-ubyte", "data/mnist/train-labels-idx1-ubyte");
	//[10000, 784]
	std::deque<datapoint> test_data =
	    MNISTImporter::importFromFile("data/mnist/t10k-images-idx3-ubyte", "data/mnist/t10k-labels-idx1-ubyte");

	for (size_t e = 0; e < epochs; e++) {

		std::cout << "Epoch " << e + 1 << std::endl << std::endl;
		nn->train(train_data, learning_rate, train_data.size() / batch_size);
		if (quit) break; // still need to send this signal to the inner functions

		nn->test(test_data);

	}

	delete nn;

	return 0;

}

int main ( int /* argc */, char ** /* argv */ ) {

	try {

		// launch a compute thread
		std::thread compute_thread(compute);

		nanogui::init();

		Manifold* screen = new Manifold();
		nanogui::mainloop ( 1 );

		delete screen;
		nanogui::shutdown();
		compute_thread.join();

	} catch ( const std::runtime_error &e ) {

		std::string error_msg = std::string ( "Caught a fatal error: " ) + std::string ( e.what() );
		return -1;

	}

	return 0;

}
