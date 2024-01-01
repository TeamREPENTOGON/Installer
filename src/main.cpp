#include <curl/curl.h>
#include <string>
#include <filesystem>
#include <document.h>
#include <miniz.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <thread>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "LogViewer.h"

#include <stdio.h>
#include <GLFW/glfw3.h>

using namespace rapidjson;

static int argc = 0;
static char** argv = NULL;

static bool HasCommandLineArgument(int argc, char* argv[], const char* arg)
{
    for (int i = 1; i < argc; ++i)
    {
        #ifdef _WIN32
        if (_stricmp(argv[i], arg) == 0) return true;
	#else
	if(strcasecmp(argv[i], arg) == 0) return true;
	#endif
    }

    return false;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

LogViewer logViewer;
bool updating;
bool automatic = false;

std::string trim(const std::string& input) {
    std::stringstream string_stream;
    for (const auto character : input) {
        if (!isspace(character)) {
            string_stream << character;
        }
    }

    return string_stream.str();
}

bool PerformUpdate(int argc, char* argv[]) {

    CURL* curl;
    CURLcode res;
    std::string repentogon_zip;
    std::string repentogon_hash;
    CryptoPP::SHA256 sha256;
    std::string calculated_hash;

    curl = curl_easy_init();
    if (curl) {
        std::string result;
        rapidjson::Document doc;
        struct curl_slist* headers = NULL;

        headers = curl_slist_append(headers, "Accept: application/vnd.github+json");
        headers = curl_slist_append(headers, "X-GitHub-Api-Version: 2022-11-28");
        headers = curl_slist_append(headers, "User-Agent: REPENTOGON");

        curl_easy_setopt(curl, CURLOPT_URL, "https://api.github.com/repos/TeamREPENTOGON/REPENTOGON/releases/latest");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            logViewer.AddLog("Failed to check for updates: %s\n",
                curl_easy_strerror(res));
            return false;
        }

        doc.Parse(result.c_str());
        if (!doc.HasMember("name")) {
            logViewer.AddLog("Failed to fetch latest release from the GitHub API. Returned JSON: %s\n", result.c_str());
            return false;
        }
        result.clear();

        std::string hash_url;
        std::string zip_url;
        for (const auto& asset : doc["assets"].GetArray()) {

            if (std::string("hash.txt").compare(asset["name"].GetString()) == 0) 
                hash_url = asset["browser_download_url"].GetString();
                
            if (std::string("REPENTOGON.zip").compare(asset["name"].GetString()) == 0) 
                zip_url = asset["browser_download_url"].GetString();
        }

        if (hash_url.empty() || zip_url.empty()) {
            logViewer.AddLog("Unable to fetch URL of release or hash, updater cannot continue.\nHash URL: %s\nZip URL: %s", hash_url.c_str(), zip_url.c_str());
            return false;
        }

        logViewer.AddLog("Fetching hash...\n");
        curl_easy_setopt(curl, CURLOPT_URL, hash_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &repentogon_hash);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            logViewer.AddLog("Failed to download hash: %s\n",
                curl_easy_strerror(res));
            return false;
        }

        logViewer.AddLog("Downloading release...\n");
        curl_easy_setopt(curl, CURLOPT_URL, zip_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &repentogon_zip);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            logViewer.AddLog("Failed to download release: %s\n",
                curl_easy_strerror(res));
            return false;
        }
        curl_easy_cleanup(curl);

    }

    CryptoPP::StringSource source = CryptoPP::StringSource(repentogon_zip, true,
        new CryptoPP::HashFilter(sha256,
            new CryptoPP::HexEncoder(
                new CryptoPP::StringSink(calculated_hash)
            )
        )
    );

    std::transform(calculated_hash.begin(), calculated_hash.end(), calculated_hash.begin(),
        [](unsigned char c) { return std::tolower(c); });

    std::transform(repentogon_hash.begin(), repentogon_hash.end(), repentogon_hash.begin(),
        [](unsigned char c) { return std::tolower(c); });

    repentogon_hash = trim(repentogon_hash);

    logViewer.AddLog("Hash: %s\n", repentogon_hash.c_str());
    if (repentogon_hash.compare(calculated_hash) != 0) {
        logViewer.AddLog("Hash mismatch, aborting!\nExpected hash: %s\nCalculated hash: %s\n", repentogon_hash.c_str(), calculated_hash.c_str());
        return false;
    }

    mz_zip_archive archive;
    mz_bool status;

    memset(&archive, 0, sizeof(archive));

    status = mz_zip_reader_init_mem(&archive, repentogon_zip.c_str(), repentogon_zip.size(), 0);
    if (!status) {
        logViewer.AddLog("Downloaded zip file is invalid or corrupt, aborting.\n");
        return false;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&archive); i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            logViewer.AddLog("Failed to read file, aborting.\n");
            mz_zip_reader_end(&archive);
            return false;
        }
        logViewer.AddLog("Extracting %s\n", file_stat.m_filename);

        std::string name = file_stat.m_filename;
        bool needs_dir = name.find("/") != name.npos;

        std::filesystem::create_directories(std::filesystem::path(std::string("./" + std::string(file_stat.m_filename))).parent_path());

        int attempts = 5;
        bool extracted = false;
        for (int i = 0; i < attempts; ++i) {
            int res = mz_zip_reader_extract_to_file(&archive, i, file_stat.m_filename, 0);
            if (res) {
                extracted = true;
                break;
            }
        } 

        if (!extracted) {
            logViewer.AddLog("Failed to extract %s after %d attempts, aborting.\n", file_stat.m_filename, attempts);
            mz_zip_reader_end(&archive);
            return false;
        }
    }

    mz_zip_reader_end(&archive);

    #ifdef _WIN32
    if (automatic) {
        std::string args;

        for (int i = 1; i < argc; ++i) {
            args.append(argv[i]).append(" ");
           
        }
        ShellExecute(NULL, "open", "isaac-ng.exe", args.c_str(), NULL, SW_SHOWDEFAULT);
        ExitProcess(1);
    }
    #endif

    logViewer.AddLog("Finished!\n");
    updating = false;
    return true;
}

static void glfw_error_callback(int error, const char* description) {
    logViewer.AddLog("GLFW Error %d: %s\n", error, description);
}

int main(int argc, char* argv[]) {
    #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

    #ifdef _WIN32
    if (HasCommandLineArgument(argc, argv, "-auto"))
        automatic = true;
    #endif

    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
        return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1024, 600, "REPENTOGON Installer", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        {

            ImGui::Begin("Installer", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);                          // Create a window called "Hello, world!" and append into it.
            ImGui::SetWindowPos(ImVec2(0, 0));
            ImGui::SetWindowSize(ImVec2(1024, 300));

            ImGui::Text("Welcome to the REPENTOGON Installer!");

            if (updating)
                ImGui::BeginDisabled();
            if (ImGui::Button(updating ? "Installing..." : "Install") || automatic) {
                if (!updating) {
                    std::thread downloader(PerformUpdate, argc, argv);
                    ImGui::BeginDisabled();
                    downloader.detach();
                }
                updating = true;
            }

            if (updating)
                ImGui::EndDisabled();

            ImGui::End();

        }

        logViewer.Draw();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
