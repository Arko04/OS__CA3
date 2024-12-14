#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>

using namespace std;
using namespace std::chrono;

void readWavFile(const string& inputFile, vector<float>& data, SF_INFO& fileInfo) {
    auto start = high_resolution_clock::now();
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
    cout << "Successfully read " << numFrames << " frames from " << inputFile << endl;
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Read: " << duration.count() << " ms." << endl;
}

void apply_Bandpass_Filter(const vector<float>& data, vector<float>& bandpassFilterData, const float& df) {
    auto start = high_resolution_clock::now();
    for (float f : data) {
        float H = (f * f) / (f * f + df);
        bandpassFilterData.push_back(H);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Bandpass Filter: " << duration.count() << " ms." << endl;
}

void apply_Notch_Filter(const vector<float>& data, vector<float>& notchFilterData, const float& f0, const int& n) {
    auto start = high_resolution_clock::now();
    for (float f : data) {
        float H = 1 / (pow((f / f0), 2 * n) + 1);
        notchFilterData.push_back(H);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Notch Filter: " << duration.count() << " ms." << endl;
}

void apply_FIR_Filter(const vector<float>& data, vector<float>& firFilterData, const vector<float>& coefficients) {
    auto start = high_resolution_clock::now();
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
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "FIR Filter: " << duration.count() << " ms." << endl;
}

void apply_IIR_Filter(const vector<float>& data, vector<float>& iirFilterData, const vector<float>& b, const vector<float>& a) {
    auto start = high_resolution_clock::now();
    int M = b.size();
    int N = a.size();

    for (size_t n = 0; n < data.size(); ++n) {
        float output = 0.0;
        for (int k = 0; k < M; ++k) {
            if (n >= k) {
                output += b[k] * data[n - k];
            }
        }
        for (int j = 1; j < N; ++j) {
            if (n >= j) {
                output -= a[j] * iirFilterData[n - j];
            }
        }
        iirFilterData.push_back(output);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "IIR Filter: " << duration.count() << " ms." << endl;
}

void writeWavFile(const string& outputFile, const vector<float>& data, SF_INFO& fileInfo) {
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    if (!outFile) {
        cerr << "Error opening output file: " << sf_strerror(NULL) << endl;
        exit(1);
    }

    sf_count_t numFrames = sf_writef_float(outFile, data.data(), fileInfo.frames);
    if (numFrames != fileInfo.frames) {
        cerr << "Error writing frames to file." << endl;
        sf_close(outFile);
        exit(1);
    }

    sf_close(outFile);
    cout << "Successfully wrote " << numFrames << " frames to " << outputFile << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_wav_file>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    SF_INFO fileInfo;
    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));
    auto start = high_resolution_clock::now();

    string outputFile = "serial_output.wav";
    readWavFile(inputFile, audioData, fileInfo);
    writeWavFile(outputFile, audioData, fileInfo);

    vector<float> bandpassFilterData;
    const float df = 2;
    apply_Bandpass_Filter(audioData, bandpassFilterData, df);
    writeWavFile("serial_bandpass_filter_output.wav", bandpassFilterData, fileInfo);

    vector<float> notchFilterData;
    const float f0 = 2;
    const int n = 2;
    apply_Notch_Filter(audioData, notchFilterData, f0, n);
    writeWavFile("serial_notch_filter_output.wav", notchFilterData, fileInfo);

    vector<float> firFilterData;
    vector<float> firCoefficients = {0.2, 0.3, 0.5};
    apply_FIR_Filter(audioData, firFilterData, firCoefficients);
    writeWavFile("serial_fir_filter_output.wav", firFilterData, fileInfo);

    vector<float> iirFilterData;
    vector<float> iirFeedforward = {0.5, 0.2};
    vector<float> iirFeedback = {1.0, -0.5};
    apply_IIR_Filter(audioData, iirFilterData, iirFeedforward, iirFeedback);
    writeWavFile("serial_iir_filter_output.wav", iirFilterData, fileInfo);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Execution: " << duration.count() << " ms." << endl;

    return 0;
}
