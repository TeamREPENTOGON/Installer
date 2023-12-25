#include <curl/curl.h>
#include <string>
#include <filesystem>
#include <document.h>
#include <miniz.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

using namespace rapidjson;

#define ERROR_DIALOG MessageBoxA(NULL, "Update failed, please see the console for more info.", "REPENTOGON Updater", MB_OK);

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

int main() {

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
            fprintf(stderr, "Failed to check for updates: %s\n",
                curl_easy_strerror(res));
            ERROR_DIALOG;
            return EXIT_FAILURE;
        }

        doc.Parse(result.c_str());
        if (doc["name"].Empty()) {
            fprintf(stderr, "Failed to fetch latest release from the GitHub API. Returned JSON: %s\n", result.c_str());
            ERROR_DIALOG;
            return EXIT_FAILURE;
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
            fprintf(stderr, "Unable to fetch URL of release or hash, updater cannot continue.\nHash URL: %s\nZip URL: %s", hash_url.c_str(), zip_url.c_str());
            ERROR_DIALOG;
            return EXIT_FAILURE;
        }

        printf("Fetching hash from GitHub...\n");
        curl_easy_setopt(curl, CURLOPT_URL, hash_url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &repentogon_hash);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to download hash: %s\n",
                curl_easy_strerror(res));
            ERROR_DIALOG;
            return EXIT_FAILURE;
        }
        printf("Hash: %s\n", repentogon_hash.c_str());

        printf("Downloading release from GitHub...\n");
        curl_easy_setopt(curl, CURLOPT_URL, zip_url);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &repentogon_zip);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "Failed to download release: %s\n",
                curl_easy_strerror(res));
            ERROR_DIALOG;
            return EXIT_FAILURE;
        }
        curl_easy_cleanup(curl);
        
    }

    CryptoPP::StringSource(repentogon_zip, true,
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

    if (repentogon_hash.compare(calculated_hash) == 0) {
        fprintf(stderr, "Hash mismatch, aborting!\nExpected hash: %s\nCalculated hash: %s\n", repentogon_hash.c_str(), calculated_hash.c_str());
        ERROR_DIALOG;
        return EXIT_FAILURE;
    }

    mz_zip_archive archive;
    mz_bool status;

    memset(&archive, 0, sizeof(archive));

    status = mz_zip_reader_init_mem(&archive, repentogon_zip.c_str(), repentogon_zip.size(), 0);
    if (!status) {
        fprintf(stderr, "Downloaded zip file is invalid or corrupt, aborting.\n");
        ERROR_DIALOG;
        return EXIT_FAILURE;
    }

    for (int i = 0; i < (int)mz_zip_reader_get_num_files(&archive); ++i) {
        mz_zip_archive_file_stat file_stat;

        if (!mz_zip_reader_file_stat(&archive, i, &file_stat)) {
            fprintf(stderr, "Failed to read file, aborting.\n");
            mz_zip_reader_end(&archive);
            ERROR_DIALOG;
            return EXIT_FAILURE;
        }

        if (mz_zip_reader_is_file_a_directory(&archive, i))
            std::filesystem::create_directory(file_stat.m_filename);
        else {
            int res = mz_zip_reader_extract_to_file(&archive, i, file_stat.m_filename, 0);
            if (!res) {
                fprintf(stderr, "Failed to extract %s, aborting.\n", file_stat.m_filename);
                mz_zip_reader_end(&archive);
                ERROR_DIALOG;
                return EXIT_FAILURE;
            }
        }
    }

    mz_zip_reader_end(&archive);

    if (MessageBoxA(NULL, "Update complete! Restart game?", "REPENTOGON Updater", MB_YESNO)) {
        //TODO relaunch isaac
    };

    return EXIT_SUCCESS;
}