#pragma once
#include <utils/math/vector.h>
#include <lua.hpp>
#include <lauxlib.h>
#include <sstream>
#include <cstring>

namespace LuaEngine {

static const char* VECTOR2_MT = "BlazeBolt.Vector2";
static const char* VECTOR3_MT = "BlazeBolt.Vector3";
static const char* VECTOR4_MT = "BlazeBolt.Vector4";
static const char* MATRIX3X3_MT = "BlazeBolt.Matrix3x3";

void registerMathTypes(lua_State* state);

// ==================== Vector2 Bindings ====================
struct Vec2Bind {
    static Vector2* To(lua_State* state, int index) {
        if (!lua_isuserdata(state, index)) return nullptr;
        void* ud = lua_touserdata(state, index);
        if (!ud) return nullptr;
        lua_getmetatable(state, index);
        luaL_getmetatable(state, VECTOR2_MT);
        int isSame = lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
        return isSame ? (Vector2*)ud : nullptr;
    }

    static Vector2* Check(lua_State* state, int index) {
        Vector2* v = To(state, index);
        if (!v) luaL_typeerror(state, index, VECTOR2_MT);
        return v;
    }

    static int Push(lua_State* state, const Vector2& v) {
        void* ud = lua_newuserdata(state, sizeof(Vector2));
        new (ud) Vector2(v);
        luaL_setmetatable(state, VECTOR2_MT);
        return 1;
    }

    static int New(lua_State* state) {
        float x = luaL_optnumber(state, 1, 0);
        float y = luaL_optnumber(state, 2, 0);
        return Push(state, Vector2(x, y));
    }

    static int GC(lua_State* state) {
        Vector2* v = (Vector2*)lua_touserdata(state, 1);
        v->~Vector2();
        return 0;
    }

    static int Index(lua_State* state) {
        Vector2* v = To(state, 1);
        if (!v) return 0;
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            if (strcmp(key, "x") == 0) { lua_pushnumber(state, v->x); return 1; }
            if (strcmp(key, "y") == 0) { lua_pushnumber(state, v->y); return 1; }
            lua_getmetatable(state, 1);
            lua_getfield(state, -1, "_methods");
            lua_getfield(state, -1, key);
            return 1;
        }
        lua_pushnil(state);
        return 1;
    }

    static int NewIndex(lua_State* state) {
        Vector2* v = Check(state, 1);
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            float val = luaL_checknumber(state, 3);
            if (strcmp(key, "x") == 0) { v->x = val; return 0; }
            if (strcmp(key, "y") == 0) { v->y = val; return 0; }
        }
        return luaL_error(state, "invalid field for Vector2");
    }

    static int Add(lua_State* state) {
        Vector2* a = To(state, 1);
        Vector2* b = To(state, 2);
        if (a && b) return Push(state, *a + *b);
        return luaL_error(state, "Vector2 + Vector2 expected");
    }

    static int Sub(lua_State* state) {
        Vector2* a = To(state, 1);
        Vector2* b = To(state, 2);
        if (a && b) return Push(state, *a - *b);
        return luaL_error(state, "Vector2 - Vector2 expected");
    }

    static int Mul(lua_State* state) {
        Vector2* a = To(state, 1);
        Vector2* b = To(state, 2);
        if (a && lua_isnumber(state, 2)) return Push(state, *a * (float)lua_tonumber(state, 2));
        if (lua_isnumber(state, 1) && b) return Push(state, (float)lua_tonumber(state, 1) * *b);
        return luaL_error(state, "Vector2 * scalar expected");
    }

    static int Div(lua_State* state) {
        Vector2* a = To(state, 1);
        if (a && lua_isnumber(state, 2)) return Push(state, *a / (float)lua_tonumber(state, 2));
        return luaL_error(state, "Vector2 / scalar expected");
    }

    static int Unm(lua_State* state) {
        Vector2* a = Check(state, 1);
        return Push(state, -*a);
    }

    static int Eq(lua_State* state) {
        Vector2* a = To(state, 1);
        Vector2* b = To(state, 2);
        lua_pushboolean(state, a && b && *a == *b);
        return 1;
    }

    static int ToString(lua_State* state) {
        Vector2* v = Check(state, 1);
        char buf[64];
        snprintf(buf, sizeof(buf), "(%g, %g)", v->x, v->y);
        lua_pushstring(state, buf);
        return 1;
    }

    static int Length(lua_State* state) {
        Vector2* v = Check(state, 1);
        lua_pushnumber(state, v->length());
        return 1;
    }

    static int LengthSquared(lua_State* state) {
        Vector2* v = Check(state, 1);
        lua_pushnumber(state, v->lengthSquared());
        return 1;
    }

    static int Normalized(lua_State* state) {
        Vector2* v = Check(state, 1);
        return Push(state, v->normalized());
    }

    static int Normalize(lua_State* state) {
        Vector2* v = Check(state, 1);
        v->normalize();
        return 0;
    }

    static int Dot(lua_State* state) {
        Vector2* a = Check(state, 1);
        Vector2* b = Check(state, 2);
        lua_pushnumber(state, a->dot(*b));
        return 1;
    }

    static int Cross(lua_State* state) {
        Vector2* a = Check(state, 1);
        Vector2* b = Check(state, 2);
        lua_pushnumber(state, a->cross(*b));
        return 1;
    }

    static int Clone(lua_State* state) {
        Vector2* v = Check(state, 1);
        return Push(state, *v);
    }

    static void CreateMetatable(lua_State* state) {
        luaL_newmetatable(state, VECTOR2_MT);
        lua_pushcfunction(state, GC); lua_setfield(state, -2, "__gc");
        lua_pushcfunction(state, Index); lua_setfield(state, -2, "__index");
        lua_pushcfunction(state, NewIndex); lua_setfield(state, -2, "__newindex");
        lua_pushcfunction(state, Add); lua_setfield(state, -2, "__add");
        lua_pushcfunction(state, Sub); lua_setfield(state, -2, "__sub");
        lua_pushcfunction(state, Mul); lua_setfield(state, -2, "__mul");
        lua_pushcfunction(state, Div); lua_setfield(state, -2, "__div");
        lua_pushcfunction(state, Unm); lua_setfield(state, -2, "__unm");
        lua_pushcfunction(state, Eq); lua_setfield(state, -2, "__eq");
        lua_pushcfunction(state, ToString); lua_setfield(state, -2, "__tostring");
        lua_newtable(state);
        lua_pushcfunction(state, Length); lua_setfield(state, -2, "length");
        lua_pushcfunction(state, LengthSquared); lua_setfield(state, -2, "lengthSquared");
        lua_pushcfunction(state, Normalized); lua_setfield(state, -2, "normalized");
        lua_pushcfunction(state, Normalize); lua_setfield(state, -2, "normalize");
        lua_pushcfunction(state, Dot); lua_setfield(state, -2, "dot");
        lua_pushcfunction(state, Cross); lua_setfield(state, -2, "cross");
        lua_pushcfunction(state, Clone); lua_setfield(state, -2, "clone");
        lua_setfield(state, -2, "_methods");
        lua_pop(state, 1);
    }
};

// ==================== Vector3 Bindings ====================
struct Vec3Bind {
    static Vector3* To(lua_State* state, int index) {
        if (!lua_isuserdata(state, index)) return nullptr;
        void* ud = lua_touserdata(state, index);
        if (!ud) return nullptr;
        lua_getmetatable(state, index);
        luaL_getmetatable(state, VECTOR3_MT);
        int isSame = lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
        return isSame ? (Vector3*)ud : nullptr;
    }

    static Vector3* Check(lua_State* state, int index) {
        Vector3* v = To(state, index);
        if (!v) luaL_typeerror(state, index, VECTOR3_MT);
        return v;
    }

    static int Push(lua_State* state, const Vector3& v) {
        void* ud = lua_newuserdata(state, sizeof(Vector3));
        new (ud) Vector3(v);
        luaL_setmetatable(state, VECTOR3_MT);
        return 1;
    }

    static int New(lua_State* state) {
        float x = luaL_optnumber(state, 1, 0);
        float y = luaL_optnumber(state, 2, 0);
        float z = luaL_optnumber(state, 3, 0);
        return Push(state, Vector3(x, y, z));
    }

    static int GC(lua_State* state) {
        Vector3* v = (Vector3*)lua_touserdata(state, 1);
        v->~Vector3();
        return 0;
    }

    static int Index(lua_State* state) {
        Vector3* v = To(state, 1);
        if (!v) return 0;
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            if (strcmp(key, "x") == 0) { lua_pushnumber(state, v->x); return 1; }
            if (strcmp(key, "y") == 0) { lua_pushnumber(state, v->y); return 1; }
            if (strcmp(key, "z") == 0) { lua_pushnumber(state, v->z); return 1; }
            lua_getmetatable(state, 1);
            lua_getfield(state, -1, "_methods");
            lua_getfield(state, -1, key);
            return 1;
        }
        lua_pushnil(state);
        return 1;
    }

    static int NewIndex(lua_State* state) {
        Vector3* v = Check(state, 1);
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            float val = luaL_checknumber(state, 3);
            if (strcmp(key, "x") == 0) { v->x = val; return 0; }
            if (strcmp(key, "y") == 0) { v->y = val; return 0; }
            if (strcmp(key, "z") == 0) { v->z = val; return 0; }
        }
        return luaL_error(state, "invalid field for Vector3");
    }

    static int Add(lua_State* state) {
        Vector3* a = To(state, 1);
        Vector3* b = To(state, 2);
        if (a && b) return Push(state, *a + *b);
        return luaL_error(state, "Vector3 + Vector3 expected");
    }

    static int Sub(lua_State* state) {
        Vector3* a = To(state, 1);
        Vector3* b = To(state, 2);
        if (a && b) return Push(state, *a - *b);
        return luaL_error(state, "Vector3 - Vector3 expected");
    }

    static int Mul(lua_State* state) {
        Vector3* a = To(state, 1);
        Vector3* b = To(state, 2);
        if (a && lua_isnumber(state, 2)) return Push(state, *a * (float)lua_tonumber(state, 2));
        if (lua_isnumber(state, 1) && b) return Push(state, (float)lua_tonumber(state, 1) * *b);
        return luaL_error(state, "Vector3 * scalar expected");
    }

    static int Div(lua_State* state) {
        Vector3* a = To(state, 1);
        if (a && lua_isnumber(state, 2)) return Push(state, *a / (float)lua_tonumber(state, 2));
        return luaL_error(state, "Vector3 / scalar expected");
    }

    static int Unm(lua_State* state) {
        Vector3* a = Check(state, 1);
        return Push(state, -*a);
    }

    static int Eq(lua_State* state) {
        Vector3* a = To(state, 1);
        Vector3* b = To(state, 2);
        lua_pushboolean(state, a && b && *a == *b);
        return 1;
    }

    static int ToString(lua_State* state) {
        Vector3* v = Check(state, 1);
        char buf[96];
        snprintf(buf, sizeof(buf), "(%g, %g, %g)", v->x, v->y, v->z);
        lua_pushstring(state, buf);
        return 1;
    }

    static int Length(lua_State* state) { Vector3* v = Check(state, 1); lua_pushnumber(state, v->length()); return 1; }
    static int LengthSquared(lua_State* state) { Vector3* v = Check(state, 1); lua_pushnumber(state, v->lengthSquared()); return 1; }
    static int Normalized(lua_State* state) { Vector3* v = Check(state, 1); return Push(state, v->normalized()); }
    static int Normalize(lua_State* state) { Vector3* v = Check(state, 1); v->normalize(); return 0; }

    static int Dot(lua_State* state) {
        Vector3* a = Check(state, 1);
        Vector3* b = Check(state, 2);
        lua_pushnumber(state, a->dot(*b));
        return 1;
    }

    static int Cross(lua_State* state) {
        Vector3* a = Check(state, 1);
        Vector3* b = Check(state, 2);
        return Push(state, a->cross(*b));
    }

    static int Clone(lua_State* state) {
        Vector3* v = Check(state, 1);
        return Push(state, *v);
    }

    static void CreateMetatable(lua_State* state) {
        luaL_newmetatable(state, VECTOR3_MT);
        lua_pushcfunction(state, GC); lua_setfield(state, -2, "__gc");
        lua_pushcfunction(state, Index); lua_setfield(state, -2, "__index");
        lua_pushcfunction(state, NewIndex); lua_setfield(state, -2, "__newindex");
        lua_pushcfunction(state, Add); lua_setfield(state, -2, "__add");
        lua_pushcfunction(state, Sub); lua_setfield(state, -2, "__sub");
        lua_pushcfunction(state, Mul); lua_setfield(state, -2, "__mul");
        lua_pushcfunction(state, Div); lua_setfield(state, -2, "__div");
        lua_pushcfunction(state, Unm); lua_setfield(state, -2, "__unm");
        lua_pushcfunction(state, Eq); lua_setfield(state, -2, "__eq");
        lua_pushcfunction(state, ToString); lua_setfield(state, -2, "__tostring");
        lua_newtable(state);
        lua_pushcfunction(state, Length); lua_setfield(state, -2, "length");
        lua_pushcfunction(state, LengthSquared); lua_setfield(state, -2, "lengthSquared");
        lua_pushcfunction(state, Normalized); lua_setfield(state, -2, "normalized");
        lua_pushcfunction(state, Normalize); lua_setfield(state, -2, "normalize");
        lua_pushcfunction(state, Dot); lua_setfield(state, -2, "dot");
        lua_pushcfunction(state, Cross); lua_setfield(state, -2, "cross");
        lua_pushcfunction(state, Clone); lua_setfield(state, -2, "clone");
        lua_setfield(state, -2, "_methods");
        lua_pop(state, 1);
    }
};

// ==================== Vector4 Bindings ====================
struct Vec4Bind {
    static Vector4* To(lua_State* state, int index) {
        if (!lua_isuserdata(state, index)) return nullptr;
        void* ud = lua_touserdata(state, index);
        if (!ud) return nullptr;
        lua_getmetatable(state, index);
        luaL_getmetatable(state, VECTOR4_MT);
        int isSame = lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
        return isSame ? (Vector4*)ud : nullptr;
    }

    static Vector4* Check(lua_State* state, int index) {
        Vector4* v = To(state, index);
        if (!v) luaL_typeerror(state, index, VECTOR4_MT);
        return v;
    }

    static int Push(lua_State* state, const Vector4& v) {
        void* ud = lua_newuserdata(state, sizeof(Vector4));
        new (ud) Vector4(v);
        luaL_setmetatable(state, VECTOR4_MT);
        return 1;
    }

    static int New(lua_State* state) {
        float x = luaL_optnumber(state, 1, 0);
        float y = luaL_optnumber(state, 2, 0);
        float z = luaL_optnumber(state, 3, 0);
        float w = luaL_optnumber(state, 4, 0);
        return Push(state, Vector4(x, y, z, w));
    }

    static int GC(lua_State* state) {
        Vector4* v = (Vector4*)lua_touserdata(state, 1);
        v->~Vector4();
        return 0;
    }

    static int Index(lua_State* state) {
        Vector4* v = To(state, 1);
        if (!v) return 0;
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            if (strcmp(key, "x") == 0) { lua_pushnumber(state, v->x); return 1; }
            if (strcmp(key, "y") == 0) { lua_pushnumber(state, v->y); return 1; }
            if (strcmp(key, "z") == 0) { lua_pushnumber(state, v->z); return 1; }
            if (strcmp(key, "w") == 0) { lua_pushnumber(state, v->w); return 1; }
            lua_getmetatable(state, 1);
            lua_getfield(state, -1, "_methods");
            lua_getfield(state, -1, key);
            return 1;
        }
        lua_pushnil(state);
        return 1;
    }

    static int NewIndex(lua_State* state) {
        Vector4* v = Check(state, 1);
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            float val = luaL_checknumber(state, 3);
            if (strcmp(key, "x") == 0) { v->x = val; return 0; }
            if (strcmp(key, "y") == 0) { v->y = val; return 0; }
            if (strcmp(key, "z") == 0) { v->z = val; return 0; }
            if (strcmp(key, "w") == 0) { v->w = val; return 0; }
        }
        return luaL_error(state, "invalid field for Vector4");
    }

    static int Add(lua_State* state) {
        Vector4* a = To(state, 1);
        Vector4* b = To(state, 2);
        if (a && b) return Push(state, *a + *b);
        return luaL_error(state, "Vector4 + Vector4 expected");
    }

    static int Sub(lua_State* state) {
        Vector4* a = To(state, 1);
        Vector4* b = To(state, 2);
        if (a && b) return Push(state, *a - *b);
        return luaL_error(state, "Vector4 - Vector4 expected");
    }

    static int Mul(lua_State* state) {
        Vector4* a = To(state, 1);
        Vector4* b = To(state, 2);
        if (a && lua_isnumber(state, 2)) return Push(state, *a * (float)lua_tonumber(state, 2));
        if (lua_isnumber(state, 1) && b) return Push(state, (float)lua_tonumber(state, 1) * *b);
        return luaL_error(state, "Vector4 * scalar expected");
    }

    static int Div(lua_State* state) {
        Vector4* a = To(state, 1);
        if (a && lua_isnumber(state, 2)) return Push(state, *a / (float)lua_tonumber(state, 2));
        return luaL_error(state, "Vector4 / scalar expected");
    }

    static int Unm(lua_State* state) {
        Vector4* a = Check(state, 1);
        return Push(state, -*a);
    }

    static int Eq(lua_State* state) {
        Vector4* a = To(state, 1);
        Vector4* b = To(state, 2);
        lua_pushboolean(state, a && b && *a == *b);
        return 1;
    }

    static int ToString(lua_State* state) {
        Vector4* v = Check(state, 1);
        char buf[128];
        snprintf(buf, sizeof(buf), "(%g, %g, %g, %g)", v->x, v->y, v->z, v->w);
        lua_pushstring(state, buf);
        return 1;
    }

    static int Length(lua_State* state) { Vector4* v = Check(state, 1); lua_pushnumber(state, v->length()); return 1; }
    static int LengthSquared(lua_State* state) { Vector4* v = Check(state, 1); lua_pushnumber(state, v->lengthSquared()); return 1; }
    static int Normalized(lua_State* state) { Vector4* v = Check(state, 1); return Push(state, v->normalized()); }
    static int Normalize(lua_State* state) { Vector4* v = Check(state, 1); v->normalize(); return 0; }
    static int Dot(lua_State* state) { Vector4* a = Check(state, 1); Vector4* b = Check(state, 2); lua_pushnumber(state, a->dot(*b)); return 1; }
    static int ToVector3(lua_State* state) { Vector4* v = Check(state, 1); return Vec3Bind::Push(state, v->toVector3()); }
    static int Clone(lua_State* state) { Vector4* v = Check(state, 1); return Push(state, *v); }

    static void CreateMetatable(lua_State* state) {
        luaL_newmetatable(state, VECTOR4_MT);
        lua_pushcfunction(state, GC); lua_setfield(state, -2, "__gc");
        lua_pushcfunction(state, Index); lua_setfield(state, -2, "__index");
        lua_pushcfunction(state, NewIndex); lua_setfield(state, -2, "__newindex");
        lua_pushcfunction(state, Add); lua_setfield(state, -2, "__add");
        lua_pushcfunction(state, Sub); lua_setfield(state, -2, "__sub");
        lua_pushcfunction(state, Mul); lua_setfield(state, -2, "__mul");
        lua_pushcfunction(state, Div); lua_setfield(state, -2, "__div");
        lua_pushcfunction(state, Unm); lua_setfield(state, -2, "__unm");
        lua_pushcfunction(state, Eq); lua_setfield(state, -2, "__eq");
        lua_pushcfunction(state, ToString); lua_setfield(state, -2, "__tostring");
        lua_newtable(state);
        lua_pushcfunction(state, Length); lua_setfield(state, -2, "length");
        lua_pushcfunction(state, LengthSquared); lua_setfield(state, -2, "lengthSquared");
        lua_pushcfunction(state, Normalized); lua_setfield(state, -2, "normalized");
        lua_pushcfunction(state, Normalize); lua_setfield(state, -2, "normalize");
        lua_pushcfunction(state, Dot); lua_setfield(state, -2, "dot");
        lua_pushcfunction(state, ToVector3); lua_setfield(state, -2, "toVector3");
        lua_pushcfunction(state, Clone); lua_setfield(state, -2, "clone");
        lua_setfield(state, -2, "_methods");
        lua_pop(state, 1);
    }
};

// ==================== Matrix3x3 Bindings ====================
struct Mat3x3Bind {
    static Matrix3x3* To(lua_State* state, int index) {
        if (!lua_isuserdata(state, index)) return nullptr;
        void* ud = lua_touserdata(state, index);
        if (!ud) return nullptr;
        lua_getmetatable(state, index);
        luaL_getmetatable(state, MATRIX3X3_MT);
        int isSame = lua_rawequal(state, -1, -2);
        lua_pop(state, 2);
        return isSame ? (Matrix3x3*)ud : nullptr;
    }

    static Matrix3x3* Check(lua_State* state, int index) {
        Matrix3x3* m = To(state, index);
        if (!m) luaL_typeerror(state, index, MATRIX3X3_MT);
        return m;
    }

    static int Push(lua_State* state, const Matrix3x3& m) {
        void* ud = lua_newuserdata(state, sizeof(Matrix3x3));
        new (ud) Matrix3x3(m);
        luaL_setmetatable(state, MATRIX3X3_MT);
        return 1;
    }

    static int New(lua_State* state) {
        return Push(state, Matrix3x3());
    }

    static int Identity(lua_State* state) {
        return Push(state, Matrix3x3::identity());
    }

    static int Translation(lua_State* state) {
        float x = luaL_checknumber(state, 1);
        float y = luaL_checknumber(state, 2);
        return Push(state, Matrix3x3::translation(x, y));
    }

    static int Scale(lua_State* state) {
        float x = luaL_checknumber(state, 1);
        float y = luaL_optnumber(state, 2, x);
        return Push(state, Matrix3x3::scale(x, y));
    }

    static int Rotation(lua_State* state) {
        float deg = luaL_checknumber(state, 1);
        return Push(state, Matrix3x3::rotation(deg));
    }

    static int GC(lua_State* state) {
        Matrix3x3* m = (Matrix3x3*)lua_touserdata(state, 1);
        m->~Matrix3x3();
        return 0;
    }

    static int Index(lua_State* state) {
        Matrix3x3* m = To(state, 1);
        if (!m) return 0;
        if (lua_isstring(state, 2)) {
            const char* key = lua_tostring(state, 2);
            lua_getmetatable(state, 1);
            lua_getfield(state, -1, "_methods");
            lua_getfield(state, -1, key);
            return 1;
        }
        if (lua_isinteger(state, 2)) {
            int idx = lua_tointeger(state, 2);
            if (idx >= 1 && idx <= 9) {
                int col = (idx - 1) / 3;
                int row = (idx - 1) % 3;
                lua_pushnumber(state, m->m[col][row]);
                return 1;
            }
        }
        lua_pushnil(state);
        return 1;
    }

    static int NewIndex(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        if (lua_isinteger(state, 2)) {
            int idx = lua_tointeger(state, 2);
            if (idx >= 1 && idx <= 9) {
                float val = luaL_checknumber(state, 3);
                int col = (idx - 1) / 3;
                int row = (idx - 1) % 3;
                m->m[col][row] = val;
                return 0;
            }
        }
        return luaL_error(state, "Matrix3x3 index out of range [1..9]");
    }

    static int Mul(lua_State* state) {
        Matrix3x3* a = To(state, 1);
        Matrix3x3* b = To(state, 2);
        if (a && b) return Push(state, *a * *b);
        return luaL_error(state, "Matrix3x3 * Matrix3x3 expected");
    }

    static int Eq(lua_State* state) {
        Matrix3x3* a = To(state, 1);
        Matrix3x3* b = To(state, 2);
        if (a && b) {
            bool equal = true;
            for (int i = 0; i < 3 && equal; i++)
                for (int j = 0; j < 3 && equal; j++)
                    if (a->m[i][j] != b->m[i][j]) equal = false;
            lua_pushboolean(state, equal);
        } else {
            lua_pushboolean(state, false);
        }
        return 1;
    }

    static int ToString(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        char buf[256];
        snprintf(buf, sizeof(buf), "[[%g %g %g] [%g %g %g] [%g %g %g]]",
            m->m[0][0], m->m[0][1], m->m[0][2],
            m->m[1][0], m->m[1][1], m->m[1][2],
            m->m[2][0], m->m[2][1], m->m[2][2]);
        lua_pushstring(state, buf);
        return 1;
    }

    static int Get(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        int col = luaL_checkinteger(state, 2) - 1;
        int row = luaL_checkinteger(state, 3) - 1;
        if (col < 0 || col > 2 || row < 0 || row > 2)
            return luaL_error(state, "Matrix3x3: col and row must be 1..3");
        lua_pushnumber(state, m->m[col][row]);
        return 1;
    }

    static int Set(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        int col = luaL_checkinteger(state, 2) - 1;
        int row = luaL_checkinteger(state, 3) - 1;
        float val = luaL_checknumber(state, 4);
        if (col < 0 || col > 2 || row < 0 || row > 2)
            return luaL_error(state, "Matrix3x3: col and row must be 1..3");
        m->m[col][row] = val;
        return 0;
    }

    static int ToArray(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        lua_newtable(state);
        for (int i = 0; i < 9; i++) {
            int col = i / 3;
            int row = i % 3;
            lua_pushnumber(state, m->m[col][row]);
            lua_rawseti(state, -2, i + 1);
        }
        return 1;
    }

    static int Clone(lua_State* state) {
        Matrix3x3* m = Check(state, 1);
        return Push(state, *m);
    }

    static void CreateMetatable(lua_State* state) {
        luaL_newmetatable(state, MATRIX3X3_MT);
        lua_pushcfunction(state, GC); lua_setfield(state, -2, "__gc");
        lua_pushcfunction(state, Index); lua_setfield(state, -2, "__index");
        lua_pushcfunction(state, NewIndex); lua_setfield(state, -2, "__newindex");
        lua_pushcfunction(state, Mul); lua_setfield(state, -2, "__mul");
        lua_pushcfunction(state, Eq); lua_setfield(state, -2, "__eq");
        lua_pushcfunction(state, ToString); lua_setfield(state, -2, "__tostring");
        lua_newtable(state);
        lua_pushcfunction(state, Get); lua_setfield(state, -2, "get");
        lua_pushcfunction(state, Set); lua_setfield(state, -2, "set");
        lua_pushcfunction(state, ToArray); lua_setfield(state, -2, "toArray");
        lua_pushcfunction(state, Clone); lua_setfield(state, -2, "clone");
        lua_pushcfunction(state, Identity); lua_setfield(state, -2, "identity");
        lua_pushcfunction(state, Translation); lua_setfield(state, -2, "translation");
        lua_pushcfunction(state, Scale); lua_setfield(state, -2, "scale");
        lua_pushcfunction(state, Rotation); lua_setfield(state, -2, "rotation");
        lua_setfield(state, -2, "_methods");
        lua_pop(state, 1);
    }
};

// ==================== Registration ====================
void registerMathTypes(lua_State* state) {
    Vec2Bind::CreateMetatable(state);
    Vec3Bind::CreateMetatable(state);
    Vec4Bind::CreateMetatable(state);
    Mat3x3Bind::CreateMetatable(state);

    lua_pushcfunction(state, Vec2Bind::New);
    lua_setglobal(state, "Vector2");

    lua_pushcfunction(state, Vec3Bind::New);
    lua_setglobal(state, "Vector3");

    lua_pushcfunction(state, Vec4Bind::New);
    lua_setglobal(state, "Vector4");

    lua_pushcfunction(state, Mat3x3Bind::New);
    lua_setglobal(state, "Matrix3x3");
}

} // namespace LuaEngine
