#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <random>
#include <string>

using namespace std;

int main() {
    const int NUM_RECORDS = 10000000; // 10 million records
    cout << "Generating " << NUM_RECORDS << " random records. Please wait..." << endl;

    // Generate sequential IDs
    vector<int> ids(NUM_RECORDS);
    for (int i = 0; i < NUM_RECORDS; ++i) {
        ids[i] = i + 1; // IDs from 1 to 10,000,000
    }

    // Shuffle IDs randomly (Fisher-Yates Shuffle)
    // Forces the B+ Tree to split randomly instead of only right-leaning splits
    random_device rd;
    mt19937 g(rd());
    shuffle(ids.begin(), ids.end(), g);

    // Open CSV file for writing
    ofstream outFile("dataset_large.csv");
    if (!outFile.is_open()) {
        cerr << "Error: Could not create dataset_large.csv!" << endl;
        return 1;
    }

    outFile << "ID,Payload\n"; // CSV Header

    for (int i = 0; i < NUM_RECORDS; ++i) {
        // Generate placeholder payload to fill up the length
        string payload = "DummyData_Payload_For_ID_" + to_string(ids[i]);
        
        // Pad with character 'X' to make payload length exactly 59 characters (+1 null terminator is 60 bytes)
        if (payload.length() < 59) {
            payload.append(59 - payload.length(), 'X'); 
        }
        
        outFile << ids[i] << "," << payload << "\n";
    }

    outFile.close();
    cout << "Successfully created 'dataset_large.csv' (Size: ~700MB)!" << endl;
    return 0;
}
