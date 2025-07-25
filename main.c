#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <SDL3/SDL.h>

#define RENDER_LINES 0x01
#define FILL_POLYGONS 0x02

#define PI 3.1415926535897932384626433832795

#define SENSITIVITY 0.02

//set from 1 to 0 for best results. set to 0 if no perspective
#define PERSPECTIVE 0.5

//max 100fps
#define MS_DELAY_PER_FRAME 10

#define RED (color_t) 0xffff0000
#define WHITE (color_t) 0xffffffff
#define BLACK (color_t) 0xff000000

#define HEIGHT 720
#define WIDTH (HEIGHT * 16 / 9)

int scale = 300;

typedef uint32_t color_t;

typedef struct
{
    float x;
    float y;
    float z;
} point3d;

typedef struct
{
    point3d a;
    point3d b;
    point3d c;
    color_t color;
} polygon_t;

point3d rotatex(float angle, point3d point);

point3d rotatey(float angle, point3d point);

point3d rotatez(float angle, point3d point);

polygon_t newpolygon(color_t color, point3d a, point3d b, point3d c);

void polyrender(uint32_t* screen, polygon_t* polygonlist, long long number_of_polygons, float xangle, float yangle, uint8_t mode);

void line(uint32_t* screen, point3d a, point3d b, color_t color);

point3d calculated_position_to_screen_position(point3d point);

void put_pixel(uint32_t* screen, unsigned x, unsigned y, uint32_t color);

point3d model_to_2d(point3d point, float xangle, float yangle);

void numberrender(uint32_t* screen, int number, point3d offset, int count);

point3d addpoint(point3d a, float x, float y, float z)
{
    point3d ret;
    ret.x = a.x + x;
    ret.y = a.y + y;
    ret.z = a.z + z;
    return ret;
}


int main(int argc, char** argv)
{
    //<initilize SDL>
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Unable to Initilize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Prospect Software 3D model viewer", WIDTH, HEIGHT, 0);
    if (!window) {SDL_Log("Unable to create window: %s", SDL_GetError()); return 1;}

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        SDL_Log("Renderer Error: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    //icon
    SDL_Surface* icon = SDL_LoadBMP("icon.bmp");
    if (icon == NULL)
    {
        printf("No Icon\n");
    }
    else
    {
        SDL_SetWindowIcon(window, icon);
    }

    //create screen texture
    SDL_Texture* screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WIDTH, HEIGHT);
    //<initilize SDL>

    int number_of_polygons = 0;
    polygon_t* polygonlist = NULL;
    if (argc > 1)
    {
        //load file
        FILE* file = fopen(argv[1], "r");
        if (file == NULL)
        {
            SDL_Log("Error 01: File Not Open");
            number_of_polygons = 0;
        }

        fseek(file, 0L, SEEK_END);
        long long size = ftell(file);
        fseek(file, 0L, SEEK_SET);
        number_of_polygons = size / sizeof(polygon_t);

        polygonlist = malloc(sizeof(polygon_t) * number_of_polygons);
        size_t read = fread(polygonlist, sizeof(polygon_t), number_of_polygons, file);
        
        fclose(file);
    }
    else
    {
        //front
        point3d p1 = {1.0f, 1.0f, 1.0f}; //top right
        point3d p2 = {1.0f, -1.0f, 1.0f}; //bottom right
        point3d p3 = {-1.0f, 1.0f, 1.0f}; //top left
        point3d p4 = {-1.0f, -1.0f, 1.0f}; //bottom left
        //back
        point3d p5 = {1.0f, 1.0f, -1.0f}; //top right
        point3d p6 = {1.0f, -1.0f, -1.0f}; //bottom right
        point3d p7 = {-1.0f, 1.0f, -1.0f}; //top left
        point3d p8 = {-1.0f, -1.0f, -1.0f}; //bottom left

        polygon_t polygon = newpolygon(RED, p1, p4, p3);
        polygon_t polygon2 = newpolygon(WHITE, p2, p1, p4);

        number_of_polygons = 12;
        polygonlist = malloc(sizeof(polygon_t) * number_of_polygons);
        //front face
        polygonlist[0] = polygon;
        polygonlist[1] = polygon2;
        //left face
        polygonlist[2] = newpolygon(RED, p3, p4, p8);
        polygonlist[3] = newpolygon(WHITE, p7, p8, p3);
        //top face
        polygonlist[4] = newpolygon(RED, p1, p3, p5);
        polygonlist[5] = newpolygon(WHITE, p3, p5, p7);

        //back face
        polygonlist[6] = newpolygon(RED, p5, p7, p6);
        polygonlist[7] = newpolygon(WHITE, p8, p7, p6);
        //right face
        polygonlist[8] = newpolygon(RED, p1, p2, p5);
        polygonlist[9] = newpolygon(WHITE, p2, p5, p6);
        //bottom face
        polygonlist[10] = newpolygon(RED, p2, p8, p4);
        polygonlist[11] = newpolygon(WHITE, p2, p8, p6);

        /*for (int i = 0; i < 480; i++)
        {
            printf("%02x", *(i + (uint8_t*) polygonlist));
        }*/
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Message", "There is no file selected. Default cube opened.", window);
    }

    char running = 1;
    SDL_Event event;
    float yrotation = 0.35f;
    float xrotation = 0.35f;
    long long fps = 0;
    while (running)
    {
        SDL_Time start_time;
        SDL_GetCurrentTime(&start_time);
        //handle events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    running = 0;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    SDL_Keycode key = event.key.key;
                    if (event.key.repeat == 0) //one time event
                    {
                        if (key == SDLK_X)
                        {
                            running = 0;
                        }
                        else if (key == SDLK_0)
                        {
                            yrotation = 0.35;
                            xrotation = 0.35;
                        }
                        else if (key == SDLK_S)
                        {
                            char buffer[64];
                            sprintf(buffer, "Xrot: %f\nYrot: %f\n", xrotation, yrotation);
                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Current Rotation", buffer, NULL);
                        }
                    }
                    break;
            }
        }

        

        void* pixels;
        int pitch; //bytes per horizontal line
        SDL_LockTexture(screen_texture, NULL, &pixels, &pitch);

        //clear screen with black
        memset(pixels, 0, pitch * HEIGHT);
        
        uint32_t* pixels32 = (uint32_t*) pixels;
        
        //real-time keys
        const bool* keystate = SDL_GetKeyboardState(NULL);

        if (keystate[SDL_SCANCODE_RIGHT]) {yrotation -= SENSITIVITY;}
        if (keystate[SDL_SCANCODE_LEFT]) {yrotation += SENSITIVITY;}
        if (keystate[SDL_SCANCODE_UP]) {xrotation += SENSITIVITY;}
        if (keystate[SDL_SCANCODE_DOWN]) {xrotation -= SENSITIVITY;}
        if (keystate[SDL_SCANCODE_EQUALS]) {scale = scale * (1 + SENSITIVITY);}
        if (keystate[SDL_SCANCODE_MINUS]) {scale = scale * (1 - SENSITIVITY);}
        numberrender(pixels32, 1234567890, (point3d) {100.0f, 100.0f, 0.0f}, 10);
        
        for (int i = 0; i < WIDTH; i++)
        {
            put_pixel(pixels32, i, 10, RED);
        }
        //display fps
        numberrender(pixels32, fps, (point3d) {.x=10.0f, .y=10.0f, .z=0.0f}, 3);
        //render the polygons
        polyrender(pixels32, polygonlist, number_of_polygons, xrotation, yrotation, RENDER_LINES | FILL_POLYGONS);

        SDL_UnlockTexture(screen_texture);

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(MS_DELAY_PER_FRAME);

        SDL_Time end_time;
        SDL_GetCurrentTime(&end_time);
        fps = 1000000000 / (end_time - start_time);
        //printf("FPS: %lld\n", fps);
    }

    //end the SDL stuff and heap
    free(polygonlist);

    SDL_DestroySurface(icon);
    SDL_DestroyTexture(screen_texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}



void polyrender(uint32_t* screen, polygon_t* polygonlist, long long number_of_polygons, float xangle, float yangle, uint8_t mode)
{
    for (long long i = 0; i < number_of_polygons; i++)
    {
        polygon_t current_polygon = polygonlist[i];
        //rotate the polygon
        //current_polygon.a = rotatex(xangle, rotatey(yangle, current_polygon.a));
        current_polygon.a = model_to_2d(current_polygon.a, xangle, yangle);
        current_polygon.b = model_to_2d(current_polygon.b, xangle, yangle);
        current_polygon.c = model_to_2d(current_polygon.c, xangle, yangle);

        //render the polygon
        if (mode && RENDER_LINES)
        {
            line(screen, calculated_position_to_screen_position(current_polygon.a), calculated_position_to_screen_position(current_polygon.b), current_polygon.color);
            line(screen, calculated_position_to_screen_position(current_polygon.a), calculated_position_to_screen_position(current_polygon.c), current_polygon.color);
            line(screen, calculated_position_to_screen_position(current_polygon.c), calculated_position_to_screen_position(current_polygon.b), current_polygon.color);
        }
        if (mode && FILL_POLYGONS)
        {
            //fill polygon
        }
    }
}

void line(uint32_t* screen, point3d a, point3d b, color_t color)
{
    point3d scaleda = a;
    point3d scaledb = b;

    if (scaleda.x == scaledb.x)
    {
        int lessy = scaleda.y > scaledb.y ? scaledb.y : scaleda.y;
        int greatery = scaleda.y > scaledb.y ? scaleda.y : scaledb.y;
        for (int i = lessy; i <= greatery; i++)
        {
            put_pixel(screen, (int) scaleda.x, i, WHITE);
        }
    }
    else
    {
        float slope = (scaleda.y - scaledb.y) / (scaleda.x - scaledb.x);
        if (abs(slope) <= 1)
        {
            int least_x;
            int most_x;
            float y;
            if (scaleda.x > scaledb.x) {least_x = scaledb.x; most_x = scaleda.x; y = scaledb.y;}
            else {least_x = scaleda.x; most_x = scaledb.x; y = scaleda.y;}

            for (int i = least_x; i <= most_x; i++)
            {
                put_pixel(screen, i, y, color);
                y += slope;
            }
        }
        else
        {
            slope = (scaleda.x - scaledb.x) / (scaleda.y - scaledb.y);
            int least_y;
            int most_y;
            float x;
            if (scaleda.y > scaledb.y) {least_y = scaledb.y; most_y = scaleda.y; x = scaledb.x;}
            else {least_y = scaleda.y; most_y = scaledb.y; x = scaleda.x;}

            for (int i = least_y; i <= most_y; i++)
            {
                put_pixel(screen, x, i, WHITE);
                x += slope;
            }
        }
    }
}

void numberrender(uint32_t* screen, int number, point3d offset, int count)
{
    //loop over each char
    for (int i = 0; i < count; i++)
    {
        uint8_t character = (number / (int)pow(10, (count - (i + 1)))) % 10;
        switch (character)
        {
            case 0:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, addpoint(offset, 10, 20, 0), addpoint(offset, 10, 0, 0), WHITE);
                line(screen, offset, addpoint(offset, 0, 20, 0), WHITE);
                line(screen, addpoint(offset, 10, 20, 0), addpoint(offset, 0, 20, 0), WHITE);
                break;
            case 1:
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 2:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 10, 10, 0), addpoint(offset, 0, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 0, 20, 0), WHITE);
                line(screen, addpoint(offset, 0, 20, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 3:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 20, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 4:
                line(screen, offset, addpoint(offset, 0, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 5:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, addpoint(offset, 0, 0, 0), addpoint(offset, 0, 10, 0), WHITE);
                line(screen, addpoint(offset, 10, 10, 0), addpoint(offset, 0, 10, 0), WHITE);
                line(screen, addpoint(offset, 10, 10, 0), addpoint(offset, 10, 20, 0), WHITE);
                line(screen, addpoint(offset, 0, 20, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 6:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, offset, addpoint(offset, 0, 20, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 20, 0), addpoint(offset, 10, 20, 0), WHITE);
                line(screen, addpoint(offset, 10, 10, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 7:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 8:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, offset, addpoint(offset, 0, 20, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 20, 0), addpoint(offset, 10, 20, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            case 9:
                line(screen, offset, addpoint(offset, 10, 0, 0), WHITE);
                line(screen, offset, addpoint(offset, 0, 10, 0), WHITE);
                line(screen, addpoint(offset, 0, 10, 0), addpoint(offset, 10, 10, 0), WHITE);
                line(screen, addpoint(offset, 10, 0, 0), addpoint(offset, 10, 20, 0), WHITE);
                break;
            default:
                break;
        }
        offset.x += 14;
    }
}

point3d calculated_position_to_screen_position(point3d point)
{
    point3d ret;
    ret.x = roundf(point.x * scale) + (WIDTH / 2);
    ret.y = roundf(0 - point.y * scale) + (HEIGHT / 2);
    return ret;
}

point3d model_to_2d(point3d point, float xangle, float yangle)
{
    point3d ret = rotatex(xangle, rotatey(yangle, point));
    if (PERSPECTIVE)
    {
        ret.x = ret.x / (2 - ret.z * PERSPECTIVE);
        ret.y = ret.y / (2 - ret.z * PERSPECTIVE);
    }
    return ret;
}

point3d rotatex(float angle, point3d point)
{
    point3d ret;
    float cosa = cos(angle);
    float sina = sin(angle);

    ret.x = point.x;
    ret.y = (point.y * cosa) - (point.z * sina);
    ret.z = (point.y * sina) + (point.z * cosa);
    return ret;
}

point3d rotatey(float angle, point3d point)
{
    point3d ret;
    float cosa = cos(angle);
    float sina = sin(angle);

    ret.x = (point.x * cosa) + (point.z * sina);
    ret.y = point.y;
    ret.z = (point.z * cosa) - (point.x * sina);
    return ret;
}

point3d rotatez(float angle, point3d point)
{
    point3d ret;
    float cosa = cos(angle);
    float sina = sin(angle);

    ret.x = (point.x * cosa) - (point.y * sina);
    ret.y = (point.x * sina) + (point.y * cosa);
    ret.z = point.x;
    return ret;
}

polygon_t newpolygon(color_t color, point3d a, point3d b, point3d c)
{
    polygon_t ret;
    ret.color = color;
    ret.a = a;
    ret.b = b;
    ret.c = c;
    return ret;
}

void put_pixel(uint32_t* screen, unsigned x, unsigned y, uint32_t color)
{
    if (x < WIDTH && y < HEIGHT)
    {
        screen[x + (y * WIDTH)] = color;
    }
}