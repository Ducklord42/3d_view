//TODO:
/*
Make binary STL files work.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#define RENDER_LINES 0x01
#define FILL_POLYGONS 0x02

#define PI 3.1415926535897932384626433832795

#define RED (color_t) 0xffff0000
#define WHITE (color_t) 0xffffffff
#define BLACK (color_t) 0xff000000

typedef uint32_t color_t;

typedef struct
{
    float x;
    float y;
    float z;
} point3d;

typedef struct
{
    point3d normal_vector;
    point3d a;
    point3d b;
    point3d c;
    color_t color;
} polygon_t;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
    char text[32];
} button_t;

typedef struct
{
    float scale;
    int height;
    int width;
    int occlude;
    float perspective;
    long long ms_delay_per_frame;
    float sensitivity;
    uint8_t rendermode;

    //SDL_Keycode for non-repeat events and SDL_Scancode for repeat events
    SDL_Keycode keybind_exit;
    SDL_Keycode keybind_settings;
    SDL_Keycode keybind_show_view;
    SDL_Keycode keybind_default_view;
    SDL_Keycode keybind_switch_xyz;
    SDL_Keycode keybind_debug;

    SDL_Scancode keybind_xrotate_plus;
    SDL_Scancode keybind_xrotate_minus;
    SDL_Scancode keybind_yrotate_plus;
    SDL_Scancode keybind_yrotate_minus;
    SDL_Scancode keybind_zoom_in;
    SDL_Scancode keybind_zoom_out;
} settings_t;

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

button_t new_button(int x, int y, int width, int height, char *text);

void renderbutton(uint32_t* screen, button_t button);

int check_button_pressed(button_t button, int x, int y);

point3d addpoint(point3d a, float x, float y, float z)
{
    point3d ret;
    ret.x = a.x + x;
    ret.y = a.y + y;
    ret.z = a.z + z;
    return ret;
}


settings_t settings;

int main(int argc, char** argv)
{
    settings.width = 1280;
    settings.height = 720;
    settings.ms_delay_per_frame = 10;
    settings.occlude = 0;
    settings.perspective = 0; //between 0 and 1
    settings.scale = 300;
    settings.sensitivity = 0.02;
    settings.rendermode = RENDER_LINES | FILL_POLYGONS;

    settings.keybind_yrotate_minus = SDL_SCANCODE_RIGHT;
    settings.keybind_yrotate_plus = SDL_SCANCODE_LEFT;
    settings.keybind_xrotate_minus = SDL_SCANCODE_DOWN;
    settings.keybind_xrotate_plus = SDL_SCANCODE_UP;
    settings.keybind_zoom_out = SDL_SCANCODE_MINUS;
    settings.keybind_zoom_in = SDL_SCANCODE_EQUALS;

    settings.keybind_exit = SDLK_X;
    settings.keybind_default_view = SDLK_D;
    settings.keybind_settings = SDLK_S;
    settings.keybind_switch_xyz = SDLK_F;
    settings.keybind_show_view = SDLK_V;
    settings.keybind_debug = SDLK_9;
    
    //<initilize SDL>
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Unable to Initilize SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Prospect Software 3D model viewer", settings.width, settings.height, 0);
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

    //init SDL_ttf
    TTF_Init();

    //load font
    TTF_Font *font = TTF_OpenFont("font.ttf", 18);
    if (!font)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Error", "Font couldn't be loaded.", window);
    }

    //create screen texture
    SDL_Texture* screen_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, settings.width, settings.height);
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

        fseek(file, 80, SEEK_SET);
        fread(&number_of_polygons, sizeof(uint32_t), 1, file);

        polygonlist = malloc(sizeof(polygon_t) * number_of_polygons);
        
        uint16_t *trash = malloc(2);
        for (int i = 0; i < number_of_polygons; i++)
        {
            //read vectors
            fread(&(polygonlist[i]), sizeof(float), 12, file);
            polygonlist[i].color = WHITE;
            //Attribute byte count
            fread(trash, 2, 1, file);
        }
        free(trash);
        
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
    long long frame_latency_ms = 0;
    float mouse_x_new = 0;
    float mouse_y_new = 0;
    float mouse_x_old = 0;
    float mouse_y_old = 0;

    //define buttons
    button_t buttonup = new_button(0, 100, 30, 20, "Up");
    button_t buttondown = new_button(0, 150, 30, 20, "Down");
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
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    //event.button.x and event.button.y for positions in window
                    if (event.button.button == 1) //left click
                    {
                        if (check_button_pressed(buttonup, event.button.x, event.button.y))
                        {
                            settings.perspective += 0.01;
                        }
                        if (check_button_pressed(buttondown, event.button.x, event.button.y))
                        {
                            settings.perspective -= 0.01;
                        }
                    }
                    else if (event.button.button == 2) //middle click
                    {
                    }
                    else if (event.button.button == 3) //right click
                    {
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    SDL_Keycode key = event.key.key;
                    if (event.key.repeat == 0) //one time event
                    {
                        if (key == settings.keybind_exit)
                        {
                            running = 0;
                        }
                        else if (key == settings.keybind_default_view)
                        {
                            yrotation = 0.35;
                            xrotation = 0.35;
                        }
                        else if (key == settings.keybind_show_view)
                        {
                            char buffer[64];
                            sprintf(buffer, "Xrot: %f\nYrot: %f\nZoom/Scale: %f\n", xrotation, yrotation, settings.scale);
                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Current Rotation", buffer, NULL);
                        }
                        else if (key == settings.keybind_switch_xyz)
                        {
                            polygon_t temp;
                            for (int i = 0; i < number_of_polygons; i++)
                            {
                                temp = polygonlist[i];
                                polygonlist[i].a = (point3d) {.x = temp.a.z, .y = temp.a.x, .z = temp.a.y};
                                polygonlist[i].b = (point3d) {.x = temp.b.z, .y = temp.b.x, .z = temp.b.y};
                                polygonlist[i].c = (point3d) {.x = temp.c.z, .y = temp.c.x, .z = temp.c.y};
                            }
                        }
                        else if (key == settings.keybind_debug)
                        {
                            char buffer[64];
                            sprintf(buffer, "Frames Per Second: %lld\nLatency: %lldms", fps, frame_latency_ms);
                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "DEBUG", buffer, NULL);
                        }
                    }
                    break;
            }
        }

        

        void* pixels;
        int pitch; //bytes per horizontal line
        SDL_LockTexture(screen_texture, NULL, &pixels, &pitch);

        //clear screen with black
        memset(pixels, 0, pitch * settings.height);
        
        uint32_t* pixels32 = (uint32_t*) pixels;
        
        //real-time keys
        const bool* keystate = SDL_GetKeyboardState(NULL);

        if (keystate[settings.keybind_yrotate_minus]) {yrotation -= settings.sensitivity;}
        if (keystate[settings.keybind_yrotate_plus]) {yrotation += settings.sensitivity;}
        if (keystate[settings.keybind_xrotate_plus]) {xrotation += settings.sensitivity;}
        if (keystate[settings.keybind_xrotate_minus]) {xrotation -= settings.sensitivity;}
        if (keystate[settings.keybind_zoom_in]) {settings.scale = settings.scale + (100 * settings.sensitivity);}
        if (keystate[settings.keybind_zoom_out]) {settings.scale = settings.scale < 1 ? 1: settings.scale - (100 * settings.sensitivity);}
        
        //check if right mouse button is being held for mouse rotation
        SDL_MouseButtonFlags mbf = SDL_GetMouseState(&mouse_x_new, &mouse_y_new);
        //numberrender(pixels32, (mbf & 0b0100) >> 2, (point3d) {.x = 50.0f, .y = 50.0f, .z = 0.0f}, 4);
        if (((mbf & 0b0100) >> 2) == 1)
        {
            //rotate based on difference of new and old mouse positions
            
        }

        //test line
        for (int i = 0; i < settings.width; i++)
        {
            put_pixel(pixels32, i, 10, RED);
        }
        //display buttons
        renderbutton(pixels32, buttonup);
        renderbutton(pixels32, buttondown);
        //display perspective number not float
        numberrender(pixels32, (int) (settings.perspective * 100.0f), (point3d) {.x = 100.0f, .y = 0.0f, .z = 0.0f}, 3);
        //display fps
        numberrender(pixels32, fps, (point3d) {.x=10.0f, .y=10.0f, .z=0.0f}, 3);
        //render the polygons
        polyrender(pixels32, polygonlist, number_of_polygons, xrotation, yrotation, settings.rendermode);

        SDL_UnlockTexture(screen_texture);

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(settings.ms_delay_per_frame);

        SDL_Time end_time;
        SDL_GetCurrentTime(&end_time);
        fps = 1000000000 / (end_time - start_time);
        frame_latency_ms = (end_time - start_time) / 1000;
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

        current_polygon.normal_vector = rotatex(xangle, rotatey(yangle, current_polygon.normal_vector));
        if (settings.occlude && (current_polygon.normal_vector.z < 0)) {return;}

        //rotate the polygon
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
            case 0: // Actually good coding by Noah (discord: whyareyoureadingthis)
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
    ret.x = roundf(point.x * settings.scale) + (settings.width / 2);
    ret.y = roundf(0 - point.y * settings.scale) + (settings.height / 2);
    return ret;
}

point3d model_to_2d(point3d point, float xangle, float yangle)
{
    point3d ret = rotatex(xangle, rotatey(yangle, point));
    if (settings.perspective)
    {
        ret.x = ret.x / (2 - ret.z * settings.perspective);
        ret.y = ret.y / (2 - ret.z * settings.perspective);
    }
    return ret;
}

point3d rotatex(float angle, point3d point)
{
    point3d ret;
    float cosa = cosf(angle);
    float sina = sinf(angle);

    ret.x = point.x;
    ret.y = (point.y * cosa) - (point.z * sina);
    ret.z = (point.y * sina) + (point.z * cosa);
    return ret;
}

point3d rotatey(float angle, point3d point)
{
    point3d ret;
    float cosa = cosf(angle);
    float sina = sinf(angle);

    ret.x = (point.x * cosa) + (point.z * sina);
    ret.y = point.y;
    ret.z = (point.z * cosa) - (point.x * sina);
    return ret;
}

point3d rotatez(float angle, point3d point)
{
    point3d ret;
    float cosa = cosf(angle);
    float sina = sinf(angle);

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
    if (x < settings.width && y < settings.height)
    {
        screen[x + (y * settings.width)] = color;
    }
}

button_t new_button(int x, int y, int width, int height, char *text)
{
    button_t ret;
    ret.height = height;
    strcpy(ret.text, text);
    ret.width = width;
    ret.x = x;
    ret.y = y;
    return ret;
}

void renderbutton(uint32_t* screen, button_t button)
{
    point3d topleft = (point3d) {.x = button.x, .y = button.y, .z = 0.0f};
    line(screen, topleft, addpoint(topleft, button.width, 0, 0), WHITE);
    line(screen, addpoint(topleft, button.width, button.height, 0), addpoint(topleft, button.width, 0, 0), WHITE);
    line(screen, addpoint(topleft, 0, button.height, 0), addpoint(topleft, button.width, button.height, 0), WHITE);
    line(screen, topleft, addpoint(topleft, 0, button.height, 0), WHITE);
}

int check_button_pressed(button_t button, int x, int y)
{
    return ((x >= button.x) && ((button.x + button.width) >= x)) && ((y >= button.y) && ((button.y + button.height) >= y));
}