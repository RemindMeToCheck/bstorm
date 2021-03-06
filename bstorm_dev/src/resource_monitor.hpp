﻿#pragma once

#include <vector>

#include <bstorm/type.hpp>

namespace bstorm {
  class Engine;
  class Texture;
  class Font;
  class RenderTarget;
  class ResourceMonitor {
  public:
    ResourceMonitor(int left, int top, int width, int height);
    ~ResourceMonitor();
    void draw(const std::shared_ptr<Engine>& engine);
    bool isOpened() const { return openFlag; }
    void setOpen(bool b) { openFlag = b; }
  private:
    int iniLeft;
    int iniTop;
    int iniWidth;
    int iniHeight;
    bool openFlag;
  };

  void drawTextureInfo(const std::shared_ptr<Texture>& texture, const std::vector<Rect<int>>& rects);
  void drawFontInfo(const std::shared_ptr<Font>& font);
  void drawRenderTargetInfo(const std::shared_ptr<RenderTarget>& renderTarget, const std::vector<Rect<int>>& rects);
}