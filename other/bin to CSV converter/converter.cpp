#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>

using namespace std;
namespace fs = std::filesystem;

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef int int32_t;
typedef short int16_t;

int main(){
	string s = "\\input";
	s = fs::current_path().string() + s;
	for (const auto& inputEntry : fs::directory_iterator(s)){
		// if (inputEntry.path().extension() != ".bin") break;
		ifstream input(inputEntry.path());
		ofstream output;
		input.seekg(256);
		uint16_t throttleLeft[6400];
		uint16_t throttleRight[6400];
		int16_t accRaw[6400];
		double accConv[6400];
		double speed[6400];
		double dist[6400];
		uint8_t memblock[38400];
		input.read((char*)memblock, 38400);
		for (int i = 0; i < 6400; i++){
			throttleLeft[i] = ((int)memblock[1+2*i]) << 8 | (int)memblock[2*i];
			throttleRight[i] = ((int)memblock[12801+2*i]) << 8 | (int)memblock[12800+2*i];
			accRaw[i] = ((int)memblock[25601+2*i]) << 8 |(int) memblock[25600+2*i];
		}
		double cSpeed = 0;
		double cDist = 0;
		for (int i = 0; i < 6400; i++){
			accConv[i] = accRaw[i] * 9.81 / 16384;
			speed[i] = cSpeed + accConv[i] / 1600.0;
			dist[i] = cDist + speed[i] / 1600.0;
			cSpeed = speed[i];
			cDist = dist[i];
		}
		string str = fs::current_path().string() + "\\output\\" + inputEntry.path().filename().string() + ".csv";
		cout << str << endl;
		output.open(str);
		output << "Throttle L;Throttle R;Beschleunigung;Geschwindigkeit;Strecke\n";
		for (int i = 0; i < 6400; i++){
			output << throttleLeft[i] << ';' << throttleRight[i] << ';' << accConv[i] << ';' << speed[i] << ';' << dist[i] << '\n';
		}
		output.close();
	}
	return 0;
}