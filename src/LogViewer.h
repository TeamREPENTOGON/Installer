#pragma once
#include "imgui.h"
#include <iostream>
#include <sstream>
#include <string>

extern bool imguiResized;
extern ImVec2 imguiSizeModifier;


struct LogViewer {
    bool autoscroll;

    ImGuiTextBuffer logBuf;
    ImVector<int> offsets;

    LogViewer() {
        autoscroll = true;
    }

    void AddLog( const char* fmt, ...) IM_FMTARGS(2)
    {
        std::stringstream ss(fmt);
        std::string token;
        int old_size = logBuf.size();
        while (std::getline(ss, token, '\n')) {
            int size = std::snprintf(nullptr, 0, "%s\n", token.c_str());
            std::string res(size + 1, '\0');
            std::sprintf(&res[0], "%s\n", token.c_str());

            va_list args;
            va_start(args, fmt);
            logBuf.appendfv(res.c_str(), args);
            va_end(args);
        }
        for (int new_size = logBuf.size(); old_size < new_size; old_size++)
            if (logBuf[old_size] == '\n')
                offsets.push_back(old_size + 1);
    }

    void Draw()
    {
        if (ImGui::Begin("Logs", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

            ImGui::SetWindowSize(ImVec2(1024, 300));
            ImGui::SetWindowPos(ImVec2(0, 300));
            if (ImGui::BeginChild("LogViewScrollable", ImVec2(0, 0), false, 0)) {
                const char* buf_begin = logBuf.begin();
                const char* buf_end = logBuf.end();

                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                ImGui::TextWrapped(logBuf.c_str());

                if (autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                    ImGui::SetScrollHereY(1.0f);
                ImGui::PopStyleVar();
                ImGui::EndChild();
            }
            ImGui::End(); // close window element
        }
    
    }
};

extern LogViewer logViewer;
