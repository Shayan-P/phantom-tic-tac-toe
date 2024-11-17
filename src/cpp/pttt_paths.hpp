#include <filesystem>
#include <string>

namespace pttt
{
    static std::filesystem::path get_data_dir() {
        std::filesystem::path current_file_path = __FILE__;
        std::filesystem::path project_dir = current_file_path.parent_path().parent_path().parent_path();
        return project_dir / "data";
    }

    static std::filesystem::path get_checkpoints_dir() {
        std::filesystem::path current_file_path = __FILE__;
        std::filesystem::path project_dir = current_file_path.parent_path().parent_path().parent_path();
        return project_dir / "checkpoints";
    }

    static std::string get_player0_infoset_path() {
        return get_data_dir() / "player0-infoset.txt";
    }

    static std::string get_player1_infoset_path() {
        return get_data_dir() / "player1-infoset.txt";
    }    
} // namespace pttt
