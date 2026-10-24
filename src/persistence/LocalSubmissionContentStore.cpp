#include "lms/persistence/LocalSubmissionContentStore.h"

#include <sodium.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <utility>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace lms {
namespace {
namespace fs=std::filesystem;
constexpr std::uintmax_t limit=10ULL*1024*1024;

bool validToken(std::string_view token)
{
    const auto period=token.find('.');
    if(period!=32)return false;
    for(std::size_t i=0;i<32;++i) {
        const char c=token[i];
        if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;
    }
    const auto extension=token.substr(period);
    return extension==".pdf"||extension==".docx"||extension==".txt";
}

std::string extensionFor(const fs::path& source)
{
    std::string extension=source.extension().string();
    std::transform(extension.begin(),extension.end(),extension.begin(),
                   [](unsigned char c){return static_cast<char>(std::tolower(c));});
    if(extension!=".pdf"&&extension!=".docx"&&extension!=".txt")
        throw std::invalid_argument("Allowed file formats are PDF, DOCX and TXT");
    return extension;
}

std::string createToken(std::string_view extension)
{
    std::array<unsigned char,16> random{};
    std::array<char,33> hex{};
    randombytes_buf(random.data(),random.size());
    sodium_bin2hex(hex.data(),hex.size(),random.data(),random.size());
    return std::string(hex.data())+std::string(extension);
}
} // namespace

LocalSubmissionContentStore::LocalSubmissionContentStore(std::string rootDirectory)
    : root_(std::move(rootDirectory))
{
    if(sodium_init()<0)throw std::runtime_error("Cannot initialise secure random generator");
    if(root_.empty())throw std::invalid_argument("Attachment storage directory is empty");
    fs::create_directories(root_/"staging");
    fs::create_directories(root_/"files");
}

StagedRef LocalSubmissionContentStore::stage(std::string_view sourcePath)
{
    if(sourcePath.empty())throw std::invalid_argument("No attachment selected");
    const fs::path source{std::string(sourcePath)};
    if(!fs::is_regular_file(source))throw std::invalid_argument("Attachment does not exist");
    const auto extension=extensionFor(source);
    const auto size=fs::file_size(source);
    if(size==0||size>limit)
        throw std::invalid_argument("Attachment must be between 1 byte and 10 MB");
    for(int attempt=0;attempt<4;++attempt) {
        const auto token=createToken(extension);
        const auto temp=root_/"staging"/token;
        if(fs::exists(temp)||fs::exists(root_/"files"/token))continue;
        fs::copy_file(source,temp,fs::copy_options::none);
        if(fs::file_size(temp)>limit) {
            fs::remove(temp);
            throw std::invalid_argument("Attachment exceeds 10 MB");
        }
        return StagedRef{token,extension};
    }
    throw std::runtime_error("Could not reserve a unique attachment name");
}

StoredRef LocalSubmissionContentStore::commit(const StagedRef& staged)
{
    if(!validToken(staged.token))
        throw std::invalid_argument("Invalid staged attachment identifier");
    const auto temp=root_/"staging"/staged.token;
    const auto final=root_/"files"/staged.token;
    if(fs::exists(final))
        throw std::runtime_error("Attachment name collision");
    fs::rename(temp,final);
    return StoredRef{staged.token};
}

void LocalSubmissionContentStore::discard(const StagedRef& staged) noexcept
{
    if(!validToken(staged.token))return;
    std::error_code ec;
    fs::remove(root_/"staging"/staged.token,ec);
}

void LocalSubmissionContentStore::remove(const StoredRef& stored) noexcept
{
    if(!validToken(stored.token))return;
    std::error_code ec;
    fs::remove(root_/"files"/stored.token,ec);
}

std::string LocalSubmissionContentStore::read(const StoredRef& stored) const
{
    if(!validToken(stored.token))
        throw std::invalid_argument("Invalid stored attachment identifier");
    const auto path=root_/"files"/stored.token;
    const auto size=fs::file_size(path);
    if(size>limit)throw std::runtime_error("Stored attachment exceeds size limit");
    std::ifstream in(path,std::ios::binary);
    if(!in)throw std::runtime_error("Cannot read stored attachment");
    return std::string(std::istreambuf_iterator<char>(in),
                       std::istreambuf_iterator<char>());
}
} // namespace lms
