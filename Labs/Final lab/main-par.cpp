#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <cstring>
#include <omp.h>
#include <sys/types.h>
#include <iomanip>
#include <chrono>
#include <dirent.h>
using namespace std;

int nthreads = 16;


int kernel[5][5] = {1, 4, 7, 4, 1,
                    4, 16, 26, 16, 4,
                    7, 26, 41, 26, 7,
                    4, 16, 26, 16, 4,
                    1, 4, 7, 4, 1};

int accessPixel(unsigned char * arr, int col, int row, int k, int width, int height)
{
    int sum = 0;
    int sumKernel = 0;

        for (int j = -2; j <= 2; j++)
        {
            for (int i = -2; i <= 2; i++)
            {
                if ((row + j) >= 0 && (row + j) < height && (col + i) >= 0 && (col + i) < width)
                {
                    int color = arr[(row + j) * 3 * width + (col + i) * 3 + k];
                    sum += color * kernel[i + 2][j + 2];
                    sumKernel += kernel[i + 2][j + 2];
                }
            }
        }
    //in this case sumKernel should be equal to 273
    return sum / sumKernel;
}

void guassian_blur2D(unsigned char * arr, unsigned char * result, int width, int height)
{
    #pragma omp parallel for schedule(dynamic) num_threads(nthreads)
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col++)
            {
                for (int k = 0; k < 3; k++)
                {
                    result[3 * row * width + 3 * col + k] = accessPixel(arr, col, row, k, width, height);
                }
            }
        }
}
int sobel_x[3][3] =
        {{1, 2, 1},
         {0, 0, 0},
         {-1, -2, -1}};

int sobel_y[3][3] =
        {{-1, 0, 1},
         {-2,  0,  2},
         {-1,  0,  1}};

int accessPixelSobel(unsigned char * arr, int col, int row, int k, int width, int height)
{
    int pixel_x = 0;
    int pixel_y = 0;
    int weight = 8;

        for (int j = -1; j <= 1; j++)
        {
            for (int i = -1; i <= 1; i++)
            {
                if ((row + j) >= 0 && (row + j) < height && (col + i) >= 0 && (col + i) < width)
                {
                    int color = arr[(row + j) * 3 * width + (col + i) * 3 + k];
                    pixel_x += color * sobel_x[i + 1][j + 1];
                    pixel_y += color * sobel_y[i + 1][j + 1];
                }
            }
        }
    int val_x = abs(pixel_x / weight);
    int val_y = abs (pixel_y / weight);
    int val = val_x + val_y;
    if (val < 0) val = 0;
    if (val > 255) val = 255;
    return val;
}

void sobel_filter(unsigned char * arr, unsigned char * result, int width, int height)
{
    #pragma omp parallel for schedule(dynamic) num_threads(nthreads)
        for (int row = 0; row < height; row++)
        {
            for (int col = 0; col < width; col++)
            {
                for (int k = 0; k < 3; k++)
                {
                    result[3 * row * width + 3 * col + k] = accessPixelSobel(arr, col, row, k, width, height);
                }
            }
        }
}

int main(int argc, char* argv[]) {
    string option = argv[1];
    DIR *dp;
    struct dirent *direntp;
    if (argc == 4) {
        cout << "Input path: [" << argv[2] << "]" << endl;
        cout << "Output path: [" << argv[3] << "]" << endl;

        if((dp = opendir(argv[3])) == NULL){
            cout << "Output directory does not exist [" << argv[3] << "]" << "does not exist" << endl;
            cout << "	image-seq operation in_path out_path" << endl;
            cout << "		operation: copy, gauss, sobel" << endl;
        }else if((dp = opendir(argv[2])) == NULL) {
            cout << "Cannot open directory [" << argv[2] << "]" << endl;
            cout << "	image-seq operation in_path out_path" << endl;
            cout << "		operation: copy, gauss, sobel" << endl;
        }
        else if (!option.compare("copy")) {
            //COPY
            while ((direntp = readdir(dp)) != NULL) {
                auto startTotal = std::chrono::high_resolution_clock::now();
                string s = direntp->d_name;
                if (s.compare(".") && s.compare("..")) {
                    char str[100] = "";
                    strcat(str, argv[2]);
                    strcat(str, "/");
                    auto start = std::chrono::high_resolution_clock::now();
                    FILE *f = fopen(strcat(str, direntp->d_name), "rb");
                    unsigned char info[54];

                    // read the 54-byte header
                    fread(info, sizeof(unsigned char), 54, f);

                    // extract image height and width from header
                    int width = *(int *) &info[18];
                    int height = *(int *) &info[22];

                    // allocate 3 bytes per pixel
                    int size = 3 * width * height;
                    unsigned char *data = new unsigned char[size];

                    // read the rest of the data at once
                    fread(data, sizeof(unsigned char), size, f);
                    fclose(f);

                    auto elapsed = std::chrono::high_resolution_clock::now() - start;
                    long long loadTime = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();


                    //store

                    auto startStore = std::chrono::high_resolution_clock::now();
                    char str2[100] = "";
                    strcat(str2, argv[3]);
                    strcat(str2, "/");
                    strcat(str2, direntp->d_name);
                    FILE *f2 = fopen(str2, "wb");
                    fwrite(info, sizeof(unsigned char), 54, f2);


                    // read the rest of the data at once
                    fwrite(data, sizeof(unsigned char), size, f2);
                    fclose(f2);
                    auto elapsedStore = std::chrono::high_resolution_clock::now() - startStore;
                    long long storeTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedStore).count();
                    auto elapsedTotal = std::chrono::high_resolution_clock::now() - startTotal;
                    long long totalTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedTotal).count();
                    cout << "File: \"" << str << "\"" << "(time: " << totalTime << ")" << endl;
                    cout << "	Load time: " << loadTime << endl;
                    cout << "	Store time: " << storeTime << endl;
            	}
            }
        }

        else if (!option.compare("gauss")) {

            //GAUSS
            while ((direntp = readdir(dp)) != NULL) {
                auto startTotal = std::chrono::high_resolution_clock::now();
                string s = direntp->d_name;
                if (s.compare(".") && s.compare("..")) {
                    char str[100] = "";
                    strcat(str, argv[2]);
                    strcat(str, "/");
                    auto start = std::chrono::high_resolution_clock::now();
                    FILE *f = fopen(strcat(str, direntp->d_name), "rb");
                    unsigned char info[54];

                    // read the 54-byte header
                    fread(info, sizeof(unsigned char), 54, f);

                    // extract image height and width from header
                    int width = *(int *) &info[18];
                    int height = *(int *) &info[22];

                    // allocate 3 bytes per pixel
                    int size = 3 * width * height;
                    unsigned char *data = new unsigned char[size];

                    // read the rest of the data at once
                    fread(data, sizeof(unsigned char), size, f);
                    fclose(f);

                    auto elapsed = std::chrono::high_resolution_clock::now() - start;
                    long long loadTime = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();

                    /*for(int i = 0; i < size; i += 3)
                    {
                    // change from BGR TO RGB
                    unsigned char tmp = data[i];
                    data[i] = data[i+2];
                    data[i+2] = tmp;
                    }*/

                    //apllying gauss filter

                    auto startGauss = std::chrono::high_resolution_clock::now();
                    unsigned char *res = new unsigned char[size];
                    guassian_blur2D(data, res, width, height);
                    auto elapsedGauss = std::chrono::high_resolution_clock::now() - startGauss;
                    long long gaussTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedGauss).count();



                    //store

                    auto startStore = std::chrono::high_resolution_clock::now();
                    char str2[100] = "";
                    strcat(str2, argv[3]);
                    strcat(str2, "/");
                    strcat(str2, direntp->d_name);
                    FILE *f2 = fopen(str2, "wb");
                    fwrite(info, sizeof(unsigned char), 54, f2);


                    // read the rest of the data at once
                    fwrite(res, sizeof(unsigned char), size, f2);
                    fclose(f2);
                    auto elapsedStore = std::chrono::high_resolution_clock::now() - startStore;
                    long long storeTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedStore).count();
                    auto elapsedTotal = std::chrono::high_resolution_clock::now() - startTotal;
                    long long totalTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedTotal).count();
                    cout << "File: \"" << direntp->d_name << "\"" << "(time: " << totalTime << ")" << endl;
                    cout << "		Load time: " << loadTime << endl;
                    cout << "		Gauss time: " << gaussTime << endl;
                    cout << "		Store time: " << storeTime << endl;
                }
            }
        }

        else if (!option.compare("sobel")) {

            while ((direntp = readdir(dp)) != NULL) {

                //GAUSS

                auto startTotal = std::chrono::high_resolution_clock::now();
                string s = direntp->d_name;
                if (s.compare(".") && s.compare("..")) {
                    char str[100] = "";
                    strcat(str, argv[2]);
                    strcat(str, "/");
                    auto start = std::chrono::high_resolution_clock::now();
                    FILE *f = fopen(strcat(str, direntp->d_name), "rb");
                    unsigned char info[54];

                    // read the 54-byte header
                    fread(info, sizeof(unsigned char), 54, f);

                    // extract image height and width from header
                    int width = *(int *) &info[18];
                    int height = *(int *) &info[22];

                    // allocate 3 bytes per pixel
                    int size = 3 * width * height;
                    unsigned char *data = new unsigned char[size];

                    // read the rest of the data at once
                    fread(data, sizeof(unsigned char), size, f);
                    fclose(f);

                    auto elapsed = std::chrono::high_resolution_clock::now() - start;
                    long long loadTime = std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count();


                    //apllying gauss filter

                    auto startGauss = std::chrono::high_resolution_clock::now();
                    unsigned char *res = new unsigned char[size];
                    guassian_blur2D(data, res, width, height);
                    auto elapsedGauss = std::chrono::high_resolution_clock::now() - startGauss;
                    long long gaussTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedGauss).count();


                    //SOBEL

                    auto new_elapsed = std::chrono::high_resolution_clock::now() - start;

                    //applying sobel filter

                    auto startSobel = std::chrono::high_resolution_clock::now();
                    unsigned char *new_res = new unsigned char[size];
                    sobel_filter(res, new_res, width, height);
                    auto elapsedSobel = std::chrono::high_resolution_clock::now() - startSobel;
                    long long sobelTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedSobel).count();


                    //store

                    auto startStore = std::chrono::high_resolution_clock::now();
                    char str3[100] = "";
                    strcat(str3, argv[3]);
                    strcat(str3, "/");
                    strcat(str3, direntp->d_name);
                    FILE *f3 = fopen(str3, "wb");
                    fwrite(info, sizeof(unsigned char), 54, f3);


                    // read the rest of the data at once
                    fwrite(new_res, sizeof(unsigned char), size, f3);
                    fclose(f3);
                    auto elapsedStore = std::chrono::high_resolution_clock::now() - startStore;
                    long long storeTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedStore).count();
                    auto elapsedTotal = std::chrono::high_resolution_clock::now() - startTotal;
                    long long totalTime = std::chrono::duration_cast<std::chrono::microseconds>(
                            elapsedTotal).count();
                    cout << "File: \"" << direntp->d_name << "\"" << "(time: " << totalTime << ")" << endl;
                    cout << "   Load time:" << loadTime << endl;
                    cout << "   Gauss time: " << gaussTime << endl;
                    cout << "   Sobel time: " << sobelTime << endl;
                    cout << "   Store time: " << storeTime << endl;
                }
            }
        }
        else {
            cout << "Unexpected operation: " << option << endl;
            cout << "	image-seq operation in_path out_path" << endl;
            cout << "		operation: copy, gauss, sobel" << endl;

        }
    }
    else{
        cout << "Wrong format:" << endl;
        cout << "	image-seq operation in_path out_path" << endl;
        cout << "		operation: copy, gauss, sobel" << endl;
    }
}
