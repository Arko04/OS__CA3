#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>

using namespace std;


void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* inFile = sf_open(inputFile.c_str(), SFM_READ, &fileInfo);
    if (!inFile) {
        cerr << "Error opening input file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    data.resize(fileInfo.frames * fileInfo.channels);
    sf_count_t numFrames = sf_readf_float(inFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        cerr << "Error reading frames from file." << endl;
        sf_close(inFile);
        exit(1);
    }

    sf_close(inFile);
    std::cout << "Successfully read " << numFrames << " frames from " << inputFile << std::endl;
}

void apply_Bandpass_Filter(const vector<float>& data, vector<float>& bandpassFilterData, const float& df) {
    for (float f : data) {
        float H = (f * f) / (f * f + df);
        bandpassFilterData.push_back(H);
    }
}

void apply_Notch_Filter(const vector<float>& data, vector<float>& notchFilterData, const float &f0, const int &n) {
    for (float f : data) {
        float H = 1 / (pow((f / f0), 2 * n) + 1);
        notchFilterData.push_back(H);
    }
}

void apply_FIR_Filter(const vector<float>& data, vector<float>& firFilterData, const vector<float>& coefficients) {
    int M = coefficients.size();
    for (size_t n = 0; n < data.size(); ++n) {
        float output = 0.0;
        for (int k = 0; k < M; ++k) {
            if (n >= k) {
                output += coefficients[k] * data[n - k];
            }
        }
        firFilterData.push_back(output);
    }
}

void apply_IIR_Filter(const vector<float>& data, vector<float>& iirFilterData, const vector<float>& b, const vector<float>& a) {
    int M = b.size();  // Feedforward coefficients length
    int N = a.size();  // Feedback coefficients length

    for (size_t n = 0; n < data.size(); ++n) {
        float output = 0.0;
        // Apply feedforward coefficients
        for (int k = 0; k < M; ++k) {
            if (n >= k) {
                output += b[k] * data[n - k];
            }
        }
        // Apply feedback coefficients
        for (int j = 1; j < N; ++j) {
            if (n >= j) {
                output -= a[j] * iirFilterData[n - j];
            }
        }

        iirFilterData.push_back(output);
    }
}

void writeWavFile(const std::string& outputFile, const std::vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        std::cerr << "Error opening output file: " << sf_strerror(NULL) << std::endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        std::cerr << "Error writing frames to file." << std::endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    std::cout << "Successfully wrote " << numFrames << " frames to " << outputFile << std::endl;
}

int main(int argc, char *argv[])
{
    string inputFile = argv[1];
    SF_INFO fileInfo;
    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));
    readWavFile(inputFile, audioData, fileInfo);

    vector<float> bandpassFilterData;
    const float df = 2;
    apply_Bandpass_Filter(audioData, bandpassFilterData, df);

    vector<float> notchFilterData;
    const float f0 = 2;
    const int n = 2;
    apply_Notch_Filter(audioData, notchFilterData, f0, n);

    vector<float> firFilterData;
    vector<float> firCoefficients = {0.2, 0.3, 0.5}; 
    apply_FIR_Filter(audioData, firFilterData, firCoefficients);        

    vector<float> iirFilterData;
    vector<float> iirFeedforward = {0.5, 0.2};
    vector<float> iirFeedback = {1.0, -0.5};
    apply_IIR_Filter(audioData, iirFilterData, iirFeedforward, iirFeedback);
}