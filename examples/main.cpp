#include "resdefs.h"

#include <iostream>
#include <string>

using namespace resman;

void printRes(ResourceHandle hnd) {
	std::cout << hnd.id() << ": " << std::string(hnd.begin(), hnd.end()) << '\n';
	std::cout << "Resource size: " << hnd.size() << '\n';
	for (char c : hnd) {
		std::cout << c << ' ';
	}
	std::cout << '\n';
}


int main() {
	printRes(gResFoo);
	printRes(gResBar);

#ifdef _WIN32
	system("pause");
#endif
	return 0;
}
