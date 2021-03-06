//
// Created by monty on 10/10/16.
//
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "glm/glm.hpp"
#include <memory>

#ifdef __APPLE__
#if TARGET_IOS
#import <OpenAl/al.h>
#import <OpenAl/alc.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#endif
#else

#include <AL/al.h>
#include <AL/alc.h>

#endif

#include <unistd.h>

#include "SoundClip.h"
#include "SoundListener.h"
#include "SoundEmitter.h"

namespace odb {
    void playSource(void* arg) {
        ALuint source = static_cast<ALuint>(reinterpret_cast<intptr_t>(arg));
        alSourcePlay(source);
    }


    SoundEmitter::SoundEmitter( std::shared_ptr<SoundClip> aSample ) : mSample(aSample) {
        ALuint sources[1];
        alGenSources(1, sources);
        mEmitterHandle = sources[ 0 ];
        alSourcei(mEmitterHandle, AL_BUFFER, aSample->mBufferHandle );
    }

    void SoundEmitter::play(std::shared_ptr<SoundListener> listener) {
        ALfloat listenerPos[] = {0.0, 0.0, 0.0};
        ALfloat listenerVel[] = {0.0, 0.0, 0.0};
        ALfloat listenerOri[] = {0.0, 0.0, -1.0, 0.0, 1.0, 0.0};

        alListenerfv(AL_POSITION, listenerPos);
        alListenerfv(AL_VELOCITY, listenerVel);
        alListenerfv(AL_ORIENTATION, listenerOri);
        alListenerf(AL_GAIN, mVolume);



#ifdef __EMSCRIPTEN__
        emscripten_async_call(playSource, reinterpret_cast<void*>(mEmitterHandle), 700);
#else
        playSource(reinterpret_cast<void*>(mEmitterHandle));
#endif

        ALint state;
        alGetSourcei(mEmitterHandle, AL_SOURCE_STATE, &state);



    }
}

