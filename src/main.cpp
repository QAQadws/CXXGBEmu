#include"cart.h"

using namespace std;

int main(int argc, char* argv[]) {
    
    //CART cart("dmg-acid2.gb");
    if(argc>=2){
        CART cart(argv[1]);
    }
    return 0;
}