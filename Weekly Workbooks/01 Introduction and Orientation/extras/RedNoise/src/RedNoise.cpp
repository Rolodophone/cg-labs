#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>

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

uint32_t packColour(Colour colour) {
	return (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
}

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour) {
	float deltaX = to.x - from.x;
	float deltaY = to.y - from.y;
	float numberOfSteps = std::max(abs(deltaX), abs(deltaY));
	float stepX = deltaX/numberOfSteps;
	float stepY = deltaY/numberOfSteps;
	for (float i = 0; i < numberOfSteps; i++) {
		float x = from.x + i*stepX;
		float y = from.y + i*stepY;
		window.setPixelColour(round(x), round(y), packColour(colour));
	}
}

void draw(DrawingWindow &window) {
	window.clearPixels();
	drawLine(window, CanvasPoint(0, 0), CanvasPoint(WIDTH/2, HEIGHT/2),
		Colour(255, 255, 255));
	drawLine(window, CanvasPoint(WIDTH/2, HEIGHT/2), CanvasPoint(WIDTH, 0),
		Colour(255, 255, 255));
	drawLine(window, CanvasPoint(WIDTH/2, 0), CanvasPoint(WIDTH/2, HEIGHT),
		Colour(255, 255, 255));
	drawLine(window, CanvasPoint(WIDTH/3, HEIGHT/2), CanvasPoint(2*WIDTH/3, HEIGHT/2),
		Colour(255, 255, 255));
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

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
