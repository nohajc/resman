#include <iostream>
#include <string>

extern "C" {
	extern char binaryString[];
	extern uint32_t binaryString_size;
}

int main() {
	std::string str(binaryString, binaryString_size);
	std::cout << str << '\n';
	system("pause");
}
