﻿#include <memory>
#include <imgui.h>
#include <IconsFontAwesome_c.h>

#include <bstorm/file_loader.hpp>
#include <bstorm/script_info.hpp>
#include <bstorm/parser.hpp>
#include <bstorm/util.hpp>
#include <bstorm/logger.hpp>
#include <bstorm/const.hpp>
#include <bstorm/script.hpp>

#include "script_explorer.hpp"

static std::string join(const std::vector<std::string>& ss, const std::string& delim) {
  std::string ret; ret.reserve(128);
  for (int i = 0; i < ss.size(); i++) {
    if (i != 0) ret += delim;
    ret += ss[i];
  }
  return ret;
}

namespace bstorm {
  static void drawFlatView(const std::map<std::wstring, ScriptInfo>& scripts, std::wstring& selectedPath, const std::string& searchText) {
    int uiId = 0;
    for (const auto& entry : scripts) {
      const ScriptInfo& script = entry.second;
      const auto pathU8 = toUTF8(script.path);
      const auto titleU8 = toUTF8(script.title);
      if (!searchText.empty()) {
        if (!(matchString(searchText, titleU8) || matchString(searchText, pathU8))) continue;
      }
      std::string icon;
      if (script.type == SCRIPT_TYPE_PLURAL) {
        icon = ICON_FA_CUBES;
      } else if (script.type == SCRIPT_TYPE_STAGE) {
        icon = ICON_FA_FIGHTER_JET;
      } else if (script.type == SCRIPT_TYPE_PACKAGE) {
        icon = ICON_FA_BRIEFCASE;
      } else if (script.type == SCRIPT_TYPE_PLAYER) {
        icon = ICON_FA_USER;
      } else {
        icon = ICON_FA_CUBE;
      }
      ImGui::PushID(uiId++);
      bool isSelected = selectedPath == script.path;
      if (ImGui::Selectable((icon + " " + (titleU8.empty() ? pathU8 : titleU8)).c_str(), isSelected)) {
        selectedPath = isSelected ? L"" : script.path;
      }
      ImGui::PopID();
    }
  }

  static void drawTreeView(ScriptExplorer::TreeView& view, std::wstring& selectedPath, const std::string& searchText) {
    std::string uiId = "##" + view.uniq;
    if (view.isLeaf()) {
      if (!searchText.empty()) {
        // filter
        if (!matchString(searchText, view.uniq)) return;
      }
      std::wstring path = toUnicode(view.uniq);
      bool isSelected = selectedPath == path;
      auto nodeFlag = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
      if (isSelected) nodeFlag |= ImGuiTreeNodeFlags_Selected;
      ImGui::TreeNodeEx((ICON_FA_FILE" " + view.name + uiId).c_str(), nodeFlag);
      if (ImGui::IsItemClicked()) {
        selectedPath = isSelected ? L"" : path;
      }
    } else {
      std::string icon = view.isOpen ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER;
      if (view.isOpen = ImGui::TreeNodeEx((view.name + uiId).c_str(), ImGuiTreeNodeFlags_DefaultOpen, (icon + " " + view.name).c_str())) {
        for (auto& entry : view.children) {
          drawTreeView(entry.second, selectedPath, searchText);
        }
        ImGui::TreePop();
      }
    }
  }

  static ScriptExplorer::TreeView createTreeView(const std::map<std::wstring, ScriptInfo>& scripts) {
    ScriptExplorer::TreeView root;
    for (const auto& entry : scripts) {
      const auto& script = entry.second;
      std::vector<std::wstring> trail = split(script.path, L'/');
      ScriptExplorer::TreeView* current = &root;
      std::string uniq;
      for (int k = 0; k < trail.size(); k++) {
        if (current->name.empty()) current->name = toUTF8(trail[k]);
        if (k == trail.size() - 1) {
          current->uniq = toUTF8(script.path);
        } else {
          uniq += current->name + "/";
          std::string child = toUTF8(trail[k + 1]);
          if (current->uniq.empty()) current->uniq = uniq;
          if (current->children.count(child) == 0) {
            current->children[child] = ScriptExplorer::TreeView();
          }
          current = &(current->children.find(child)->second);
        }
      }
    }
    return root;
  }

  ScriptExplorer::ScriptExplorer(int left, int top, int width, int height) :
    iniLeft(left),
    iniTop(top),
    iniWidth(width),
    iniHeight(height),
    useTreeView(false),
    showAllPlayerScripts(false)
  {
    filterInputMain.fill('\0');
    filterInputPlayer.fill('\0');
    reload();
  }

  ScriptExplorer::~ScriptExplorer() {
    while (isLoadingNow()) Sleep(1);
  }

  void ScriptExplorer::draw() {
    std::lock_guard<std::mutex> lock(memberAccessSection);
    ImGui::SetNextWindowPos(ImVec2(iniLeft, iniTop), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(iniWidth, iniHeight), ImGuiSetCond_FirstUseEver);
    if (ImGui::Begin("Script Explorer", NULL, ImGuiWindowFlags_ResizeFromAnySide)) {
      float contentWidth = ImGui::GetContentRegionAvailWidth();
      {
        // ツールバー
        ImGui::BeginGroup();
        ImGui::InputText(ICON_FA_SEARCH"##filterMain", filterInputMain.data(), filterInputMain.size());
        ImGui::SameLine(contentWidth - 45);
        if (isLoadingNow()) {
          // reload中
          ImGui::Button(ICON_FA_SPINNER);
        } else {
          if (ImGui::Button(ICON_FA_REFRESH)) {
            reload();
          }
        }
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip("reload");
        ImGui::SameLine();
        if (ImGui::Button(useTreeView ? ICON_FA_LIST : ICON_FA_FILES_O)) {
          useTreeView = !useTreeView;
        }
        if (ImGui::IsItemHovered())
          ImGui::SetTooltip(useTreeView ? "flat view" : "tree view");
        ImGui::EndGroup();
      }

      float contentHeight = ImGui::GetContentRegionAvail().y;

      bool mainScriptListOpened = false;
      if (mainScriptListOpened = ImGui::CollapsingHeader("Main Script", ImGuiTreeNodeFlags_DefaultOpen)) {
        // スクリプト一覧
        ImGui::BeginChildFrame(ImGui::GetID("main script list"), ImVec2(contentWidth, contentHeight * 0.4), ImGuiWindowFlags_HorizontalScrollbar);
        std::string searchTextMain(filterInputMain.data()); // delete null sequence
        if (useTreeView) {
          drawTreeView(mainScriptTreeView, selectedMainScriptPath, searchTextMain);
        } else {
          drawFlatView(mainScripts, selectedMainScriptPath, searchTextMain);
        }
        ImGui::EndChildFrame();
      }

      if (ImGui::CollapsingHeader("Player Script", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Playerスクリプト一覧
        {
          ImGui::BeginGroup();
          ImGui::InputText(ICON_FA_SEARCH"##filterPlayer", filterInputPlayer.data(), filterInputPlayer.size());
          ImGui::SameLine(contentWidth - 80);
          // 全てのPlayerスクリプトを表示するか、Mainスクリプトに関連したものだけを表示するかの選択
          ImGui::Checkbox("Show All", &showAllPlayerScripts);
          ImGui::EndGroup();
        }
        ImGui::BeginChildFrame(ImGui::GetID("player script list"), ImVec2(contentWidth, contentHeight * (mainScriptListOpened ? 0.2 : 0.4)), ImGuiWindowFlags_HorizontalScrollbar);
        std::string searchTextPlayer(filterInputPlayer.data()); // delete null sequence
        if (showAllPlayerScripts) {
          // 全Playerスクリプト
          if (useTreeView) {
            drawTreeView(playerScriptTreeView, selectedPlayerScriptPath, searchTextPlayer);
          } else {
            drawFlatView(playerScripts, selectedPlayerScriptPath, searchTextPlayer);
          }
        } else {
          // #Playerで指定されたスクリプト
          if (mainScripts.count(selectedMainScriptPath) != 0) {
            const ScriptInfo& mainScript = mainScripts.at(selectedMainScriptPath);
            const std::vector<std::wstring>& paths = (mainScript.playerScripts.empty() || mainScript.playerScripts[0] == L"DEFAULT") ? freePlayerScriptPaths : mainScript.playerScripts;
            int uiId = 0;
            for (auto& path : paths) {
              if (playerScripts.count(path) == 0) continue;
              const ScriptInfo& playerScript = playerScripts.at(path);
              const auto pathU8 = toUTF8(playerScript.path);
              const auto titleU8 = toUTF8(playerScript.title);
              if (!searchTextPlayer.empty()) {
                if (!(matchString(searchTextPlayer, titleU8) || matchString(searchTextPlayer, pathU8))) continue;
              }
              ImGui::PushID(uiId++);
              bool isSelected = selectedPlayerScriptPath == path;
              if (ImGui::Selectable((ICON_FA_USER " " + (titleU8.empty() ? pathU8 : titleU8)).c_str(), isSelected)) {
                selectedPlayerScriptPath = isSelected ? L"" : path;
              }
              ImGui::PopID();
            }
          }
        }
        ImGui::EndChildFrame();
      }

      if (ImGui::CollapsingHeader("Main Script Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        // スクリプト情報
        if (mainScripts.count(selectedMainScriptPath) != 0) {
          const ScriptInfo& script = mainScripts.at(selectedMainScriptPath);
          std::string type = toUTF8(script.type);
          std::string title = toUTF8(script.title);
          std::string location = toUTF8(script.path);
          std::string version = toUTF8(script.version);
          std::string id = toUTF8(script.id);
          std::string text = toUTF8(script.text);
          std::string image = toUTF8(script.imagePath);
          std::string system = toUTF8(script.systemPath);
          std::string background = toUTF8(script.backgroundPath);
          std::string bgm = toUTF8(script.bgmPath);
          std::string replayName = toUTF8(script.replayName);
          std::vector<std::string> labels{ "Title", "Location", "Type", "Image", "System", "Background", "BGM", "ReplayName" };
          std::vector<std::string> colons(labels.size(), ":");
          std::vector<std::string> contents{ title, location, type + " (ver." + version + ")", image, system, background, bgm, replayName };
          ImGui::Text(join(labels, "\n").c_str());
          ImGui::SameLine();
          ImGui::Text(join(colons, "\n").c_str());
          ImGui::SameLine();
          ImGui::Text(join(contents, "\n").c_str());
        //  説明文
          float textAreaWidth = ImGui::GetContentRegionAvailWidth();
          float textAreaHeight = ImGui::GetContentRegionAvail().y;
          ImGui::BeginChild("script description", ImVec2(textAreaWidth, textAreaHeight - 5), true);
          ImGui::PushTextWrapPos(textAreaWidth);
          ImGui::Text(text.c_str());
          ImGui::PopTextWrapPos();
          ImGui::EndChild();
        }
      }
    }
    ImGui::End();
  }

  ScriptInfo ScriptExplorer::getSelectedMainScript() const {
    std::lock_guard<std::mutex> lock(memberAccessSection);
    auto it = mainScripts.find(selectedMainScriptPath);
    if (it != mainScripts.end()) {
      return it->second;
    }
    return ScriptInfo();
  }

  ScriptInfo ScriptExplorer::getSelectedPlayerScript() const {
    std::lock_guard<std::mutex> lock(memberAccessSection);
    auto it = playerScripts.find(selectedPlayerScriptPath);
    if (it != playerScripts.end()) {
      return it->second;
    }
    return ScriptInfo();
  }

  bool ScriptExplorer::isLoadingNow() const {
    return reloadThread.joinable();
  }

  void ScriptExplorer::reload() {
    std::lock_guard<std::mutex> lock(reloadSection);
    if (isLoadingNow()) return; // 既にロード中なら無視
    reloadThread = std::thread([this]() {
      {
        std::lock_guard<std::mutex> lock(memberAccessSection);
        mainScripts.clear();
        playerScripts.clear();
        freePlayerScriptPaths.clear();
      }

      // script/以下のスクリプトを再帰的に取得
      std::vector<std::wstring> scriptPaths;
      getFilePathsRecursively(L"script", scriptPaths, ignoreScriptExts);

      auto loader = std::make_shared<FileLoaderFromTextFile>();

      for (auto& path : scriptPaths) {
        try {
          ScriptInfo script = scanDnhScriptInfo(path, loader);

          if (script.systemPath.empty() || script.systemPath == L"DEFAULT") {
            script.systemPath = DEFAULT_SYSTEM_PATH;
          }

          {
            std::lock_guard<std::mutex> lock(memberAccessSection);
            if (script.type == SCRIPT_TYPE_PLAYER) {
              playerScripts[script.path] = script;
              if (script.path.find_first_of(FREE_PLAYER_DIR) == 0) {
                freePlayerScriptPaths.push_back(script.path);
              }
            } else {
              mainScripts[script.path] = script;
            }
          }
        } catch (Log&) {
        }
      }

      // TreeViewの作成
      {
        std::lock_guard<std::mutex> lock(memberAccessSection);
        mainScriptTreeView = createTreeView(mainScripts);
        playerScriptTreeView = createTreeView(playerScripts);
      }
      {
        std::lock_guard<std::mutex> lock(reloadSection);
        reloadThread.detach();
      }
    });
  }
}