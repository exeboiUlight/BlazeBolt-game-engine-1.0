#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "imgui.h"

enum class PinType {
    Flow,
    Number,
    Integer,
    String,
    Bool,
    Entity,
    Table,
    Vector2,
    Vector3,
    Any
};

enum class NodeCategory {
    Flow,
    Variables,
    Math,
    Comparison,
    Sprites,
    Animation,
    Text,
    Camera,
    Physics,
    Audio,
    Window,
    Scenes,
    Noise,
    MathTypes,
    Constants
};

struct PinDef {
    std::string name;
    PinType type;
};

struct NodeTypeDef {
    int id;
    const char* name;
    NodeCategory category;
    std::vector<PinDef> inputs;
    std::vector<PinDef> outputs;
    bool has_inline = false;
    const char* inline_label = "";
    PinType inline_type = PinType::Any;
    const char* lua_template = "";
};

inline PinType GetFlowType() { return PinType::Flow; }

inline const char* GetPinTypeName(PinType t) {
    switch (t) {
        case PinType::Flow:    return "Flow";
        case PinType::Number:  return "Number";
        case PinType::Integer: return "Integer";
        case PinType::String:  return "String";
        case PinType::Bool:    return "Bool";
        case PinType::Entity:  return "Entity";
        case PinType::Table:   return "Table";
        case PinType::Vector2: return "Vector2";
        case PinType::Vector3: return "Vector3";
        case PinType::Any:     return "Any";
    }
    return "Unknown";
}

inline const char* GetCategoryName(NodeCategory cat) {
    switch (cat) {
        case NodeCategory::Flow:      return "Flow";
        case NodeCategory::Variables: return "Variables";
        case NodeCategory::Math:      return "Math";
        case NodeCategory::Comparison:return "Comparison";
        case NodeCategory::Sprites:   return "Sprites";
        case NodeCategory::Animation: return "Animation";
        case NodeCategory::Text:      return "Text";
        case NodeCategory::Camera:    return "Camera";
        case NodeCategory::Physics:   return "Physics";
        case NodeCategory::Audio:     return "Audio";
        case NodeCategory::Window:    return "Window";
        case NodeCategory::Scenes:    return "Scenes & Scripts";
        case NodeCategory::Noise:     return "Noise";
        case NodeCategory::MathTypes: return "Math Types";
        case NodeCategory::Constants: return "Constants";
    }
    return "Unknown";
}

inline ImColor GetCategoryColor(NodeCategory cat) {
    switch (cat) {
        case NodeCategory::Flow:       return ImColor(255, 255, 255);
        case NodeCategory::Variables:  return ImColor(100, 200, 100);
        case NodeCategory::Math:       return ImColor(100, 150, 255);
        case NodeCategory::Comparison: return ImColor(255, 200, 100);
        case NodeCategory::Sprites:    return ImColor(255, 100, 100);
        case NodeCategory::Animation:  return ImColor(255, 150, 100);
        case NodeCategory::Text:       return ImColor(200, 200, 255);
        case NodeCategory::Camera:     return ImColor(100, 255, 200);
        case NodeCategory::Physics:    return ImColor(255, 255, 100);
        case NodeCategory::Audio:      return ImColor(200, 100, 255);
        case NodeCategory::Window:     return ImColor(180, 180, 180);
        case NodeCategory::Scenes:     return ImColor(100, 200, 255);
        case NodeCategory::Noise:      return ImColor(150, 255, 150);
        case NodeCategory::MathTypes:  return ImColor(80, 180, 255);
        case NodeCategory::Constants:  return ImColor(220, 220, 220);
    }
    return ImColor(255, 255, 255);
}

inline const std::vector<NodeTypeDef>& GetNodeTypeDefs() {
    static const std::vector<NodeTypeDef> defs = {
        // =========================================================================
        // FLOW (15 nodes)
        // =========================================================================
        {
            /*id*/ 1000, /*name*/ "Start", /*cat*/ NodeCategory::Flow,
            /*inputs*/ {},
            /*outputs*/ {{".exec", PinType::Flow}},
            /*has_inline*/ false, /*inline_label*/ "", /*inline_type*/ PinType::Any,
            /*lua*/ ">>function Start()\n>>{0}\n>>end"
        },
        {
            1001, "Update", NodeCategory::Flow,
            {},
            {{"dt", PinType::Number}},
            false, "", PinType::Any,
            ">>function Update(dt)\n>>{0}\n>>end"
        },
        {
            1002, "Draw", NodeCategory::Flow,
            {},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>function Draw()\n>>{0}\n>>end"
        },
        {
            1003, "End", NodeCategory::Flow,
            {},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>function End()\n>>{0}\n>>end"
        },
        {
            1004, "OnSceneLoad", NodeCategory::Flow,
            {{"sceneName", PinType::String}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>function On{0}Load()\n>>{1}\n>>end"
        },
        {
            1005, "OnSceneUnload", NodeCategory::Flow,
            {{"sceneName", PinType::String}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>function On{0}Unload()\n>>{1}\n>>end"
        },
        {
            1006, "If", NodeCategory::Flow,
            {{"condition", PinType::Bool}, {".exec", PinType::Flow}},
            {{"then", PinType::Flow}, {"else", PinType::Flow}},
            false, "", PinType::Any,
            ">>if ${0} then\n>>{then}\n>>else\n>>{else}\n>>end"
        },
        {
            1007, "ForLoop", NodeCategory::Flow,
            {{"from", PinType::Number}, {"to", PinType::Number}, {"step", PinType::Number}, {".exec", PinType::Flow}},
            {{"index", PinType::Number}, {"body", PinType::Flow}, {"done", PinType::Flow}},
            false, "", PinType::Any,
            ">>for ${index} = {0}, {1}, {2} do\n>>{body}\n>>end\n>>{done}"
        },
        {
            1008, "WhileLoop", NodeCategory::Flow,
            {{"condition", PinType::Bool}, {".exec", PinType::Flow}},
            {{"body", PinType::Flow}, {"done", PinType::Flow}},
            false, "", PinType::Any,
            ">>while ${0} do\n>>{body}\n>>end\n>>{done}"
        },
        {
            1009, "Delay", NodeCategory::Flow,
            {{"frames", PinType::Integer}, {".exec", PinType::Flow}},
            {{"done", PinType::Flow}},
            false, "", PinType::Any,
            ">>delay({0})\n>>{done}"
        },
        {
            1010, "Print", NodeCategory::Flow,
            {{"text", PinType::String}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.Print({0})\n>>{1}"
        },
        {
            1011, "CustomFunction", NodeCategory::Flow,
            {{"name", PinType::String}, {".exec", PinType::Flow}},
            {{"call", PinType::Flow}},
            false, "", PinType::Any,
            ">>function {0}()\n>>{1}\n>>end"
        },
        {
            1012, "CallFunction", NodeCategory::Flow,
            {{"name", PinType::String}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>{0}()\n>>{1}"
        },
        {
            1013, "Return", NodeCategory::Flow,
            {{"value", PinType::Any}, {".exec", PinType::Flow}},
            {},
            false, "", PinType::Any,
            ">>return {0}"
        },
        {
            1014, "Comment", NodeCategory::Flow,
            {},
            {},
            true, "Comment", PinType::String,
            ""
        },

        // =========================================================================
        // VARIABLES (11 nodes)
        // =========================================================================
        {
            2000, "VarGet", NodeCategory::Variables,
            {},
            {{"value", PinType::Any}},
            true, "Variable Name", PinType::String,
            "local ${0} = {inline}"
        },
        {
            2001, "VarSet", NodeCategory::Variables,
            {{"value", PinType::Any}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            true, "Variable Name", PinType::String,
            ">>{inline} = {0}\n>>{1}"
        },
        {
            2002, "LocalVar", NodeCategory::Variables,
            {{"value", PinType::Any}, {".exec", PinType::Flow}},
            {{"var", PinType::Any}, {".exec", PinType::Flow}},
            true, "Variable Name", PinType::String,
            ">>local ${var} = {0}\n>>{1}"
        },
        {
            2003, "TableCreate", NodeCategory::Variables,
            {},
            {{"table", PinType::Table}},
            false, "", PinType::Any,
            "local ${0} = {}"
        },
        {
            2004, "TableGet", NodeCategory::Variables,
            {{"table", PinType::Table}, {"key", PinType::String}},
            {{"value", PinType::Any}},
            false, "", PinType::Any,
            "local ${0} = {0}[{1}]"
        },
        {
            2005, "TableSet", NodeCategory::Variables,
            {{"table", PinType::Table}, {"key", PinType::String}, {"value", PinType::Any}},
            {},
            false, "", PinType::Any,
            "{0}[{1}] = {2}"
        },
        {
            2006, "TableInsert", NodeCategory::Variables,
            {{"table", PinType::Table}, {"value", PinType::Any}},
            {},
            false, "", PinType::Any,
            "table.insert({0}, {1})"
        },
        {
            2007, "TableLength", NodeCategory::Variables,
            {{"table", PinType::Table}},
            {{"length", PinType::Integer}},
            false, "", PinType::Any,
            "local ${0} = #({0})"
        },
        {
            2008, "StringConcat", NodeCategory::Variables,
            {{"a", PinType::String}, {"b", PinType::String}},
            {{"result", PinType::String}},
            false, "", PinType::Any,
            "local ${0} = {0} .. {1}"
        },
        {
            2009, "ToString", NodeCategory::Variables,
            {{"value", PinType::Any}},
            {{"result", PinType::String}},
            false, "", PinType::Any,
            "local ${0} = tostring({0})"
        },
        {
            2010, "ToNumber", NodeCategory::Variables,
            {{"value", PinType::Any}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = tonumber({0})"
        },

        // =========================================================================
        // MATH (20 nodes)
        // =========================================================================
        {
            3000, "Add", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} + {1}"
        },
        {
            3001, "Subtract", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} - {1}"
        },
        {
            3002, "Multiply", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} * {1}"
        },
        {
            3003, "Divide", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} / {1}"
        },
        {
            3004, "Modulo", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} % {1}"
        },
        {
            3005, "Power", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} ^ {1}"
        },
        {
            3006, "Negate", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = -({0})"
        },
        {
            3007, "Abs", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.abs({0})"
        },
        {
            3008, "Sin", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.sin({0})"
        },
        {
            3009, "Cos", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.cos({0})"
        },
        {
            3010, "Sqrt", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.sqrt({0})"
        },
        {
            3011, "Min", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.min({0}, {1})"
        },
        {
            3012, "Max", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.max({0}, {1})"
        },
        {
            3013, "Floor", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.floor({0})"
        },
        {
            3014, "Ceil", NodeCategory::Math,
            {{"a", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.ceil({0})"
        },
        {
            3015, "Clamp", NodeCategory::Math,
            {{"value", PinType::Number}, {"min", PinType::Number}, {"max", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = math.max({1}, math.min({0}, {2}))"
        },
        {
            3016, "Lerp", NodeCategory::Math,
            {{"a", PinType::Number}, {"b", PinType::Number}, {"t", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0} + ({1} - {0}) * {2}"
        },
        {
            3017, "RandomFloat", NodeCategory::Math,
            {{"min", PinType::Number}, {"max", PinType::Number}},
            {{"result", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.Random({0}, {1})"
        },
        {
            3018, "RandomInt", NodeCategory::Math,
            {{"min", PinType::Integer}, {"max", PinType::Integer}},
            {{"result", PinType::Integer}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.RandomInt({0}, {1})"
        },
        {
            3019, "RandomSeed", NodeCategory::Math,
            {{"seed", PinType::Integer}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.SetRandomSeed({0})"
        },

        // =========================================================================
        // COMPARISON (11 nodes)
        // =========================================================================
        {
            4000, "Equal", NodeCategory::Comparison,
            {{"a", PinType::Any}, {"b", PinType::Any}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} == {1})"
        },
        {
            4001, "NotEqual", NodeCategory::Comparison,
            {{"a", PinType::Any}, {"b", PinType::Any}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} ~= {1})"
        },
        {
            4002, "Greater", NodeCategory::Comparison,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} > {1})"
        },
        {
            4003, "Less", NodeCategory::Comparison,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} < {1})"
        },
        {
            4004, "GreaterEqual", NodeCategory::Comparison,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} >= {1})"
        },
        {
            4005, "LessEqual", NodeCategory::Comparison,
            {{"a", PinType::Number}, {"b", PinType::Number}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} <= {1})"
        },
        {
            4006, "And", NodeCategory::Comparison,
            {{"a", PinType::Bool}, {"b", PinType::Bool}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} and {1})"
        },
        {
            4007, "Or", NodeCategory::Comparison,
            {{"a", PinType::Bool}, {"b", PinType::Bool}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = ({0} or {1})"
        },
        {
            4008, "Not", NodeCategory::Comparison,
            {{"a", PinType::Bool}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = not ({0})"
        },
        {
            4009, "IsKeyPressed", NodeCategory::Comparison,
            {{"key", PinType::Integer}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.IsKeyPressed({0})"
        },
        {
            4010, "IsKeyJustPressed", NodeCategory::Comparison,
            {{"key", PinType::Integer}},
            {{"result", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.IsKeyJustPressed({0})"
        },

        // =========================================================================
        // SPRITES (11 nodes)
        // =========================================================================
        {
            5000, "CreateSprite", NodeCategory::Sprites,
            {{"texture", PinType::String}, {"x", PinType::Number}, {"y", PinType::Number}},
            {{"entity", PinType::Entity}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.CreateSprite({0}, {1}, {2})"
        },
        {
            5001, "SpriteSetPosition", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"x", PinType::Number}, {"y", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetPosition({0}, {1}, {2})\n>>{3}"
        },
        {
            5002, "SpriteGetPosition", NodeCategory::Sprites,
            {{"entity", PinType::Entity}},
            {{"x", PinType::Number}, {"y", PinType::Number}},
            false, "", PinType::Any,
            "local ${x}, ${y} = BlazeBolt.SpriteGetPosition({0})"
        },
        {
            5003, "SpriteSetSize", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"w", PinType::Number}, {"h", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetSize({0}, {1}, {2})\n>>{3}"
        },
        {
            5004, "SpriteSetRotation", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"rotation", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetRotation({0}, {1})\n>>{2}"
        },
        {
            5005, "SpriteGetRotation", NodeCategory::Sprites,
            {{"entity", PinType::Entity}},
            {{"rotation", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.SpriteGetRotation({0})"
        },
        {
            5006, "SpriteSetOrigin", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"x", PinType::Number}, {"y", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetOrigin({0}, {1}, {2})\n>>{3}"
        },
        {
            5007, "SpriteSetColor", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"r", PinType::Number}, {"g", PinType::Number}, {"b", PinType::Number}, {"a", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetColor({0}, {1}, {2}, {3}, {4})\n>>{5}"
        },
        {
            5008, "SpriteSetVisible", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"visible", PinType::Bool}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetVisible({0}, {1})\n>>{2}"
        },
        {
            5009, "SpriteSetTexture", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"texture", PinType::String}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetTexture({0}, {1})\n>>{2}"
        },
        {
            5010, "SpriteSetTextureRect", NodeCategory::Sprites,
            {{"entity", PinType::Entity}, {"u", PinType::Number}, {"v", PinType::Number}, {"w", PinType::Number}, {"h", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.SpriteSetTextureRect({0}, {1}, {2}, {3}, {4})\n>>{5}"
        },

        // =========================================================================
        // ANIMATION (7 nodes)
        // =========================================================================
        {
            6000, "CreateAnimatedSprite", NodeCategory::Animation,
            {{"path", PinType::String}, {"x", PinType::Number}, {"y", PinType::Number}},
            {{"entity", PinType::Entity}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.CreateAnimatedSprite({0}, {1}, {2})"
        },
        {
            6001, "AnimatedSpritePlay", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpritePlay({0})\n>>{1}"
        },
        {
            6002, "AnimatedSpritePause", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpritePause({0})\n>>{1}"
        },
        {
            6003, "AnimatedSpriteStop", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpriteStop({0})\n>>{1}"
        },
        {
            6004, "AnimatedSpriteSetLooping", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {"looping", PinType::Bool}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpriteSetLooping({0}, {1})\n>>{2}"
        },
        {
            6005, "AnimatedSpriteSetPlaybackSpeed", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {"speed", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpriteSetPlaybackSpeed({0}, {1})\n>>{2}"
        },
        {
            6006, "AnimatedSpriteSetPosition", NodeCategory::Animation,
            {{"entity", PinType::Entity}, {"x", PinType::Number}, {"y", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.AnimatedSpriteSetPosition({0}, {1}, {2})\n>>{3}"
        },

        // =========================================================================
        // TEXT (5 nodes)
        // =========================================================================
        {
            7000, "CreateText", NodeCategory::Text,
            {{"font", PinType::String}, {"text", PinType::String}, {"x", PinType::Number}, {"y", PinType::Number}},
            {{"entity", PinType::Entity}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.CreateText({0}, {1}, {2}, {3})"
        },
        {
            7001, "TextSetString", NodeCategory::Text,
            {{"entity", PinType::Entity}, {"text", PinType::String}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.TextSetString({0}, {1})\n>>{2}"
        },
        {
            7002, "TextSetPosition", NodeCategory::Text,
            {{"entity", PinType::Entity}, {"x", PinType::Number}, {"y", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.TextSetPosition({0}, {1}, {2})\n>>{3}"
        },
        {
            7003, "TextSetColor", NodeCategory::Text,
            {{"entity", PinType::Entity}, {"r", PinType::Number}, {"g", PinType::Number}, {"b", PinType::Number}, {"a", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.TextSetColor({0}, {1}, {2}, {3}, {4})\n>>{5}"
        },
        {
            7004, "TextSetScale", NodeCategory::Text,
            {{"entity", PinType::Entity}, {"sx", PinType::Number}, {"sy", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.TextSetScale({0}, {1}, {2})\n>>{3}"
        },

        // =========================================================================
        // CAMERA (4 nodes)
        // =========================================================================
        {
            8000, "CreateCamera", NodeCategory::Camera,
            {},
            {{"entity", PinType::Entity}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.CreateCamera()"
        },
        {
            8001, "CameraSetPosition", NodeCategory::Camera,
            {{"entity", PinType::Entity}, {"x", PinType::Number}, {"y", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.CameraSetPosition({0}, {1}, {2})\n>>{3}"
        },
        {
            8002, "CameraSetZoom", NodeCategory::Camera,
            {{"entity", PinType::Entity}, {"zoom", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.CameraSetZoom({0}, {1})\n>>{2}"
        },
        {
            8003, "CameraSetRotation", NodeCategory::Camera,
            {{"entity", PinType::Entity}, {"rotation", PinType::Number}, {".exec", PinType::Flow}},
            {{".exec", PinType::Flow}},
            false, "", PinType::Any,
            ">>BlazeBolt.CameraSetRotation({0}, {1})\n>>{2}"
        },

        // =========================================================================
        // PHYSICS (10 nodes)
        // =========================================================================
        {
            9000, "PhysicsInit", NodeCategory::Physics,
            {{"gx", PinType::Number}, {"gy", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsInit({0}, {1})"
        },
        {
            9001, "PhysicsCreateBody", NodeCategory::Physics,
            {{"bodyType", PinType::Integer}, {"x", PinType::Number}, {"y", PinType::Number}, {"mass", PinType::Number}},
            {{"entity", PinType::Entity}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.PhysicsCreateBody({0}, {1}, {2}, {3})"
        },
        {
            9002, "PhysicsAddCircle", NodeCategory::Physics,
            {{"entity", PinType::Entity}, {"radius", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsAddCircle({0}, {1})"
        },
        {
            9003, "PhysicsAddRectangle", NodeCategory::Physics,
            {{"entity", PinType::Entity}, {"halfW", PinType::Number}, {"halfH", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsAddRectangle({0}, {1}, {2})"
        },
        {
            9004, "PhysicsApplyForce", NodeCategory::Physics,
            {{"entity", PinType::Entity}, {"fx", PinType::Number}, {"fy", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsApplyForce({0}, {1}, {2})"
        },
        {
            9005, "PhysicsApplyImpulse", NodeCategory::Physics,
            {{"entity", PinType::Entity}, {"ix", PinType::Number}, {"iy", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsApplyImpulse({0}, {1}, {2})"
        },
        {
            9006, "PhysicsSyncSprite", NodeCategory::Physics,
            {{"bodyEntity", PinType::Entity}, {"spriteEntity", PinType::Entity}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsSyncSprite({0}, {1})"
        },
        {
            9007, "PhysicsGetPosition", NodeCategory::Physics,
            {{"entity", PinType::Entity}},
            {{"x", PinType::Number}, {"y", PinType::Number}},
            false, "", PinType::Any,
            "local ${x}, ${y} = BlazeBolt.PhysicsGetPosition({0})"
        },
        {
            9008, "PhysicsGetLinearVelocity", NodeCategory::Physics,
            {{"entity", PinType::Entity}},
            {{"vx", PinType::Number}, {"vy", PinType::Number}},
            false, "", PinType::Any,
            "local ${vx}, ${vy} = BlazeBolt.PhysicsGetLinearVelocity({0})"
        },
        {
            9009, "PhysicsSetLinearVelocity", NodeCategory::Physics,
            {{"entity", PinType::Entity}, {"vx", PinType::Number}, {"vy", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PhysicsSetLinearVelocity({0}, {1}, {2})"
        },

        // =========================================================================
        // AUDIO (5 nodes)
        // =========================================================================
        {
            10000, "LoadSound", NodeCategory::Audio,
            {{"filename", PinType::String}, {"soundName", PinType::String}, {"loop", PinType::Bool}},
            {{"soundId", PinType::Integer}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.LoadSound({0}, {1}, {2})"
        },
        {
            10001, "PlaySound", NodeCategory::Audio,
            {{"soundName", PinType::String}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.PlaySound({0})"
        },
        {
            10002, "StopSound", NodeCategory::Audio,
            {{"soundName", PinType::String}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.StopSound({0})"
        },
        {
            10003, "SetSoundVolume", NodeCategory::Audio,
            {{"soundName", PinType::String}, {"volume", PinType::Number}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.SetSoundVolume({0}, {1})"
        },
        {
            10004, "IsSoundPlaying", NodeCategory::Audio,
            {{"soundName", PinType::String}},
            {{"playing", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.IsSoundPlaying({0})"
        },

        // =========================================================================
        // WINDOW (6 nodes)
        // =========================================================================
        {
            11000, "SetMainWindowTitle", NodeCategory::Window,
            {{"title", PinType::String}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.SetMainWindowTitle({0})"
        },
        {
            11001, "SetMainWindowSize", NodeCategory::Window,
            {{"width", PinType::Integer}, {"height", PinType::Integer}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.SetMainWindowSize({0}, {1})"
        },
        {
            11002, "ToggleFullscreen", NodeCategory::Window,
            {},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.ToggleMainWindowFullscreen()"
        },
        {
            11003, "SetMainWindowShouldClose", NodeCategory::Window,
            {{"flag", PinType::Bool}},
            {},
            false, "", PinType::Any,
            ">>BlazeBolt.SetMainWindowShouldClose({0})"
        },
        {
            11004, "GetScreenWidth", NodeCategory::Window,
            {},
            {{"width", PinType::Integer}},
            false, "", PinType::Any,
            "local ${0} = GetScreenWidth()"
        },
        {
            11005, "GetScreenHeight", NodeCategory::Window,
            {},
            {{"height", PinType::Integer}},
            false, "", PinType::Any,
            "local ${0} = GetScreenHeight()"
        },

        // =========================================================================
        // SCENES & SCRIPTS (4 nodes)
        // =========================================================================
        {
            12000, "LoadScene", NodeCategory::Scenes,
            {{"sceneName", PinType::String}},
            {{"success", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.LoadScene({0})"
        },
        {
            12001, "GetCurrentScene", NodeCategory::Scenes,
            {},
            {{"sceneName", PinType::String}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.GetCurrentScene()"
        },
        {
            12002, "LoadScript", NodeCategory::Scenes,
            {{"scriptPath", PinType::String}},
            {{"success", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.LoadScript({0})"
        },
        {
            12003, "ReloadAllScripts", NodeCategory::Scenes,
            {},
            {{"success", PinType::Bool}},
            false, "", PinType::Any,
            "local ${0} = BlazeBolt.ReloadAllScripts()"
        },

        // =========================================================================
        // NOISE (4 nodes)
        // =========================================================================
        {
            13000, "PerlinNoise2D", NodeCategory::Noise,
            {{"x", PinType::Number}, {"y", PinType::Number}},
            {{"value", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = PerlinNoise2D({0}, {1})"
        },
        {
            13001, "SimplexNoise2D", NodeCategory::Noise,
            {{"x", PinType::Number}, {"y", PinType::Number}},
            {{"value", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = SimplexNoise2D({0}, {1})"
        },
        {
            13002, "FbmNoise2D", NodeCategory::Noise,
            {{"x", PinType::Number}, {"y", PinType::Number}, {"octaves", PinType::Integer}, {"lacunarity", PinType::Number}, {"gain", PinType::Number}},
            {{"value", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = FbmNoise2D({0}, {1}, {2}, {3}, {4})"
        },
        {
            13003, "DomainWarpNoise2D", NodeCategory::Noise,
            {{"x", PinType::Number}, {"y", PinType::Number}, {"warpScale", PinType::Number}},
            {{"value", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = DomainWarpNoise2D({0}, {1}, {2})"
        },

        // =========================================================================
        // MATH_TYPES (4 nodes)
        // =========================================================================
        {
            14000, "Vector2Create", NodeCategory::MathTypes,
            {{"x", PinType::Number}, {"y", PinType::Number}},
            {{"vector", PinType::Vector2}},
            false, "", PinType::Any,
            "local ${0} = Vector2({0}, {1})"
        },
        {
            14001, "Vector3Create", NodeCategory::MathTypes,
            {{"x", PinType::Number}, {"y", PinType::Number}, {"z", PinType::Number}},
            {{"vector", PinType::Vector3}},
            false, "", PinType::Any,
            "local ${0} = Vector3({0}, {1}, {2})"
        },
        {
            14002, "Vector2Length", NodeCategory::MathTypes,
            {{"vector", PinType::Vector2}},
            {{"length", PinType::Number}},
            false, "", PinType::Any,
            "local ${0} = {0}:length()"
        },
        {
            14003, "Vector2Normalize", NodeCategory::MathTypes,
            {{"vector", PinType::Vector2}},
            {{"result", PinType::Vector2}},
            false, "", PinType::Any,
            "local ${0} = {0}:normalize()"
        },

        // =========================================================================
        // CONSTANTS (4 nodes)
        // =========================================================================
        {
            15000, "ConstantNumber", NodeCategory::Constants,
            {},
            {{"value", PinType::Number}},
            true, "Value", PinType::Number,
            "local ${0} = {inline}"
        },
        {
            15001, "ConstantString", NodeCategory::Constants,
            {},
            {{"value", PinType::String}},
            true, "Value", PinType::String,
            "local ${0} = \"{inline}\""
        },
        {
            15002, "ConstantBool", NodeCategory::Constants,
            {},
            {{"value", PinType::Bool}},
            true, "Value", PinType::Bool,
            "local ${0} = {inline}"
        },
        {
            15003, "KeyConstant", NodeCategory::Constants,
            {},
            {{"key", PinType::Integer}},
            true, "Key", PinType::Integer,
            "local ${0} = {inline}"
        }
    };
    return defs;
}

inline const NodeTypeDef* FindNodeTypeDef(int id) {
    const auto& defs = GetNodeTypeDefs();
    for (const auto& d : defs) {
        if (d.id == id) return &d;
    }
    return nullptr;
}

inline std::vector<const NodeTypeDef*> GetNodesByCategory(NodeCategory cat) {
    std::vector<const NodeTypeDef*> result;
    const auto& defs = GetNodeTypeDefs();
    for (const auto& d : defs) {
        if (d.category == cat) result.push_back(&d);
    }
    return result;
}
