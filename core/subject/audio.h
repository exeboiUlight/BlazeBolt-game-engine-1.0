#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <AL/efx.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <utils/dr_mp3.h>

class Audio {
public:
    struct SoundSource {
        ALuint source;
        ALuint buffer;
        std::string name;
        bool looping;
        float volume;
        float pitch;
        float position[3];
        float velocity[3];
        float rolloff;
        float referenceDistance;
        float maxDistance;
        bool spatial;
        
        SoundSource() : source(0), buffer(0), looping(false), volume(1.0f), pitch(1.0f),
            rolloff(1.0f), referenceDistance(1.0f), maxDistance(100.0f), spatial(false) {
            position[0] = position[1] = position[2] = 0.0f;
            velocity[0] = velocity[1] = velocity[2] = 0.0f;
        }
    };

    Audio() : device(nullptr), context(nullptr), initialized(false),
        efxSupported(false),         alGenEffects(nullptr), alDeleteEffects(nullptr),
        alEffecti(nullptr), alEffectf(nullptr),
        alGenFilters(nullptr), alDeleteFilters(nullptr),
        alFilteri(nullptr), alFilterf(nullptr),
        alGenAuxiliaryEffectSlots(nullptr), alDeleteAuxiliaryEffectSlots(nullptr),
        alAuxiliaryEffectSloti(nullptr), alAuxiliaryEffectSlotf(nullptr),
        alSourcei(nullptr) {}
    
    ~Audio() {
        shutdown();
    }

    bool init() {
        if (initialized) return true;

        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "Failed to open audio device" << std::endl;
            return false;
        }

        context = alcCreateContext(device, nullptr);
        if (!context) {
            std::cerr << "Failed to create audio context" << std::endl;
            alcCloseDevice(device);
            device = nullptr;
            return false;
        }

        if (!alcMakeContextCurrent(context)) {
            std::cerr << "Failed to make context current" << std::endl;
            alcDestroyContext(context);
            alcCloseDevice(device);
            context = nullptr;
            device = nullptr;
            return false;
        }

        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "OpenAL error: " << error << std::endl;
            shutdown();
            return false;
        }

        loadEfxFunctions();

        initialized = true;
        std::cout << "Audio initialized" << std::endl;
        return true;
    }

    void shutdown() {
        if (!initialized) return;

        stopAll();

        for (auto& effect : effects) {
            if (effect.effect != 0) alDeleteEffects(1, &effect.effect);
        }
        effects.clear();

        for (auto& filter : filters) {
            if (filter.filter != 0) alDeleteFilters(1, &filter.filter);
        }
        filters.clear();

        for (auto& slot : effectSlots) {
            if (slot.slot != 0) alDeleteAuxiliaryEffectSlots(1, &slot.slot);
        }
        effectSlots.clear();

        for (auto& source : sources) {
            if (source.source != 0) alDeleteSources(1, &source.source);
        }
        sources.clear();

        for (auto& pair : soundBuffers) {
            if (pair.second != 0) alDeleteBuffers(1, &pair.second);
        }
        soundBuffers.clear();

        alcMakeContextCurrent(nullptr);
        if (context) {
            alcDestroyContext(context);
            context = nullptr;
        }
        if (device) {
            alcCloseDevice(device);
            device = nullptr;
        }

        initialized = false;
    }

    // --- Sound loading & playback ---

    int loadSound(const std::string& filename, const std::string& soundName, bool loop = false) {
        std::lock_guard<std::mutex> lock(mutex);
        
        auto it = soundBuffers.find(soundName);
        if (it != soundBuffers.end()) {
            std::cerr << "Sound already loaded: " << soundName << std::endl;
            return -1;
        }

        ALuint buffer = 0;
        alGenBuffers(1, &buffer);
        
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "Failed to generate buffer for: " << filename << std::endl;
            return -1;
        }

        bool loaded = false;
        std::string ext = filename.substr(filename.find_last_of('.') + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == "mp3") {
            loaded = loadMp3File(filename, buffer);
        } else {
            loaded = loadWavFile(filename, buffer);
        }

        if (!loaded) {
            std::cerr << "Failed to load sound: " << filename << std::endl;
            alDeleteBuffers(1, &buffer);
            return -1;
        }

        ALuint source = 0;
        alGenSources(1, &source);
        
        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "Failed to generate source for: " << soundName << std::endl;
            alDeleteBuffers(1, &buffer);
            return -1;
        }

        alSourcei(source, AL_BUFFER, buffer);
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcef(source, AL_GAIN, 1.0f);
        alSourcef(source, AL_PITCH, 1.0f);

        SoundSource soundSource;
        soundSource.source = source;
        soundSource.buffer = buffer;
        soundSource.name = soundName;
        soundSource.looping = loop;
        soundSource.volume = 1.0f;
        soundSource.pitch = 1.0f;

        sources.push_back(soundSource);
        soundBuffers[soundName] = buffer;
        
        std::cout << "Loaded: " << soundName << " (ID: " << sources.size() - 1 << ")" << std::endl;
        return static_cast<int>(sources.size() - 1);
    }

    void play(int index) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        alSourcePlay(sources[index].source);
    }

    void play(const std::string& soundName) {
        for (auto& source : sources) {
            if (source.name == soundName) {
                alSourcePlay(source.source);
                return;
            }
        }
    }

    void stop(int index) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        alSourceStop(sources[index].source);
    }

    void stop(const std::string& soundName) {
        for (auto& source : sources) {
            if (source.name == soundName) {
                alSourceStop(source.source);
                return;
            }
        }
    }

    void pause(int index) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        alSourcePause(sources[index].source);
    }

    void setVolume(int index, float volume) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].volume = volume;
        alSourcef(sources[index].source, AL_GAIN, volume);
    }

    void setVolume(const std::string& soundName, float volume) {
        for (auto& source : sources) {
            if (source.name == soundName) {
                source.volume = volume;
                alSourcef(source.source, AL_GAIN, volume);
                return;
            }
        }
    }

    void setPitch(int index, float pitch) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].pitch = pitch;
        alSourcef(sources[index].source, AL_PITCH, pitch);
    }

    void setLooping(int index, bool loop) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].looping = loop;
        alSourcei(sources[index].source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }

    bool isPlaying(int index) const {
        if (index < 0 || index >= static_cast<int>(sources.size())) return false;
        ALint state;
        alGetSourcei(sources[index].source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

    void stopAll() {
        for (auto& source : sources) {
            if (source.source != 0) alSourceStop(source.source);
        }
    }

    void update() {
        ALenum error = alGetError();
        if (error != AL_NO_ERROR) {
            std::cerr << "OpenAL error: " << error << std::endl;
        }
    }

    int getSourceIndex(const std::string& soundName) const {
        for (size_t i = 0; i < sources.size(); ++i) {
            if (sources[i].name == soundName) return static_cast<int>(i);
        }
        return -1;
    }

    ALuint getSourceID(int index) const {
        if (index < 0 || index >= static_cast<int>(sources.size())) return 0;
        return sources[index].source;
    }

    // --- 3D Positional Audio ---

    void setSourcePosition(int index, float x, float y, float z) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].position[0] = x;
        sources[index].position[1] = y;
        sources[index].position[2] = z;
        alSource3f(sources[index].source, AL_POSITION, x, y, z);
    }

    void setSourcePosition(const std::string& soundName, float x, float y, float z) {
        for (auto& source : sources) {
            if (source.name == soundName) {
                source.position[0] = x;
                source.position[1] = y;
                source.position[2] = z;
                alSource3f(source.source, AL_POSITION, x, y, z);
                return;
            }
        }
    }

    void getSourcePosition(int index, float& x, float& y, float& z) {
        if (index < 0 || index >= static_cast<int>(sources.size())) { x = y = z = 0; return; }
        x = sources[index].position[0];
        y = sources[index].position[1];
        z = sources[index].position[2];
    }

    void setSourceVelocity(int index, float x, float y, float z) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].velocity[0] = x;
        sources[index].velocity[1] = y;
        sources[index].velocity[2] = z;
        alSource3f(sources[index].source, AL_VELOCITY, x, y, z);
    }

    void setSourceRolloff(int index, float rolloff) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].rolloff = rolloff;
        alSourcef(sources[index].source, AL_ROLLOFF_FACTOR, rolloff);
    }

    void setSourceReferenceDistance(int index, float dist) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].referenceDistance = dist;
        alSourcef(sources[index].source, AL_REFERENCE_DISTANCE, dist);
    }

    void setSourceMaxDistance(int index, float dist) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].maxDistance = dist;
        alSourcef(sources[index].source, AL_MAX_DISTANCE, dist);
    }

    void setSourceSpatial(int index, bool spatial) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        sources[index].spatial = spatial;
        alSourcei(sources[index].source, AL_SOURCE_RELATIVE, spatial ? AL_FALSE : AL_TRUE);
    }

    void setSourceCone(int index, float innerAngle, float outerAngle, float outerGain) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        alSourcef(sources[index].source, AL_CONE_INNER_ANGLE, innerAngle);
        alSourcef(sources[index].source, AL_CONE_OUTER_ANGLE, outerAngle);
        alSourcef(sources[index].source, AL_CONE_OUTER_GAIN, outerGain);
    }

    void setSourceDirection(int index, float x, float y, float z) {
        if (index < 0 || index >= static_cast<int>(sources.size())) return;
        alSource3f(sources[index].source, AL_DIRECTION, x, y, z);
    }

    // --- Listener ---

    void setListenerPosition(float x, float y, float z) {
        alListener3f(AL_POSITION, x, y, z);
    }

    void getListenerPosition(float& x, float& y, float& z) {
        alGetListener3f(AL_POSITION, &x, &y, &z);
    }

    void setListenerVelocity(float x, float y, float z) {
        alListener3f(AL_VELOCITY, x, y, z);
    }

    void setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                                 float upX, float upY, float upZ) {
        float orientation[6] = { forwardX, forwardY, forwardZ, upX, upY, upZ };
        alListenerfv(AL_ORIENTATION, orientation);
    }

    void setListenerGain(float gain) {
        alListenerf(AL_GAIN, gain);
    }

    // --- EFX Effects ---

    int createEffect() {
        if (!efxSupported) return -1;
        ALuint effect = 0;
        alGenEffects(1, &effect);
        if (alGetError() != AL_NO_ERROR) return -1;
        EffectSlotData data;
        data.effect = effect;
        data.type = AL_EFFECT_NULL;
        effects.push_back(data);
        return static_cast<int>(effects.size() - 1);
    }

    void destroyEffect(int index) {
        if (index < 0 || index >= static_cast<int>(effects.size())) return;
        if (effects[index].effect != 0) {
            alDeleteEffects(1, &effects[index].effect);
            effects[index].effect = 0;
        }
        effectFloatParams.erase(index);
        effectIntParams.erase(index);
    }

    bool setEffectType(int effectIndex, int type) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return false;
        if (!efxSupported || effects[effectIndex].effect == 0) return false;
        alEffecti(effects[effectIndex].effect, AL_EFFECT_TYPE, type);
        if (alGetError() != AL_NO_ERROR) return false;
        effects[effectIndex].type = type;
        return true;
    }

    bool setEffectf(int effectIndex, int param, float value) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return false;
        if (!efxSupported || effects[effectIndex].effect == 0) return false;
        alEffectf(effects[effectIndex].effect, param, value);
        if (alGetError() == AL_NO_ERROR) {
            effectFloatParams[effectIndex][param] = value;
            return true;
        }
        return false;
    }

    bool setEffecti(int effectIndex, int param, int value) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return false;
        if (!efxSupported || effects[effectIndex].effect == 0) return false;
        alEffecti(effects[effectIndex].effect, param, value);
        if (alGetError() == AL_NO_ERROR) {
            effectIntParams[effectIndex][param] = value;
            return true;
        }
        return false;
    }

    float getEffectf(int effectIndex, int param) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return 0;
        if (!efxSupported || effects[effectIndex].effect == 0) return 0;
        auto it = effectFloatParams.find(effectIndex);
        if (it != effectFloatParams.end()) {
            auto pit = it->second.find(param);
            if (pit != it->second.end()) return pit->second;
        }
        return 0;
    }

    int getEffecti(int effectIndex, int param) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return 0;
        if (!efxSupported || effects[effectIndex].effect == 0) return 0;
        auto it = effectIntParams.find(effectIndex);
        if (it != effectIntParams.end()) {
            auto pit = it->second.find(param);
            if (pit != it->second.end()) return pit->second;
        }
        return 0;
    }

    // --- Filters ---

    int createFilter() {
        if (!efxSupported) return -1;
        ALuint filter = 0;
        alGenFilters(1, &filter);
        if (alGetError() != AL_NO_ERROR) return -1;
        FilterData data;
        data.filter = filter;
        data.type = AL_FILTER_NULL;
        filters.push_back(data);
        return static_cast<int>(filters.size() - 1);
    }

    void destroyFilter(int index) {
        if (index < 0 || index >= static_cast<int>(filters.size())) return;
        if (filters[index].filter != 0) {
            alDeleteFilters(1, &filters[index].filter);
            filters[index].filter = 0;
        }
    }

    bool setFilterType(int filterIndex, int type) {
        if (filterIndex < 0 || filterIndex >= static_cast<int>(filters.size())) return false;
        if (!efxSupported || filters[filterIndex].filter == 0) return false;
        alFilteri(filters[filterIndex].filter, AL_FILTER_TYPE, type);
        if (alGetError() != AL_NO_ERROR) return false;
        filters[filterIndex].type = type;
        return true;
    }

    bool setFilterf(int filterIndex, int param, float value) {
        if (filterIndex < 0 || filterIndex >= static_cast<int>(filters.size())) return false;
        if (!efxSupported || filters[filterIndex].filter == 0) return false;
        alFilterf(filters[filterIndex].filter, param, value);
        return alGetError() == AL_NO_ERROR;
    }

    bool setFilteri(int filterIndex, int param, int value) {
        if (filterIndex < 0 || filterIndex >= static_cast<int>(filters.size())) return false;
        if (!efxSupported || filters[filterIndex].filter == 0) return false;
        alFilteri(filters[filterIndex].filter, param, value);
        return alGetError() == AL_NO_ERROR;
    }

    // --- Auxiliary Effect Slots ---

    int createEffectSlot() {
        if (!efxSupported) return -1;
        ALuint slot = 0;
        alGenAuxiliaryEffectSlots(1, &slot);
        if (alGetError() != AL_NO_ERROR) return -1;
        EffectSlotData data;
        data.slot = slot;
        data.effect = 0;
        data.type = AL_EFFECT_NULL;
        effectSlots.push_back(data);
        return static_cast<int>(effectSlots.size() - 1);
    }

    void destroyEffectSlot(int index) {
        if (index < 0 || index >= static_cast<int>(effectSlots.size())) return;
        if (effectSlots[index].slot != 0) {
            alDeleteAuxiliaryEffectSlots(1, &effectSlots[index].slot);
            effectSlots[index].slot = 0;
        }
    }

    bool setEffectSlotEffect(int slotIndex, int effectIndex) {
        if (slotIndex < 0 || slotIndex >= static_cast<int>(effectSlots.size())) return false;
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return false;
        if (!efxSupported) return false;
        alAuxiliaryEffectSloti(effectSlots[slotIndex].slot, AL_EFFECTSLOT_EFFECT, effects[effectIndex].effect);
        if (alGetError() != AL_NO_ERROR) return false;
        effectSlots[slotIndex].effect = effects[effectIndex].effect;
        effectSlots[slotIndex].type = effects[effectIndex].type;
        return true;
    }

    bool clearEffectSlotEffect(int slotIndex) {
        if (slotIndex < 0 || slotIndex >= static_cast<int>(effectSlots.size())) return false;
        if (!efxSupported) return false;
        alAuxiliaryEffectSloti(effectSlots[slotIndex].slot, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);
        if (alGetError() != AL_NO_ERROR) return false;
        effectSlots[slotIndex].effect = 0;
        effectSlots[slotIndex].type = AL_EFFECT_NULL;
        return true;
    }

    bool setEffectSlotGain(int slotIndex, float gain) {
        if (slotIndex < 0 || slotIndex >= static_cast<int>(effectSlots.size())) return false;
        if (!efxSupported) return false;
        alAuxiliaryEffectSlotf(effectSlots[slotIndex].slot, AL_EFFECTSLOT_GAIN, gain);
        return alGetError() == AL_NO_ERROR;
    }

    // --- Link source to effect slot ---

    bool attachEffectToSource(int sourceIndex, int slotIndex) {
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size())) return false;
        if (slotIndex < 0 || slotIndex >= static_cast<int>(effectSlots.size())) return false;
        if (!efxSupported) return false;
        alSourcei(sources[sourceIndex].source, AL_DIRECT_FILTER, AL_FILTER_NULL);
        alSource3i(sources[sourceIndex].source, AL_AUXILIARY_SEND_FILTER,
                   effectSlots[slotIndex].slot, 0, AL_FILTER_NULL);
        return alGetError() == AL_NO_ERROR;
    }

    bool detachEffectFromSource(int sourceIndex) {
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size())) return false;
        if (!efxSupported) return false;
        alSource3i(sources[sourceIndex].source, AL_AUXILIARY_SEND_FILTER,
                   AL_EFFECTSLOT_NULL, 0, AL_FILTER_NULL);
        return alGetError() == AL_NO_ERROR;
    }

    bool attachFilterToSource(int sourceIndex, int filterIndex) {
        if (sourceIndex < 0 || sourceIndex >= static_cast<int>(sources.size())) return false;
        if (!efxSupported) return false;
        if (filterIndex < 0 || filterIndex >= static_cast<int>(filters.size())) {
            alSourcei(sources[sourceIndex].source, AL_DIRECT_FILTER, AL_FILTER_NULL);
        } else {
            alSourcei(sources[sourceIndex].source, AL_DIRECT_FILTER, filters[filterIndex].filter);
        }
        return alGetError() == AL_NO_ERROR;
    }

    // --- Convenience: Reverb preset ---

    void setReverbPreset(int effectIndex, float density, float diffusion, float gain,
                          float gainHF, float decayTime, float decayHFRatio) {
        if (effectIndex < 0 || effectIndex >= static_cast<int>(effects.size())) return;
        setEffectType(effectIndex, AL_EFFECT_EAXREVERB);
        setEffectf(effectIndex, AL_EAXREVERB_DENSITY, density);
        setEffectf(effectIndex, AL_EAXREVERB_DIFFUSION, diffusion);
        setEffectf(effectIndex, AL_EAXREVERB_GAIN, gain);
        setEffectf(effectIndex, AL_EAXREVERB_GAINHF, gainHF);
        setEffectf(effectIndex, AL_EAXREVERB_DECAY_TIME, decayTime);
        setEffectf(effectIndex, AL_EAXREVERB_DECAY_HFRATIO, decayHFRatio);
    }

    bool getEfxSupported() const { return efxSupported; }

private:
    ALCdevice* device;
    ALCcontext* context;
    std::vector<SoundSource> sources;
    std::unordered_map<std::string, ALuint> soundBuffers;
    std::mutex mutex;
    bool initialized;

    struct EffectSlotData {
        ALuint effect;
        ALuint slot;
        int type;
        EffectSlotData() : effect(0), slot(0), type(AL_EFFECT_NULL) {}
    };

    struct FilterData {
        ALuint filter;
        int type;
        FilterData() : filter(0), type(AL_FILTER_NULL) {}
    };

    std::vector<EffectSlotData> effects;
    std::vector<EffectSlotData> effectSlots;
    std::vector<FilterData> filters;
    std::unordered_map<int, std::unordered_map<int, float>> effectFloatParams;
    std::unordered_map<int, std::unordered_map<int, int>> effectIntParams;
    bool efxSupported;

    // EFX function pointers
    LPALGENEFFECTS alGenEffects;
    LPALDELETEEFFECTS alDeleteEffects;
    LPALEFFECTI alEffecti;
    LPALEFFECTF alEffectf;
    LPALGENFILTERS alGenFilters;
    LPALDELETEFILTERS alDeleteFilters;
    LPALFILTERI alFilteri;
    LPALFILTERF alFilterf;
    LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
    LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
    LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
    LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
    LPALSOURCEI alSourcei;

    void loadEfxFunctions() {
        if (!alcIsExtensionPresent(device, "ALC_EXT_EFX")) {
            std::cout << "EFX not supported, effects disabled" << std::endl;
            efxSupported = false;
            return;
        }

        alGenEffects = reinterpret_cast<LPALGENEFFECTS>(alcGetProcAddress(device, "alGenEffects"));
        alDeleteEffects = reinterpret_cast<LPALDELETEEFFECTS>(alcGetProcAddress(device, "alDeleteEffects"));
        alEffecti = reinterpret_cast<LPALEFFECTI>(alcGetProcAddress(device, "alEffecti"));
        alEffectf = reinterpret_cast<LPALEFFECTF>(alcGetProcAddress(device, "alEffectf"));

        alGenFilters = reinterpret_cast<LPALGENFILTERS>(alcGetProcAddress(device, "alGenFilters"));
        alDeleteFilters = reinterpret_cast<LPALDELETEFILTERS>(alcGetProcAddress(device, "alDeleteFilters"));
        alFilteri = reinterpret_cast<LPALFILTERI>(alcGetProcAddress(device, "alFilteri"));
        alFilterf = reinterpret_cast<LPALFILTERF>(alcGetProcAddress(device, "alFilterf"));

        alGenAuxiliaryEffectSlots = reinterpret_cast<LPALGENAUXILIARYEFFECTSLOTS>(alcGetProcAddress(device, "alGenAuxiliaryEffectSlots"));
        alDeleteAuxiliaryEffectSlots = reinterpret_cast<LPALDELETEAUXILIARYEFFECTSLOTS>(alcGetProcAddress(device, "alDeleteAuxiliaryEffectSlots"));
        alAuxiliaryEffectSloti = reinterpret_cast<LPALAUXILIARYEFFECTSLOTI>(alcGetProcAddress(device, "alAuxiliaryEffectSloti"));
        alAuxiliaryEffectSlotf = reinterpret_cast<LPALAUXILIARYEFFECTSLOTF>(alcGetProcAddress(device, "alAuxiliaryEffectSlotf"));

        alSourcei = reinterpret_cast<LPALSOURCEI>(alcGetProcAddress(device, "alSourcei"));

        efxSupported = alGenEffects && alDeleteEffects && alEffecti && alEffectf &&
                       alGenFilters && alDeleteFilters && alFilteri && alFilterf &&
                       alGenAuxiliaryEffectSlots && alDeleteAuxiliaryEffectSlots &&
                       alAuxiliaryEffectSloti && alAuxiliaryEffectSlotf;

        if (efxSupported) {
            std::cout << "EFX effects supported" << std::endl;
        } else {
            std::cout << "EFX partially supported, some effects unavailable" << std::endl;
        }
    }

    bool loadWavFile(const std::string& filename, ALuint buffer) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open: " << filename << std::endl;
            return false;
        }

        char chunkId[5] = {0};
        uint32_t chunkSize;
        char format[5] = {0};

        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);
        file.read(format, 4);

        if (std::string(chunkId) != "RIFF" || std::string(format) != "WAVE") {
            std::cerr << "Not a WAV file: " << filename << std::endl;
            return false;
        }

        char subchunkId[5] = {0};
        uint32_t subchunkSize;
        uint16_t audioFormat, numChannels, blockAlign, bitsPerSample;
        uint32_t sampleRate, byteRate;

        bool foundFmt = false;
        bool foundData = false;
        std::vector<char> audioData;

        while (!foundData && file.good()) {
            file.read(subchunkId, 4);
            file.read(reinterpret_cast<char*>(&subchunkSize), 4);

            if (std::string(subchunkId) == "fmt ") {
                foundFmt = true;
                file.read(reinterpret_cast<char*>(&audioFormat), 2);
                file.read(reinterpret_cast<char*>(&numChannels), 2);
                file.read(reinterpret_cast<char*>(&sampleRate), 4);
                file.read(reinterpret_cast<char*>(&byteRate), 4);
                file.read(reinterpret_cast<char*>(&blockAlign), 2);
                file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
                
                if (subchunkSize > 16) {
                    file.seekg(subchunkSize - 16, std::ios::cur);
                }
            }
            else if (std::string(subchunkId) == "data") {
                foundData = true;
                audioData.resize(subchunkSize);
                file.read(audioData.data(), subchunkSize);
            }
            else {
                file.seekg(subchunkSize, std::ios::cur);
            }
        }

        if (!foundFmt || !foundData) {
            std::cerr << "Missing fmt or data chunk: " << filename << std::endl;
            return false;
        }

        if (audioFormat != 1) {
            std::cerr << "Not PCM WAV: " << filename << std::endl;
            return false;
        }

        ALenum formatAL;
        if (numChannels == 1) {
            formatAL = (bitsPerSample == 8) ? AL_FORMAT_MONO8 : AL_FORMAT_MONO16;
        } else if (numChannels == 2) {
            formatAL = (bitsPerSample == 8) ? AL_FORMAT_STEREO8 : AL_FORMAT_STEREO16;
        } else {
            std::cerr << "Unsupported channels: " << numChannels << std::endl;
            return false;
        }

        alBufferData(buffer, formatAL, audioData.data(), 
                     static_cast<ALsizei>(audioData.size()), sampleRate);

        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "Failed to load buffer data" << std::endl;
            return false;
        }

        return true;
    }

    bool loadMp3File(const std::string& filename, ALuint buffer) {
        drmp3_config config;
        drmp3_uint64 totalFrames;
        drmp3_int16* pcmData = drmp3_open_file_and_read_pcm_frames_s16(
            filename.c_str(), &config, &totalFrames, nullptr
        );

        if (!pcmData) {
            std::cerr << "Failed to decode MP3: " << filename << std::endl;
            return false;
        }

        ALenum formatAL;
        if (config.channels == 1) {
            formatAL = AL_FORMAT_MONO16;
        } else if (config.channels == 2) {
            formatAL = AL_FORMAT_STEREO16;
        } else {
            std::cerr << "Unsupported MP3 channels: " << config.channels << std::endl;
            drmp3_free(pcmData, nullptr);
            return false;
        }

        ALsizei dataSize = static_cast<ALsizei>(totalFrames * config.channels * sizeof(drmp3_int16));
        alBufferData(buffer, formatAL, pcmData, dataSize,
                     static_cast<ALsizei>(config.sampleRate));

        drmp3_free(pcmData, nullptr);

        if (alGetError() != AL_NO_ERROR) {
            std::cerr << "Failed to load MP3 buffer data" << std::endl;
            return false;
        }

        return true;
    }
};
