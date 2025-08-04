#include"platform.h"
using namespace std;

int main(int argc, char* argv[]) {
    PLATFORM platform;
    platform.init(argc, argv);
    platform.run();
    return 0;
}