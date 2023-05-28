#include "Core/NetPlayConfig.h"
#include <fstream>

NetPlayConfig::NetPlayConfig(const std::string& configFile) {
    std::ifstream file(configFile);
    if (file.is_open()) {
        std::getline(file, nickname);
        std::getline(file, password);
        std::getline(file, room);
        std::getline(file, region);
        std::getline(file, filename);
        file.close();
    }
}

const std::string& NetPlayConfig::GetNickname() const {
    return nickname;
}

const std::string& NetPlayConfig::GetPassword() const {
    return password;
}

const std::string& NetPlayConfig::GetRoom() const {
    return room;
}

const std::string& NetPlayConfig::GetRegion() const {
    return region;
}

const std::string& NetPlayConfig::GetFilename() const {
    return filename;
}

bool NetPlayConfig::IsValid() const {
    return !nickname.empty() && !password.empty() && !room.empty() && !region.empty() && !filename.empty();
}
