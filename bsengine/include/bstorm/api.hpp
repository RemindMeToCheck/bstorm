﻿#pragma once

#include <string>
#include <unordered_map>
#include <luajit/lua.hpp>

#include <bstorm/env.hpp>

namespace bstorm {
  struct SourcePos;
  void addStandardAPI(const std::wstring& type, const std::wstring& version, NameTable& table);

  // helper
  int getCurrentLine(lua_State* L);
  std::shared_ptr<SourcePos> getSourcePos(lua_State* L);
  int __UnsafeFunctionCommon(lua_State* L, lua_CFunction func);
  template <lua_CFunction func>
  int UnsafeFunction(lua_State* L) {
    return __UnsafeFunctionCommon(L, func);
  }
  void* getPointer(lua_State* L, const char* key);
  void setPointer(lua_State* L, const char* key, void* p);

  class Engine;
  Engine* getEngine(lua_State* L);
  void setEngine(lua_State* L, Engine*);

  class Script;
  Script* getScript(lua_State* L);
  void setScript(lua_State* L, Script*);

  // runtime helper function
  int c_chartonum(lua_State* L);
  int c_succchar(lua_State* L);
  int c_predchar(lua_State* L);
  int c_raiseerror(lua_State* L);
}