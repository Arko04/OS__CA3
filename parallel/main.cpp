#include <iostream>
#include <sndfile.h>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include <thread>

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
vector<float> coefficients = generateRandomNumbers(0.1, 10, 0.1, 100);
vector<float> iirFeedforward = generateRandomNumbers(0.9, 1.1, 0.1, 100);
vector<float> iirFeedback = generateRandomNumbers(0.9, 1.1, 0.1, 100);
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
    const float df = 0.2;
    for (int i = 0; i < data.size(); i++) {
        float H;
        float f = data[i];
        if (f <= up && f >= down)
        {
            H = (f * f) / (f * f + pow(df, 2));
        }
        else
        {
            H = 0;
        }
        bandpassFilterData.push_back(H * f);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
}

void apply_Notch_Filter(const vector<float>& data, vector<float>& notchFilterData) {
    auto start = high_resolution_clock::now();
    const float f0 = 50;
    const int n = 1;
    for (int i = 0; i < data.size(); i++) {
        float f = data[i];
        float H = 1 / (pow((f / f0), 2 * n) + 1);
        notchFilterData.push_back(H * f);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
}

void apply_FIR_Filter(const vector<float>& data, vector<float>& firFilterData) {
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
}

// Feedforward (FIR-like) processing
void apply_Feedforward(const vector<float>& data, vector<float>& feedforwardOutput) {

   int M = iirFeedforward.size();
    for (size_t n = 0; n < data.size(); ++n) {
        float output = 0.0;
        for (int k = 0; k < M; ++k) {
            if (n >= k) {
                output += iirFeedforward[k] * data[n - k];
            }
        }
        feedforwardOutput.push_back(output);
    }
}

// Feedback processing and final IIR output
void apply_Feedback(const vector<float>& feedforwardOutput, vector<float>& iirFilterData) {

    int N = iirFeedback.size();
    for (size_t n = 0; n < feedforwardOutput.size(); ++n) {
        float output = feedforwardOutput[n];
        for (int j = 1; j < N; ++j) {
            if (n >= j) {
                output -= iirFeedback[j] * iirFilterData[n - j];
            }
        }
        iirFilterData.push_back(output);
    }
}


int processWithThreads(int numThreads, const vector<float>& data, void (*filterFunc)(const vector<float>&, vector<float>&), vector<float>& result) {

    int chunkSize = data.size() / numThreads;
    vector<vector<float>> dataChunks(numThreads);
    vector<thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        int startIdx = i * chunkSize;
        int endIdx = (i == numThreads - 1) ? data.size() : (i + 1) * chunkSize;
        dataChunks[i] = vector<float>(data.begin() + startIdx, data.begin() + endIdx);
    }
    vector<vector<float>> filterResults(numThreads);

    auto overallStart = high_resolution_clock::now();

    for (int i = 0; i < numThreads; ++i) {
        threads.push_back(thread(filterFunc, ref(dataChunks[i]), ref(filterResults[i])));
    }
    for (auto& t : threads) {
        t.join();
    }
    auto overallEnd = high_resolution_clock::now();
    auto overallDuration = duration_cast<milliseconds>(overallEnd - overallStart);
    for (int i = 0; i < numThreads; ++i) {
        result.insert(result.end(), filterResults[i].begin(), filterResults[i].end());
    }
    return overallDuration.count();
}


// Main function to apply the IIR filter
void apply_IIR_Filter(const vector<float>& data, vector<float>& iirFilterData) {
    auto start = high_resolution_clock::now();

    // Generate random coefficients

    vector<float> feedforwardOutput;

    // Step 1: Parallelized Feedforward processing
    int numThreads = thread::hardware_concurrency(); // Optimal thread count
    // cout << "Using " << numThreads << " threads for Feedforward processing." << endl;

    processWithThreads(numThreads, data, apply_Feedforward, feedforwardOutput);

    // Step 2: Sequential Feedback processing
    apply_Feedback(feedforwardOutput, iirFilterData);

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "IIR filter with " << numThreads << " threads: " <<duration.count() << " ms." << endl;
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
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <input_wav_file>" << endl;
        return 1;
    }

    string inputFile = argv[1];

    vector<float> audioData;

    memset(&fileInfo, 0, sizeof(fileInfo));

    readWavFile(inputFile, audioData, fileInfo);
    writeWavFile("parallel_output.wav", audioData, fileInfo);

    vector<int> threadCounts;
    for (int i = 1; i <= 32; ++i) {
        threadCounts.push_back(i);
    }

    int num_threads_1 = -1;
    int lowest_overall_duration_1 = 1e9;
    for (int threads : threadCounts) {
        vector<float> bandpassFilterData;
        int overall_duration = processWithThreads(threads, audioData, apply_Bandpass_Filter, bandpassFilterData);
        if(lowest_overall_duration_1 > overall_duration)
        {
            num_threads_1 = threads;
            lowest_overall_duration_1 = overall_duration;
        }
    }


    int num_threads_2 = -1;
    int lowest_overall_duration_2 = 1e9;
    for (int threads : threadCounts) {
        vector<float> notchFilterData;
        int overall_duration = processWithThreads(threads, audioData, apply_Notch_Filter, notchFilterData);
        if(lowest_overall_duration_2 > overall_duration)
        {
            num_threads_2 = threads;
            lowest_overall_duration_2 = overall_duration;
        }
    }


    int num_threads_3 = -1;
    int lowest_overall_duration_3 = 1e9;
    for (int threads : threadCounts) {
        vector<float> firFilterData;
        int overall_duration = processWithThreads(threads, audioData, apply_FIR_Filter, firFilterData);
        if(lowest_overall_duration_3 > overall_duration)
        {
            num_threads_3 = threads;
            lowest_overall_duration_3 = overall_duration;
        }
    }


    // num_threads = -1;
    // lowest_overall_duration = 1e9;
    // for (int threads : threadCounts) {
    //     vector<float> iirFilterData;
    //     int overall_duration = processWithThreads(threads, audioData, apply_IIR_Filter, iirFilterData);
    //     if(lowest_overall_duration > overall_duration)
    //     {
    //         num_threads = threads;
    //         lowest_overall_duration = overall_duration;
    //     }
    // }
    auto start = high_resolution_clock::now();
    vector<float> bandpassFilterData;
    int overall_duration = processWithThreads(num_threads_1, audioData, apply_Bandpass_Filter, bandpassFilterData);
    writeWavFile("parallel_bandpass_filter_output.wav", bandpassFilterData, fileInfo);
    cout << "Bandpass Filter with " << num_threads_1 << " threads: "<<lowest_overall_duration_1 <<" ms. "<<endl;



    vector<float> notchFilterData;
    overall_duration = processWithThreads(num_threads_2, audioData, apply_Notch_Filter, notchFilterData);
    writeWavFile("parallel_notch_filter_output.wav", notchFilterData, fileInfo);
    cout << "Notch Filter with " << num_threads_2 << " threads: "<<lowest_overall_duration_2 <<" ms. "<<endl;


    vector<float> firFilterData;
    overall_duration = processWithThreads(num_threads_3, audioData, apply_FIR_Filter, firFilterData);
    writeWavFile("parallel_fir_filter_output.wav", firFilterData, fileInfo);
    cout << "FIR Filter with " << num_threads_3 << " threads: "<<lowest_overall_duration_3 << " ms. "<<endl;



    vector<float> iirFilterData;
    apply_IIR_Filter(audioData, iirFilterData);
    // overall_duration = processWithThreads(num_threads, audioData, apply_IIR_Filter, iirFilterData);
    writeWavFile("parallel_iir_filter_output.wav", iirFilterData, fileInfo);
    // cout << "IIR Filter with " << num_threads << " threads: "<<lowest_overall_duration << " ms. " <<endl;

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);
    cout << "Execution: " << duration.count() << " ms." << endl;

    return 0;
}
