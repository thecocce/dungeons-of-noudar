#include <string>
#include <map>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLES2/gl2.h>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <map>
#include <array>

#include "SoundClip.h"
#include "SoundUtils.h"
#include "SoundListener.h"
#include "SoundEmitter.h"

#include "IFileLoaderDelegate.h"
#include "CPlainFileLoader.h"

#include "Vec2i.h"
#include "IMapElement.h"
#include "CTeam.h"

#include "CActor.h"
#include "CGameDelegate.h"
#include "CMap.h"

#include "NativeBitmap.h"
#include "Common.h"
#include "NoudarDungeonSnapshot.h"
#include "GameNativeAPI.h"
#include "WindowOperations.h"

int main(int argc, char *argv[]) {
	readMap( std::make_shared<Knights::CPlainFileLoader>("res/"), "tiles.properties" );
	initWindow();
    setMainLoop();
	destroyWindow();
	return 0;
}
