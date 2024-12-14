#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>

using namespace std;
using namespace std::chrono;
SF_INFO fileInfo;
std::vector<float> generateRandomNumbers(float a, float b, float step, int count) {
    std::vector<float> randomNumbers;
    std::srand(static_cast<unsigned int>(std::time(0)));
    for (int i = 0; i < count; ++i) {
        float randomNumber = a + (std::rand() % static_cast<int>((b - a) / step + 1)) * step;
        randomNumbers.push_back(randomNumber);
    }
    
    return randomNumbers;
}

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

void apply_Bandpass_Filter(const vector<float>& data, vector<float>& bandpassFilterData) {
    float up = 1e8;
    float down = 0;
    auto start = high_resolution_clock::now();
    const float df = 1;
    for (int i = 0; i < data.size(); i++) {
        float H;
        float f = (i * fileInfo.samplerate) / data.size();
        if (f <= up && f >= down)
        {
            H = (f * f) / (f * f + pow(df, 2));
        }
        else
        {
            H = 0;
        }
        bandpassFilterData.push_back(H * data[i]);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Bandpass Filter: " << duration.count() << " ms." << endl;
}

void apply_Notch_Filter(const vector<float>& data, vector<float>& notchFilterData) {
    auto start = high_resolution_clock::now();
    const float f0 = 50;
    const int n = 1;
    for (int i = 0; i < data.size(); i++) {
        float f = (i * fileInfo.samplerate) / data.size();
        float H = 1 / (pow((f / f0), 2 * n) + 1);
        notchFilterData.push_back(H * data[i]);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Notch Filter: " << duration.count() << " ms." << endl;
}


void apply_FIR_Filter(const vector<float>& data, vector<float>& firFilterData) {
    auto start = high_resolution_clock::now();
    vector<float> coefficients = generateRandomNumbers(0.1, 10, 0.1, 100);
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

void apply_IIR_Filter(const vector<float>& data, vector<float>& iirFilterData) {
    auto start = high_resolution_clock::now();
    vector<float> iirFeedforward = generateRandomNumbers(0.1, 1, 0.1, 100);
    vector<float> iirFeedback = generateRandomNumbers(-1, 1, 0.1, 100);
    int M = iirFeedforward.size();
    int N = iirFeedback.size();

    for (size_t n = 0; n < data.size(); ++n) {
        float output = 0.0;
        for (int k = 0; k < M; ++k) {
            if (n >= k) {
                output += iirFeedforward[k] * data[n - k];
            }
        }
        for (int j = 1; j < N; ++j) {
            if (n >= j) {
                output -= iirFeedback[j] * iirFilterData[n - j];
            }
        }
        iirFilterData.push_back(output);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "IIR Filter: " << duration.count() << " ms." << endl;
}

void writeWavFile(const string& outputFile, const vector<float>& data, SF_INFO& fileInfo) {
    const int originalFrames = fileInfo.frames;
    SNDFILE* outFile = sf_open(outputFile.c_str(), SFM_WRITE, &fileInfo);
    fileInfo.frames = originalFrames;
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
    // cout << "Successfully wrote " << numFrames << " frames to " << outputFile << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_wav_file>" << endl;
        return 1;
    }

    string inputFile = argv[1];
    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));
    auto start = high_resolution_clock::now();

    string outputFile = "serial_output.wav";
    readWavFile(inputFile, audioData, fileInfo);
    writeWavFile(outputFile, audioData, fileInfo);

    vector<float> bandpassFilterData;
    apply_Bandpass_Filter(audioData, bandpassFilterData);
    writeWavFile("serial_bandpass_filter_output.wav", bandpassFilterData, fileInfo);

    vector<float> notchFilterData;
    apply_Notch_Filter(audioData, notchFilterData);
    writeWavFile("serial_notch_filter_output.wav", notchFilterData, fileInfo);

    vector<float> firFilterData;
    apply_FIR_Filter(audioData, firFilterData);
    writeWavFile("serial_fir_filter_output.wav", firFilterData, fileInfo);

    vector<float> iirFilterData;
    apply_IIR_Filter(audioData, iirFilterData);
    writeWavFile("serial_iir_filter_output.wav", iirFilterData, fileInfo);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Execution: " << duration.count() << " ms." << endl;

    return 0;
}
