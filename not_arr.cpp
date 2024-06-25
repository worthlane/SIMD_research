#include <SFML/Graphics.hpp>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

static const int R2_MAX = 10000;

static const int LENGTH = 400;
static const int WIDTH  = 200;

static const int N_MAX  = 256;

static const int RGB_MAX = 256;

static const float LIN_SHIFT   = 0.10;
static const float SCALE_SHIFT = 0.1;           // 10 percent size increase/decrease
static const float X_PRE_SHIFT = -1.325;
static const float Y_PRE_SHIFT = 0.f;

static const float dx = ((float) 1 / (float) LENGTH);
static const float dy = ((float) 1 / (float) WIDTH);

extern "C" u_int64_t _GetTick();

static const size_t TEST_AMT = 300;
#define TEST_RUN 1

void GenerateColor(u_int8_t* points, const int x, const int y, const int N);

void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale);

void HandleKeyboard(sf::Event* event, float* x_shift, float* y_shift, float* scale);

void DumpPoints(sf::RenderWindow* window, u_int8_t* points);

// =========================================================================

int main()
{
    sf::RenderWindow window(sf::VideoMode(LENGTH, WIDTH), "Mandelbrot");

    float x_shift = 0.f;
    float y_shift = 0.f;
    float scale   = 1.00;

    u_int64_t time = 0;
    size_t    run  = 0;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            HandleKeyboard(&event, &x_shift, &y_shift, &scale);
        }

        window.clear();

        u_int64_t start = _GetTick();

        u_int8_t points[WIDTH * LENGTH * 4] = {};
        CalcMandelbrot(&window, points, x_shift, y_shift, scale);

        u_int64_t end   = _GetTick();

        time += end - start;
        run++;

        #if TEST_RUN
        if (run >= TEST_AMT)
            window.close();

        printf("%ld/%ld\n", run, TEST_AMT);
        #else
        DumpPoints(&window, points);
        #endif
    }

    #if TEST_RUN
    printf("RUNS: %ld, TIME: %ld, TIME PER RUN: %ld\n", run, time, time / run);
    #endif

    return 0;
}

// -------------------------------------------------------

void GenerateColor(u_int8_t* points, const int x, const int y, const int N)
{
    assert(points);

    float I = sqrtf( sqrtf( (float) N / (float) N_MAX )) * 255.f;
    u_int8_t color = (u_int8_t) I;

    size_t point_pos = (y * LENGTH + x) * 4;

    points[point_pos]     = RGB_MAX - color;
    points[point_pos + 1] = RGB_MAX - color;
    points[point_pos + 2] = color;
    points[point_pos + 3] = 255;
}

// -------------------------------------------------------

void CalcMandelbrot(sf::RenderWindow* window, u_int8_t* points, const float x_shift, const float y_shift, const float scale)
{
    for (volatile size_t y = 0; y < WIDTH; y++)
    {
        float x0 = ((-(float) LENGTH / 2) * dx) * scale + x_shift + X_PRE_SHIFT;
        float y0 = (((float) y - (float) WIDTH / 2) * dy) * scale + y_shift + Y_PRE_SHIFT;

        for (volatile size_t x = 0; x < LENGTH; x++, x0 += dx * scale)
        {
            float X  = x0;
            float Y  = y0;

            volatile size_t N = 0;

            for (; N < N_MAX; N++)
            {
                float x2 = X * X;
                float y2 = Y * Y;
                float xy = X * Y;

                float R2 = x2 + y2;

                if (R2 > R2_MAX)    break;

                X = x2 - y2 + x0;
                Y = xy + xy + y0;
            }

            GenerateColor(points, x, y, N);
        }
    }
}

// -------------------------------------------------------

void HandleKeyboard(sf::Event* event, float* x_shift, float* y_shift, float* scale)
{
    assert(event);
    assert(x_shift);
    assert(y_shift);
    assert(scale);

    if (event->type == sf::Event::KeyReleased)
    {
        switch (event->key.code)
        {
            case sf::Keyboard::A:
                *x_shift -= LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::D:
                *x_shift += LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::W:
                *y_shift -= LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::S:
                *y_shift += LIN_SHIFT * *scale;
                break;

            case sf::Keyboard::Up:
                *scale += SCALE_SHIFT * *scale;
                break;

            case sf::Keyboard::Down:
                *scale -= SCALE_SHIFT * *scale;
                break;

            default:
                break;
        }
    }
}

// -------------------------------------------------------

void DumpPoints(sf::RenderWindow* window, u_int8_t* points)
{
    assert(points);
    assert(window);

    sf::Texture texture;
    texture.create(LENGTH, WIDTH);
    sf::Sprite sprite;

    texture.update(points);
    sprite.setTexture(texture);

    window->draw(sprite);

    window->display();
}
