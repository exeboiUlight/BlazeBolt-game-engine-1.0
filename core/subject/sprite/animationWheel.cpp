#include "animationWheel.hpp"

namespace BlazeBolt {

    AnimationWheel::AnimationWheel()
        : spriteEntity(0), autoAdvance(false) {}

    void AnimationWheel::setSpriteEntity(uint32_t entity) {
        spriteEntity = entity;
    }

    uint32_t AnimationWheel::getSpriteEntity() const {
        return spriteEntity;
    }

    void AnimationWheel::addState(const std::string &name, const std::string &gifPath, float playbackSpeed, bool looping) {
        states[name] = AnimationState(gifPath, playbackSpeed, looping);
    }

    void AnimationWheel::removeState(const std::string &name) {
        states.erase(name);
    }

    bool AnimationWheel::hasState(const std::string &name) const {
        return states.find(name) != states.end();
    }

    void AnimationWheel::setInitialState(const std::string &name) {
        initialState = name;
    }

    const std::string &AnimationWheel::getInitialState() const {
        return initialState;
    }

    void AnimationWheel::setCurrentState(const std::string &name) {
        if (hasState(name)) {
            currentState = name;
        }
    }

    const std::string &AnimationWheel::getCurrentState() const {
        return currentState;
    }

    void AnimationWheel::setPlaybackSpeed(const std::string &stateName, float speed) {
        auto it = states.find(stateName);
        if (it != states.end()) {
            it->second.playbackSpeed = speed;
        }
    }

    float AnimationWheel::getPlaybackSpeed(const std::string &stateName) const {
        auto it = states.find(stateName);
        return (it != states.end()) ? it->second.playbackSpeed : 1.0f;
    }

    void AnimationWheel::setLooping(const std::string &stateName, bool looping) {
        auto it = states.find(stateName);
        if (it != states.end()) {
            it->second.looping = looping;
        }
    }

    bool AnimationWheel::isLooping(const std::string &stateName) const {
        auto it = states.find(stateName);
        return (it != states.end()) ? it->second.looping : true;
    }

    void AnimationWheel::setGifPath(const std::string &stateName, const std::string &gifPath) {
        auto it = states.find(stateName);
        if (it != states.end()) {
            it->second.gifPath = gifPath;
        }
    }

    const std::string &AnimationWheel::getGifPath(const std::string &stateName) const {
        static const std::string empty;
        auto it = states.find(stateName);
        return (it != states.end()) ? it->second.gifPath : empty;
    }

    void AnimationWheel::setAutoAdvance(bool autoAdvance) {
        this->autoAdvance = autoAdvance;
    }

    bool AnimationWheel::getAutoAdvance() const {
        return autoAdvance;
    }

    const AnimationState *AnimationWheel::getState(const std::string &name) const {
        auto it = states.find(name);
        return (it != states.end()) ? &it->second : nullptr;
    }

    const std::unordered_map<std::string, AnimationState> &AnimationWheel::getAllStates() const {
        return states;
    }

    std::vector<std::string> AnimationWheel::getStateNames() const {
        std::vector<std::string> names;
        names.reserve(states.size());
        for (const auto &pair : states) {
            names.push_back(pair.first);
        }
        return names;
    }
}
