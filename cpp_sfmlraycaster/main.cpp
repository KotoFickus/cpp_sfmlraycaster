#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <windows.h>
#include <iostream>
#include <processenv.h>
#include <algorithm>
#include "config.h"

int worldMap[mapWidth][mapHeight] = { 
    {1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,1,1,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,1},
    {1,0,0,1,0,0,0,1},
    {1,1,1,1,1,1,1,1},
};

double average(double* nums, int len) {
    double result = 0;
    for (int i = 0; i < len; i++) {
        result += nums[i];
    }
    return result / len;
}

void fillArr(double* arr, int len, double val) {
    for (int i = 0; i < len; i++) {
        arr[i] = val;
    }
}

float clip(float n, float lower, float upper) {
    return std::max(lower, std::min(n, upper));
}
double clip(double n, double lower, double upper) {
    return std::max(lower, std::min(n, upper));
}

int main() {
    bool loadedSuccessfully = true;
    sf::Image images[texNums];
    images[0] = sf::Image();
    loadedSuccessfully &= images[0].loadFromFile("wall.png");
    if (!loadedSuccessfully) {
        return -1;
    }

    int frameCounter = 0;
    bool onGround = true;

    double posX = 2, posY = 2;  //x and y start position
    double dirX = -1, dirY = 0; //initial direction vector
    double velX = 0, velY = 0;
    double planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane

    double time = 0; //time of current frame
    double oldTime = 0; //time of previous frame

    double cameraHeight = 0;
    double heightAdditional = 0;
    double pitch = 0;
    double movementSpeed = 0.5;
    double movementFriction = 0.8;

    double verticalVelocity = 0;

	sf::RenderWindow renderWindow(sf::VideoMode(width, height), "sfml test", sf::Style::None);
	renderWindow.setFramerateLimit(fps_limit);
    renderWindow.setMouseCursorVisible(false);
    renderWindow.setMouseCursorGrabbed(true);

    sf::Image frameImg;
    sf::Texture frame;
    frame.create(width, height);
    sf::Sprite sprite;
    sprite.setTexture(frame);

    sf::Clock clock;

    sf::Font font;

    char fontPath[_MAX_PATH];
    ExpandEnvironmentStringsA("%windir%\\Fonts\\Arial.ttf", fontPath, _MAX_PATH);

    font.loadFromFile(fontPath);

    sf::Text text;
    text.setFont(font);

    double fps = 0;

    double fpsRecords[fpsRecordsSize];
    fillArr(fpsRecords, fpsRecordsSize, 0);
    int recordsIndex = 0;
    int mdeltaX = 0;
    int mdeltaY = 0;

    while (renderWindow.isOpen())
    {
        sf::Event event;
        while (renderWindow.pollEvent(event))
        {
            if (event.type == sf::Event::Closed) {
                renderWindow.close();
            }
            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2i windowPos = renderWindow.getPosition();
                sf::Event::MouseMoveEvent mmoved = event.mouseMove;
                mdeltaX = screenCenterX - mmoved.x;
                mdeltaY = screenCenterY - mmoved.y;
                sf::Mouse::setPosition(sf::Vector2i(screenCenterX, screenCenterY) + windowPos);
            }
        }

        frameImg.create(width, height);

        for (int x = 0; x < width; x++) {
            double cameraX = 2 * x / double(width) - 1; //x-coordinate in camera space
            double rayDirX = dirX + planeX * cameraX;
            double rayDirY = dirY + planeY * cameraX;

            //which box of the map we're in
            int mapX = int(posX);
            int mapY = int(posY);

            //length of ray from current position to next x or y-side
            double sideDistX;
            double sideDistY;

            //length of ray from one x or y-side to next x or y-side
            double deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
            double deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);
            double perpWallDist;

            //what direction to step in x or y-direction (either +1 or -1)
            int stepX;
            int stepY;

            int hit = 0; //was there a wall hit?
            int side; //was a NS or a EW wall hit?

            //calculate step and initial sideDist
            if (rayDirX < 0)
            {
                stepX = -1;
                sideDistX = (posX - mapX) * deltaDistX;
            }
            else
            {
                stepX = 1;
                sideDistX = (mapX + 1.0 - posX) * deltaDistX;
            }
            if (rayDirY < 0)
            {
                stepY = -1;
                sideDistY = (posY - mapY) * deltaDistY;
            }
            else
            {
                stepY = 1;
                sideDistY = (mapY + 1.0 - posY) * deltaDistY;
            }

            //perform DDA
            while (hit == 0)
            {
                //jump to next map square, either in x-direction, or in y-direction
                if (sideDistX < sideDistY)
                {
                    sideDistX += deltaDistX;
                    mapX += stepX;
                    side = 0;
                }
                else
                {
                    sideDistY += deltaDistY;
                    mapY += stepY;
                    side = 1;
                }
                //Check if ray has hit a wall
                if (worldMap[mapX][mapY] > 0) hit = 1;
            }

            if (side == 0) perpWallDist = (sideDistX - deltaDistX);
            else           perpWallDist = (sideDistY - deltaDistY);

            //Calculate height of line to draw on screen
            int lineHeight = (int)(height / perpWallDist);

            //calculate lowest and highest pixel to fill in current stripe
            int drawStart = -lineHeight / 2 + height / 2 + pitch + ((cameraHeight + heightAdditional) / perpWallDist);
            if (drawStart < 0) drawStart = 0;
            int drawEnd = lineHeight / 2 + height / 2 + pitch + ((cameraHeight + heightAdditional) / perpWallDist);
            if (drawEnd >= height) drawEnd = height - 1;

            //texturing calculations
            int texNum = worldMap[mapX][mapY] - 1; //1 subtracted from it so that texture 0 can be used!

            //calculate value of wallX
            double wallX; //where exactly the wall was hit
            if (side == 0) wallX = posY + perpWallDist * rayDirY;
            else           wallX = posX + perpWallDist * rayDirX;
            wallX -= floor((wallX));

            //x coordinate on the texture
            int texX = int(wallX * double(texWidth));
            if (side == 0 && rayDirX > 0) texX = texWidth - texX - 1;
            if (side == 1 && rayDirY < 0) texX = texWidth - texX - 1;

            sf::Color color(200, 0, 0);
            if (side == 1) color = sf::Color(color.r / 2, color.g / 2, color.b / 2);

            // fog?
            double intense = 1 / perpWallDist;
            intense = std::min(1.0, intense * lightning_intense);

            double step = 1.0 * texHeight / lineHeight;
            // Starting texture coordinate
            //double texPos = (drawStart - pitch - (cameraHeight / perpWallDist) - height / 2 + lineHeight / 2) * step;
            double texPos = (drawStart - pitch - ((cameraHeight + heightAdditional) / perpWallDist) - height / 2 + lineHeight / 2) * step;

            for (int y = drawStart; y < drawEnd; y++) {
                int texY = (int)texPos & (texHeight - 1);
                texPos += step;
                color = images[texNum].getPixel(texX, texY);
                color.r *= intense;
                color.g *= intense;
                color.b *= intense;
                frameImg.setPixel(x, y, color);
            }
        }


        //timing for input and FPS counter

        text.setString(std::to_string(int(average(fpsRecords, fpsRecordsSize))));

        frame.update(frameImg);
        renderWindow.draw(sprite);
        renderWindow.draw(text);
        renderWindow.display();

        oldTime = time;
        time = clock.getElapsedTime().asMilliseconds();
        double frameTime = (time - oldTime) / 1000.0; //frameTime is the time this frame has taken, in seconds
        fps = 1.0 / frameTime;
        fpsRecords[recordsIndex] = fps;
        if (++recordsIndex == fpsRecordsSize) recordsIndex = 0;

        //speed modifiers
        double moveSpeed = frameTime * 5.0; //the constant value is in squares/second
        //double rotSpeed = frameTime * 3.0; //the constant value is in radians/second

        verticalVelocity -= gravity * frameTime;
        cameraHeight += verticalVelocity;
        if (cameraHeight < 0) {
            cameraHeight = 0;
            verticalVelocity = 0;
        }
        onGround = cameraHeight <= 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W))
        {
            
            velX += dirX * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
            velY += dirY * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S))
        {
            velX -= dirX * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
            velY -= dirY * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
        {

            double rot = 90 * M_PI / 180;
            double rightDirX = dirX * cos(rot) - dirY * sin(rot);
            double rightDirY = dirX * sin(rot) + dirY * cos(rot);
            velX -= rightDirX * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
            velY -= rightDirY * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
        {

            double rot = 90 * M_PI / 180;
            double rightDirX = dirX * cos(rot) - dirY * sin(rot);
            double rightDirY = dirX * sin(rot) + dirY * cos(rot);
            velX += rightDirX * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
            velY += rightDirY * movementSpeed * frameTime * (cameraHeight == 0 ? groundSpeed : airSpeed);
        }

        //collision check
        if (worldMap[int(posX + velX)][int(posY)] == false) posX += velX;
        else velX = 0;

        if (worldMap[int(posX)][int(posY + velY)] == false) posY += velY;
        else velY = 0;

        if (cameraHeight == 0) {
            velX *= movementFriction;
            velY *= movementFriction;
        }

        double rotSpeed = mdeltaX * frameTime * mouseSensetivity;

        //both camera direction and camera plane must be rotated
        double oldDirX = dirX;
        dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
        dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
        double oldPlaneX = planeX;
        planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
        planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);

        double velocityLen = sqrt((velX * velX) + (velY * velY));

        //camera up and down;
        pitch += mdeltaY * mouseSensetivity * 2;
        if (cameraHeight == 0)
            heightAdditional = (sin((double)frameCounter / 10) * clip(velocityLen, 0, 1)) * 500;
        else heightAdditional = 0;

        //jump
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
            if(cameraHeight == 0){
                verticalVelocity = 11;
            }
        }
        frameCounter++;
    }
	return 0;
}