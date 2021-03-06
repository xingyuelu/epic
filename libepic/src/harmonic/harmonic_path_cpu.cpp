/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2015 Kyle Hollins Wray, University of Massachusetts
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *  the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <epic/harmonic/harmonic.h>
#include <epic/harmonic/harmonic_path_cpu.h>
#include <epic/error_codes.h>
#include <epic/constants.h>

#include <iostream>
#include <stdio.h>
#include <math.h>
#include <cmath>
#include <vector>


namespace epic {

#define PATH_STUCK_HISTORY_LENGTH 5

int harmonic_compute_potential_2d_cpu(Harmonic *harmonic, float x, float y, float &potential)
{
    if (harmonic == nullptr || harmonic->m == nullptr ||
            harmonic->u == nullptr || harmonic->locked == nullptr) {
        fprintf(stderr, "Error[harmonic_compute_potential_2d_cpu]: %s\n", "Invalid data.");
        return EPIC_ERROR_INVALID_DATA;
    }

    unsigned int xCellIndex = (unsigned int)(x + 0.5f);
    unsigned int yCellIndex = (unsigned int)(y + 0.5f);

    if (xCellIndex < 0.0f || yCellIndex < 0.0f ||
            xCellIndex >= harmonic->m[1] || yCellIndex >= harmonic->m[0] ||
            (harmonic->locked[yCellIndex * harmonic->m[1] + xCellIndex] == 1 &&
            harmonic->u[yCellIndex * harmonic->m[1] + xCellIndex] < 0.0f)) {
        fprintf(stderr, "Error[harmonic_compute_potential_2d_cpu]: %s\n", "Invalid location.");
        return EPIC_ERROR_INVALID_LOCATION;
    }

    unsigned int xtl = (unsigned int)(x - 0.5f);
    unsigned int ytl = (unsigned int)(y - 0.5f);

    unsigned int xtr = (unsigned int)(x + 0.5f);
    unsigned int ytr = (unsigned int)(y - 0.5f);

    unsigned int xbl = (unsigned int)(x - 0.5f);
    unsigned int ybl = (unsigned int)(y + 0.5f);

    unsigned int xbr = (unsigned int)(x + 0.5f);
    unsigned int ybr = (unsigned int)(y + 0.5f);

    float alpha = (x - xtl);
    float beta = (y - ytl);

    float one = (1.0f - alpha) * harmonic->u[ytl * harmonic->m[1] + xtl] +
                alpha * harmonic->u[ytr * harmonic->m[1] + xtr];
    float two = (1.0f - alpha) * harmonic->u[ybl * harmonic->m[1] + xbl] +
                alpha * harmonic->u[ybr * harmonic->m[1] + xbr];
    potential = (1.0f - beta) * one + beta * two;

    return EPIC_SUCCESS;
}


int harmonic_compute_gradient_2d_cpu(Harmonic *harmonic, float x, float y, float cdPrecision,
        float &partialX, float &partialY)
{
    if (harmonic == nullptr || harmonic->m == nullptr ||
            harmonic->u == nullptr || harmonic->locked == nullptr) {
        fprintf(stderr, "Error[harmonic_compute_gradient_2d_cpu]: %s\n", "Invalid data.");
        return EPIC_ERROR_INVALID_DATA;
    }

    float value0 = 0.0f;
    float value1 = 0.0f;
    float value2 = 0.0f;
    float value3 = 0.0f;

    int result = harmonic_compute_potential_2d_cpu(harmonic, x - cdPrecision, y, value0);
    result += harmonic_compute_potential_2d_cpu(harmonic, x + cdPrecision, y, value1);
    result += harmonic_compute_potential_2d_cpu(harmonic, x, y - cdPrecision, value2);
    result += harmonic_compute_potential_2d_cpu(harmonic, x, y + cdPrecision, value3);

    if (result != EPIC_SUCCESS) {
        fprintf(stderr, "Error[harmonic_compute_gradient_2d_cpu]: %s\n",
                        "Failed to compute potential values.");
        return EPIC_ERROR_INVALID_GRADIENT;
    }

    partialX = (value1 - value0) / (2.0f * cdPrecision);
    partialY = (value3 - value2) / (2.0f * cdPrecision);

    float denom = std::sqrt(std::pow(partialX, 2) + std::pow(partialY, 2));
    partialX /= denom;
    partialY /= denom;

    return EPIC_SUCCESS;
}


bool harmonic_is_path_stuck_cpu(const std::vector<float> &pathVector, float stepSize)
{
    unsigned int n = pathVector.size();

    // There is an error (it is stuck or something) if the path vector is odd.
    if (n % 2 == 1) {
        return true;
    }

    // Empty paths are fine.
    if (n == 0) {
        return false;
    }

    float x = pathVector[n - 2];
    float y = pathVector[n - 1];

    // If the path reached a point where it backtracked to a recently visited region, then there is a problem.
    for (unsigned int i = n - 2; i > std::max(0, (int)n - 2 * PATH_STUCK_HISTORY_LENGTH - 2); i -= 2) {
        float xi = pathVector[i - 2];
        float yi = pathVector[i - 1];

        float distance = std::sqrt(std::pow(x - xi, 2) + std::pow(y - yi, 2));

        if (distance < stepSize / 2.0f) {
            return true;
        }
    }

    return false;
}


int harmonic_compute_path_2d_cpu(Harmonic *harmonic, float x, float y,
        float stepSize, float cdPrecision, unsigned int maxLength,
        unsigned int &k, float *&path)
{
    if (harmonic == nullptr || harmonic->m == nullptr ||
            harmonic->u == nullptr || harmonic->locked == nullptr ||
            path != nullptr) {
        fprintf(stderr, "Error[harmonic_compute_path_2d_cpu]: %s\n", "Invalid data.");
        return EPIC_ERROR_INVALID_DATA;
    }

    unsigned int xCellIndex = (unsigned int)(x + 0.5f);
    unsigned int yCellIndex = (unsigned int)(y + 0.5f);

    if (xCellIndex < 0.0f || yCellIndex < 0.0f ||
            xCellIndex >= harmonic->m[1] || yCellIndex >= harmonic->m[0] ||
            (harmonic->locked[yCellIndex * harmonic->m[1] + xCellIndex] == 1 &&
            harmonic->u[yCellIndex * harmonic->m[1] + xCellIndex] < 0.0f)) {
        fprintf(stderr, "Error[harmonic_compute_path_2d_cpu]: %s\n", "Invalid location.");
        return EPIC_ERROR_INVALID_LOCATION;
    }

    std::vector<float> pathVector;

    pathVector.push_back(x);
    pathVector.push_back(y);
    //pathVector.push_back(theta);

    while (harmonic->locked[yCellIndex * harmonic->m[1] + xCellIndex] != 1 &&
            harmonic_is_path_stuck_cpu(pathVector, stepSize) == false &&
            pathVector.size() < 2 * maxLength) {
        float partialX = 0.0f;
        float partialY = 0.0f;

        int result = harmonic_compute_gradient_2d_cpu(harmonic, x, y, cdPrecision, partialX, partialY);
        if (result != EPIC_SUCCESS) {
            fprintf(stderr, "Error[harmonic_compute_path_2d_cpu]: %s\n", "Could not compute gradient.");
            return EPIC_ERROR_INVALID_GRADIENT;
        }

        x += partialX * stepSize;
        y += partialY * stepSize;

        pathVector.push_back(x);
        pathVector.push_back(y);

        //float theta = std::atan2(partialY, partialX);
        //pathVector.push_back(theta);

        xCellIndex = (unsigned int)(x + 0.5f);
        yCellIndex = (unsigned int)(y + 0.5f);
    }

    // If there are only two points, then there's a 99.999% the gradient was invalid and it
    // is not done relaxing enough. Thus, return a special error.
    if (pathVector.size() / 2 <= 2) {
        fprintf(stderr, "Error[harmonic_compute_path_2d_cpu]: %s\n", "Could not compute a valid path.");
        return EPIC_ERROR_INVALID_PATH;
    }

    k = pathVector.size() / 2;
    path = new float[2 * k];
    for (unsigned int i = 0; i < 2 * k; i++) {
        path[i] = pathVector[i];
    }

    return EPIC_SUCCESS;
}


int harmonic_free_path_cpu(float *&path)
{
    if (path != nullptr) {
        delete [] path;
    }
    path = nullptr;

    return EPIC_SUCCESS;
}

}; // namespace epic

