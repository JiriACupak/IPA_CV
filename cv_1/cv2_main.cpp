#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <chrono>

// Fix pro Windows kolize maker min/max
#define NOMINMAX 
#include <opencv2/opencv.hpp>
#include <immintrin.h> // Pro AVX intrinsics

#define INPUT "input/input.jpg"

using namespace cv;
using namespace std;
using namespace std::chrono;

// --- KONFIGURACE ---
struct Config {
    static const int BRIGHTNESS_ADD = 50;         // Hodnota jasu k přičtení
    static const int BENCHMARK_ITERATIONS = 1000; // Počet iterací pro měření
    static const bool SHOW_GUI = true;            // Zobrazit okno s výsledkem?
};

// --- EXTERNÍ ASSEMBLER FUNKCE ---
extern "C" {
    // Windows x64 ABI (MinGW):
    // arg1 (data) -> RCX
    // arg2 (count) -> RDX
    // arg3 (brightness) -> R8
    void brightness_mmx(uint8_t* data, size_t count, int brightness);
    void brightness_scalar_asm(uint8_t* data, size_t count, int brightness);
}

using ProcessingFunc = std::function<void(uint8_t*, size_t, int)>;

// --- IMPLEMENTACE ALGORITMŮ ---

void runScalarASM(uint8_t* data, size_t count, int brightness) {
    brightness_scalar_asm(data, count, brightness);
}

void runMMX(uint8_t* data, size_t count, int brightness) {
    brightness_mmx(data, count, brightness);
}

void runAVX2(uint8_t* data, size_t count, int brightness) {
    // --- TODO: CVICENI AVX2 ---
    
    // Provizorní řešení
    for (size_t i = 0; i < count; ++i) {
        int val = data[i] + brightness;
        data[i] = (val > 255) ? 255 : val;
    }
}

// --- POMOCNÉ TŘÍDY ---

class Benchmarker {
public:
    static double measure(ProcessingFunc func, const Mat& src, int brightness, int iterations) {
        long long totalTime = 0;
        Mat temp;

        // Warmup
        for(int i=0; i<5; i++) {
            temp = src.clone();
            func(temp.data, temp.total() * temp.elemSize(), brightness);
        }

        for (int i = 0; i < iterations; ++i) {
            temp = src.clone(); 
            size_t size = temp.total() * temp.elemSize();
            uint8_t* ptr = temp.data;

            auto start = high_resolution_clock::now();
            func(ptr, size, brightness); 
            auto stop = high_resolution_clock::now();

            totalTime += duration_cast<microseconds>(stop - start).count();
        }
        return static_cast<double>(totalTime) / iterations;
    }
};

class Visualizer {
public:
    static void showGrid(const Mat& original, const vector<pair<string, Mat>>& results) {
        if (results.empty()) return;

        Size dispSize(480, 360);
        Mat combined, row1, row2;
        
        Mat imgOrig;
        resize(original, imgOrig, dispSize);
        putText(imgOrig, "Original", Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255,255,255), 2);

        vector<Mat> displayImages;
        displayImages.push_back(imgOrig);

        for (const auto& item : results) {
            Mat r;
            resize(item.second, r, dispSize);
            putText(r, item.first, Point(10, 30), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(255,255,255), 2);
            displayImages.push_back(r);
        }

        if (displayImages.size() >= 4) {
            hconcat(displayImages[0], displayImages[1], row1);
            hconcat(displayImages[2], displayImages[3], row2);
            vconcat(row1, row2, combined);
        } else {
            combined = displayImages[0];
        }

        namedWindow("Result Comparison", WINDOW_AUTOSIZE);
        imshow("Result Comparison", combined);
        
        cout << "Press any key in the window to exit..." << endl;
        waitKey(0);
        destroyAllWindows();
    }
};

// --- HLAVNÍ FUNKCE ---

int main() {
    string imagePath = INPUT;
    Mat img = imread(imagePath);
    
    if (img.empty()) {
        cerr << "ERROR: Could not load image: " << imagePath << endl;
        return 1;
    }
    if (img.channels() != 3) cvtColor(img, img, COLOR_GRAY2BGR);

    cout << "=== MMX/AVX Image Brightness Benchmark (Windows/MinGW) ===" << endl;
    cout << "Resolution: " << img.cols << "x" << img.rows << endl;
    cout << "Iterations: " << Config::BENCHMARK_ITERATIONS << endl << endl;

    struct Method {
        string name;
        ProcessingFunc func;
        double avgTime;
        Mat resultImg;
    };

    vector<Method> methods = {
        {"Scalar ASM", runScalarASM, 0.0, Mat()},
        {"MMX ASM",    runMMX,       0.0, Mat()},
        {"AVX2 C++",   runAVX2,      0.0, Mat()}
    };

    size_t dataSize = img.total() * img.elemSize();
    
    for (auto& m : methods) {
        m.avgTime = Benchmarker::measure(m.func, img, Config::BRIGHTNESS_ADD, Config::BENCHMARK_ITERATIONS);
        m.resultImg = img.clone();
        m.func(m.resultImg.data, dataSize, Config::BRIGHTNESS_ADD);
        
        cout << "Method: " << m.name << "\t| Time: " << m.avgTime << " us";
        if (m.name != "Scalar ASM") {
             double speedup = methods[0].avgTime / m.avgTime;
             cout << "\t| Speedup: " << speedup << "x";
        }
        cout << endl;
    }

    if (Config::SHOW_GUI) {
        vector<pair<string, Mat>> vizData;
        for(const auto& m : methods) {
            vizData.push_back({m.name, m.resultImg});
        }
        Visualizer::showGrid(img, vizData);
    }

    return 0;
}