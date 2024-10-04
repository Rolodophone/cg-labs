#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>

#define WIDTH 320
#define HEIGHT 240

std::vector<float> interpolateSingleFloats(float from, float to, float numberOfValues) {
	std::vector<float> result;
	for (size_t i = 0; i < numberOfValues; i++) {
		result.push_back(from + i*((to-from)/(numberOfValues-1)));
	}
	return result;
}

std::vector<glm::vec3> interpolateThreeElementValues(glm::vec3 from, glm::vec3 to, float numberOfValues) {
	std::vector<glm::vec3> result;
	for (size_t i = 0; i < numberOfValues; i++) {
		result.push_back(glm::vec3(from[0] + i*((to[0]-from[0])/(numberOfValues-1)),
			from[1] + i*((to[1]-from[1])/(numberOfValues-1)),
			from[2] + i*((to[2]-from[2])/(numberOfValues-1))));
	}
	return result;
}

void draw(DrawingWindow &window) {
	window.clearPixels();
	std::vector<glm::vec3> leftGradient = interpolateThreeElementValues(glm::vec3(255, 0, 0),
		glm::vec3(255, 255, 0), HEIGHT);
	std::vector<glm::vec3> rightGradient = interpolateThreeElementValues(glm::vec3(0, 0, 255),
		glm::vec3(0, 255, 0), HEIGHT);
	for (size_t y = 0; y < window.height; y++) {
		std::vector<glm::vec3> rowGradient = interpolateThreeElementValues(leftGradient[y],
			rightGradient[y], WIDTH);
		for (size_t x = 0; x < window.width; x++) {
			glm::vec3 pixelColour = rowGradient[x];
			uint32_t colour = (255 << 24) + (int(pixelColour[0]) << 16) + (int(pixelColour[1]) << 8) +
				int(pixelColour[2]);
			window.setPixelColour(x, y, colour);
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	std::vector<glm::vec3> result;
	result = interpolateThreeElementValues(glm::vec3(1.0, 4.0, 9.2),
		glm::vec3(4.0, 1.0, 9.8), 4);
	for (size_t i=0; i<result.size(); i++) {
		for (int j=0; j < 3; j++) {
			std::cout << result[i][j] << " ";
		}
		std::cout << std::endl;
	}

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
