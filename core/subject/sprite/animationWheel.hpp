#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

namespace BlazeBolt {

    struct AnimationState {
        std::string gifPath;
        float playbackSpeed = 1.0f;
        bool looping = true;

        AnimationState() = default;
        AnimationState(const std::string &path, float speed, bool loop)
            : gifPath(path), playbackSpeed(speed), looping(loop) {}
    };

    class AnimationWheel {
    private:
        std::unordered_map<std::string, AnimationState> states;
        std::string currentState;
        std::string initialState;
        uint32_t spriteEntity;
        bool autoAdvance;

    public:
        AnimationWheel();
        ~AnimationWheel() = default;

        void setSpriteEntity(uint32_t entity);
        uint32_t getSpriteEntity() const;

        void addState(const std::string &name, const std::string &gifPath, float playbackSpeed, bool looping);
        void removeState(const std::string &name);
        bool hasState(const std::string &name) const;

        void setInitialState(const std::string &name);
        const std::string &getInitialState() const;

        void setCurrentState(const std::string &name);
        const std::string &getCurrentState() const;

        void setPlaybackSpeed(const std::string &stateName, float speed);
        float getPlaybackSpeed(const std::string &stateName) const;

        void setLooping(const std::string &stateName, bool looping);
        bool isLooping(const std::string &stateName) const;

        void setGifPath(const std::string &stateName, const std::string &gifPath);
        const std::string &getGifPath(const std::string &stateName) const;

        void setAutoAdvance(bool autoAdvance);
        bool getAutoAdvance() const;

        const AnimationState *getState(const std::string &name) const;
        const std::unordered_map<std::string, AnimationState> &getAllStates() const;
        std::vector<std::string> getStateNames() const;
    };
}
