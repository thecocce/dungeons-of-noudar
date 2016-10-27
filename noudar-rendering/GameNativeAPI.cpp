#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <functional>
#include <memory>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <map>
#include <cstdint>
#include <string>
#include <tuple>
#include <utility>
#include <array>
#include <stdio.h>
#include <cmath>

#include "NativeBitmap.h"
#include "Texture.h"
#include "Material.h"
#include "Trig.h"
#include "TrigBatch.h"
#include "MeshObject.h"
#include "MaterialList.h"
#include "Scene.h"
#include "WavefrontOBJReader.h"
#include "Logger.h"

#include "AudioSink.h"
#include "SoundClip.h"
#include "SoundUtils.h"
#include "SoundListener.h"
#include "SoundEmitter.h"

#include "DungeonGLES2Renderer.h"
#include "LightningStrategy.h"
#include "GameNativeAPI.h"

#include "Vec2i.h"
#include "IMapElement.h"
#include "CActor.h"
#include "CGameDelegate.h"
#include "CMap.h"
#include "IRenderer.h"
#include "NativeBitmap.h"
#include "CFalconKnight.h"
#include "CBullKnight.h"
#include "CTurtleKnight.h"
#include "CGame.h"
#include "Common.h"
#include "NoudarGLES2Bridge.h"
#include "SplatAnimation.h"

std::shared_ptr<odb::DungeonGLES2Renderer> gles2Renderer = nullptr;
std::map<int, glm::vec2> mPositions;


bool hasActiveSplats;
odb::IntGameMap map;
odb::IntGameMap snapshot;
odb::IntGameMap splat;
odb::IntField ids;
odb::LightMap lightMap;
odb::AnimationList animationList;
long animationTime = 0;
bool hasCache = false;
odb::LightMap lightMapCache;

std::vector<std::shared_ptr<odb::NativeBitmap>> textures;

std::shared_ptr<Knights::CGame> game;
std::shared_ptr<odb::NoudarGLES2Bridge> render;


std::vector<std::shared_ptr<odb::SoundEmitter>> soundEmitters;
std::vector<std::shared_ptr<odb::SplatAnimation>> splatAnimation;
std::shared_ptr<odb::SoundListener> mainListener;

bool setupGraphics(int w, int h, std::string vertexShader, std::string fragmentShader, std::vector<std::shared_ptr<odb::NativeBitmap>> textureList ) {
	textures = textureList;

	gles2Renderer = std::make_shared<odb::DungeonGLES2Renderer>();

	gles2Renderer->setTexture(textures);
	animationTime = 0;

	hasCache = false;

	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			lightMapCache[ y ][ x ] = 0;
		}
	}

	return gles2Renderer->init(w, h, vertexShader.c_str(), fragmentShader.c_str());
}

void renderFrame(long delta) {
	if (gles2Renderer != nullptr && textures.size() > 0) {

		gles2Renderer->updateFadeState(delta);
		gles2Renderer->render(map, snapshot, splat, lightMap, ids, animationList, animationTime);
		gles2Renderer->updateCamera(delta);
	}
}

void shutdown() {
	gles2Renderer->shutdown();
	animationList.clear();
	mPositions.clear();
	animationTime = 0;
	textures.clear();
	hasCache = false;

	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			lightMapCache[y][x] = 0;
		}
	}

	gles2Renderer = nullptr;
}

void updateAnimations( long delta ) {
	auto it = animationList.begin();
	while (it != animationList.end()) {
		if (animationTime - (std::get<2>(it->second)) >= odb::kAnimationLength) {
			it = animationList.erase(it);
		} else {
			it = next(it);
		}
	}

	hasActiveSplats = splatAnimation.size() > 0;

	for ( auto splat : splatAnimation ) {
		odb::Logger::log( "updating animatings" );
		splat->update( delta );
	}

	splatAnimation.erase( std::remove_if( splatAnimation.begin(), splatAnimation.end(),
	                              [](std::shared_ptr<odb::SplatAnimation> splat){ return splat->isFinished();}
	), splatAnimation.end() );

	if ( hasActiveSplats ) {
		game->tick();
	}


	animationTime += delta;
}

void addCharacterMovement(int id, glm::vec2 previousPosition, glm::vec2 newPosition) {
	auto movement = std::make_tuple<>(previousPosition, newPosition, animationTime);

	if (animationList.count(id) > 0) {

		auto animation = animationList[id];
		auto prevPosition = std::get<0>(animation);
		animation = std::make_tuple<>(prevPosition, newPosition, animationTime);
	}

	animationList[id] = movement;

	char floorType = map[ newPosition.y ][ newPosition.x ];
	char prevFloorType = map[ newPosition.y ][ newPosition.x ];

	if ( floorType  == '.' || floorType == '-' ) {
		soundEmitters[0]->play(mainListener);
	} else if ( floorType == '_' || floorType == '=') {
		soundEmitters[1]->play(mainListener);
	} else {
		if ( floorType == '{' || floorType == '(' || floorType == ')' || floorType == '}' || floorType == '2' || floorType == '7' || '~' ) {
			soundEmitters[1]->play(mainListener);
		}
	}


}

void updateCharacterMovements(const int *idsLocal) {
	int position;

	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			position = (y * 20) + x;
			int id = idsLocal[position];
			ids[y][x] = id;
			if (id != 0) {
				auto previousPosition = mPositions[id];

				if (previousPosition != glm::vec2(x, y) ) {
					mPositions[id] = glm::vec2(x, y);

					if ( game->getTurn() > 0  ) {
						addCharacterMovement(id, previousPosition, mPositions[id]);
					}
				}
			}
		}
	}
}

void setCameraPosition( int x, int y ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setCameraPosition(x, y);
	}
}

void updateLevelSnapshot(const int *level, const int *actors, const int *splats) {
	hasActiveSplats = false;
	int position;

	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {
			position = (y * 20) + x;
			map[y][x] = (odb::ETextures) level[position];
			snapshot[y][x] = (odb::ETextures) actors[position];
			splat[ y ][ x ] = static_cast<odb::ETextures>( -1 );
			lightMap[y][x] = lightMapCache[y][x];
		}
	}

	for ( auto& splatAnim : splatAnimation ) {
		auto pos = splatAnim->getPosition();
		splat[pos.y][pos.x] = static_cast<odb::ETextures >( splatAnim->getSplatFrame());
	}

	for (int y = 0; y < 20; ++y) {
		for (int x = 0; x < 20; ++x) {

			if (map[y][x] == '|') {

				if (!hasCache) {
					odb::LightningStrategy::castLightInAllDirections(lightMapCache, 255, map, x, y);
					odb::LightningStrategy::castLightInAllDirections(lightMap, 255, map, x, y);
				}
			}
		}
	}

	hasCache = true;
}


void startFadingIn() {
	if (gles2Renderer != nullptr) {
		gles2Renderer->startFadingIn();
	}
}

void startFadingOut() {
	if (gles2Renderer != nullptr) {
		gles2Renderer->startFadingOut();
	}
}

bool isAnimating() {

	if (gles2Renderer != nullptr) {
		return gles2Renderer->isAnimating() && !hasActiveSplats;
	}

	return false;
}

void rotateCameraLeft() {

	if (gles2Renderer != nullptr&& !isAnimating() ) {
		gles2Renderer->rotateLeft();
		render->setNextCommand('i');
		game->tick();
		render->setNextCommand('.');
	}
}

void rotateCameraRight() {

	if (gles2Renderer != nullptr&& !isAnimating() ) {
		gles2Renderer->rotateRight();
		render->setNextCommand('p');
		game->tick();
		render->setNextCommand('.');
	}
}

void setFloorNumber( long floor ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setFloorNumber(floor);
	}
}

void onReleasedLongPressingMove() {
	if (gles2Renderer != nullptr) {
		gles2Renderer->onReleasedLongPressingMove();
	}
}

void onLongPressingMove() {
	if (gles2Renderer != nullptr) {
		gles2Renderer->onLongPressingMove();
	}
}

void setEyeViewMatrix( float* eyeView ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setEyeView(eyeView);
	}
}

void setPerspectiveMatrix( float* perspectiveMatrix ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setPerspectiveMatrix(perspectiveMatrix);
	}
}


void setAngleXZ( float XZAngle ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setAngleXZ(XZAngle);
	}
}

void setAngleYZ( float YZAngle ) {
	if (gles2Renderer != nullptr) {
		gles2Renderer->setAngleYZ(YZAngle);
	}
}

void loadMapData( std::string geoFile, std::string petFile ) {
}

void readMap( std::string data ) {
	render = std::make_shared<odb::NoudarGLES2Bridge>();

	std::string::size_type n = 0;
	while ( ( n = data.find( "\n", n ) ) != std::string::npos )
	{
		data.replace( n, 1, "" );
		++n;
	}

	auto onMonsterDead = [&]( Knights::Vec2i pos ) {
		soundEmitters[ 4 ]->play( mainListener );
	};

	auto onPlayerDead = [&](Knights::Vec2i pos) {
		soundEmitters[ 6 ]->play( mainListener );
	};

	auto onPlayerAttack = [&](Knights::Vec2i pos) {
		soundEmitters[ 7 ]->play( mainListener );
	};

	auto onMonsterAttack = [&](Knights::Vec2i pos) {

	};

	auto onMonsterDamaged = [&](Knights::Vec2i pos) {
		auto splat = std::make_shared<odb::SplatAnimation>( pos );
		splatAnimation.push_back( splat );
		splat->startSplatAnimation();

		soundEmitters[ 3 ]->play( mainListener );
	};

	auto onPlayerDamaged = [&](Knights::Vec2i pos) {
		soundEmitters[ 5 ]->play( mainListener );
	};

	auto gameDelegate = std::make_shared<Knights::CGameDelegate>();

	gameDelegate->setMonsterAttackedCallback( onMonsterAttack );
	gameDelegate->setMonsterDiedCallback( onMonsterDead );
	gameDelegate->setMonsterDamagedCallback(onMonsterDamaged);
	gameDelegate->setPlayerAttackedCallback( onPlayerAttack );
	gameDelegate->setPlayerDiedCallback( onPlayerDead );
	gameDelegate->setPlayerDamagedCallback( onPlayerDamaged );

	game = std::make_shared<Knights::CGame>( data, render, gameDelegate );
	setFloorNumber(0);
	if ( game != nullptr ) {
		game->tick();
	}
}

void moveUp() {

	if ( game != nullptr && !isAnimating() ) {
		render->setNextCommand('o');
		game->tick();
		render->setNextCommand('.');
	}
}

void moveDown() {

	if ( game != nullptr && !isAnimating() ) {
		render->setNextCommand('p');
		game->tick();
		render->setNextCommand('p');
		game->tick();
		render->setNextCommand('o');
		game->tick();
		render->setNextCommand('i');
		game->tick();
		render->setNextCommand('i');
		game->tick();
		render->setNextCommand('.');
	}
}

void moveLeft() {

	if ( game != nullptr && !isAnimating() ) {
		render->setNextCommand('i');
		game->tick();
		render->setNextCommand('o');
		game->tick();
		render->setNextCommand('p');
		game->tick();
		render->setNextCommand('.');
	}
}

void moveRight() {

	if ( game != nullptr && !isAnimating() ) {
		render->setNextCommand('p');
		game->tick();
		render->setNextCommand('o');
		game->tick();
		render->setNextCommand('i');
		game->tick();
		render->setNextCommand('.');
	}
}

void gameLoopTick( long ms ) {
    updateAnimations( ms );
}

void setSoundEmitters( std::vector<std::shared_ptr<odb::SoundEmitter>> emitters, std::shared_ptr<odb::SoundListener> listener ) {
	soundEmitters = emitters;
	mainListener = listener;
}

void forceDirection( int direction ) {
	char directions[] = { 'r', 'f', 'c', 'd'};
	render->setNextCommand( directions[ direction ] );
	game->tick();
	render->setNextCommand('.');

}