#include "Problem.h"


using namespace std;


namespace szx {

#pragma region Problem::Input
void Problem::Input::load(const String &batchFilePath, const String &defectsFilePath) {
    // TODO[szx][1]: load input.
    // EXTEND[szx][8]: check file existence first.
}
#pragma endregion Problem::Input

#pragma region Problem::Output
void Problem::Output::load(const String &filePath) {
    // EXTEND[szx][4]: load output.
}

void Problem::Output::save(const String &filePath) const {
    // TODO[szx][2]: save output.
}
#pragma endregion Problem::Output

}
