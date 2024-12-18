#include <algorithm>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <Colour.h>
#include <map>
#include <ModelTriangle.h>
#include <RayTriangleIntersection.h>
#include <TextureMap.h>

#define WIDTH 320
#define HEIGHT 240


float (*depthBuffer)[320] = new float[HEIGHT][WIDTH];
glm::vec3 cameraPosition = glm::vec3(0, 0, 16);
glm::mat3 cameraOrientation = glm::mat3();  // right, up, forward - init to identity matrix
std::map<std::string, Colour> colours;
std::vector<ModelTriangle> triangles = {};
glm::vec3 lightPosition = glm::vec3(0, 2.6, 0);


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

float lerp(float start, float end, float t) {
	return start + t*(end-start);
}
CanvasPoint lerp(CanvasPoint start, CanvasPoint end, float t) {
	return CanvasPoint(lerp(start.x, end.x, t), lerp(start.y, end.y, t), lerp(start.depth, end.depth, t));;
}
TexturePoint lerp(TexturePoint start, TexturePoint end, float t) {
	return TexturePoint(lerp(start.x, end.x, t), lerp(start.y, end.y, t));
}

uint32_t packColour(Colour colour) {
	return (255 << 24) + (int(colour.red) << 16) + (int(colour.green) << 8) + int(colour.blue);
}

void drawLine(DrawingWindow &window, CanvasPoint from, CanvasPoint to, Colour colour) {
	float deltaX = to.x - from.x;
	float deltaY = to.y - from.y;
	float deltaDepth = to.depth - from.depth;
	float numberOfSteps = std::max(abs(deltaX), abs(deltaY));
	float stepX = deltaX/numberOfSteps;
	float stepY = deltaY/numberOfSteps;
	float stepDepth = deltaDepth/numberOfSteps;
	for (float i = 0; i < numberOfSteps; i++) {
		float x = from.x + i*stepX;
		float y = from.y + i*stepY;
		float depth = from.depth + i*stepDepth;
		int xInt = round(x);
		int yInt = round(y);
		if (1/depth > depthBuffer[yInt][xInt]) {
			depthBuffer[yInt][xInt] = 1/depth;
			window.setPixelColour(xInt, yInt, packColour(colour));
		}
	}
}

void drawUnfilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
	drawLine(window, triangle.vertices[0], triangle.vertices[1], colour);
	drawLine(window, triangle.vertices[1], triangle.vertices[2], colour);
	drawLine(window, triangle.vertices[2], triangle.vertices[0], colour);
}

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
	std::sort(triangle.vertices.begin(), triangle.vertices.end(), [](CanvasPoint a, CanvasPoint b) {
		return a.y < b.y;
	});
	CanvasPoint top = triangle.vertices[0];
	CanvasPoint middle1 = triangle.vertices[1];
	CanvasPoint bottom = triangle.vertices[2];
	CanvasPoint middle2 = lerp(top, bottom, (middle1.y-top.y)/(bottom.y-top.y));

	for (float y = top.y; y < middle1.y; y++) {
		float t = (y-top.y)/(middle1.y-top.y);
		CanvasPoint a = lerp(top, middle1, t);
		CanvasPoint b = lerp(top, middle2, t);
		drawLine(window, a, b, colour);
	}

	for (float y = middle1.y; y < bottom.y; y++) {
		float t = (y-middle1.y)/(bottom.y-middle1.y);
		CanvasPoint a = lerp(middle1, bottom, t);
		CanvasPoint b = lerp(middle2, bottom, t);
		drawLine(window, a, b, colour);
	}

	// drawUnfilledTriangle(window, triangle, Colour(255, 255, 255));
}

void drawTexturedTriangle(DrawingWindow &window, CanvasTriangle canvasTriangle, TextureMap textureMap,
		std::vector<TexturePoint> texturePoints) {

	// haven't implemented depth buffer for texture mapping yet

	// printf("canvasTriangle: ");
	// std::cout << canvasTriangle << std::endl;
	// printf("texturePoints: ");
	// for (TexturePoint texturePoint : texturePoints) {
	// 	std::cout << "(" << texturePoint << ") ";
	// }
	// std::cout << std::endl;

	//store correspondence between canvas and texture points
	for (int i = 0; i < 3; i++) {
		canvasTriangle.vertices[i].texturePoint = texturePoints[i];
	}

	//sort canvas points so that I can identify top, middle1, bottom
	std::sort(canvasTriangle.vertices.begin(), canvasTriangle.vertices.end(),
		[](CanvasPoint a, CanvasPoint b) { return a.y < b.y; });

	//identify top, middle1, bottom of triangle and interpolate middle2
	CanvasPoint top = canvasTriangle.vertices[0];
	CanvasPoint middle1 = canvasTriangle.vertices[1];
	CanvasPoint bottom = canvasTriangle.vertices[2];
	float t = (middle1.y-top.y)/(bottom.y-top.y);
	CanvasPoint middle2 = lerp(top, bottom, t);
	middle2.texturePoint = lerp(top.texturePoint, bottom.texturePoint, t);

	// printf("top: ");
	// std::cout << top << std::endl;
	// printf("middle1: ");
	// std::cout << middle1 << std::endl;
	// printf("middle2: ");
	// std::cout << middle2 << std::endl;
	// printf("bottom: ");
	// std::cout << bottom << std::endl;

	//draw top half of triangle, from y=top.y to y=middle1.y
	for (float y = top.y; y < middle1.y; y++) {
		float ty = (y-top.y)/(middle1.y-top.y);
		CanvasPoint a_canvas = lerp(top, middle1, ty);
		CanvasPoint b_canvas = lerp(top, middle2, ty);
		TexturePoint a_texture = lerp(top.texturePoint, middle1.texturePoint, ty);
		TexturePoint b_texture = lerp(top.texturePoint, middle2.texturePoint, ty);
		for (float x = a_canvas.x; x < b_canvas.x; x++) {
			float tx = (x-a_canvas.x)/(b_canvas.x-a_canvas.x);
			TexturePoint texturePoint = lerp(a_texture, b_texture, tx);
			uint32_t colour = textureMap.pixels[round(texturePoint.x) + round(texturePoint.y)*textureMap.width];
			window.setPixelColour(round(x), round(y), colour);
		}
	}

	//draw bottom half of triangle, from y=middle1.y to y=bottom.y
	for (float y = middle1.y; y < bottom.y; y++) {
		float ty = (y-middle1.y)/(bottom.y-middle1.y);
		CanvasPoint a_canvas = lerp(middle1, bottom, ty);
		CanvasPoint b_canvas = lerp(middle2, bottom, ty);
		TexturePoint a_texture = lerp(middle1.texturePoint, bottom.texturePoint, ty);
		TexturePoint b_texture = lerp(middle2.texturePoint, bottom.texturePoint, ty);
		for (float x = a_canvas.x; x < b_canvas.x; x++) {
			float tx = (x-a_canvas.x)/(b_canvas.x-a_canvas.x);
			TexturePoint texturePoint = lerp(a_texture, b_texture, tx);
			uint32_t colour = textureMap.pixels[round(texturePoint.x) + round(texturePoint.y)*textureMap.width];
			window.setPixelColour(round(x), round(y), colour);
		}
	}

	//draw outline of triangle to check that it is correct
	drawUnfilledTriangle(window, canvasTriangle, Colour(255, 255, 255));
}

void readObjFile(std::string fileName, std::vector<ModelTriangle> &triangles, float scale,
		std::map<std::string, Colour> colours) {
	std::ifstream file(fileName);
	std::string line;
	std::vector<glm::vec3> vertices;
	Colour currentColour;
	while (std::getline(file, line)) {
		printf("getline");
		std::vector<std::string> lineSplit = split(line, ' ');
		if (lineSplit[0] == "v") {
			vertices.push_back(glm::vec3(stof(lineSplit[1]), stof(lineSplit[2]), stof(lineSplit[3])) * scale);
		} else if (lineSplit[0] == "f") {
			triangles.push_back(ModelTriangle(
				vertices[stoi(lineSplit[1].substr(0, lineSplit[1].length()-1))-1],
				vertices[stoi(lineSplit[2].substr(0, lineSplit[2].length()-1))-1],
				vertices[stoi(lineSplit[3].substr(0, lineSplit[3].length()-1))-1],
				currentColour));
		}
		else if (lineSplit[0] == "usemtl") {
			currentColour = colours.at(lineSplit[1]);
		}
	}
}

void readMtlFile(std::string fileName, std::map<std::string, Colour> &colours) {
	std::ifstream file(fileName);
	std::string line;
	std::string matName;
	while (std::getline(file, line)) {
		std::vector<std::string> lineSplit = split(line, ' ');
		if (lineSplit[0] == "newmtl") {
			matName = lineSplit[1];
		} else if (lineSplit[0] == "Kd") {
			Colour newColour = Colour(
				int(stof(lineSplit[1])*255),
				int(stof(lineSplit[2])*255),
				int(stof(lineSplit[3])*255));
			newColour.name = matName;
			colours.insert({matName, newColour});
		}
	}
}

CanvasPoint projectVertexOntoCanvasPoint(float focalLength, glm::vec3 vertexPosition,
		float imagePlaneScale) {
	glm::vec3 vertexWrtCamera = vertexPosition - cameraPosition;
	// -vertexWrtCamera.z is the depth as z is pointing out of the screen
	float u = vertexWrtCamera.x * (focalLength / -vertexWrtCamera.z);
	// negated because the model uses y pointing up, but the canvas uses y pointing down
	float v = -vertexWrtCamera.y * (focalLength / -vertexWrtCamera.z);
	u = u*imagePlaneScale + WIDTH/2;
	v = v*imagePlaneScale + HEIGHT/2;
	return CanvasPoint(u, v, -vertexWrtCamera.z);  // store depth (note: this is not z!)
}

void drawRasterised(DrawingWindow &window) {
	// initialise depth buffer
	for (size_t y = 0; y < HEIGHT; y++) {
		for (size_t x = 0; x < WIDTH; x++) {
			depthBuffer[y][x] = 0;
		}
	}

	window.clearPixels();

	for (size_t i = 0; i < triangles.size(); i++) {
		CanvasTriangle canvasTriangle;
		for (int j = 0; j < 3; j++) {
			canvasTriangle.vertices[j] = projectVertexOntoCanvasPoint(2, triangles[i].vertices[j], 280);
		}
		drawFilledTriangle(window, canvasTriangle, triangles[i].colour);
	}
}

RayTriangleIntersection getClosestIntersection(glm::vec3 rayStart, glm::vec3 rayDirection) {
	int i_closest = -1;
	float t_closest = FLT_MAX;

	for (int i = 0; i < triangles.size(); i++) {
		ModelTriangle triangle = triangles[i];
		glm::vec3 e0 = triangle.vertices[1] - triangle.vertices[0];
		glm::vec3 e1 = triangle.vertices[2] - triangle.vertices[0];
		glm::vec3 SPVector = rayStart - triangle.vertices[0];
		glm::mat3 DEMatrix(-rayDirection, e0, e1);
		glm::vec3 possibleSolution = inverse(DEMatrix) * SPVector;
		float t = possibleSolution[0];
		float u = possibleSolution[1];
		float v = possibleSolution[2];
		if (t > 0 && u >= 0 && u <= 1 && v >= 0 && v <= 1 && u + v <= 1 && t > 0.001 && t < t_closest) {
			i_closest = i;
			t_closest = t;
		}
	}

	if (i_closest == -1) {
		return RayTriangleIntersection(glm::vec3(), -1, ModelTriangle(), -1);
	}

	ModelTriangle triangle = triangles[i_closest];
	glm::vec3 intersectionPoint = cameraPosition + t_closest*rayDirection;
	return RayTriangleIntersection(intersectionPoint, t_closest, triangle, i_closest);
}

bool isPointInShadow(glm::vec3 point) {
	glm::vec3 rayDirection = normalize(lightPosition - point);
	RayTriangleIntersection intersection = getClosestIntersection(point, rayDirection);
	return intersection.triangleIndex != -1 && intersection.distanceFromCamera < length(lightPosition - point);
}

void draw(DrawingWindow &window) {
	window.clearPixels();

	float focalLength = 2;
	float imagePlaneScale = 280;

	for (int y = 0; y < HEIGHT; y++) {
		for (int x = 0; x < WIDTH; x++) {
			float u = (x - WIDTH/2) / imagePlaneScale;
			float v = -(y - HEIGHT/2) / imagePlaneScale;
			glm::vec3 cameraToImagePlanePixel = glm::vec3(u, v, -focalLength);
			glm::vec3 rayDirection = normalize(cameraToImagePlanePixel * cameraOrientation);
			RayTriangleIntersection intersection = getClosestIntersection(cameraPosition, rayDirection);
			if (intersection.triangleIndex != -1) {
				if (!isPointInShadow(intersection.intersectionPoint)) {
					window.setPixelColour(x, y, packColour(intersection.intersectedTriangle.colour));
				}
			}
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) {
			glm::vec3 left = cameraOrientation * glm::vec3(-1, 0, 0);
			cameraPosition += left * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_RIGHT) {
			glm::vec3 right = cameraOrientation * glm::vec3(1, 0, 0);
			cameraPosition += right * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_UP) {
			glm::vec3 back = cameraOrientation * glm::vec3(0, 0, -1);
			cameraPosition += back * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_DOWN) {
			glm::vec3 forward = cameraOrientation * glm::vec3(0, 0, 1);
			cameraPosition += forward * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_SPACE) {
			glm::vec3 up = cameraOrientation * glm::vec3(0, 1, 0);
			cameraPosition += up * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_LSHIFT) {
			glm::vec3 down = cameraOrientation * glm::vec3(0, -1, 0);
			cameraPosition += down * 0.1f;
		}
		else if (event.key.keysym.sym == SDLK_u) {
			drawUnfilledTriangle(window, CanvasTriangle(CanvasPoint(rand()%WIDTH, rand()%HEIGHT),
				CanvasPoint(rand()%WIDTH, rand()%HEIGHT), CanvasPoint(rand()%WIDTH, rand()%HEIGHT)),
				Colour(rand()%256, rand()%256, rand()%256));
		}
		else if (event.key.keysym.sym == SDLK_f) {
			drawFilledTriangle(window, CanvasTriangle(CanvasPoint(rand()%WIDTH, rand()%HEIGHT),
				CanvasPoint(rand()%WIDTH, rand()%HEIGHT), CanvasPoint(rand()%WIDTH, rand()%HEIGHT)),
				Colour(rand()%256, rand()%256, rand()%256));
		}
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;

	readMtlFile("../cornell-box.mtl", colours);

	readObjFile("../cornell-box.obj", triangles, 1, colours);
	// std::cout << triangles.size() << std::endl;
	// for (size_t i = 0; i < triangles.size(); i++) {
	// 	std::cout << triangles[i].colour << std::endl;
	// 	std::cout << triangles[i] << std::endl;
	// }

	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
