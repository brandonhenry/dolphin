#include <string>

class NetPlayConfig {
private:
    std::string nickname;
    std::string password;
    std::string room;
    std::string region;
    std::string filename;

public:
    NetPlayConfig(const std::string& configFile);

    const std::string& GetNickname() const;
    const std::string& GetPassword() const;
    const std::string& GetRoom() const;
    const std::string& GetRegion() const;
    const std::string& GetFilename() const;

    bool IsValid() const;
};
