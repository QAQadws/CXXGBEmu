#include"platform.h"
using namespace std;

int main(int argc, char* argv[]) {
    PLATFORM platform(argc, argv);
    platform.init();
    platform.run();
    platform.clean();
    return 0;
}