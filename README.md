# Lab Report: Exploring SIMD Functions

## Objective

In this project, we will try to speed up the algorithm for rendering the Mandelbrot set using SIMD functions. We will also study how different optimization methods affect the performance of computationally intensive algorithms.

## Algorithm Overview

As an example of a computationally intensive algorithm, we will consider rendering the [Mandelbrot set](https://ru.wikipedia.org/wiki/Множество_Мандельброта). Briefly, the visualization is constructed as follows:

1. Choose the radius of a circle within which we will construct the set.
2. Select a point with coordinates $(x_0, y_0)$ inside this circle.
3. Based on the coordinates of the selected point, consider subsequent points with coordinates $(x_n, y_n)$, where
   $$x_n = x_{n - 1}^2 + y_{n - 1}^2 + x_0$$
   $$y_n = 2 \cdot x_{n - 1} \cdot y_{n - 1} + y_0$$.
5. Continue until the $n$th point is farther than $R_{MAX}$ (chosen in the first step) from the center of the circle.
6. Color the initial point $(x_0, y_0)$ using a color determined by $n$.

This lets us generate the Mandelbrot set and display its visualization on the screen:

![Fig. 1](pictures/set_normal.jpg "Fig. 1 (Mandelbrot Set)")

## Methodology

We will compare three implementations of this algorithm.

1) A basic version without any optimizations. Each point is processed in turn, and its color is calculated sequentially.
2) An array-based version. We will process arrays of several consecutive points, calculating the color of each point in the array sequentially. This is intended to give the compiler a hint that the algorithm can be simplified.
3) A fast version using AVX instructions.

To evaluate the results, we need to compare the execution times of the three programs. A sufficiently precise approach is to use the ```rdtsc``` assembly instruction, which returns the processor's timestamp counter in the ```EDX:EAX``` registers. We will write an assembly function that returns this counter and measure the difference between the start and end of the Mandelbrot calculations. This gives us the algorithm's execution time in clock ticks. Comparing these counts lets us assess the performance differences between optimization methods.

For more precise measurements, each program run will measure the execution time for 300 frames and calculate the average tick count per frame. We will run each program 3 times to obtain the most accurate result possible using the least squares method. We will then run the program once more to obtain a control measurement and compare it with the previously calculated average to assess whether the results are reasonable. We will refer to this additional measurement as the control measurement.

For all programs, we will measure execution time with different compiler optimization flags. We will also use the ```-mavx2``` flag so that the compiler can use AVX instructions to speed up the program.

## Implementation

### Basic Version

We will implement sequential color calculation for each point and measure the time required to render one frame:

$t_i$ - frame execution time in clock ticks, where $i$ is the measurement number.
$t_{avg}$ - average execution time calculated using the least squares method.
$t_{control}$ - control timing measurement.

| Compiler optimization   | $t_{1}$  | $t_{2}$  | $t_{3}$  | $t_{avg}$ | $t_{control}$ |
|:-----------------------:|:--------:|:--------:|:--------:|:---------:|:------------:|
|  -O3                    | 50382199 | 50533575 | 50390267 | 50435347  |  50683631    |
|  -O0                    | 98117403 | 97898647 | 98284039 | 98100029  |  98130876    |

Compiler optimization speeds up the program by approximately $\frac{98100029}{50435347} \approx 1.94$ times. This is not particularly surprising: the compiler does a great job of optimizing the code.

![](pictures/compiler.jpg "Fig. 2 (Modern Compiler)")

### Array-Based Implementation

Here, we will try to hint to the compiler that AVX instructions could be used for optimization. We will write equivalents of SIMD functions that take arrays of several ```float``` values as arguments. The only difference is that our implementation performs calculations sequentially for each array element, whereas SIMD functions perform them in parallel. Such parallel execution could speed up the program by a factor equal to the number of values in each array. My processor supports 256-bit vectors, so we can use arrays of 8 floating-point numbers: each ```float``` occupies 32 bits, and $32 \cdot 8 = 256$. Measuring the tick count per frame gives the following results:

| Compiler optimization   | $t_{1}$   | $t_{2}$   | $t_{3}$   | $t_{avg}$ | $t_{control}$ |
|:-----------------------:|:---------:|:---------:|:---------:|:---------:|:------------:|
|  -O3                    | 26382920  | 27258303  | 26851765  | 26830996  |  27890640    |
|  -O0                    | 406515603 | 415656116 | 415192044 | 412454587 |  422255688   |

The results are quite interesting: different compiler optimization settings produce very different outcomes. With -O3, the array-based implementation speeds up the program by $\frac{50435347}{26830996} \approx 1.88$ times. Apparently, the compiler did not recognize the optimization we intended, although it did find some opportunities to speed up the program that were unavailable in the first version. Now consider the results with optimization disabled (-O0). In this case, the first version is $\frac{412454587}{98100029} \approx 4.2$ times faster. I suspect this is because we recalculate the coordinates of every point in an array as long as at least one point remains inside the circle. When processing points individually, we never spend time calculating new coordinates for a point that has already left the circle. This may not be the only reason for the slowdown, but I consider it one of the most significant. Nested loops also introduce overhead, including more frequent memory accesses, which take considerable time.

This suggests that giving the compiler hints about which instructions to use does not produce a significant benefit. The success of the optimization also depends heavily on compiler flags. In our case, the program became much slower with -O0.

### SIMD Instructions

We can now implement the algorithm using intrinsics. Essentially, all we need to change in the previous program is to replace the arrays of numbers with variables of type ```__m256```. We will likewise treat these variables as arrays, with their components holding data for independent points in the set. Let us measure the execution time of this implementation:

| Compiler optimization   | $t_{1}$   | $t_{2}$   | $t_{3}$   | $t_{avg}$ | $t_{control}$ |
|:-----------------------:|:---------:|:---------:|:---------:|:---------:|:------------:|
|  -O3                    | 10078038  | 10051705  | 10018111  | 10049284  |  10292526    |
|  -O0                    | 33861935  | 33916503  | 33827301  | 33868579  |  33894652    |

The results show that SIMD instructions speed up the program by $\frac{50435347}{10049284} \approx 5.01$ times.

![](pictures/wow.jpg "Fig. 3 (My Reaction)")

This is less than expected, since one ```__m256``` variable allows us to process 8 points simultaneously. Nevertheless, we have outperformed the compiler's optimization of the first version, which is an excellent result. Why did the program become only 5 times faster? I suspect this is again due to unnecessary coordinate calculations for points that have already left the circle. These points continue to be processed simply because they share a variable with another point that is still inside the circle.

## Conclusion

We can draw two conclusions from this project:

1) SIMD instructions substantially speed up programs. The ability to calculate several coordinates in parallel plays a major role in optimization.
2) Rather than hinting to the compiler that an optimization is possible, it is better to implement it directly. In our second implementation, the compiler clearly found more optimization opportunities than in the first version. However, it still could not achieve the speedup of the third version, where we explicitly used SIMD instructions.

# Sources

1) Surprised blue emoji [Fig. 3]. [Grizly Club](https://grizly.club/bez_fona/47250-kartinki-udivlennyj-smajlik-bez-fona-51-foto.html) (11 August 2023)
2) Muscular Minion [Fig. 2]. [Danila Kulizhnikov's VK profile picture](https://vk.com/kulizhnik) (9 March 2024)
